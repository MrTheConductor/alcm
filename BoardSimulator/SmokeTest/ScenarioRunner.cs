using System.Text.Json;

namespace SmokeTest;

// ── Result record ─────────────────────────────────────────────────────────────

record StepResult(string Name, bool Passed, string Detail);

// ── Scenario runner ───────────────────────────────────────────────────────────

/// Loads and executes a JSONC scenario file against a SmokeTestEngine.
///
/// Supported step types:
///   "await"    – advance until a matching event fires (or timeout)
///   "inject"   – set hardware inputs (footpads, RPM, battery, button, refloat LCM state)
///   "run_for"  – advance a fixed duration and evaluate assert_* checks
class ScenarioRunner
{
    private readonly SmokeTestEngine _engine;
    private readonly List<StepResult> _results = new();

    public IReadOnlyList<StepResult> Results  => _results;
    public bool AllPassed => _results.Count > 0 && _results.TrueForAll(r => r.Passed);

    public ScenarioRunner(SmokeTestEngine engine) => _engine = engine;

    // ── Entry point ──────────────────────────────────────────────────────────

    public void Run(string jsonPath)
    {
        var json = File.ReadAllText(jsonPath);
        using var doc = JsonDocument.Parse(json, new JsonDocumentOptions
        {
            CommentHandling = JsonCommentHandling.Skip
        });
        var root = doc.RootElement;

        // Optional VESC initial state
        float batteryV = 58.8f;
        if (root.TryGetProperty("vesc", out var vescEl) &&
            vescEl.TryGetProperty("battery_voltage", out var bvEl))
            batteryV = bvEl.GetSingle();

        // Initialize
        Console.Write("  Initializing ALCM library... ");
        if (!_engine.Initialize(batteryV))
        {
            Console.WriteLine("FAILED\n");
            Record("ALCM initialization", false, "alcm_init() returned error");
            return;
        }
        Console.WriteLine("OK\n");

        string? currentPhase = null;

        foreach (var step in root.GetProperty("steps").EnumerateArray())
        {
            // Optional phase label printed once per phase group
            if (step.TryGetProperty("phase", out var phEl))
            {
                string phase = phEl.GetString() ?? "";
                if (phase != currentPhase)
                {
                    currentPhase = phase;
                    Console.WriteLine($"  Phase: {phase}");
                }
            }

            string type = step.TryGetProperty("type", out var typeEl) ? typeEl.GetString() ?? "" : "";
            string name = step.TryGetProperty("name", out var nameEl) ? nameEl.GetString() ?? "" : type;

            switch (type)
            {
                case "await":
                    if (!RunAwait(name, step))
                        return; // abort — nothing downstream is meaningful
                    break;

                case "inject":
                    RunInject(name, step);
                    break;

                case "run_for":
                    RunFor(name, step);
                    break;

                default:
                    Console.WriteLine($"  [WARN] Unknown step type '{type}' — skipping");
                    break;
            }
        }
    }

    // ── Step executors ───────────────────────────────────────────────────────

    bool RunAwait(string name, JsonElement step)
    {
        string evType = step.GetProperty("event").GetString() ?? "";
        double timeout = step.TryGetProperty("timeout_ms", out var t) ? t.GetDouble() : 1000;
        JsonElement? where = step.TryGetProperty("where", out var w) ? w : null;

        var pred = BuildPredicate(evType, where);
        var (passed, elapsed, _, allEvents) = _engine.RunUntil(pred, timeout);

        Record(name, passed,
            passed ? $"@ {elapsed:F0}ms sim" : $"timeout after {timeout:F0}ms");

        if (!passed)
        {
            // Show what DID fire so the user knows what to look for
            var relevant = allEvents.Where(e => e.Kind.ToString().ToLower() == evType).ToList();
            if (relevant.Count > 0)
            {
                Detail($"No matching '{evType}' event — but {relevant.Count} '{evType}' event(s) did fire:");
                foreach (var e in relevant.Take(5))
                    Detail($"  {FormatEvent(e)}");
            }
            else if (allEvents.Count > 0)
            {
                Detail($"No '{evType}' events received at all. Events seen ({allEvents.Count} total):");
                foreach (var e in allEvents.Take(8))
                    Detail($"  {FormatEvent(e)}");
                if (allEvents.Count > 8)
                    Detail($"  ... ({allEvents.Count - 8} more)");
            }
            else
            {
                Detail("No events of any kind were received during the window.");
            }
        }

        return passed;
    }

