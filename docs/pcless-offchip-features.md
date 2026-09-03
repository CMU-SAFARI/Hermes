# PC-less Feature Ideation for the Off-chip Predictor (uncore-faithful)

## Context

POPET's selected feature set (`ocp_hermes.ini`: features 5,8,9,11,16) uses the PC in
4 of 5 features. For an uncore/beside-LLC placement, the PC is a friction point with
production architects, so we want to run feature selection over **PC-less** features
only. The current PC-less inventory is thin and, crucially, **all history/sequence
features are PC-based** (`LastNLoadPCs`=16, `LastNPCs`=17) — the PC-less space has only
single-request address decompositions plus a binary `first_access`.

This doc catalogs **new PC-less features** to add to the candidate pool, designed on the
**physical address** (the predictor will run after the L2 miss, where paddr is live).
Scope: **page footprint & reuse**, **address-stream history**, **address granularities**.
(DRAM-BW / LLC-occupancy system signals deliberately excluded.) This is feature *thinking*;
implementation (enum/name/`process_*`/state plumbing, the per-page tracker, the
post-L2-miss relocation) is handled separately.

Framework reminder: a feature is just a `process_*()` that turns `state_info_t` into a
hashed weight-table index ([perc_pred_helper.cc](../src/perc_pred_helper.cc)). History
features fold a precomputed `uint64` signature exactly like `process_LastNLoadPCs`. The
perceptron does **not** need a signal to be monotonic w.r.t. off-chip — it learns a
per-bucket weight — so the bar for a feature is "does this signal partition requests into
buckets with *different* off-chip probability," not "bigger = more off-chip."

## Existing PC-less features (don't re-propose)

`Offset`(in-page line index 0–63), `Page`(vpage), `Addr`(full vaddr), `FirstAccess`,
`Offset_FirstAccess`, `CLOffset`(byte-in-line), `CLWordOffset`, `CLDWordOffset`.

---

## Family A — Page footprint & reuse

Needs a physical-page-indexed tracker beside the LLC. This is **the same structure XPT
introduces** ([offchip_pred_xpt](../src/offchip_pred_xpt.cc)) — share it. Conceptually this
family lets the perceptron **subsume XPT**: XPT = one per-page off-chip counter + a hard
threshold; here the same counter becomes a soft, learned feature alongside others.

| Feature | Raw signal | Why it partitions off-chip |
|---|---|---|
| **PageReuseCount** | saturating per-page access count | Cold/low-reuse pages stream → off-chip; hot pages stay resident. Log-quantize the count. |
| **PageOffchipCount** | per-page count of lines that went off-chip (XPT's exact stat) | Direct XPT signal as a soft feature — the perceptron weights "page has had N off-chip lines" instead of thresholding it. |
| **PageFootprint (density)** | `popcount(bmp_access)` of distinct lines touched | Dense vs sparse touch pattern separates spatial-locality pages from scattered/streaming ones. `BitmapHelper::count_bits_set` already exists. |
| **PageMissRatio** | per-page off-chip / total (bucketed) | The per-page conditional off-chip probability directly (guard early small-N buckets). |
| **PageRecency / age** | accesses (or LRU-stack position) since page last seen | Recently-active pages likely still cached; stale pages → off-chip. |

Note: at the uncore the access stream is interleaved across cores/threads, so per-page
counters are noisier than core-side — worth measuring, not a blocker.

---

## Family B — Address-stream history (the main gap)

PC-less analogs of `LastNLoadPCs`: build a `uint64` signature from the recent **physical**
address stream with the same XOR-shift used in `get_control_flow_signatures`, then fold it.

| Feature | Raw signal | Why it partitions off-chip |
|---|---|---|
| **LastNDeltas** ⭐ | signature over last N deltas of consecutive **block** addresses (paddr>>6) | Strided/streaming miss streams correlate strongly with off-chip. The prime PC-less analog of the PC history feature. |
| **PerPageStride** ⭐ | delta between current offset and last offset **in the same page** | Intra-page stride, **translation-invariant** (offsets unaffected by paging) and not polluted by interleaving — cleaner than global delta. |
| **LastNOffsets** | signature over last N in-page offsets | Stream's intra-page access shape (e.g. struct strides). Translation-invariant. |
| **LastNPages** | signature over last N distinct physical pages | Working-set churn / page-walk recurrence: streaming touches many fresh pages. |
| **CurrentDelta** | single signed delta to previous block address (log-quantized) | Cheapest stride/streaming signal; no sequence state. |

Research caveat to keep in mind: a virtually-sequential stream has scattered **physical**
frame-deltas, so `LastNDeltas`/`LastNPages` will be **weaker on physical than on virtual**.
That's exactly why we evaluate them physically — measuring this gap is itself a result.

---

## Family C — Address granularities (stateless physical slices)

| Feature | Raw signal | Why it partitions off-chip / notes |
|---|---|---|
| **Region / SuperPage** ⭐ | `paddr >> k` for k>12 (e.g. 2 MB region) | Coarse working-set bucket — "this whole region is hot/cold." Complements fine offset features; genuinely new (coarser than `Page`). |
| **PhysPage** | `paddr >> 12` | Physical-page version of `Page`; the uncore-native bucket and exactly what XPT keys on. |
| **DRAM-mapping fields** ⭐ | bank / rank / channel / row bits from the phys→DRAM map | Uncore-only, physically meaningful: row-buffer locality & bank state correlate with memory latency/behavior. A novel angle the current set lacks. |
| **OffsetRegion** | `voffset >> j` (which quarter/eighth of the page) | Coarse offset bin; some workloads bias toward page regions. Cheap. |
| **BlockAddr** | `paddr >> 6` | The natural off-chip granularity, but **largely redundant with `Addr`** once hashed — include only to confirm it adds nothing. |

---

## Composite (PC-less) ideas

The current set leans on PC composites (`PC_Offset`, …); the PC-less analogs are worth
seeding too, since conditional correlations often beat marginals:
`Footprint_FirstAccess`, `PageReuse_Offset`, `Delta_Offset`, `Region_Offset`.
Let the selection search decide — don't hand-pick.

## The framing payoff

A perceptron over **PageOffchipCount + PageReuseCount + PerPageStride + Region** is a
strict generalization of XPT (XPT = the first of those, thresholded). If the PC-less
selection lands near POPET's accuracy/coverage, the thesis story is clean: *a learned,
PC-less, uncore-available predictor matches PC-based POPET and subsumes XPT.*

## Recommended seed set for the first selection run

Start the search from these 8 (covers all three families, cheap + high-prior):
`PageReuseCount, PageOffchipCount, PageFootprint, PageRecency` (A);
`LastNDeltas, PerPageStride, LastNOffsets` (B);
`Region` (C) — plus the existing PC-less ones (`Offset, Page, Addr, FirstAccess,
Offset_FirstAccess, CLOffset, CLWordOffset, CLDWordOffset`) already in the pool.

## How to evaluate

Run the existing automated feature-selection search (§6.1.3 method) restricted to the
PC-less pool above, on the predictor relocated to post-L2-miss. Compare the winning
PC-less set against (a) POPET's PC-based 5-feature set and (b) XPT, on the same traces,
using the `Core_*_offchip_pred_precision/recall` stats — accuracy (precision) and
coverage (recall) are the headline metrics, same as the XPT sweep.
