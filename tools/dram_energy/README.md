# dram_energy — DRAM energy estimation from ChampSim counters

ChampSim models DRAM *timing* but not DRAM *power*, so a change that trades
extra memory traffic for performance (a prefetcher, a speculative DRAM access)
looks free in the simulator. This tool closes that gap the way the DRAM
community does it: multiply the activity counts the simulator already prints by
per-event energies from the device datasheet.

It complements, rather than overlaps, the power infrastructure already in the
tree — [`mcpat/`](../../mcpat) + `scripts/run_mcpat.pl` cover core and caches,
[`pcacti/`](../../pcacti) covers SRAM arrays. Neither models the DRAM devices.

## Layout

```
dram_energy.py                            the equations + .out parsing (no device numbers)
devices/micron_mt40a1g8_ddr4_3200.ini     device parameters, one file per DRAM model
```

Device parameters are deliberately **not** in the script: adding a DRAM model
is a new `.ini`, never an edit to the code.

## Usage

```bash
# one or many runs, human-readable
tools/dram_energy/dram_energy.py results/*.out

# machine-readable, for downstream analysis/plotting
tools/dram_energy/dram_energy.py --csv energy.csv results/*.out

# a different DRAM model, and what is available
tools/dram_energy/dram_energy.py --device ddr5_6400 run.out
tools/dram_energy/dram_energy.py --list-devices

# non-default core clock (cycles -> seconds); default is 4000 MHz (glc)
tools/dram_energy/dram_energy.py --cpu-freq-mhz 3400 run.out
```

`--device` takes a bare profile name (resolved against `devices/`) or a path.

Runs that did not reach `Finished CPU` (deadlock, timeout) are skipped with a
note — they are never silently counted as zero. Output columns are absolute
Joules per component plus the raw event counts; normalizing to a baseline is
left to the analysis step, since the choice of baseline is a study decision.

## Adding a DRAM model

Copy `devices/micron_mt40a1g8_ddr4_3200.ini`, name it after the part and speed
grade (`<vendor>_<part>_<type>_<rate>.ini`), and fill in these sections:

| Section | Keys |
|---|---|
| `[device]` | `name`, `part`, `source`, `verified` (`true` once transcribed from a datasheet) |
| `[organization]` | `devices_per_rank`, `burst_transfers`, `line_bits`, `data_rate_mtps` |
| `[voltage_v]` | `vdd`, `vpp` (set `vpp`/`ipp0` to 0 for DDR3, which has no VPP rail) |
| `[current_ma]` | `idd0`, `idd2n`, `idd3n`, `idd4r`, `idd4w`, `idd5b`, `ipp0` |
| `[timing_ns]` | `tras`, `trc`, `trfc`, `trefi` |
| `[io_pj_per_bit]` | `read`, `write` |

Set `verified = true` only when the values come from a specific datasheet
revision; the tool prints a caveat note for unverified profiles. Comment each
value with where it came from — the shipped profile shows the style.

The equations in `dram_energy.py` are device-independent, so a new profile
needs no code change.

## The model

Four buckets, each from a counter the simulator prints:

| Component | Events counted | Source in the `.out` |
|---|---|---|
| Activation (ACT+PRE) | row activations | `Channel_*_RQ_row_buffer_miss` + `Channel_*_WQ_row_buffer_miss` + `DRAM_DDRP_row_open_act` |
| Read + I/O | 64B column reads | `Channel_*_RQ_row_buffer_hit` + `..._miss` |
| Write + I/O | 64B column writes | `Channel_*_WQ_row_buffer_hit` + `..._miss` |
| Background + refresh | elapsed time | `Finished CPU … cycles:` ÷ core clock |

Counters are summed across all channels present.

Per-event energies follow Micron's methodology [1] (its DDR3 twin TN-41-01 [2]
carries the same equations with worked examples):

