using System;

namespace BoardSimulator.Vesc
{
    /// <summary>
    /// Simulates a VESC motor controller - REACTIVE mode
    /// Listens for COMM_GET_VALUES_SETUP_SELECTIVE requests from ALCM
    /// and responds with current state
    /// </summary>
    public class VescSimulator
    {
        // VESC state (set by GUI controls)
        public float TempFet { get; set; } = 25.0f;
        public float TempMotor { get; set; } = 25.0f;
        public float AvgMotorCurrent { get; set; } = 0.0f;
        public float AvgInputCurrent { get; set; } = 0.0f;
        public float DutyCycle { get; set; } = 0.0f;
        public int Rpm { get; set; } = 0;
        public float InputVoltage { get; set; } = 58.8f;
        public float AmpHours { get; set; } = 0.0f;
        public float AmpHoursCharged { get; set; } = 0.0f;
        public float WattHours { get; set; } = 0.0f;
        public float WattHoursCharged { get; set; } = 0.0f;
        public byte FaultCode { get; set; } = 0;
        public byte VescId { get; set; } = 0;

        // IMU state, in degrees (converted to radians on the wire, matching
        // vesc_serial.c's RADIANS_TO_DEGREES conversion on receive)
        public float ImuPitch { get; set; } = 0.0f;
        public float ImuRoll { get; set; } = 0.0f;

        // refloat app-integration state (COMMAND_LCM_POLL), simulating the
        // phone app's headlight/status brightness sliders.
        //
        // RefloatInstalled simulates a VESC that doesn't run refloat at all
        // (an older/stock firmware, or a different balance app) - when
        // false, the VESC doesn't respond to COMM_CUSTOM_APP_DATA at all,
        // exactly like any other command it doesn't recognize.
        //
        // ExternalLedsEnabled mirrors refloat's own hardware.leds.mode
        // setting having "External" checked - refloat IS installed and
        // responds, but when this is off it only sends the 2-byte
        // package/command header (no brightness bytes), matching
        // lcm_poll_response()'s behavior when lcm->enabled is false.
        public bool RefloatInstalled { get; set; } = true;
        public bool ExternalLedsEnabled { get; set; } = true;
        public byte HeadlightBrightnessPercent { get; set; } = 50;
        public byte HeadlightIdleBrightnessPercent { get; set; } = 20;
        public byte StatusBrightnessPercent { get; set; } = 50;

        // Whether refloat reports the board as locked (phone-app "Lock"
        // feature, RunState.STATE_DISABLED) in the poll reply's state byte.
        public bool Locked { get; set; } = false;

        // Event fired when VESC has a response ready
        public event Action<byte[]>? ResponseReady;

        private bool _enabled = false;

        public bool IsEnabled => _enabled;

        /// <summary>
        /// Enable VESC - starts responding to requests
        /// </summary>
        public void Enable()
        {
            _enabled = true;
        }

        /// <summary>
        /// Disable VESC - stops responding to requests
        /// </summary>
        public void Disable()
        {
            _enabled = false;
        }

        /// <summary>
        /// Handle incoming request from ALCM
        /// Decodes request and generates appropriate response(s)
        ///
        /// When ENABLE_IMU_EVENTS is on, vesc_serial.c's polling timer
        /// concatenates the COMM_GET_VALUES_SETUP_SELECTIVE and
        /// COMM_GET_IMU_DATA requests into a single hardware write, so a
        /// single call here may contain more than one VESC frame. Walk the
        /// buffer and dispatch each frame found.
        /// </summary>
        public void HandleRequest(byte[] request)
        {
            if (!_enabled)
            {
                // VESC is disabled, don't respond
                return;
            }

            // Frame format:
            // byte 0: start byte (0x02)
            // byte 1: payload length
            // byte 2: command
            // bytes 3..N: payload
            // bytes N+1,N+2: CRC-16
            // byte N+3: end byte (0x03)

            int offset = 0;
            while (offset < request.Length)
            {
                if (request[offset] != 0x02)
                {
                    System.Diagnostics.Debug.WriteLine("[VescSimulator] Invalid request packet (bad start byte)");
                    return;
                }

                if (offset + 2 > request.Length)
                {
                    System.Diagnostics.Debug.WriteLine("[VescSimulator] Truncated request (missing length byte)");
                    return;
                }

                byte payloadLength = request[offset + 1];
                int frameLength = 2 + payloadLength + 3; // start + length + payload + crc(2) + end
                if (payloadLength == 0 || offset + frameLength > request.Length)
                {
                    System.Diagnostics.Debug.WriteLine("[VescSimulator] Truncated request packet");
                    return;
                }

                byte[] payload = new byte[payloadLength];
                Array.Copy(request, offset + 2, payload, 0, payloadLength);
                HandleCommand(payload);

                offset += frameLength;
            }
        }

