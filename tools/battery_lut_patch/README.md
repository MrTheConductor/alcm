# battery_lut_patch

Patches a rider-specific battery voltage-to-state-of-charge curve directly
into an ALCM `.hex` file, before flashing. No firmware rebuild, no
per-pack firmware builds - ALCM ships one image for everyone, and this
tool edits the handful of bytes it reserves for exactly this purpose.

If you never run this tool, battery percentage comes from the VESC's own
calculation, unchanged - that's the default, and it's what you get if you
just flash the stock `.hex`.

Pure standard library, no dependencies to install, works on Windows/Mac/Linux.

## Known cell types

`examples/` ships ready-to-use curves for the cell chemistries floatwheel's
own firmware already has field-proven tables for (transcribed from
`CheckPowerLevel()` in `floatwheel/LCM/Code/App/task.c`):

| File | Cell |
| --- | --- |
| `examples/s50s.json` | Samsung INR21700-50S |
| `examples/p42a.json` | LG INR21700-P42A |
| `examples/dg40.json` | DG40 21700 |
| `examples/vtc6.json` | Sony/Murata US18650VTC6 |

These files deliberately **don't** include a cell count - floatwheel itself
pairs the same chemistry with different series counts on different boards
(e.g. P42A ships at both 18S and 20S). You supply your pack's count with
`--cell-count` at patch time, so there's no risk of silently inheriting the
wrong one from the file. If you know your board's series count from
floatwheel's own build (`BATTERY_STRING` in its project defines), each
table above documents which pairing floatwheel itself used - use that only
if it matches your actual board, not as a generic default.

## Usage

```sh
# Patch a known chemistry, telling it your pack's series count
python battery_lut_patch.py patch --input ALCM.hex --output ALCM_patched.hex --curve examples/vtc6.json --cell-count 15

# Same, plus IR-drop (load-sag) compensation - see below
python battery_lut_patch.py patch --input ALCM.hex --output ALCM_patched.hex --curve examples/vtc6.json --cell-count 15 --r-int-milliohms 80

# Or a curve you've measured/sourced yourself
python battery_lut_patch.py patch --input ALCM.hex --output ALCM_patched.hex --curve my_pack.json

# Check what's currently patched into a .hex (or confirm it's unpatched)
python battery_lut_patch.py verify --input ALCM_patched.hex

# Revert to the VESC's own calculation
python battery_lut_patch.py clear --input ALCM_patched.hex --output ALCM_default.hex
```

Flash whatever `--output` produced the same way you'd flash the stock `.hex`.

## IR-drop (load-sag) compensation

### What this is actually correcting for