```
E_ACT+PRE = [IDD0·tRC − (IDD3N·tRAS + IDD2N·(tRC − tRAS))]·VDD + IPP0·tRC·VPP
E_RD      = (IDD4R − IDD3N)·VDD·tBURST   + I/O
E_WR      = (IDD4W − IDD3N)·VDD·tBURST   + I/O
P_BG      = IDD3N·VDD
P_REF     = (IDD5B − IDD3N)·VDD·tRFC/tREFI
```

all scaled by `devices_per_rank`. With the shipped 8Gb x8 DDR4-3200 profile
this gives **E_ACT ≈ 10.8 nJ**, **E_RD+I/O ≈ 5.6 nJ**, **P_BG+REF ≈ 552 mW**
per rank — note an activation costs roughly
twice a 64B read, which is why "bandwidth-free" mechanisms are not
automatically energy-free.

## Why row-opens need their own counter

A row-open (`--ddrp_row_open`, the speculative activate-without-read action)
never reaches the code that increments the RQ row-buffer counters — the request
is dropped in `MEMORY_CONTROLLER::process()` before the data bus and before the
counting site. Its activations are therefore invisible in `RQ_row_buffer_miss`.

Two tempting substitutes are both wrong, measurably so:

- `DRAM_DDRP_ddrp_req_went_to_dram` counts *every* row-open issued, including
  those that land on an already-open row and cost nothing (`LATENCY = 0`).
  Measured overcount: **~3.3x**.
- The paired full-read config's `RQ_row_buffer_miss` (same gated request
  stream) still overcounts by **~1.6x**.

`DRAM_DDRP_row_open_act` increments at the schedule site only when a row-open
finds the row closed, which is exactly the activation count. It is 0 for every
configuration that does not use the row-open action, so this tool is correct
for ordinary runs regardless.

**Provenance:** the counter was added on `rbdev` in "Add row_open_act counter
for exact DRAM activation accounting". Older `.out` files predating it have no
`row_open_act` line at all, which for a row-open run would silently *undercount*
activations. The tool detects exactly that case — a run whose knob dump says
`ddrp_row_open 1` but which has no counter line — and warns loudly rather than
reporting a wrong number. Re-run those experiments with a build that has the
counter.

## Caveats — read before quoting absolute Joules

The comparison between configurations is robust (every configuration shares the
model), but the absolute numbers carry roughly ±10–20%:

1. **The shipped IDD values are representative, not datasheet-verified**
   (`verified = false` in the profile, and the tool says so on every run).
   Transcribe them from the real part before publishing. One is known-odd:
   `idd4w < idd4r` is atypical, and writes are a small share here.
2. **VPP is simplified** to `IPP0·tRC·VPP`, without the IPP2N/IPP3N background
   subtraction the VDD term gets — overstates E_ACT by ~5% (~1.5% of total).
3. **Background assumes 100% active-standby** (IDD3N, CKE high). Power-down
   states are not modelled, and the precharge/active time split needed for
   Micron's full background equation is not available from the counters.
4. **I/O at 4 pJ/bit (read) and 6 pJ/bit (write) is not from the Micron note** —
   it is a literature approximation standing in for ODT/termination, and is the
   weakest assumption in the model.
5. **Refresh is analytic** (tRFC/tREFI), since ChampSim does not simulate
   refresh at all. It is workload-independent per unit time by construction.
6. **Only the burst time rescales with `dram_io_freq`.** A profile's IDD
   values and timings describe one data rate (`data_rate_mtps`); the tool warns
   when a run used a different one. For a serious study at another speed grade,
   add a profile for it rather than relying on the rescale.

Items 2–5 are common-mode across configurations, so they largely cancel in a
comparison.

## References

1. Micron, *TN-40-07: Calculating Memory Power for DDR4 SDRAM*.
   <https://www.mouser.com/pdfDocs/tn4007_ddr4_power_calculation.pdf>
2. Micron, *TN-41-01: Calculating Memory System Power for DDR3*.
   <https://kolegite.com/EE_library/application_notes/memories/TN41_01DDR3_Power.pdf>
