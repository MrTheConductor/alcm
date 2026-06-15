using SmokeTest;
using System.Diagnostics;

// Resolve scenario path: first CLI arg or default
var scenarioPath = args.Length > 0
    ? args[0]
    : Path.Combine(AppContext.BaseDirectory, "Scenarios", "boot_test.json");

if (!File.Exists(scenarioPath))
{
    Console.Error.WriteLine($"Scenario file not found: {scenarioPath}");
    Console.Error.WriteLine("Usage: SmokeTest [path/to/scenario.json]");
    return 1;
}

var sw = Stopwatch.StartNew();

Console.WriteLine();
Console.WriteLine("  ALCM Board Smoke Test");
Console.WriteLine("  " + new string('═', 54));
Console.WriteLine($"  Scenario : {Path.GetFileName(scenarioPath)}");
Console.WriteLine();

using var engine = new SmokeTestEngine();
var runner = new ScenarioRunner(engine);
runner.Run(scenarioPath);

sw.Stop();

int passCount = runner.Results.Count(r => r.Passed);
int total     = runner.Results.Count;
bool allPassed = runner.AllPassed;

Console.WriteLine();
Console.WriteLine("  " + new string('─', 54));
Console.ForegroundColor = allPassed ? ConsoleColor.Green : ConsoleColor.Red;
Console.Write($"  RESULT: {(allPassed ? "PASS" : "FAIL")}");
Console.ResetColor();
Console.WriteLine($"  ({passCount}/{total} checks)");
Console.WriteLine($"  Simulated: {engine.SimTimeMs:F0}ms  |  Real: {sw.Elapsed.TotalSeconds:F2}s");
Console.WriteLine();

return allPassed ? 0 : 1;
