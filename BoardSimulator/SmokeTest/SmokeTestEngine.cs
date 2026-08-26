using BoardSimulator.Native;
using BoardSimulator.Services;
using BoardSimulator.Vesc;

namespace SmokeTest;

// ── Hardware event types ─────────────────────────────────────────────────────

enum HwEventKind { StatusLed, Headlight, Buzzer, Power, Debug, VescRequest }

/// Represents a single hardware event emitted by the ALCM firmware.
class HwEvent
{
    public HwEventKind Kind     { get; init; }
    public double      SimTimeMs { get; init; }

    // StatusLed fields
    public byte LedIndex { get; init; }
    public byte R        { get; init; }
    public byte G        { get; init; }
    public byte B        { get; init; }

    // Headlight fields
    public byte   HeadlightDirection  { get; init; }
    public ushort HeadlightBrightness { get; init; }

    // Power fields
    public bool PowerEnabled { get; init; }

    // Debug fields
    public string? DebugMessage { get; init; }
}

// ── Smoke test engine ────────────────────────────────────────────────────────

/// Thin synchronous wrapper around AlcmWrapper + VescSimulator.
/// Drives the tick loop directly (no DispatcherTimer) and surfaces all
/// hardware events through a single EventFired event.
///
/// Events fired during Initialize() (inside alcm_init()) are captured in a
/// replay buffer and replayed to the first RunUntil() caller, so boot-sequence
/// events (power latch, headlight direction, etc.) are not silently dropped.
class SmokeTestEngine : IDisposable
{
    private readonly AlcmWrapper   _alcm = new();
    private readonly VescSimulator _vesc = new();
    private double _simTimeMs;

    // Captures events that fire before the first RunUntil() subscribes.
    // Set to null once consumed or explicitly discarded.
    private Queue<HwEvent>? _replayBuffer = new();

    /// Total simulated time advanced so far.
    public double SimTimeMs => _simTimeMs;

    /// Fires synchronously on every hardware callback from the C firmware.
    public event Action<HwEvent>? EventFired;

    public SmokeTestEngine()
    {
        _alcm.StatusLedChanged += (i, r, g, b) =>
            Fire(new HwEvent { Kind = HwEventKind.StatusLed, SimTimeMs = _simTimeMs,
                               LedIndex = i, R = r, G = g, B = b });

        _alcm.HeadlightChanged += (d, br) =>
            Fire(new HwEvent { Kind = HwEventKind.Headlight, SimTimeMs = _simTimeMs,
                               HeadlightDirection = d, HeadlightBrightness = br });

        _alcm.BuzzerTriggered += (_, _) =>
            Fire(new HwEvent { Kind = HwEventKind.Buzzer, SimTimeMs = _simTimeMs });

        _alcm.PowerChanged += en =>
            Fire(new HwEvent { Kind = HwEventKind.Power, SimTimeMs = _simTimeMs, PowerEnabled = en });

        _alcm.DebugMessage += msg =>
            Fire(new HwEvent { Kind = HwEventKind.Debug, SimTimeMs = _simTimeMs, DebugMessage = msg });

        // VESC request/response: ALCM → VescSimulator → ALCM (synchronous, ring-buffer safe)
        _alcm.VescRequest += req =>
        {
            Fire(new HwEvent { Kind = HwEventKind.VescRequest, SimTimeMs = _simTimeMs });
            _vesc.HandleRequest(req);
        };
        _vesc.ResponseReady += resp => _alcm.InjectVescData(resp);
    }

    void Fire(HwEvent e)
    {
        // Buffer events that fire before the first RunUntil() call
        _replayBuffer?.Enqueue(e);
        EventFired?.Invoke(e);
    }

    /// Initialize ALCM and enable the VESC simulator with the given battery voltage.
    public bool Initialize(float batteryVoltage = 58.8f)
    {
        _vesc.Enable();
        _vesc.InputVoltage = batteryVoltage;
        return _alcm.Initialize();
    }

    public void SetFootpadVoltages(float left, float right) => _alcm.SetFootpadVoltages(left, right);
    public void SetRpm(int rpm)              => _vesc.Rpm = rpm;
    public void SetBatteryVoltage(float v)   => _vesc.InputVoltage = v;
    public void SetButtonState(bool pressed) => _alcm.SetButtonState(pressed);
    public void SetImuPitch(float degrees)   => _vesc.ImuPitch = degrees;
    public void SetImuRoll(float degrees)    => _vesc.ImuRoll = degrees;

    // refloat app-integration (COMMAND_LCM_POLL) simulator state
    public void SetRefloatInstalled(bool installed)            => _vesc.RefloatInstalled = installed;
    public void SetExternalLedsEnabled(bool enabled)           => _vesc.ExternalLedsEnabled = enabled;
    public void SetLcmHeadlightBrightnessPercent(byte pct)     => _vesc.HeadlightBrightnessPercent = pct;
    public void SetLcmHeadlightIdleBrightnessPercent(byte pct) => _vesc.HeadlightIdleBrightnessPercent = pct;
    public void SetLcmStatusBrightnessPercent(byte pct)        => _vesc.StatusBrightnessPercent = pct;
    public void SetLocked(bool locked)                          => _vesc.Locked = locked;

    /// Advance simulation until <paramref name="predicate"/> matches an event or
    /// <paramref name="timeoutMs"/> of simulated time elapses.
    /// Pre-init events captured in the replay buffer are checked first.
    /// <paramref name="allEvents"/> receives every event seen during the window (for diagnostics).
    public (bool passed, double elapsed, HwEvent? trigger, List<HwEvent> allEvents) RunUntil(
        Func<HwEvent, bool> predicate, double timeoutMs, double tickMs = 5.0)
    {
        HwEvent? trigger = null;
        var all = new List<HwEvent>();

        void Handler(HwEvent e)
        {
            all.Add(e);
            if (trigger is null && predicate(e)) trigger = e;
        }
        EventFired += Handler;

        // Drain pre-init replay buffer (events that fired during alcm_init())
        if (_replayBuffer is not null)
        {
            var buf = _replayBuffer;
            _replayBuffer = null; // consumed — new events now go direct to EventFired
            while (buf.TryDequeue(out var replayed))
            {
                Handler(replayed);
                if (trigger is not null)
                {
                    EventFired -= Handler;
                    return (true, 0, trigger, all);
                }
            }
        }

        double elapsed = 0;
        while (elapsed < timeoutMs && trigger is null)
        {
            double step = Math.Min(tickMs, timeoutMs - elapsed);
            Tick(step);
            elapsed += step;
        }

        EventFired -= Handler;
        return (trigger is not null, elapsed, trigger, all);
    }

    /// Advance simulation for exactly <paramref name="ms"/> simulated milliseconds
    /// and return every event that fired during that window.
    /// The replay buffer (pre-init events) is discarded — this method is time-window based.
    public List<HwEvent> RunFor(double ms, double tickMs = 5.0)
    {
        _replayBuffer = null; // discard pre-init events; RunFor is a time-window check

        var captured = new List<HwEvent>();
        void Handler(HwEvent e) => captured.Add(e);
        EventFired += Handler;

        double elapsed = 0;
        while (elapsed < ms)
        {
            double step = Math.Min(tickMs, ms - elapsed);
            Tick(step);
            elapsed += step;
        }

        EventFired -= Handler;
        return captured;
    }

    private void Tick(double stepMs)
    {
        _alcm.Tick((float)stepMs);
        _simTimeMs += stepMs;
        int n = 0;
        while (_alcm.ProcessEvents() && n++ < 100) { }
    }

    public void Dispose() => _alcm.Dispose();
}
