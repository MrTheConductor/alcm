using System;
using System.Collections.Generic;
using System.IO;

namespace BoardSimulator.Vesc
{
    /// <summary>
    /// VESC serial protocol implementation
    /// Generates authentic VESC messages for ALCM consumption
    /// </summary>
    public class VescProtocol
    {
        // VESC packet structure:
        // [Start Byte] [Length] [Payload] [CRC] [Stop Byte]
        private const byte StartByte = 0x02;      // Short packet
        private const byte StartByteLong = 0x03;  // Long packet
        private const byte StopByte = 0x03;

        // VESC command IDs
        private enum CommPacketId : byte
        {
            COMM_GET_VALUES = 4,
            COMM_GET_VALUES_SELECTIVE = 50,
            COMM_GET_IMU_DATA = 65,
            COMM_CUSTOM_APP_DATA = 36,
        }

        // refloat's custom app data protocol (see refloat/src/lcm.h)
        private const byte LcmPackageId = 101;
        private const byte LcmCommandPoll = 24;

        /// <summary>
        /// Generates a COMM_GET_VALUES response packet
        /// This is the most common message type that ALCM processes
        /// </summary>
        public static byte[] GenerateValuesMessage(
            float tempFet,
            float tempMotor,
            float avgMotorCurrent,
            float avgInputCurrent,
            float avgId,
            float avgIq,
            float dutyCycleNow,
            int rpm,
            float inputVoltage,
            float ampHours,
            float ampHoursCharged,
            float wattHours,
            float wattHoursCharged,
            int tachometer,
            int tachometerAbs,
            byte faultCode,
            float pidPos,
            byte vescId)
        {
            using var ms = new MemoryStream();
            using var writer = new BinaryWriter(ms);

            // Write command ID
            writer.Write((byte)CommPacketId.COMM_GET_VALUES);

            // Write all fields in the order VESC sends them (big-endian)
            WriteFloat16(writer, tempFet, 10.0f);           // Temperature FET (scaled)
            WriteFloat16(writer, tempMotor, 10.0f);         // Temperature Motor (scaled)
            WriteFloat32(writer, avgMotorCurrent);          // Average motor current
            WriteFloat32(writer, avgInputCurrent);          // Average input current
            WriteFloat32(writer, avgId);                    // Average D current
            WriteFloat32(writer, avgIq);                    // Average Q current
            WriteFloat16(writer, dutyCycleNow, 1000.0f);   // Duty cycle now (scaled)
            WriteInt32(writer, rpm);                        // RPM
            WriteFloat16(writer, inputVoltage, 10.0f);     // Input voltage (scaled)
            WriteFloat32(writer, ampHours);                 // Amp hours used
            WriteFloat32(writer, ampHoursCharged);          // Amp hours charged
            WriteFloat32(writer, wattHours);                // Watt hours used
            WriteFloat32(writer, wattHoursCharged);         // Watt hours charged
            WriteInt32(writer, tachometer);                 // Tachometer
            WriteInt32(writer, tachometerAbs);              // Tachometer absolute
            writer.Write(faultCode);                        // Fault code
            WriteFloat32(writer, pidPos);                   // PID position
            writer.Write(vescId);                           // VESC ID

            byte[] payload = ms.ToArray();

            // Build packet: [Start] [Length] [Payload] [CRC] [Stop]
            return BuildPacket(payload);
        }

        /// <summary>
        /// Generates a COMM_GET_VALUES_SETUP_SELECTIVE response packet (command 0x33)
        /// This is what ALCM actually requests with its selective polling
        /// 16-byte payload format matching COMM_GET_VALUES_SETUP_SELECTIVE_MASK 0x101b0
        /// </summary>
        public static byte[] GenerateSelectiveValuesMessage(
            float dutyCycle,
            int rpm,
            float inputVoltage,
            float batteryLevel,
            byte faultCode)
        {
            using var ms = new MemoryStream();
            using var writer = new BinaryWriter(ms);

            // Write command ID for selective response
            writer.Write((byte)0x33); // COMM_GET_VALUES_SETUP_SELECTIVE

            // Write mask (uint32) - must match COMM_GET_VALUES_SETUP_SELECTIVE_MASK
            WriteInt32(writer, 0x101b0);

            // Write duty cycle (float16, scale 10.0)
            WriteFloat16(writer, dutyCycle, 10.0f);

            // Write RPM (float32, scale 1.0)
            WriteFloat32(writer, rpm);

            // Write input voltage (float16, scale 10.0)
            WriteFloat16(writer, inputVoltage, 10.0f);

            // Write battery level (float16, scale 10.0)
            WriteFloat16(writer, batteryLevel, 10.0f);

            // Write fault code (uint8)
            writer.Write(faultCode);

            byte[] payload = ms.ToArray();

            // Verify payload is exactly 16 bytes as expected
            if (payload.Length != 16) // Should be 16 bytes total
            {
                throw new InvalidOperationException($"Selective values payload must be 16 bytes, got {payload.Length}");
            }

            // Build packet with framing and CRC
            return BuildPacket(payload);
        }

