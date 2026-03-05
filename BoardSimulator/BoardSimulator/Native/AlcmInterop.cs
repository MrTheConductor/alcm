using System;
using System.Runtime.InteropServices;

namespace BoardSimulator.Native
{
    /// <summary>
    /// P/Invoke declarations for ALCM.Library.dll
    /// </summary>
    internal static class AlcmInterop
    {
        private const string DllName = "ALCM.Library.dll";
        private const CallingConvention CallConv = CallingConvention.Cdecl;

        // Callback delegates (must match C function pointer types)
        [UnmanagedFunctionPointer(CallConv)]
        public delegate void StatusLedCallback(byte index, byte r, byte g, byte b);

        [UnmanagedFunctionPointer(CallConv)]
        public delegate void HeadlightCallback(byte direction, ushort brightness);

        [UnmanagedFunctionPointer(CallConv)]
        public delegate void BuzzerCallback(ushort frequency, ushort durationMs);

        [UnmanagedFunctionPointer(CallConv)]
        public delegate void PowerCallback([MarshalAs(UnmanagedType.I1)] bool enabled);

        [UnmanagedFunctionPointer(CallConv)]
        public delegate void DebugCallback([MarshalAs(UnmanagedType.LPStr)] string message);

        [UnmanagedFunctionPointer(CallConv)]
        public delegate void VescRequestCallback(IntPtr data, ushort len);

        // Callback registration structure
        [StructLayout(LayoutKind.Sequential)]
        public struct AlcmCallbacks
        {
            public IntPtr StatusLedCallback;
            public IntPtr HeadlightCallback;
            public IntPtr BuzzerCallback;
            public IntPtr PowerCallback;
            public IntPtr DebugCallback;
            public IntPtr VescRequestCallback;
        }

        // Initialization and control
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern int alcm_init();

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_shutdown();

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_register_callbacks(ref AlcmCallbacks callbacks);

        // Simulation control
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_tick(float deltams);

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern int alcm_process_events();

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern uint alcm_get_tick_count();

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_reset_tick_count();

        // Input control
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_set_footpad_voltages(float left, float right);

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_set_button_state([MarshalAs(UnmanagedType.I1)] bool pressed);

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_inject_vesc_data(byte[] data, ushort length);

        // State query (not currently implemented in C, but declared for future use)
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_get_led_state(byte index, out byte r, out byte g, out byte b);

        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_get_headlight_state(out byte direction, out ushort brightness);

        // Testing
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern void alcm_test_debug_callback();
        
        [DllImport(DllName, CallingConvention = CallConv)]
        public static extern int alcm_are_callbacks_registered();
    }
}