        private void HandleCommand(byte[] payload)
        {
            byte command = payload[0];

            if (command == 0x33) // COMM_GET_VALUES_SETUP_SELECTIVE
            {
                // Calculate battery percentage from voltage (67.2V = 100%, 40V = 0%)
                float batteryPercent = ((InputVoltage - 40.0f) / (67.2f - 40.0f)) * 100.0f;
                batteryPercent = Math.Max(0.0f, Math.Min(100.0f, batteryPercent));

                // Generate SELECTIVE response (16-byte payload, not full 64-byte response)
                byte[] response = VescProtocol.GenerateSelectiveValuesMessage(
                    dutyCycle: DutyCycle,
                    rpm: Rpm,
                    inputVoltage: InputVoltage,
                    batteryLevel: batteryPercent,
                    faultCode: FaultCode
                );

                System.Diagnostics.Debug.WriteLine($"[VescSimulator] VESC → ALCM response: {response.Length} bytes (RPM={Rpm}, V={InputVoltage:F1}V, Batt={batteryPercent:F1}%, Duty={DutyCycle:F2})");

                ResponseReady?.Invoke(response);
            }
            else if (command == 0x41) // COMM_GET_IMU_DATA
            {
                const float DegToRad = (float)(Math.PI / 180.0);

                byte[] response = VescProtocol.GenerateImuDataMessage(
                    rollRadians: ImuRoll * DegToRad,
                    pitchRadians: ImuPitch * DegToRad
                );

                System.Diagnostics.Debug.WriteLine($"[VescSimulator] VESC → ALCM IMU response: {response.Length} bytes (Pitch={ImuPitch:F1}°, Roll={ImuRoll:F1}°)");

                ResponseReady?.Invoke(response);
            }
            else if (command == 0x24) // COMM_CUSTOM_APP_DATA
            {
                if (!RefloatInstalled)
                {
                    // Simulates a VESC running firmware that doesn't understand
                    // COMM_CUSTOM_APP_DATA at all - no reply of any kind.
                    System.Diagnostics.Debug.WriteLine("[VescSimulator] refloat not installed, ignoring COMM_CUSTOM_APP_DATA");
                }
                // refloat's custom app data protocol: byte 1 is the package
                // id (101 for refloat), byte 2 is the command id. ALCM only
                // ever sends COMMAND_LCM_POLL (24); anything else is ignored,
                // matching real refloat behavior of not responding to
                // commands it doesn't recognize.
                else if (payload.Length >= 3 && payload[1] == 101 && payload[2] == 24) // COMMAND_LCM_POLL
                {
                    byte[] response = VescProtocol.GenerateLcmPollResponseMessage(
                        enabled: ExternalLedsEnabled,
                        headlightBrightnessPercent: HeadlightBrightnessPercent,
                        headlightIdleBrightnessPercent: HeadlightIdleBrightnessPercent,
                        statusBrightnessPercent: StatusBrightnessPercent,
                        locked: Locked
                    );

                    System.Diagnostics.Debug.WriteLine($"[VescSimulator] VESC → ALCM LCM poll response: {response.Length} bytes (external_leds={ExternalLedsEnabled}, locked={Locked}, headlight={HeadlightBrightnessPercent}%, idle={HeadlightIdleBrightnessPercent}%, status={StatusBrightnessPercent}%)");

                    ResponseReady?.Invoke(response);
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine("[VescSimulator] Unrecognized COMM_CUSTOM_APP_DATA command, ignoring");
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"[VescSimulator] Unknown command: 0x{command:X2}");
            }
        }
    }
}
