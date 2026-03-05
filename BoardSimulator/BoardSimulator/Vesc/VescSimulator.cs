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
        /// Decodes request and generates appropriate response
        /// </summary>
        public void HandleRequest(byte[] request)
        {
            if (!_enabled)
            {
                // VESC is disabled, don't respond
                return;
            }

            // Decode VESC protocol
            // Expected format:
            // byte 0: start byte (0x02)
            // byte 1: packet length
            // byte 2: command
            // bytes 3..N: payload
            // bytes N+1,N+2: CRC-16
            // byte N+3: end byte (0x03)

            if (request.Length < 4 || request[0] != 0x02)
            {
                System.Diagnostics.Debug.WriteLine("[VescSimulator] Invalid request packet");
                return;
            }

            byte command = request[2];

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
                // For now, don't respond to IMU requests (optional feature)
                System.Diagnostics.Debug.WriteLine("[VescSimulator] IMU request ignored (not implemented)");
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"[VescSimulator] Unknown command: 0x{command:X2}");
            }
        }
    }
}