    void RunInject(string name, JsonElement step)
    {
        if (step.TryGetProperty("footpad_left_v",  out var lv) &&
            step.TryGetProperty("footpad_right_v", out var rv))
            _engine.SetFootpadVoltages(lv.GetSingle(), rv.GetSingle());

        if (step.TryGetProperty("rpm", out var rpm))
            _engine.SetRpm(rpm.GetInt32());

        if (step.TryGetProperty("battery_v", out var bv))
            _engine.SetBatteryVoltage(bv.GetSingle());

        if (step.TryGetProperty("button_pressed", out var btn))
            _engine.SetButtonState(btn.GetBoolean());

        if (step.TryGetProperty("imu_pitch_deg", out var pitch))
            _engine.SetImuPitch(pitch.GetSingle());

        if (step.TryGetProperty("imu_roll_deg", out var roll))
            _engine.SetImuRoll(roll.GetSingle());

        if (step.TryGetProperty("refloat_installed", out var refloatInstalled))
            _engine.SetRefloatInstalled(refloatInstalled.GetBoolean());

        if (step.TryGetProperty("external_leds_enabled", out var externalLeds))
            _engine.SetExternalLedsEnabled(externalLeds.GetBoolean());

        if (step.TryGetProperty("lcm_headlight_brightness_pct", out var lcmHeadlight))
            _engine.SetLcmHeadlightBrightnessPercent(lcmHeadlight.GetByte());

        if (step.TryGetProperty("lcm_headlight_idle_brightness_pct", out var lcmHeadlightIdle))
            _engine.SetLcmHeadlightIdleBrightnessPercent(lcmHeadlightIdle.GetByte());

        if (step.TryGetProperty("lcm_status_brightness_pct", out var lcmStatus))
            _engine.SetLcmStatusBrightnessPercent(lcmStatus.GetByte());

        Console.WriteLine($"    Inject: {name}");
    }

    void RunFor(string name, JsonElement step)
    {
        double duration = step.GetProperty("duration_ms").GetDouble();
        Console.WriteLine($"  Running {duration:F0}ms sim: {name}");
        var events = _engine.RunFor(duration);

        if (!step.TryGetProperty("checks", out var checksEl))
            return;

        foreach (var check in checksEl.EnumerateArray())
        {
            string checkName = check.TryGetProperty("name", out var cn) ? cn.GetString() ?? "" : "";

            // assert_no: no matching event must have occurred
            if (check.TryGetProperty("assert_no", out var an))
            {
                string evType = an.GetProperty("event").GetString() ?? "";
                JsonElement? where = an.TryGetProperty("where", out var w) ? w : null;
                var pred = BuildPredicate(evType, where);
                var badEvent = events.Find(e => pred(e));
                Record(checkName, badEvent is null,
                    badEvent is null ? "no unwanted events" : "unwanted event occurred");
                if (badEvent is not null)
                    Detail($"Triggered by: {FormatEvent(badEvent)}");
            }
            // assert_count_gte: at least N matching events must have occurred
            else if (check.TryGetProperty("assert_count_gte", out var ac))
            {
                string evType = ac.GetProperty("event").GetString() ?? "";
                int minCount = ac.TryGetProperty("count", out var mc) ? mc.GetInt32() : 1;
                JsonElement? where = ac.TryGetProperty("where", out var w) ? w : null;
                var pred = BuildPredicate(evType, where);
                int count = events.Count(e => pred(e));
                Record(checkName, count >= minCount,
                    $"{count} event{(count == 1 ? "" : "s")} (min {minCount})");
                if (count < minCount)
                    Detail($"Window contained {events.Count} total events: {FormatEventSummary(events)}");
            }
            // assert_any: at least one matching event must have occurred
            else if (check.TryGetProperty("assert_any", out var aa))
            {
                string evType = aa.GetProperty("event").GetString() ?? "";
                JsonElement? where = aa.TryGetProperty("where", out var w) ? w : null;
                var pred = BuildPredicate(evType, where);
                bool any = events.Exists(e => pred(e));
                Record(checkName, any,
                    any ? "event received" : "no matching event found");
                if (!any)
                {
                    var sameType = events.Where(e => e.Kind.ToString().ToLower() == evType).ToList();
                    if (sameType.Count > 0)
                    {
                        Detail($"{sameType.Count} '{evType}' event(s) fired but none matched the filter:");
                        foreach (var e in sameType.Take(5))
                            Detail($"  {FormatEvent(e)}");
                    }
                    else
                    {
                        Detail($"No '{evType}' events at all. Window summary: {FormatEventSummary(events)}");
                    }
                }
            }
        }
    }

