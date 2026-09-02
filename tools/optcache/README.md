# optcache — Belady's OPT limit study for the LLC

Belady's MIN needs future knowledge, so it cannot run inside the timing
simulator. This tool computes it the standard way instead: the simulator dumps
the LLC access stream to a file, a second pass annotates every access with its
*forward* reuse distance, and a third pass replays the stream through a
set-associative cache that evicts the block reused furthest in the future.

What you get is an **MPKI ceiling**, not an IPC. The replay carries no timing
model, so OPT can only be compared against online policies on miss counts.

This complements [`../dram_energy/`](../dram_energy), which post-processes the
counters an online run already prints; optcache instead post-processes an
access *trace* the run has to be asked to emit.

## Layout

```
optcache.h            the OPT cache model (header-only): set indexing, victim
                      selection by max forward reuse distance, bypass, stats
optcache_driver.cc    replays a (trace, reuse) file pair through OptCache
gen_fwd_reuse.cc      annotates a trace with forward reuse distances
build_tools.sh        builds both binaries into this directory
gen_fwd_reuse.sh      wrappers that locate the binary next to themselves,
optcache_driver.sh    so they work from any cwd
```

The binaries are gitignored. Build them with:

```bash
tools/optcache/build_tools.sh
```

## Pipeline

### 1. Dump the LLC access trace from a simulator run

```bash
<champsim-binary> \
  --llc_dump_access_trace=true \
  --llc_dump_access_trace_type=4 \
  --dump_access_trace_in_warmup=true \
  --llc_access_trace_filename=sim.llc \
  ... -traces <trace>
```

Produces `sim.llc.gz`: a flat, uncounted sequence of 10-byte records —
`uint64_t full_addr`, `uint8_t type`, `bool hit` (`src/cache_tracer.cc:49-58`).
The `hit` field records what the **online** policy of that run did, which is
what makes the validation gate below possible.

> **`llc_dump_access_trace_type` defaults to `0`, which is LOAD only.**
> The filter is `if (access_type == NUM_TYPES || type == access_type)`
> (`src/cache_tracer.cc:51`), so only the sentinel `4` (= `NUM_TYPES`, "ALL")
> records all four access types. Set it explicitly to `4` unless you really
> want a load-only stream.

Type encoding is shared with the simulator — `LOAD 0`, `RFO 1`, `PREFETCH 2`,
`WRITEBACK 3` (`inc/commons.h:46-50`, mirrored at `optcache.h:18-21`).

`--dump_access_trace_in_warmup` (`inc/knobs.def:44`, default false) also records
warmup, so the replay can reach the ROI with a warm cache instead of a cold one
— see the cold-start caveat below. It costs a larger trace and nothing else: the
ROI portion is byte-identical either way.

**The ROI boundary is marked in-band.** At `finish_warmup()` the simulator writes
one record with address `0xdeadbeef` and type `255` (`src/cache_tracer.cc`,
`TRACE_ROI_MARKER_*` in `inc/cache_tracer.h`). Type 255 is outside `NUM_TYPES`,
so it cannot collide with a real access. It is written **only when
`dump_access_trace_in_warmup` is set**, so a ROI-only trace has no marker and the
plain five-argument driver invocation keeps working unchanged. `gen_fwd_reuse`
passes it through as a positional placeholder without entering it into its
reuse map.

### 2. Generate forward reuse distances

```bash
tools/optcache/gen_fwd_reuse.sh sim.llc.gz sim.llc
# -> sim.llc.reuse.gz
```

One `uint64_t` per access, positionally aligned with the trace: the number of
accesses until that block is next touched, or `UINT64_MAX` if it never is.

### 3. Replay through OPT

```bash
tools/optcache/optcache_driver.sh <sets> <assoc> <trace.gz> <reuse.gz> <bypass> \
                                  [--roi-marker]

# 3 MB glc LLC (LLC_SET 4096 x LLC_WAY 12), bypass enabled:
tools/optcache/optcache_driver.sh 4096 12 sim.llc.gz sim.llc.reuse.gz 1 --roi-marker
```