        /// <summary>
        /// Generates a COMM_GET_IMU_DATA response packet (command 0x41)
        /// This is what ALCM requests (with ENABLE_IMU_EVENTS) alongside its
        /// selective polling, mask 0x0003 (bit0=roll, bit1=pitch), matching
        /// COMM_GET_IMU_DATA_MASK in vesc_serial.c. Roll/pitch are unscaled
        /// IEEE-754 floats in radians - the firmware converts to degrees.
        /// The firmware only reads the first 11 bytes of the payload but
        /// requires a 12-byte length (COMM_GET_IMU_DATA_RESPONSE_LENGTH), so
        /// a trailing reserved/padding byte is appended.
        /// </summary>
        public static byte[] GenerateImuDataMessage(float rollRadians, float pitchRadians)
        {
            using var ms = new MemoryStream();
            using var writer = new BinaryWriter(ms);

            writer.Write((byte)CommPacketId.COMM_GET_IMU_DATA);
            WriteInt16(writer, 0x0003); // mask: bit0=roll, bit1=pitch

            WriteFloat32(writer, rollRadians);
            WriteFloat32(writer, pitchRadians);

            writer.Write((byte)0); // reserved/padding byte

            byte[] payload = ms.ToArray();

            if (payload.Length != 12)
            {
                throw new InvalidOperationException($"IMU data payload must be 12 bytes, got {payload.Length}");
            }

            return BuildPacket(payload);
        }

        /// <summary>
        /// Generates a COMM_CUSTOM_APP_DATA / COMMAND_LCM_POLL response packet
        /// (command 0x24), matching refloat's lcm_poll_response() format
        /// (refloat/src/lcm.c) that ALCM's process_comm_custom_app_data()
        /// (vesc_serial.c) parses. ALCM only reads the trailing brightness
        /// bytes (offsets 12/13/14), so the state/fault/duty/erpm/current/
        /// voltage fields ahead of them are written as zero. When `enabled`
        /// is false, only the 2-byte package/command header is sent, mirroring
        /// refloat's behavior when hardware.leds.mode has External LEDs
        /// disabled - a valid protocol state, not an error.
        /// </summary>
        public static byte[] GenerateLcmPollResponseMessage(
            bool enabled,
            byte headlightBrightnessPercent,
            byte headlightIdleBrightnessPercent,
            byte statusBrightnessPercent)
        {
            using var ms = new MemoryStream();
            using var writer = new BinaryWriter(ms);

            writer.Write((byte)CommPacketId.COMM_CUSTOM_APP_DATA);
            writer.Write(LcmPackageId);
            writer.Write(LcmCommandPoll);

            if (enabled)
            {
                writer.Write((byte)0); // state (unused by ALCM)
                writer.Write((byte)0); // fault (unused by ALCM)
                writer.Write((byte)0); // duty/pitch (unused by ALCM)
                WriteInt16(writer, 0); // erpm (unused by ALCM)
                WriteInt16(writer, 0); // avg input current (unused by ALCM)
                WriteInt16(writer, 0); // input voltage (unused by ALCM)
                writer.Write(headlightBrightnessPercent);
                writer.Write(headlightIdleBrightnessPercent);
                writer.Write(statusBrightnessPercent);
            }

            return BuildPacket(ms.ToArray());
        }

        /// <summary>
        /// Builds a complete VESC packet with framing and CRC
        /// </summary>
        private static byte[] BuildPacket(byte[] payload)
        {
            bool useLongPacket = payload.Length > 256;
            int packetSize = (useLongPacket ? 6 : 5) + payload.Length;

            byte[] packet = new byte[packetSize];
            int offset = 0;

            // Start byte
            packet[offset++] = useLongPacket ? StartByteLong : StartByte;

            // Length
            if (useLongPacket)
            {
                packet[offset++] = (byte)((payload.Length >> 8) & 0xFF);
                packet[offset++] = (byte)(payload.Length & 0xFF);
            }
            else
            {
                packet[offset++] = (byte)payload.Length;
            }

            // Payload
            Array.Copy(payload, 0, packet, offset, payload.Length);
            offset += payload.Length;

            // CRC-16 CCITT (over payload only)
            ushort crc = Crc16.Calculate(payload);
            packet[offset++] = (byte)((crc >> 8) & 0xFF);
            packet[offset++] = (byte)(crc & 0xFF);

            // Stop byte
            packet[offset] = StopByte;

            return packet;
        }

        // Helper methods for big-endian write
        private static void WriteInt16(BinaryWriter writer, short value)
        {
            writer.Write((byte)((value >> 8) & 0xFF));
            writer.Write((byte)(value & 0xFF));
        }

        private static void WriteInt32(BinaryWriter writer, int value)
        {
            writer.Write((byte)((value >> 24) & 0xFF));
            writer.Write((byte)((value >> 16) & 0xFF));
            writer.Write((byte)((value >> 8) & 0xFF));
            writer.Write((byte)(value & 0xFF));
        }

        private static void WriteFloat16(BinaryWriter writer, float value, float scale)
        {
            short scaled = (short)(value * scale);
            WriteInt16(writer, scaled);
        }

        private static void WriteFloat32(BinaryWriter writer, float value)
        {
            byte[] bytes = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(bytes);
            }
            writer.Write(bytes);
        }
    }
}