    // ── Predicate builder ────────────────────────────────────────────────────

    /// Builds a Func<HwEvent, bool> from a JSON event type name and optional
    /// "where" filter object.  Supported filter keys per event type:
    ///
    ///   power:       enabled (bool)
    ///   status_led:  r_or_g_or_b_gt (byte),  led_index (byte)
    ///   headlight:   brightness_gt (ushort),  direction (byte)
    ///   debug:       contains (string, case-insensitive)
    ///   buzzer, vesc_request: no filters (any event of that type matches)
    Func<HwEvent, bool> BuildPredicate(string eventType, JsonElement? where)
    {
        return eventType switch
        {
            "power" => e =>
            {
                if (e.Kind != HwEventKind.Power) return false;
                if (where.HasValue && where.Value.TryGetProperty("enabled", out var en))
                    return e.PowerEnabled == en.GetBoolean();
                return true;
            },

            "status_led" => e =>
            {
                if (e.Kind != HwEventKind.StatusLed) return false;
                if (where.HasValue)
                {
                    if (where.Value.TryGetProperty("r_or_g_or_b_gt", out var v))
                    {
                        byte thr = v.GetByte();
                        return e.R > thr || e.G > thr || e.B > thr;
                    }
                    if (where.Value.TryGetProperty("led_index", out var idx))
                        return e.LedIndex == idx.GetByte();
                }
                return true;
            },

            "headlight" => e =>
            {
                if (e.Kind != HwEventKind.Headlight) return false;
                if (where.HasValue)
                {
                    if (where.Value.TryGetProperty("brightness_gt", out var bgt))
                        return e.HeadlightBrightness > bgt.GetUInt16();
                    if (where.Value.TryGetProperty("direction", out var dir))
                        return e.HeadlightDirection == dir.GetByte();
                }
                return true;
            },

            "buzzer" => e => e.Kind == HwEventKind.Buzzer,

            "vesc_request" => e => e.Kind == HwEventKind.VescRequest,

            "debug" => e =>
            {
                if (e.Kind != HwEventKind.Debug) return false;
                if (where.HasValue && where.Value.TryGetProperty("contains", out var c))
                    return e.DebugMessage?.Contains(
                        c.GetString()!, StringComparison.OrdinalIgnoreCase) == true;
                return true;
            },

            _ => _ => false
        };
    }

    // ── Output helpers ───────────────────────────────────────────────────────

    void Record(string name, bool passed, string detail)
    {
        _results.Add(new StepResult(name, passed, detail));

        var saved = Console.ForegroundColor;
        Console.ForegroundColor = passed ? ConsoleColor.Green : ConsoleColor.Red;
        Console.Write($"  [{(passed ? "PASS" : "FAIL")}]");
        Console.ForegroundColor = saved;
        Console.WriteLine($"  {name,-40} {detail}");
    }

    void Detail(string msg) =>
        Console.WriteLine($"           {msg}");

    static string FormatEvent(HwEvent e) => e.Kind switch
    {
        HwEventKind.Power       => $"[power]        enabled={e.PowerEnabled}  @ {e.SimTimeMs:F0}ms",
        HwEventKind.StatusLed   => $"[status_led]   led={e.LedIndex} rgb=({e.R},{e.G},{e.B})  @ {e.SimTimeMs:F0}ms",
        HwEventKind.Headlight   => $"[headlight]    dir={e.HeadlightDirection} bri={e.HeadlightBrightness}  @ {e.SimTimeMs:F0}ms",
        HwEventKind.VescRequest => $"[vesc_request]  @ {e.SimTimeMs:F0}ms",
        HwEventKind.Buzzer      => $"[buzzer]  @ {e.SimTimeMs:F0}ms",
        HwEventKind.Debug       => $"[debug]        \"{e.DebugMessage}\"  @ {e.SimTimeMs:F0}ms",
        _                       => $"[unknown]  @ {e.SimTimeMs:F0}ms"
    };

    static string FormatEventSummary(List<HwEvent> events)
    {
        if (events.Count == 0) return "none";
        var groups = events.GroupBy(e => e.Kind)
                           .Select(g => $"{g.Key.ToString().ToLower()}: {g.Count()}");
        return $"{events.Count} total  ({string.Join(", ", groups)})";
    }
}
