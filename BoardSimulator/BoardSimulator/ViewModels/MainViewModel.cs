using BoardSimulator.Services;
using BoardSimulator.Vesc;
using BoardSimulator.Native;
using System;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace BoardSimulator.ViewModels
{
    public class MainViewModel : ViewModelBase
    {
        private readonly AlcmWrapper _alcm;
        private readonly VescSimulator _vesc;
        private readonly BuzzerService _buzzer;
        private readonly DispatcherTimer _simulationTimer;
        
        // Status LED Array (10 RGB LEDs with diffuser - simulates glowing bar)
        public StatusLedViewModel[] StatusLedArray { get; }
        
        // Headlight LEDs (Front/Rear - color changes with direction)
        public StatusLedViewModel HeadlightFront { get; }
        public StatusLedViewModel HeadlightRear { get; }
        
        // Input properties
        private double _leftFootpadVoltage;
        private double _rightFootpadVoltage;
        private double _batteryVoltage = 58.8;
        private double _motorRpm;
        private double _imuPitch;
        private double _imuRoll;
        private bool _isRunning;
        private int _timeScale = 1;
        private string _statusText = "Not initialized";
        private bool _isButtonPressed;
        private bool _vescEnabled = true;
        private bool _vescPowered = false;
        private bool _vescPowerAlert = false;
        private bool _vescEnableOverride = false;
        private double _vescBootRemainingMs = 0;
        private const double VescBootDelayMs = 2000.0; // real VESC takes ~5s to boot before it responds
        private bool _refloatInstalled = true;
        private bool _externalLedsEnabled = true;
        private bool _locked = false;
        private double _appHeadlightBrightness = 50;
        private double _appStatusBrightness = 50;
        private int _tickCounter = 0;
        private int _eventCounter = 0;

        public double LeftFootpadVoltage
        {
            get => _leftFootpadVoltage;
            set
            {
                if (SetProperty(ref _leftFootpadVoltage, value))
                {
                    UpdateFootpads();
                }
            }
        }

        public double RightFootpadVoltage
        {
            get => _rightFootpadVoltage;
            set
            {
                if (SetProperty(ref _rightFootpadVoltage, value))
                {
                    UpdateFootpads();
                }
            }
        }

        public double BatteryVoltage
        {
            get => _batteryVoltage;
            set
            {
                if (SetProperty(ref _batteryVoltage, value))
                {
                    _vesc.InputVoltage = (float)value;
                }
            }
        }

        public double MotorRpm
        {
            get => _motorRpm;
            set
            {
                if (SetProperty(ref _motorRpm, value))
                {
                    _vesc.Rpm = (int)value;
                }
            }
        }

        public double ImuPitch
        {
            get => _imuPitch;
            set
            {
                if (SetProperty(ref _imuPitch, value))
                {
                    _vesc.ImuPitch = (float)value;
                }
            }
        }

        public double ImuRoll
        {
            get => _imuRoll;
            set
            {
                if (SetProperty(ref _imuRoll, value))
                {
                    _vesc.ImuRoll = (float)value;
                }
            }
        }

        public bool IsRunning
        {
            get => _isRunning;
            set => SetProperty(ref _isRunning, value);
        }

        public int TimeScale
        {
            get => _timeScale;
            set => SetProperty(ref _timeScale, value);
        }

        public string StatusText
        {
            get => _statusText;
            set => SetProperty(ref _statusText, value);
        }

        public bool IsButtonPressed
        {
            get => _isButtonPressed;
            set
            {
                if (SetProperty(ref _isButtonPressed, value))
                {
                    _alcm.SetButtonState(value);
                }
            }
        }

        public bool VescEnabled
        {
            get => _vescEnabled;
            set
            {
                if (SetProperty(ref _vescEnabled, value))
                {
                    // Enable or disable VESC simulator based on toggle
                    if (value)
                    {
                        _vesc.Enable();
                        System.Diagnostics.Debug.WriteLine("[MainViewModel] VESC enabled - will respond to requests");
                    }
                    else
                    {
                        _vesc.Disable();
                        System.Diagnostics.Debug.WriteLine("[MainViewModel] VESC disabled - will not respond to requests");
                    }
                }
            }
        }

        // Reflects the real PWR_EN GPIO (power_hw_set_power). The LCM should
        // only ever drive this low as part of an intentional shutdown - any
        // other transition to off means the VESC just lost power mid-ride.
        public bool VescPowered
        {
            get => _vescPowered;
            private set => SetProperty(ref _vescPowered, value);
        }

        // True whenever VESC power has been cut - a critical safety event,
        // since a powered-off VESC can't hold the rider up. Cleared when
        // power is restored.
        public bool VescPowerAlert
        {
            get => _vescPowerAlert;
            private set => SetProperty(ref _vescPowerAlert, value);
        }

        // When off (default), VescEnabled is driven automatically by
        // VescPowered plus the ~5s boot delay, matching real hardware. When
        // on, the VESC Enabled checkbox can be toggled directly for testing.
        public bool VescEnableOverride
        {
            get => _vescEnableOverride;
            set
            {
                if (SetProperty(ref _vescEnableOverride, value) && !value)
                {
                    // Handing control back to the automatic state machine -
                    // resync immediately instead of waiting for the next tick.
                    VescEnabled = _vescPowered && _vescBootRemainingMs <= 0;
                }
            }
        }

        // Fake phone app (refloat COMMAND_LCM_POLL simulator)
        public bool RefloatInstalled
        {
            get => _refloatInstalled;
            set
            {
                if (SetProperty(ref _refloatInstalled, value))
                {
                    _vesc.RefloatInstalled = value;
                }
            }
        }

        public bool ExternalLedsEnabled
        {
            get => _externalLedsEnabled;
            set
            {
                if (SetProperty(ref _externalLedsEnabled, value))
                {
                    _vesc.ExternalLedsEnabled = value;
                }
            }
        }

        // refloat's phone-app "Lock" feature (RunState.STATE_DISABLED)
        public bool Locked
        {
            get => _locked;
            set
            {
                if (SetProperty(ref _locked, value))
                {
                    _vesc.Locked = value;
                }
            }
        }

        public double AppHeadlightBrightness
        {
            get => _appHeadlightBrightness;
            set
            {
                if (SetProperty(ref _appHeadlightBrightness, value))
                {
                    _vesc.HeadlightBrightnessPercent = (byte)value;
                }
            }
        }

        public double AppStatusBrightness
        {
            get => _appStatusBrightness;
            set
            {
                if (SetProperty(ref _appStatusBrightness, value))
                {
                    _vesc.StatusBrightnessPercent = (byte)value;
                }
            }
        }

        // Commands
        public ICommand StartCommand { get; }
        public ICommand StopCommand { get; }
        public ICommand ResetCommand { get; }
        public ICommand ButtonPressCommand { get; }
        public ICommand ButtonReleaseCommand { get; }
        public ICommand SilenceAlarmCommand { get; }

        public MainViewModel()
        {
            _alcm = new AlcmWrapper();
            _vesc = new VescSimulator();
            _buzzer = new BuzzerService();

            // Initialize 10-LED status array (RGB LEDs with diffuser effect)
            StatusLedArray = new StatusLedViewModel[10];
            for (int i = 0; i < 10; i++)
            {
                StatusLedArray[i] = new StatusLedViewModel(Colors.Black);
            }

            // Initialize headlight LEDs (base color will change based on direction)
            HeadlightFront = new StatusLedViewModel(Colors.White);
            HeadlightRear = new StatusLedViewModel(Colors.White);

            // Subscribe to ALCM events
            _alcm.StatusLedChanged += OnStatusLedChanged;
            _alcm.HeadlightChanged += OnHeadlightChanged;
            _alcm.BuzzerTriggered += OnBuzzerTriggered;
            _alcm.DebugMessage += OnDebugMessage;
            _alcm.PowerChanged += OnPowerChanged;
            _alcm.VescRequest += OnVescRequest; // ALCM → VESC requests

            // Subscribe to VESC responses
            _vesc.ResponseReady += OnVescResponse; // VESC → ALCM responses

            // Create simulation timer - run at higher frequency for smooth LED animations
            // Animation updates happen every 25ms, so we want to tick more frequently
            _simulationTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(5) // 200 Hz for smooth animations
            };
            _simulationTimer.Tick += SimulationTick;

            // Initialize commands
            StartCommand = new RelayCommand(Start, () => !IsRunning);
            StopCommand = new RelayCommand(Stop, () => IsRunning);
            ResetCommand = new RelayCommand(Reset);
            ButtonPressCommand = new RelayCommand(() => IsButtonPressed = true);
            ButtonReleaseCommand = new RelayCommand(() => IsButtonPressed = false);
            SilenceAlarmCommand = new RelayCommand(() => _buzzer.StopAlarm());

            // Initialize ALCM
            Initialize();
        }

        private void Initialize()
        {
            try
            {
                System.Diagnostics.Debug.WriteLine("[MainViewModel] Starting initialization...");
                
                bool success = _alcm.Initialize();
                System.Diagnostics.Debug.WriteLine($"[MainViewModel] Initialize returned: {success}");
                
                // Check if callbacks were registered
                int callbackCount = AlcmInterop.alcm_are_callbacks_registered();
                System.Diagnostics.Debug.WriteLine($"[MainViewModel] Callbacks registered: {callbackCount}/5");
                
                if (success)
                {
                    // NOTE: VescEnabled is no longer forced here - alcm_init()
                    // above already fired OnPowerChanged(true) synchronously
                    // (power_hw_init() turns PWR_EN on), which started the
                    // ~5s boot delay. The VESC won't actually respond until
                    // that elapses (or VescEnableOverride is set), same as
                    // real hardware.

                    // Give VESC a moment to generate first message, then tick simulation to process it
                    System.Threading.Thread.Sleep(150); // Wait for at least one VESC message (100ms interval)
                    
                    // Tick simulation and process events to:
                    // 1. Process boot event -> transitions to BOOTING mode
                    // 2. Start boot animation timer (fires every 25ms)
                    // 3. Process VESC message -> sends EVENT_VESC_ALIVE -> transitions to IDLE
                    // 4. Advance time to let animation timer fire and update LEDs
                    System.Diagnostics.Debug.WriteLine("[MainViewModel] Processing boot sequence...");
                    for (int i = 0; i < 40; i++) // 40 ticks * 5ms = 200ms
                    {
                        _alcm.Tick(5.0f); // Advance time by 5ms (animation timer fires every 25ms)
                        
                        int eventsProcessed = 0;
                        while (_alcm.ProcessEvents() && eventsProcessed < 20)
                        {
                            eventsProcessed++;
                        }
                    }
                    
                    // Get initial tick count to verify API is working
                    uint ticks = _alcm.GetTickCount();
                    StatusText = $"ALCM initialized - Ready to start simulation (Ticks: {ticks}ms)";
                    System.Diagnostics.Debug.WriteLine($"[MainViewModel] Boot complete. ALCM time: {ticks}ms");
                }
                else
                {
                    StatusText = "ALCM initialization returned false";
                }
            }
            catch (Exception ex)
            {
                StatusText = $"Initialization failed: {ex.Message}";
                System.Diagnostics.Debug.WriteLine($"[MainViewModel] Exception: {ex}");
            }
        }

        private void Start()
        {
            IsRunning = true;
            _tickCounter = 0;
            _eventCounter = 0;
            _simulationTimer.Start();
            StatusText = $"Running (Time scale: {TimeScale}x)";
        }

        private void Stop()
        {
            IsRunning = false;
            _simulationTimer.Stop();
            StatusText = "Paused";
        }

        private void Reset()
        {
            Stop();
            
            // Reset inputs
            LeftFootpadVoltage = 0;
            RightFootpadVoltage = 0;
            BatteryVoltage = 58.8;
            MotorRpm = 0;
            ImuPitch = 0;
            ImuRoll = 0;

            // Re-initialize ALCM
            Initialize();
            
            StatusText = "Reset complete";
        }

        private void SimulationTick(object? sender, EventArgs e)
        {
            try
            {
                // Advance time based on time scale
                // 5ms real time * time scale (timer runs at 200Hz)
                float deltaMs = 5.0f * TimeScale;
                _alcm.Tick(deltaMs);
                _tickCounter++;

                // Count down the simulated VESC boot delay; once it elapses,
                // auto-enable the VESC simulator (unless a tester has taken
                // manual control via VescEnableOverride).
                if (_vescBootRemainingMs > 0 && !VescEnableOverride)
                {
                    _vescBootRemainingMs -= deltaMs;
                    if (_vescBootRemainingMs <= 0)
                    {
                        _vescBootRemainingMs = 0;
                        VescEnabled = true;
                    }
                }

                // Process all pending events
                int eventsProcessed = 0;
                while (_alcm.ProcessEvents() && eventsProcessed < 100)
                {
                    eventsProcessed++;
                    _eventCounter++;
                }

                // Update status every 200 ticks (~1 second at 1x speed)
                if (_tickCounter % 200 == 0)
                {
                    uint ticks = _alcm.GetTickCount();
                    StatusText = $"Running - Ticks: {_tickCounter} | ALCM Time: {ticks}ms | Events: {_eventCounter} | Button: {(IsButtonPressed ? "PRESSED" : "Released")}";
                }
            }
            catch (Exception ex)
            {
                StatusText = $"Simulation error: {ex.Message}";
                Stop();
            }
        }

        // VESC Request/Response Handlers (reactive protocol)
        
        private void OnVescRequest(byte[] request)
        {
            // ALCM → VESC: Forward request to VESC simulator for processing
            _vesc.HandleRequest(request);
        }

        private void OnVescResponse(byte[] response)
        {
            try
            {
                // VESC → ALCM: Inject response back into ALCM
                _alcm.InjectVescData(response);
            }
            catch (Exception ex)
            {
                StatusText = $"VESC error: {ex.Message}";
                System.Diagnostics.Debug.WriteLine($"[MainViewModel] VESC injection error: {ex}");
            }
        }

        private void UpdateFootpads()
        {
            _alcm.SetFootpadVoltages((float)LeftFootpadVoltage, (float)RightFootpadVoltage);
        }

        private void OnStatusLedChanged(byte index, byte r, byte g, byte b)
        {
            System.Diagnostics.Debug.WriteLine($"[MainViewModel] LED {index}: R={r}, G={g}, B={b}");
            
            // Marshal to UI thread for WPF property updates
            System.Windows.Application.Current.Dispatcher.Invoke(() =>
            {
                if (index < StatusLedArray.Length)
                {
                    var led = StatusLedArray[index];
                    led.IsOn = (r > 0 || g > 0 || b > 0);
                    led.Brightness = 100.0; // Full brightness, color already has intensity baked in
                    led.Color = Color.FromRgb(r, g, b);
                }
            });
        }

        private void OnHeadlightChanged(byte direction, ushort brightness)
        {
            System.Diagnostics.Debug.WriteLine($"[MainViewModel] Headlight direction={direction}: Brightness={brightness}");
            
            // HEADLIGHTS_DIRECTION_FORWARD = 0, HEADLIGHTS_DIRECTION_REVERSE = 1, HEADLIGHTS_DIRECTION_NONE = 2
            // TIM1_PERIOD = 1024 (max brightness)
            // When FORWARD: Front=WHITE, Rear=RED
            // When REVERSE: Front=RED, Rear=WHITE
            
            // Marshal to UI thread for WPF property updates
            System.Windows.Application.Current.Dispatcher.Invoke(() =>
            {
                // Normalize brightness: 0-1024 → 0.0-1.0
                double normalizedBrightness = Math.Min(brightness / 1024.0, 1.0);
                bool isOn = brightness > 0;
                
                if (direction == 0) // HEADLIGHTS_DIRECTION_FORWARD
                {
                    // Front = WHITE, Rear = RED
                    HeadlightFront.IsOn = isOn;
                    HeadlightFront.Color = Colors.White;
                    HeadlightFront.Brightness = normalizedBrightness * 100.0;
                    
                    HeadlightRear.IsOn = isOn;
                    HeadlightRear.Color = Colors.Red;
                    HeadlightRear.Brightness = normalizedBrightness * 100.0;
                }
                else if (direction == 1) // HEADLIGHTS_DIRECTION_REVERSE
                {
                    // Front = RED, Rear = WHITE
                    HeadlightFront.IsOn = isOn;
                    HeadlightFront.Color = Colors.Red;
                    HeadlightFront.Brightness = normalizedBrightness * 100.0;
                    
                    HeadlightRear.IsOn = isOn;
                    HeadlightRear.Color = Colors.White;
                    HeadlightRear.Brightness = normalizedBrightness * 100.0;
                }
                else // HEADLIGHTS_DIRECTION_NONE or invalid
                {
                    // Both off
                    HeadlightFront.IsOn = false;
                    HeadlightFront.Brightness = 0.0;
                    
                    HeadlightRear.IsOn = false;
                    HeadlightRear.Brightness = 0.0;
                }
            });
        }

        private void OnBuzzerTriggered(ushort frequency, ushort durationMs)
        {
            // frequency=0 means buzzer off, otherwise play tone
            _buzzer.SetTone(frequency);
        }

        // Fires whenever power_hw_set_power() is called. Under normal
        // firmware operation the VESC is powered up once at boot and only
        // loses power once more, at shutdown - it should NEVER lose power
        // any other way, since that leaves the rider unsupported. So any
        // ON->OFF transition gets a loud, hard-to-miss alert here regardless
        // of why it happened.
        //
        // NOTE: power_init() (power.c) deliberately calls
        // power_hw_set_power(POWER_HW_OFF) once at startup to force the pin
        // to a known state before board_mode transitions it ON a moment
        // later - that first OFF is not a real event, so the alert only
        // fires on a transition away from an already-powered state.
        private void OnPowerChanged(bool enabled)
        {
            System.Windows.Application.Current.Dispatcher.Invoke(() =>
            {
                System.Diagnostics.Debug.WriteLine($"[MainViewModel] VESC power changed: {(enabled ? "ON" : "OFF")}");

                bool wasPowered = VescPowered;
                VescPowered = enabled;

                if (enabled)
                {
                    VescPowerAlert = false;
                    _buzzer.StopAlarm();

                    // Real VESC firmware takes ~5s to boot before it will
                    // respond on the UART bus.
                    _vescBootRemainingMs = VescBootDelayMs;
                    if (!VescEnableOverride)
                    {
                        VescEnabled = false;
                    }

                    StatusText = "VESC power ON - waiting for VESC to boot...";
                }
                else
                {
                    _vescBootRemainingMs = 0;
                    if (!VescEnableOverride)
                    {
                        VescEnabled = false;
                    }

                    if (wasPowered)
                    {
                        VescPowerAlert = true;
                        _buzzer.StartAlarm();
                        StatusText = "*** VESC POWER OFF - RIDER SAFETY HAZARD ***";
                    }
                    else
                    {
                        StatusText = "VESC power off (not yet powered on)";
                    }
                }
            });
        }

        private void OnDebugMessage(string message)
        {
            // Debug messages already visible in Debug output window
            // Don't overwrite StatusText to preserve simulation status
            System.Diagnostics.Debug.WriteLine($"[MainViewModel] C Debug: {message}");
        }

        public void Dispose()
        {
            Stop();
            _buzzer?.StopAlarm();
            _buzzer?.Dispose();
            _alcm?.Dispose();
        }
    }

    // Simple RelayCommand implementation
    public class RelayCommand : ICommand
    {
        private readonly Action _execute;
        private readonly Func<bool>? _canExecute;

        public RelayCommand(Action execute, Func<bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public event EventHandler? CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

        public void Execute(object? parameter) => _execute();
    }
}