No battery is a perfect voltage source - every pack has some internal
resistance (`R_int`), from the cells themselves plus everything between
them and the VESC: nickel strips, spot welds, connectors, wiring, and
(if it's in the current path) the BMS. When current flows through that
resistance, some voltage gets "used up" getting through it, so the
voltage the VESC actually measures (**terminal voltage**) is lower than
the pack's true, resting voltage (**open-circuit voltage**, or OCV) -
and the harder you're pulling, the bigger that gap gets:

```
V_terminal = V_ocv - (I * R_int)
```

The LUT curve was built from OCV data (a resting discharge curve), so
feeding it raw terminal voltage under load is feeding it the wrong
number - it reads as a much lower state of charge than the pack
actually has. That's the "hill climb reads critically low, then
recovers the instant you're back on flat ground" symptom. `R_int` is
just the proportionality constant in the formula above: give ALCM a
reasonable value and it reverses the sag before the voltage ever reaches
the curve, instead of just reporting the sagged number and hoping you
don't notice.

`--r-int-milliohms` is your **whole pack's** resistance, not one cell's
- series cells add their resistances together, and parallel groups
divide it, so a 15S1P pack and a 15S2P pack built from identical cells
have quite different whole-pack numbers even though the chemistry is
the same. That's also why there's no chemistry-based default the way
there is for the voltage curve: `R_int` depends on your pack's specific
series/parallel layout and build quality (wire gauge, weld quality,
connector choice), not just which cell is inside it.

### Ballpark starting points

Take these as a rough starting point to dial in, not a number to set
and forget - see "Tuning it" below for the reliable way to land on your
actual pack's value:

| Pack condition | Whole-pack R_int, very roughly |
| --- | --- |
| Fresh, high-drain cells (the chemistries in `examples/`), well-built pack, 1P (no parallel groups) | ~80-200 milliohms |
| Fresh, same cells, with parallel groups (2P) | ~40-120 milliohms (parallel groups divide the cells' contribution) |
| Moderately used, healthy pack | 1.5-2x the fresh figure |
| Old, heavily cycled, or heat/abuse-damaged pack | 2-4x the fresh figure, sometimes more |

Rising internal resistance is one of the standard ways battery wear
shows up (alongside capacity fade) - a pack that's done hundreds of
hard cycles, or spent time somewhere hot, will simply have a higher
`R_int` than the day it was built. It also rises somewhat in cold
weather. None of this needs separate handling - it's just a reason to
expect the right value to drift upward over the life of a pack, and to
revisit it occasionally rather than treating one measurement as
permanent.

### Measuring your own

The formula from above, run backwards, gives you a real measurement:
note your pack's resting voltage (`V_rest`, sit idle for a minute so it
settles), then a voltage/current pair while pulling a real, known load
(a VESC data log or the phone app's real-time view both show
current draw live):

```
R_int (ohms) = (V_rest - V_loaded) / I_loaded
```

Convert to milliohms for `--r-int-milliohms` by multiplying by 1000. A
single data point is noisy (wire resistance in your measurement setup,
sensor noise, etc. all get lumped into the number) - a few
measurements at different current levels, averaged, will get you a more
trustworthy figure than one reading.

### Tuning it

Once you've got a starting value patched in, ride normally and watch
how the gauge behaves under load compared to at rest:

- **Still sags and recovers, just less dramatically than with no
  compensation at all** -> `R_int` is set too low. Raise it.
- **Reads *higher* under hard acceleration than it did at rest, or dips
  unrealistically low under regen braking** -> `R_int` is set too high
  (over-correcting - regen current is the opposite sign, so an
  oversized value swings the reading the wrong way there too). Lower
  it.
- **Roughly steady across normal accel/braking, only moves when the
  pack's actual charge changes** -> that's the target.

If a pack that was well-tuned starts showing the "still sags" symptom
again months later with no other changes, that's consistent with the
pack's real `R_int` having crept up with wear (see the aging note
above) - worth re-measuring rather than assuming something's broken.

Like `cell_count`, it can live in the curve file (`r_int_milliohms` key)
or be supplied via `--r-int-milliohms`; an explicit flag always wins if
both are given.

## Curve file format

A curve is a **per-cell** discharge curve (as you'd find on a cell
manufacturer's datasheet), up to 11 breakpoints, any order (they get
sorted), voltage strictly descending / percent non-increasing once sorted.
Cell count - the multiplier that turns per-cell voltages into your pack's
actual voltage - can either live in the file or be supplied via
`--cell-count`; an explicit `--cell-count` always wins if both are given.

JSON:

```json
{
  "cell_count": 10,
  "breakpoints": [
    {"cell_mv": 4200, "percent": 100.0},
    {"cell_mv": 3700, "percent": 50.0},
    {"cell_mv": 3000, "percent": 0.0}
  ]
}
```

`cell_count` can be omitted entirely (as the shipped `examples/*.json`
chemistry curves do) if you'd rather always pass `--cell-count` explicitly.

CSV (same fields, `cell_count` only needs to be set once, first row, or omitted):

```csv
cell_count,cell_mv,percent
10,4200,100
,3700,50
,3000,0
```

More breakpoints in the region where your pack sags slowly (the "plateau")
gets you a more accurate gauge there - you don't need evenly-spaced
points.

## How it works

ALCM's firmware (`battery_lut.h`) reserves a fixed 64-byte region of flash
(`Project/MDK5/battery_lut.sct`) for this block, validates it at boot
(magic number, schema version, CRC16, breakpoint monotonicity), and falls
back to the VESC's own `battery_level` automatically if it's absent or
invalid - the IR compensation above rides in the same block and is
similarly a no-op (r_int_milliohms=0) whenever the block is unpatched or
invalid. This tool never touches anything outside that reserved region -
it finds the `.hex` records covering it and rewrites just those, so
everything else in your firmware image is untouched.

## Development

```sh
pip install pytest
pytest tests/
```

`tests/test_constants_sync.py` parses the real `battery_lut.h` and checks
`constants.py` hasn't drifted from it - there's no build-time codegen
tying the two together, so that's what catches it if they diverge.