`--roi-marker` says the trace contains a ROI marker: at that record the driver
zeroes its counters and keeps the cache contents, so the reported numbers cover
the ROI measured from a warm cache. **A trace containing a marker without the
flag is refused**, not silently replayed — the marker would otherwise be counted
as an access and the numbers would include warmup, which is the exact error the
marker exists to prevent.

Match `<sets>`/`<assoc>` to the LLC the online run was actually built with. The
binary prints `llc_set` and `llc_way` in its startup dump — take them from
there, they are authoritative. The `inc/uarch/*.h` headers define
`LLC_SET NUM_CPUS * <n>` (line 106), so the per-core figures below must be
multiplied by the core count:

| uarch | sets (1 core) | capacity (1 core) |
|---|---|---|
| `glc` | 4096 | 3 MB |
| `glc_llc6` | 8192 | 6 MB |
| `glc_llc12` | 16384 | 12 MB |
| `glc_llc24` | 32768 | 24 MB |

`LLC_WAY` is 12 in all four. Passing a per-core figure for a multi-core build
replays a too-small OPT cache against a bigger LLC's trace and silently
inflates the OPT miss count — and the validation gate below will *not* catch
it, because `OptCache.trace.*` is read from the trace's `hit` bit and does not
depend on `<sets>`.

Set `<bypass>` to `1` to match the simulator, which has `LLC_BYPASS` defined
(`inc/champsim.h:27`): a policy may decline to fill by returning
`way == LLC_WAY` (`src/cache.cc:177-179`). OptCache bypasses on the same
condition — incoming reuse distance worse than the victim's — and, like the
simulator (`src/cache.cc:650-654`), never bypasses a WRITEBACK
(`optcache.h:143-145`). A bypassed access still counts as a miss.

## Output

All counters go to stdout as `Name value` lines, in two families:

| prefix | meaning |
|---|---|
| `OptCache.access.*` | what **OPT** did — `total`/`hit`/`miss`, overall and per type |
| `OptCache.cache.*` | `insertion`, `eviction`, `bypass`, `promotion` |
| `OptCache.trace.*` | what the **online policy** did, replayed from the `hit` bit in the trace |

There is no MPKI here — the trace carries no instruction count. Divide
`OptCache.access.miss` by the online run's retired instructions
(`Core_0_total_instructions`) in the analysis step.

### Validation gate

`OptCache.trace.{total,hit,miss}` is the online policy's own outcome replayed
from the trace, so it must reproduce that run's `LLC TOTAL ACCESS/HIT/MISS`
from `print_roi_stats` (`src/main.cc:355-400`) to a *tiny fraction*, not
exactly. The trace counts an access when it arrives; `sim_access` counts a miss
when it fills. Three things fall in that gap, all making the trace slightly
larger:

- requests still in the LLC MSHR when the ROI ends (traced, never filled),
- reads that merged into an in-flight MSHR (`src/cache.cc:1170`) — one trace
  record, no fill of their own,
- prefetches released from the PQ without allocating an MSHR, which happens
  when `PQ.entry[].fill_level > fill_level` (`src/cache.cc:1409`),
- plus one for the duplicated final record noted below.

Measured excess, 1M warmup, LRU, `--llc_dump_access_trace_type=4`:

| run | sim access | trace | excess |
|---|---|---|---|
| 462.libquantum, 1M/5M/20M sim, no pref | 27,319 / 129,227 / 507,249 | +3 each | 0.001% |
| 605.mcf, 5M sim, no pref | 238,672 | +0 | 0% |
| 607.cactuBSSN, 5M sim, no pref | 13,262 | +0 | 0% |
| 605.mcf, 5M sim, spp_dev2 @L2C | 241,708 | +5 | 0.002% |
| 605.mcf, 20M sim, spp_dev2 @L2C | 968,692 | +10 | 0.001% |

