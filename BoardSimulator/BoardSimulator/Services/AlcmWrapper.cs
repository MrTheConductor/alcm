using System;
using System.Runtime.InteropServices;
using BoardSimulator.Native;

namespace BoardSimulator.Services
{
    /// <summary>
    /// Managed wrapper for the ALCM library
    /// Handles DLL lifecycle and provides .NET-friendly API
    /// </summary>
    public class AlcmWrapper : IDisposable
    {
        private bool _initialized;
        private bool _disposed;

        // Keep delegates alive to prevent garbage collection
        private AlcmInterop.StatusLedCallback? _statusLedCallback;
        private AlcmInterop.HeadlightCallback? _headlightCallback;
        private AlcmInterop.BuzzerCallback? _buzzerCallback;
        private AlcmInterop.PowerCallback? _powerCallback;
        private AlcmInterop.DebugCallback? _debugCallback;
        private AlcmInterop.VescRequestCallback? _vescRequestCallback;

        // Events for GUI to subscribe to
        public event Action<byte, byte, byte, byte>? StatusLedChanged;
        public event Action<byte, ushort>? HeadlightChanged;
        public event Action<ushort, ushort>? BuzzerTriggered;
        public event Action<bool>? PowerChanged;
        public event Action<string>? DebugMessage;
        public event Action<byte[]>? VescRequest; // ALCM → VESC request

        public bool Initialize()
        {
            if (_initialized)
                return true;

            System.Diagnostics.Debug.WriteLine("[AlcmWrapper] Creating callback delegates...");

            // Create callback delegates
            _statusLedCallback = OnStatusLedChanged;
            _headlightCallback = OnHeadlightChanged;
            _buzzerCallback = OnBuzzerTriggered;
            _powerCallback = OnPowerChanged;
            _debugCallback = OnDebugMessage;
            _vescRequestCallback = OnVescRequest;

            System.Diagnostics.Debug.WriteLine("[AlcmWrapper] Getting function pointers...");

            // Register callbacks with native library
            var callbacks = new AlcmInterop.AlcmCallbacks
            {
                StatusLedCallback = Marshal.GetFunctionPointerForDelegate(_statusLedCallback),
                HeadlightCallback = Marshal.GetFunctionPointerForDelegate(_headlightCallback),
                BuzzerCallback = Marshal.GetFunctionPointerForDelegate(_buzzerCallback),
                PowerCallback = Marshal.GetFunctionPointerForDelegate(_powerCallback),
                DebugCallback = Marshal.GetFunctionPointerForDelegate(_debugCallback),
                VescRequestCallback = Marshal.GetFunctionPointerForDelegate(_vescRequestCallback)
            };

            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper] Function pointers: Status={callbacks.StatusLedCallback:X}, Debug={callbacks.DebugCallback:X}");
            System.Diagnostics.Debug.WriteLine("[AlcmWrapper] Registering callbacks with C library...");

            AlcmInterop.alcm_register_callbacks(ref callbacks);

            System.Diagnostics.Debug.WriteLine("[AlcmWrapper] Testing debug callback...");

            // TEST: Verify debug callback works
            AlcmInterop.alcm_test_debug_callback();

            System.Diagnostics.Debug.WriteLine("[AlcmWrapper] Calling alcm_init()...");

            // Initialize ALCM
            int result = AlcmInterop.alcm_init();
            _initialized = (result == 0);

            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper] alcm_init() returned: {result}, initialized={_initialized}");

            return _initialized;
        }

        public void Tick(float deltaMs)
        {
            if (!_initialized)
                return;

            AlcmInterop.alcm_tick(deltaMs);
        }

        public bool ProcessEvents()
        {
            if (!_initialized)
                return false;

            return AlcmInterop.alcm_process_events() == 0;
        }

        public uint GetTickCount()
        {
            return AlcmInterop.alcm_get_tick_count();
        }

        public void ResetTickCount()
        {
            AlcmInterop.alcm_reset_tick_count();
        }

        public void SetFootpadVoltages(float left, float right)
        {
            if (!_initialized)
                return;

            AlcmInterop.alcm_set_footpad_voltages(left, right);
        }

        public void SetButtonState(bool pressed)
        {
            if (!_initialized)
                return;

            AlcmInterop.alcm_set_button_state(pressed);
        }

        public void InjectVescData(byte[] data)
        {
            if (!_initialized || data == null)
                return;

            AlcmInterop.alcm_inject_vesc_data(data, (ushort)data.Length);
        }

        // Callback handlers - marshal to managed events
        private void OnStatusLedChanged(byte index, byte r, byte g, byte b)
        {
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnStatusLedChanged] Called: index={index}, r={r}, g={g}, b={b}");
            StatusLedChanged?.Invoke(index, r, g, b);
        }

        private void OnHeadlightChanged(byte direction, ushort brightness)
        {
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnHeadlightChanged] Called: direction={direction}, brightness={brightness}");
            HeadlightChanged?.Invoke(direction, brightness);
        }

        private void OnBuzzerTriggered(ushort frequency, ushort durationMs)
        {
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnBuzzerTriggered] Called: freq={frequency}, duration={durationMs}");
            BuzzerTriggered?.Invoke(frequency, durationMs);
        }

        private void OnPowerChanged(bool enabled)
        {
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnPowerChanged] Called: enabled={enabled}");
            PowerChanged?.Invoke(enabled);
        }

        private void OnDebugMessage(string message)
        {
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnDebugMessage] Called: {message}");
            DebugMessage?.Invoke(message);
        }

        private void OnVescRequest(IntPtr data, ushort len)
        {
            // Marshal unmanaged byte array to managed
            byte[] managedData = new byte[len];
            Marshal.Copy(data, managedData, 0, len);
            
            System.Diagnostics.Debug.WriteLine($"[AlcmWrapper.OnVescRequest] ALCM → VESC request: {len} bytes");
            VescRequest?.Invoke(managedData);
        }

        public void Dispose()
        {
            if (_disposed)
                return;

            if (_initialized)
            {
                AlcmInterop.alcm_shutdown();
                _initialized = false;
            }

            _disposed = true;
            GC.SuppressFinalize(this);
        }

        ~AlcmWrapper()
        {
            Dispose();
        }
    }
}
