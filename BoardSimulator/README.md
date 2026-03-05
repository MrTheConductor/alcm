# ALCM BoardSimulator

A Windows-based hardware simulator for the ALCM (Advanced Light Control Module) embedded firmware.

## Overview

BoardSimulator wraps the ALCM embedded firmware in a C library and provides a WPF GUI that simulates the hardware environment, allowing you to test and debug the ALCM without the physical device.

**Key Features:**
- ✅ Zero modifications to ALCM source code
- ✅ Full hardware abstraction layer
- ✅ Real-time, paused, and accelerated time modes
- ✅ Authentic VESC protocol message generation
- ✅ Visual feedback for LEDs, headlights, and sensors
- ✅ Settings persistence via file-based EEPROM emulation

## Project Structure

```
BoardSimulator/
├── ALCM.Library/              # C library wrapping ALCM firmware
│   ├── api/                   # P/Invoke C API
│   ├── hw_impl/              # Windows hardware layer (*_hw.c replacements)
│   ├── hw_interface/         # Callback and input interfaces
│   ├── platform/             # CMSIS/HAL stubs, SysTick simulator
│   │   ├── cmsis/           # ARM CMSIS stubs
│   │   └── hal/             # Peripheral library stubs
│   └── CMakeLists.txt        # Build configuration
│
└── BoardSimulator/           # C# WPF GUI application
    ├── Native/              # P/Invoke declarations
    ├── Services/            # AlcmWrapper, SimulationEngine
    ├── Vesc/               # VESC protocol generator
    ├── ViewModels/         # MVVM view models (to be created)
    └── Views/              # XAML UI (to be created)
```

## Implementation Status

### ✅ Completed
1. **CMSIS/HAL Stub Headers** - Complete ARM Cortex-M0 and peripheral stubs
2. **Platform Layer** - SysTick simulator with time control
3. **Hardware Callback Interface** - Bidirectional C#/C communication
4. **Hardware Layer Implementations** - All *_hw.c files replaced:
   - status_leds_hw.c
   - headlights_hw.c
   - footpads_hw.c
   - button_driver_hw.c
   - vesc_serial_hw.c
   - buzzer_hw.c
   - power_hw.c
   - interrupts_hw.c
   - tim1.c (stub)
   - eeprom.c (file-based persistence)
   - hk32f030m_it.c (interrupt handlers)
5. **CMake Build System** - Ready to compile ALCM as DLL
6. **C API Wrapper** - Complete P/Invoke interface
7. **C# P/Invoke Wrapper** - AlcmWrapper with managed events
8. **VESC Protocol Generator** - Authentic message framing with CRC-16
9. **VESC Simulator** - 10Hz update timer with state management
10. **ViewModels** - MVVM view models with INotifyPropertyChanged (ViewModelBase, StatusLedViewModel, MainViewModel)
11. **MainWindow XAML** - Complete dark-themed UI with LED indicators, sliders, and time controls
12. **ALCM.Library Compilation** - Successfully builds ALCM.Library.dll

### 🚧 Remaining Tasks
1. **Install .NET SDK** - Required for building and running the WPF application
2. **Build BoardSimulator** - Compile the C# WPF application
3. **Integration Testing** - Verify full simulator functionality
4. ~~ViewModels~~ ✅ Completed
5. ~~MainWindow XAML~~ ✅ Completed

## Building

### Prerequisites
- CMake 3.15+
- Visual Studio 2022 or MSVC compiler
- .NET 8.0 SDK

### Build ALCM.Library (C DLL)

```powershell
cd BoardSimulator/ALCM.Library
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Output: `build/bin/ALCM.Library.dll`

### Build BoardSimulator (C# WPF)

```powershell
cd BoardSimulator/BoardSimulator
dotnet build
```

## Architecture

### Hardware Abstraction Strategy

The ALCM firmware already has excellent hardware abstraction with `*_hw.c` files. BoardSimulator:

1. **Excludes** original hardware files from build
2. **Replaces** them with Windows-specific implementations
3. **Stubs** ARM CMSIS and peripheral libraries
4. **Includes** all application logic unchanged

### Callback Flow

```
ALCM Firmware (C) → hw_callbacks.c → alcm_api.c → AlcmInterop.cs → AlcmWrapper.cs → UI (WPF)
```

### Input Flow

```
UI Controls (WPF) → AlcmWrapper.cs → alcm_api.c → hw_inputs.c → ALCM Firmware (C)
```

### VESC Message Flow

```
VescSimulator.cs → VescProtocol.cs → AlcmWrapper.InjectVescData() → vesc_serial.c ring buffer
```

## Time Control

The simulator supports three time modes:

- **Real-time**: 1ms ticks at actual wall-clock speed (default)
- **Paused**: Manual single-step control for debugging
- **Accelerated**: Run faster than real-time for stress testing

## Key Design Decisions

1. **No ALCM Source Modifications**: Build configuration selectively includes files
2. **New Stubs vs. Mock Extension**: Clean separation between unit tests and simulation
3. **Real VESC Protocol**: GUI controls generate authentic binary messages
4. **File-Based EEPROM**: Settings persist in `alcm_eeprom.bin`

## Known Limitations

- WS2812 assembly driver not needed (stubbed in C)
- USART interrupts simulated via direct injection
- ADC readings return GUI-supplied voltages directly
- No actual GPIO/Timer hardware operations

## Next Steps

To use the simulator:

1. **Install .NET 8.0 SDK** from https://dotnet.microsoft.com/download
2. Build the WPF application:
   ```powershell
   cd BoardSimulator/BoardSimulator
   dotnet build
   ```
3. Run the simulator:
   ```powershell
   dotnet run
   ```
4. Test the full functionality:
   - Click "Start" to begin simulation
   - Use "Both Pads ON" to activate footpads
   - Observe status LED boot animation
   - Verify headlights turn on when riding
   - Adjust battery voltage and motor RPM
   - Test different time scales (1x-10x)

## Usage

### Controls

- **Start/Stop/Reset**: Control simulation execution
- **Time Scale**: Adjust simulation speed (1x = real-time, 10x = 10× faster)
- **Left/Right Footpad**: Voltage sliders (0-3.3V) simulate pressure sensors
- **Battery Voltage**: VESC input voltage (40-67.2V)
- **Motor RPM**: VESC motor speed (-2000 to 2000)
- **Both Pads ON/OFF**: Quick preset buttons

### Expected Behavior

- **Boot**: Status LEDs animate in sequence on startup
- **Idle**: System waits for footpad engagement
- **Ready**: Both footpads pressed, headlights turn on
- **Riding**: Motor RPM > threshold, full operational state
- **Error**: Low battery or fault conditions trigger error state

## License

BoardSimulator is a test fixture for ALCM. ALCM is GPL-3.0 licensed.

## Author

Built as a companion tool for the Advanced LCM (ALCM) project by Mitchell White.
