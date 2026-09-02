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
  --llc_access_trace_filename=sim.llc \
  ... -traces <trace>
```

Produces `sim.llc.gz`: a flat, uncounted sequence of 10-byte records —
`uint64_t full_addr`, `uint8_t type`, `bool hit` (`src/cache_tracer.cc:35-45`).
The `hit` field records what the **online** policy of that run did, which is
what makes the validation gate below possible.

> **`llc_dump_access_trace_type` defaults to `0`, which is LOAD only.**
> The filter is `if (access_type == NUM_TYPES || type == access_type)`
> (`src/cache_tracer.cc:37`), so only the sentinel `4` (= `NUM_TYPES`, "ALL")
> records all four access types. Set it explicitly to `4` unless you really
> want a load-only stream.

Type encoding is shared with the simulator — `LOAD 0`, `RFO 1`, `PREFETCH 2`,
`WRITEBACK 3` (`inc/commons.h:46-50`, mirrored at `optcache.h:18-21`).

### 2. Generate forward reuse distances

```bash
tools/optcache/gen_fwd_reuse.sh sim.llc.gz sim.llc
# -> sim.llc.reuse.gz
```

One `uint64_t` per access, positionally aligned with the trace: the number of
accesses until that block is next touched, or `UINT64_MAX` if it never is.

### 3. Replay through OPT

```bash
tools/optcache/optcache_driver.sh <sets> <assoc> <trace.gz> <reuse.gz> <bypass>

# 3 MB glc LLC (LLC_SET 4096 x LLC_WAY 12), bypass enabled:
tools/optcache/optcache_driver.sh 4096 12 sim.llc.gz sim.llc.reuse.gz 1
```

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
simulator (`src/cache.cc:660-664`), never bypasses a WRITEBACK
(`optcache.h:138-140`). A bypassed access still counts as a miss.

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
from `print_roi_stats` (`src/main.cc:355-400`) to within the one duplicated
final record noted under Caveats — expect a difference of 0 or 1, never more.
A larger gap means the dumped stream is not a faithful record of what reached
the LLC, and any OPT number computed from it is unsound. Check this before
trusting a sweep.

## Caveats

**The trace is ROI-only, so OPT starts cold.** Every `record_trace` call site is
gated on `warmup_complete[cpu]` (`src/cache.cc:924-930`, `:482-489`, `:763-770`,
`:1291-1298`, `:352-359`). The online policy enters the ROI with a warm LLC;
OptCache enters it with an empty array and pays compulsory misses the online run
already paid during warmup. The bias is bounded by min(cache blocks, footprint)
and therefore **grows with cache size** — at 24 MB it is up to 393,216 extra
misses, ~0.79 MPKI over a 500M-instruction ROI, versus ~0.10 at 3 MB. Any
capacity sweep must correct for this or it will understate the OPT gap at large
sizes.

**Misses enter the trace at fill time, not at request time.** A hit is recorded
when the request is serviced (`src/cache.cc:925`), but a miss is recorded in
`handle_fill` when the data returns (`src/cache.cc:353`), hundreds of cycles
later. The recorded order matches the simulator's own `sim_access` accounting,
but it is not the arrival order, and forward reuse distances are computed on the
recorded order.

**The LLC bypass path is not traced.** `src/cache.cc:194` increments
`sim_access`/`sim_miss` on a bypassed fill with no matching `record_trace`, so
the trace undercounts whenever a policy actually bypasses. Inert for an
all-LRU dump run — `LRURepl::find_victim` delegates to `lru_victim`
(`replacement/lru.cc:11-18`), which never returns `LLC_WAY` — but it would bite
a trace dumped from Hawkeye or SHiP. The validation gate above catches it.

**`gen_fwd_reuse` is memory-hungry.** It holds one `uint64_t` per access in a
`vector` plus an `unordered_map` over unique blocks (`gen_fwd_reuse.cc:39`,
`:72`), all resident. Tens of millions of LLC accesses means hundreds of MB to
several GB. Size it before launching a batch.

**Trace and reuse files are consumed in lockstep with no consistency check.**
`optcache_driver.cc:52-58` reads one record from each per iteration; a reuse
file regenerated from a different trace misaligns silently. Keep the pair
together and regenerate both.

**Both readers use `while (!gzeof(f))` with unchecked `gzread`**
(`gen_fwd_reuse.cc:44-47`, `optcache_driver.cc:52-56`), so each processes its
final record twice. One duplicated access out of millions — immaterial for MPKI,
but it is there.

**OPT here is per-set, single-core, address-only.** Victim selection is optimal
*within* a set under fixed indexing (`optcache.h:181-201`), which is the
standard set-associative OPT, not fully-associative MIN. Set indexing matches
the simulator: OptCache uses `(addr >> 6) % num_sets` (`optcache.h:164-168`),
the simulator `block_addr & (NUM_SET-1)` (`src/cache.cc:1533`) — equivalent for
power-of-2 set counts, which all the uarch variants have.
