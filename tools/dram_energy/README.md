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
this gives **E_ACT ≈ 5.95 nJ**, **E_RD+I/O ≈ 5.22 nJ**, **E_WR+I/O ≈ 3.31 nJ**,
**P_BG+REF ≈ 443 mW** per rank.

So **an activation costs about the same as a 64B read** (1.14x), not double.
That matters for interpreting "bandwidth-free" mechanisms: skipping the column
read but still activating the row saves roughly half the per-access dynamic
energy, not a negligible slice — but it is not free either. An earlier version
of this profile used unsourced IDD values that put the ratio at 1.93x and
supported a stronger "activations dominate" claim; the datasheet does not.

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

1. **Every value is now datasheet-sourced and cited inline in the profile**
   (`verified = true`): currents from Micron's 8Gb DDR4 component data sheet
   Table 151 (Rev E die, 0-95C, DDR4-3200, x8), timings from Table 163,
   voltages/refresh/BL8 from JESD79-4, I/O from TN-40-07 Table 26. The residual
   uncertainty is which **die revision** you assume: Tables 148-153 span ~25% on
   IDD0. Note `idd4w < idd4r` is correct for DDR4, not a typo -- see caveat 4.
2. **VPP is an upper bound.** The activate term charges the full
   `IPP0·tRC·VPP` without the background subtraction the VDD term gets, because
   the datasheet reports `IPP0 = IPP3N = 3 mA` — the activate-specific wordline
   current is below its 1 mA resolution. This term is ~46% of E_ACT: subtracting
   the background as the VDD side does would take E_ACT from 5.95 to 3.21 nJ
   (ACT/RD ratio 1.14x -> 0.60x). Both readings support the conclusion that an
   activation is *not* expensive relative to a read; the upper bound is used, so
   activation-heavy mechanisms are charged conservatively.
3. **Background assumes 100% active-standby** (IDD3N, CKE high). Power-down
   states are not modelled, and the precharge/active time split needed for
   Micron's full background equation is not available from the counters.
4. **I/O comes from TN-40-07 Table 26** (pdqRD 11.73 mW/DQ, pdqWR 4.15 mW/DQ),
   rescaled to 3200 MT/s as 3.67 / 1.30 pJ/bit. Scope is the *accessed device
   in a single-rank system*, matching the IDD scope; a second rank would add
   idle-rank ODT (5.9 / 4.0 pJ/bit) and the full DQ link including the
   controller is higher still (6.1 / 6.5). Micron calls these "a first-order
   approximation": they are DC estimates that ignore data-toggle dependence,
   which measurements show is large. **Write < read is correct for DDR4** —
   POD12 termination is pull-up-only, so the written device pays only its ODT
   while the read device pays its full output driver. (DDR3's center-tapped
   termination gives the opposite order; carrying that intuition over is an
   easy way to get this backwards.)
5. **Refresh is analytic** (tRFC/tREFI), since ChampSim does not simulate
   refresh at all. It is workload-independent per unit time by construction.
6. **Only the burst time rescales with `dram_io_freq`.** A profile's IDD
   values and timings describe one data rate (`data_rate_mtps`); the tool warns
   when a run used a different one. For a serious study at another speed grade,
   add a profile for it rather than relying on the rescale.

Items 2–5 are common-mode across configurations, so they largely cancel in a
comparison.

## References

1. Micron, *8Gb DDR4 SDRAM* component data sheet (MT40A1G8 / MT40A512M16),
   CCMTD-1406124318-10419 Rev. L 12/2023 — currents (Table 151), timings
   (Table 163), refresh (Table 52).
   <https://www.alliancememory.com/wp-content/uploads/Micron_8gb-Commercial-ddr4-dram-Alliance_MT40A512M16TD-062ER-MT40A1G8AG-062ER_reduced.pdf>
2. Micron, *TN-40-07: Calculating Memory Power for DDR4 SDRAM*, Rev. B 8/18 —
   the equations, and I/O termination power (Table 26).
   <https://web.archive.org/web/2020/https://www.mouser.com/pdfDocs/tn4007_ddr4_power_calculation.pdf>
3. Micron, *TN-41-01: Calculating Memory System Power for DDR3* — same
   equations with worked numeric examples (Eq. 10 ACT, Eq. 13 WR).
   <https://kolegite.com/EE_library/application_notes/memories/TN41_01DDR3_Power.pdf>
4. JEDEC, *JESD79-4 DDR4 SDRAM* (Sept 2012) — VDD/VPP (Table 63), tRFC1/tREFI,
   BL8 (sec. 4.3).
   <https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/196/JESD79_2D00_4.pdf>
5. SK hynix, *8Gb DDR4 SDRAM* Rev 1.4 (Apr 2020) — corroborates the DDR4-3200
   22-22-22 tRC/tRAS/tRCD/tRP row.
   <https://product.skhynix.com/download.do?attNo=1638&attTypeCd=TRT06>