Without a prefetcher the excess is a constant boundary residue. With one it
grows slowly, and lands entirely in PREFETCH records. Either way it stays around
0.001%, far below anything that moves an MPKI figure.

**Gate:** excess must be non-negative and under ~0.1% of the access count. A
deficit, or a percentage that climbs as the run lengthens, means the dumped
stream is not a faithful record of what reached the LLC, and any OPT number
computed from it is unsound. Check this before trusting a sweep.

## Caveats

**OPT starts cold unless you trace warmup.** With `dump_access_trace_in_warmup`
off, every `record_trace` call site is gated on `warmup_complete[cpu]` — six
L2C/LLC call-site pairs, one per queue outcome, the LLC halves being
`src/cache.cc:478`, `:761`, `:921`, `:1221`, `:1303`, `:1517` — so the trace
begins at the ROI. The online policy enters the ROI with a warm LLC;
OptCache enters it with an empty array and pays compulsory misses the online run
already paid during warmup. The bias is bounded by min(cache blocks, footprint)
and therefore **grows with cache size** — at 24 MB it is up to 393,216 extra
misses, ~0.79 MPKI over a 500M-instruction ROI, versus ~0.10 at 3 MB. Any
capacity sweep must correct for this or it will understate the OPT gap at large
sizes.

Measured on 462.libquantum, 1M warmup / 5M sim, 3 MB LLC: cold-start OPT reports
117,390 misses against warm-start OPT's 112,990 — 4,400 spurious misses, +3.9%.
Cold-start OPT also lands *above* LRU's 112,987, which is impossible for an
optimal policy on the same stream, so the bias is not subtle. Set
`--dump_access_trace_in_warmup=true` and pass `--roi-marker`.

**The trace is in arrival order, which is not `sim_access` order.** Both hits
and misses are recorded when the request is released from its queue — read hits
at `src/cache.cc:921`, read misses at `:1221`, prefetch misses at `:1517`, the
latter two inside `if (miss_handled)` and immediately before the queue removal,
so a retried entry is recorded exactly once. Misses were previously recorded in
`handle_fill` when the data
returned, hundreds of cycles late, which reordered the stream and corrupted the
forward reuse distances OPT depends on. The cost of the fix is that the trace no
longer matches `sim_access` exactly — see the validation gate above.

**The trace does not say the online policy bypassed.** A read miss the LLC
later declines to fill (`way == LLC_WAY`, `src/cache.cc:179`) was already
recorded at RQ release, so it appears in the trace as an ordinary miss and
OptCache installs it. Inert for an all-LRU dump run — `LRURepl::find_victim`
delegates to `lru_victim` (`replacement/lru.cc:11-18`), which never returns
`LLC_WAY` — but a trace dumped from Hawkeye or SHiP would model installs the
online policy never performed.

**`gen_fwd_reuse` is memory-hungry.** It holds one `uint64_t` per access in a
`vector` plus an `unordered_map` over unique blocks (`gen_fwd_reuse.cc:41`,
`:82`), all resident. Tens of millions of LLC accesses means hundreds of MB to
several GB. Size it before launching a batch.

**Trace and reuse files are consumed in lockstep with no consistency check.**
`optcache_driver.cc:60-64` reads one record from each per iteration; a reuse
file regenerated from a different trace misaligns silently. Keep the pair
together and regenerate both.

**Both readers use `while (!gzeof(f))` with unchecked `gzread`**
(`gen_fwd_reuse.cc:46-49`, `optcache_driver.cc:60-64`), so each processes its
final record twice. One duplicated access out of millions — immaterial for MPKI,
but it is there.

**OPT here is per-set, single-core, address-only.** Victim selection is optimal
*within* a set under fixed indexing (`optcache.h:186-206`), which is the
standard set-associative OPT, not fully-associative MIN. Set indexing matches
the simulator: OptCache uses `(addr >> 6) % num_sets` (`optcache.h:169-173`),
the simulator `block_addr & (NUM_SET-1)` (`src/cache.cc:1551`) — equivalent for
power-of-2 set counts, which all the uarch variants have.
