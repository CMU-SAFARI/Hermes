# Hermes / POPET Mechanism Illustration — Build Handoff & Plan

**Status:** plan approved (2026-07-29), ready to build in a fresh chat.
**Owner:** Rahul Bera. **Builder:** a fresh Claude Code instance ("cousin").
**Deliverable:** an interactive **web artifact** (slide deck) that explains, cleanly
and correctly, how the Hermes uncore off-chip predictor (POPET) works — for a
HiSilicon/architect audience that liked the results but did **not** follow the
page-buffer / feature-extraction / training internals.

> How to use this doc: Sections 1–5 are the plan (what to build). **Section 6 is
> the ground truth from the code — read it fully before building; it is the whole
> point of this handoff.** Section 7 is a correctness heads-up on the source deck.
> Section 8 lists the assets the cousin needs. Section 9 is first-iteration scope.

---

## 1. Context & constraints

- The architect meeting (2026-07-29) went well on results, but they did not grasp
  *how the page buffer works, how features are extracted from incoming requests,
  and how the tables are trained.* This artifact fixes exactly that.
- **Deadline: same day.** It is pure illustration — **no code, no new results, no
  experiments.**
- **The user is on an SSH-only Ubuntu server.** Viewing/vetting files on the box is
  high-overhead. This is why the deliverable is a **web artifact** (published to a
  claude.ai URL the user opens in their laptop browser), NOT a PowerPoint file to
  scp around. Iterate: edit → republish to the **same URL** → user refreshes.
- Final export: once content is locked, print-to-PDF for a handout; port to `.pptx`
  only if the architects want an editable deck. Web-now does not forfeit either.

## 2. Format decision (already made)

- **Keyboard-navigable slide deck**, 16:9, arrow keys between slides, with
  **within-slide step reveals** for the dry-run (press → to advance each micro-step;
  values light up as the request flows through the machinery).
- Build via the **Artifact tool**. **Before writing the page you MUST load the
  `artifact-design` skill** (the Artifact tool requires it). Write the HTML to a
  file, then publish with the Artifact tool; redeploys to the same file path go to
  the same URL. **Self-contained only:** inline all CSS/JS, draw diagrams as inline
  SVG/HTML+CSS (no external assets, CDNs, or fonts — a strict CSP blocks them).
  Artifacts start **private** (only the user sees them) — good for internal work.
- Make it **theme-aware** and **responsive** per the Artifact tool rules; wide
  diagrams scroll inside their own container, page body never scrolls horizontally.

## 3. Visual language

- Match the source deck's aesthetic (clean, Huawei-red section titles are fine but
  not required). **Feature color-coding — reuse consistently everywhere:**
  - **POR (Page Off-chip Ratio) = purple**
  - **LCD (Last-N cacheline Deltas) = orange/brown**
  - **PSF (Page Spatial Footprint) = green**
  - Page buffer accent = green; cores = neutral grey/blue.
- The user's slides 27–33 are the visual reference. Key ones (described so you can
  build without the PDF, but re-attach it for pixel reference — see Section 8):
  - **Slide 27 "Training POPET":** the POPET pipeline — Extract features from L2
    miss → Feature₁ (e.g. Page Off-chip Ratio, shown as `0x0aff`) → `#` hash →
    index (`42`) → Weight Table₁ → weight; … → **Σ sum weights** → **≥ τ_act**
    activation → "Predict that the load should go off-chip". Training overlay: when
    cumulative weight < τ_act but should NOT be activated, **−1** to each indexed
    weight (and symmetrically +1 the other way).
  - **Slide 28 "Which features to use?":** the full `feature_type_t` enum image, and
    the **three chosen features**: **POR** ("what fraction of requests from a page
    have gone off-chip"), **LCD** ("SPP-style sequence of last-4 cacheline delta"),
    **PSF** ("SMS-style bitmap of cachelines accessed in a physical page").
  - **Slide 29 "Mechanics to extract features at runtime":** Core→L1-D→L2→LLC
    stack; the **Page Buffer** table with columns **Tag (20b) · Access bitmap (64b)
    · Last offset (6b) · Accesses (6b) · Off-chip (6b) · LRU age (4b)**, ~14 B/entry;
    "central structure at uncore to extract features from L2 misses"; **accessed
    twice — ① during prediction (updates access bitmap, last offset, LRU age); ②
    during training (updates #accesses, #off-chip loads).** *(Improve on this slide:
    a bigger, cleaner annotated single entry.)*
  - **Slide 30 "How to use raw features for weight-table indexing?":** each raw
    feature is hashed to a table index. Documented per feature (this is the INTENDED
    hashing — see Section 6.4 for what's code-verified vs. slide-stated):
    - **POR:** page buffer gives two 6b counters (#accesses, #off-chip). ① saturate
      each to 5b; ② concatenate (#off-chip in MSB, #accesses in LSB) → 10b; ③ embed
      CPU ID in MSB and hash down.
    - **LCD:** POPET keeps a 28b shift register per core. ① shift-left 7b, OR in the
      new delta → 28b; ② embed CPU ID in MSB and hash down.
    - **PSF:** page buffer gives the 64b footprint. ① two-fold XOR → 32b; ② embed
      CPU ID in MSB and hash down.
  - **Slide 31 "Final Hermes-UnC configuration":** Lite/Normal/Big table (see
    Section 6.9 and the **slide-31 correctness note in Section 7**).

## 4. Deck outline

**Part 1 — the three structures (what each is for), ~3–4 slides**
1. Title / framing: "How POPET predicts off-chip loads — a dry run."
2. **Page Buffer** — one clean annotated entry (Tag 20b · Access bitmap 64b · Last
   offset 6b · #Accesses 6b · #Off-chip 6b · LRU 4b ≈ **14 B/entry**); shared at the
   uncore; "read at predict, updated at train."
3. **Per-core LastNDeltas register** — 28 bits = **4 × 7-bit signed intra-page
   deltas**; one register **per core**.
4. **Weight tables** — **three** (PSF / POR / LCD), **5-bit signed weights**
   [−16, +15]; one index into each per prediction; Σ vs τ_act.

**Part 2 — the dry-run (the heart): 2 cores, requests interleaved at the uncore**
- Setup slide: two cores C0/C1, ONE shared page buffer (draw tiny: 2 sets × 2 ways),
  two per-core delta registers, three small weight tables (pre-seed a few weights),
  τ_act = +2.
- Then step through the request trace in Section 5. Each request walks the full
  loop: **arrive → page-buffer lookup → extract 3 raw features → hash (+CPU-ID) → 3
  indices → sum weights vs τ_act → predict (fire DDRP if off-chip) → fate resolves →
  train weights + update page-buffer counters.**
- Closing slide: recap the trichotomy that makes multi-core work — **shared page
  buffer, per-core history registers, CPU-ID-isolated weight indexing.**

## 5. The dry-run scenario (2 cores, one shared page)

A tiny concrete world; deliberately routes C1 into a page C0 populated, so the
cross-core isolation is visible. Pages are physical-page numbers; offset = line
index within a 4 KB page (0–63). Refine exact weight values while building; keep
them numerically consistent with Section 6.

| # | Request (cpu, page, offset) | What it demonstrates |
|---|---|---|
| 1 | C0 → page A, off 5 | Page-buffer **MISS** → insert new entry (footprint = {5}, #accesses=1, #off-chip=0, last_offset=5, **no delta** — first touch). Extract features from a fresh page (POR = 0/0, PSF={5}, LCD = C0 register unchanged). Predict (sum small → likely on-chip). Fate = **off-chip** → mispredict → **incr** all 3 indexed weights; page buffer → #trained=1, #off-chip=1. |
| 2 | C0 → page A, off 9 | Page **HIT**. Set footprint bit 9 → {5,9}. delta = 9−5 = **+4** → push into **C0's** 28b register. #accesses read=1 then →2. POR now 1/1. Hash(+CPU0) → indices; Σ now ≥ τ_act → **predict OFF-CHIP → fire DDRP**. Fate = off-chip → correct; dead-band **incr**. Page buffer → #trained=2, #off-chip=2. |
| 3 | **C1 → page A, off 6** | **SHARED PAGE.** Page **HIT** on the same shared entry: footprint bit 6 set → {5,6,9} (**shared** across cores), #accesses/#off-chip are the **shared** counters. delta = 6 − last_offset(9) = **−3** → pushed into **C1's OWN** register (core-private). Now the punchline: hashing **embeds CPU=1**, so identical shared PSF/POR values map to **different weight-table slots than C0** → C1 predicts from **its own** weights, unpolluted by C0. Σ (C1 weights, still small) < τ_act → predict on-chip. Fate = off-chip → **incr C1's** weights; page buffer → #trained=3, #off-chip=3. |
| 4–5 | C1 → page A, … | C1 grows **its own** weights on the shared page across a couple more accesses; show C0 and C1 weight slots diverging for the same page. Then the recap slide. |

Optional flourish: a small side panel that persists the state (page-buffer entry,
both delta registers, the six touched weight slots) and highlights what each step
mutates.

---

## 6. GROUND TRUTH FROM THE CODE  ← read this fully

All facts below were read from the frozen Hermes source (branch `rbdev`,
single-core-identical binary `fc49264`). File:line pointers are exact so you can
re-verify. **Code-verified** items were read directly; **slide-stated** items
(the exact per-feature hash bit-ops) were taken from slide 30 — I read the dispatch
but not each `process_*` body; verify those before relying on exact bits.

### 6.1 The three data structures

**(a) Page buffer entry** — `inc/offchip_pred_perc.h:33-55`, class
`ocp_perc_page_buf_entry_t`. Fields (simulation types shown; hardware bit-widths
from slide 29 / the storage accounting):
- `uint64_t page;` — the **tag** (physical page number, `addr >> LOG2_PAGE_SIZE`).
  HW: ~**20-bit** partial tag.
- `Bitmap bmp_access;` — the **spatial footprint**. `Bitmap = std::bitset<64>`
  (`inc/bitmap.h:6-8`, `BITMAP_MAX_SIZE = 64`). One bit per cache line in a 4 KB
  page (4096/64 = **64 lines** → 64 bits). This is the **PSF** raw feature.
- `uint32_t age;` — LRU age. HW ~4 bits.
- `uint32_t reuse_count;` — # accesses to this page while resident ("#accesses").
- `uint32_t offchip_count;` — # trained-off-chip outcomes while resident ("#off-chip").
- `uint32_t trained_count;` — # trained outcomes either way (the POR denominator).
- `uint32_t last_offset;` — line offset of the last access (for the intra-page delta).
  HW 6 bits.
- **HW entry total ≈ 14 B = 112 bits** = 64 (bitmap) + ~20 (tag) + 6 (last_offset)
  + ~4 (age) + ~18 (the 6-bit counters). The **64-bit bitmap dominates**; the tag is
  a small fraction — a good talking point.

The buffer itself: `inc/offchip_pred_perc.h:61`
`vector<deque<ocp_perc_page_buf_entry_t*>> m_page_buffer;` — **set-associative,
shared across all cores** (no per-cpu indexing). Sets = `ocp_perc_page_buf_sets`,
ways = `ocp_perc_page_buf_assoc`. LRU is deque order (front = LRU, back = MRU).

**(b) Per-core history registers** — `inc/offchip_pred_perc.h:66-69`:
- `deque<uint64_t> last_n_load_pcs[NUM_CPUS];` — per-core (not used by the 3
  shipping features, but shows the per-core-history pattern).
- `uint32_t last_n_deltas_sig[NUM_CPUS];` — **the LCD register: 28-bit, per core**,
  "last 4 intra-page deltas of the request stream, 7-bit signed each, packed as a
  28-bit shift register." This is the **LCD** raw feature.

**(c) Weight tables** — `inc/perc_pred.h:102-114` `weight_array_t { uint32_t size;
vector<float> array; }`; the predictor holds `vector<weight_array_t> weights;`
(`inc/perc_pred.h:129`), **one array per activated feature**. HW: each weight is a
**5-bit signed integer**, range **[−16, +15]** — this is exactly the saturation
bound `ocp_perc_max_weight = 15`, `ocp_perc_min_weight = -16`
(`config/ocp_hermes.ini:10-11`, `inc/knobs.def:402-403`).

### 6.2 The features & the config mapping (IMPORTANT — get this right)

Feature enum — `inc/perc_pred.h:35-51` (values in comments), names in
`src/perc_pred.cc:24-54`:
- **20 = `PageSpatialFootprint` (PSF)** — SMS-style 64-bit access bitmap of a page.
- **21 = `PageMissRatio` (a.k.a. POR, "Page Off-chip Ratio")** — fraction of a
  page's loads that have gone off-chip, from the #off-chip / #trained counters.
- **22 = `LastNDeltas` (LCD)** — the 28-bit per-core delta shift register.

The shipping configs activate exactly these three: `ocp_perc_activated_features =
20,21,22`. The weight-array sizes are paired **positionally** with the feature list
(`src/perc_pred.cc:92-94` pairs `weight_array_sizes[i]` with `activated_features[i]`;
`src/perc_pred.cc:274-285` indexes `weights[i]` with `activated_features[i]`). So:

| Feature (position) | Lite | Normal | Big |
|---|---|---|---|
| **PSF** (20, pos 0) | 2048 | 8192 | 8192 |
| **POR** (21, pos 1) | 1024 | 1024 | 1024 |
| **LCD** (22, pos 2) | 1024 | 4096 | 16384 |

(From `config/hermes_uncore_{lite,normal,big}.ini` `ocp_perc_weight_array_sizes =
2048,1024,1024 / 8192,1024,4096 / 8192,1024,16384`.)

### 6.3 Predict path — feature extraction & page-buffer read

`OffchipPredPerc::get_data_flow_signatures(state, addr, req_cpu)` —
`src/offchip_pred_perc.cc:288-367`. Runs on every request that reaches the uncore.
Steps (all code-verified):
1. Address decomposition (`:293-300`): `page = addr >> LOG2_PAGE_SIZE` (12 →
   4 KB pages); `offset = (addr >> LOG2_BLOCK_SIZE) & 63` (6 → 64 B lines, line
   index 0–63); plus `page_offset_region` (unused by the 3 features).
2. `set = get_set(page)` = `hash % ocp_perc_page_buf_sets` (`:285`), find the entry
   with matching `page` in that set (`:310-313`).
3. **Page HIT (`:315-338`):**
   - `first_access = !bmp_access.test(offset)` then `bmp_access.set(offset)` — set
     this line's footprint bit. `page_spatial_footprint = value(bmp_access)`
     **including the current access** (the PSF raw feature).
   - `age = 0` and LRU-promote: `erase(it); push_back(entry)` (`:336-337`).
   - `page_reuse_count = entry->reuse_count` (prior count), then `reuse_count++`.
   - `page_offchip_count = entry->offchip_count`, `page_trained_count =
     entry->trained_count` (the POR numerator/denominator, as of now).
   - **Delta / LCD register update (`:331-335`):**
     `delta = (int)offset − (int)last_offset; last_offset = offset;`
     `last_n_deltas_sig[req_cpu] = ((sig << 7) | (delta & 0x7F)) & 0x0FFFFFFF;`
     → a **7-bit signed delta pushed into that CORE's 28-bit register.** Intra-page
     only (never across pages). This is the LCD raw feature.
4. **Page MISS / insert (`:339-363`):** evict LRU front if the set is full
   (`:340-345`); new entry with `bmp_access={offset}`, `reuse_count=1`,
   `offchip_count=0`, `trained_count=0`, `last_offset=offset`; `first_access=true`;
   **no delta pushed** (no prior offset in this page).
5. `info->last_n_deltas_sig = last_n_deltas_sig[req_cpu]` (`:366`).

So the **predict-time page-buffer writes** are: footprint bit, last_offset,
reuse_count, LRU/age — matching slide 29's "① during prediction."

### 6.4 Hashing raw feature → table index

`perceptron_pred_t::generate_index_from_feature(feature, state, metadata,
hash_type, weight_array_size)` — dispatch at `src/perc_pred_helper.cc:397-487`
(code-verified dispatch). For the three features it calls
`process_PageSpatialFootprint` / `process_PageMissRatio` / `process_LastNDeltas`
(`:452-458`) — **their bodies are elsewhere in `src/perc_pred_helper.cc`; I did NOT
read the exact bit-ops.** Use **slide 30** as the intended behavior (Section 3), and
**read those `process_*` functions to confirm exact bits before finalizing any
number in the artifact:**
- PSF: 64b footprint → two-fold XOR to 32b → embed CPU ID → fold to `weight_array_size`.
- POR: two 6b counters → saturate to 5b each → concat (#off-chip MSB, #accesses LSB)
  = 10b → embed CPU ID → fold.
- LCD: 28b register → embed CPU ID → fold.
- The shared knobs used: `hash_type = 2` and `feature_region_size_log2 = 21`
  (2 MB) per feature (from the shipping ini). `metadata` param carries the region
  size log2. The final index is reduced to `weight_array_size`.
- **The CPU-ID embed is the cross-core isolation mechanism** (see 6.7). Confirmed by
  the design comment at `inc/perc_pred.h:60` ("requesting cpu, embedded into every
  feature index for cross-core weight …").

### 6.5 Prediction

`perceptron_pred_t::predict(state, &prediction, &perc_weight_sum)` —
`src/perc_pred.cc:141-165` (code-verified): generate one index per feature, **sum
the weights at those indices**, `prediction = (sum >= activation_threshold)`.
`activation_threshold = τ_act = +2` (shipping). If predicted off-chip and DDRP is
enabled, a speculative direct-DRAM read is fired (the "action").

### 6.6 Training (weights + page-buffer counters)

Two updates happen when the true fate (did the load actually go off-chip = LLC
miss) is known:

**(a) Weight update** — `perceptron_pred_t::train(state, perc_weight_sum,
pred_output, true_output)` — `src/perc_pred.cc:167-222` (code-verified). Logic:
- `true_output == true` (really off-chip):
  - correct (`pred==true`): **only if** `neg_train_thresh ≤ sum ≤ pos_train_thresh`
    → `incr_weights` (dead-band; skip if already confident). Else nothing.
  - wrong (`pred==false`): **always** `incr_weights`.
- `true_output == false` (really on-chip): symmetric with `decr_weights`.
- Dead-band = `[neg_train_thresh, pos_train_thresh] = [−21, +8]` (shipping;
  asymmetric → precision-leaning). `incr_weights` (`:224-247`) adds
  `pos_weight_delta` to each indexed weight, **saturating at `max_weight = +15`**;
  `decr_weights` (`:249-272`) subtracts `neg_weight_delta`, **saturating at
  `min_weight = −16`**. Deltas are the standard **±1** (slide 27; confirm
  `ocp_perc_pos/neg_weight_delta` in the ini).

**(b) Page-buffer counter update** — `OffchipPredPerc::record_page_outcome(page,
went_offchip)` — `src/offchip_pred_perc.cc:369-386` (code-verified): find the page's
entry; `trained_count++`; if `went_offchip` also `offchip_count++`. **Deliberately
no LRU promotion and no insert on miss** — the train path must not perturb
predict-path state (the page may have been evicted since predict). This is slide
29's "② during training."

### 6.7 Cross-core isolation (the multi-core punchline)

Three facts, all confirmed above, combine:
1. **Page buffer is SHARED** (`m_page_buffer` is one structure; entries keyed by
   physical page, not cpu). A page touched by C0 and C1 shares one entry → the
   footprint and #accesses/#off-chip counters accumulate across cores. This is
   physically correct: LLC residency is a shared-machine property.
2. **History registers are PER-CORE** (`last_n_deltas_sig[req_cpu]`,
   `last_n_load_pcs[req_cpu]`). Each core's delta sequence is private, because the
   uncore interleaves cores and a global register would shred each core's pattern.
3. **CPU ID is embedded into every feature's weight-table index** (slide 30 step
   "embed CPU ID in MSB", design comment `inc/perc_pred.h:60`). So for the *same
   shared* PSF/POR value, C0 and C1 land in **different weight slots** → per-core
   weights, no cross-core pollution. (This is the "isolate cross-core interference
   at the shared uncore perc predictor" work, commit 5d97ad9.)

The dry-run in Section 5 is designed to surface all three at request #3.

### 6.8 Two-phase page-buffer access per request (slide 29)

- **Phase ① at predict** (`get_data_flow_signatures`): footprint bit, last_offset,
  reuse_count, LRU/age.
- **Phase ② at train** (`record_page_outcome`, once fate known): #trained, #off-chip.

### 6.9 Operating-point knobs (shipping "Normal" = Hermes-UnC)

From `config/hermes_uncore_normal.ini` layered on `config/ocp_hermes.ini`:
- `ocp_perc_activated_features = 20,21,22` (PSF, POR, LCD)
- `ocp_perc_weight_array_sizes = 8192,1024,4096`
- `ocp_perc_feature_hash_types = 2,2,2`
- `ocp_perc_feature_region_size_log2s = 21,21,21` (2 MB)
- `ocp_perc_activation_threshold = 2` (τ_act)
- `ocp_perc_pos_train_thresh = 8`, `ocp_perc_neg_train_thresh = -21`
- `ocp_perc_max_weight = 15`, `ocp_perc_min_weight = -16` (→ 5-bit weights)
- `ocp_perc_page_buf_sets = 32`, `ocp_perc_page_buf_assoc = 16`
- `offchip_pred_location = uncore`, `ocp_perc_use_physical_address = true`

Storage breakdown (verified this session; useful for the Part 1 slide):
| Version | Page buffer | Weight tables | Total |
|---|---|---|---|
| Lite | 1.75 KB (128 e × 14 B) | 2.50 KB (4096 w × 5 b) | 4.25 KB |
| Normal | 7.00 KB (512 e) | 8.125 KB (13312 w) | 15.125 KB |
| Big | 14.00 KB (1024 e) | 15.625 KB (25600 w) | 29.625 KB |

### 6.10 Source file map

- `inc/offchip_pred_perc.h` — page-buffer entry, shared buffer, per-core registers.
- `src/offchip_pred_perc.cc` — feature extraction / page-buffer read (`:288-367`),
  train-time counter update (`:369-386`), address decomposition (`:293-300`).
- `inc/perc_pred.h` — feature enum (`:35-51`), weight_array/perceptron classes.
- `src/perc_pred.cc` — predict (`:141-165`), train (`:167-222`), incr/decr
  (`:224-272`), index generation (`:274-285`).
- `src/perc_pred_helper.cc` — per-feature hashing (`generate_index_from_feature`
  dispatch `:397-487`; the `process_*` bodies elsewhere — read them for exact bits).
- `inc/bitmap.h` — `Bitmap = bitset<64>`.
- `config/hermes_uncore_{lite,normal,big}.ini`, `config/ocp_hermes.ini`,
  `inc/knobs.def` — the knobs.

---

## 7. Correctness heads-up on the source deck (slide 31)

On **slide 31**, the **LCD and PSF weight-table-entry columns appear swapped for
Lite and Normal** (Big is correct). Ground truth (Section 6.2): features are
20/21/22 = PSF/POR/LCD, sizes paired positionally, so **PSF = 2048/8192/8192** and
**LCD = 1024/4096/16384**. The slide shows PSF = 1024/4096/8192 and LCD =
2048/8192/16384. Verified against the `.ini` files and the perceptron's positional
feature↔weight-array pairing (`src/perc_pred.cc:92-94, 274-285`). **The artifact
must use the correct mapping**, and it's worth telling the user to fix slide 31
before the next showing. (Already flagged to the user 2026-07-29.)

## 8. Assets the cousin needs

- **This doc** — the plan + code ground truth.
- **The source deck PDF** — `Hermes_HiSilicon_SPEC26_20260729.pdf` (slides 27–33 are
  the relevant ones). It was uploaded into the *previous* chat; that upload does NOT
  carry over. **Ask the user to re-attach it in the new chat** for pixel reference
  (Section 3 describes those slides well enough to start without it).
- **SMS paper (ISCA'06)** for stepwise-figure inspiration (user's suggestion):
  `https://jilp.org/vol13/v13paper8.pdf` — look at **figs 2, 3, 4, 5** (accumulation
  table → pattern history table, region-based, a running example). *Inspiration for
  the dry-run style, not a spec to copy.*
- **The `artifact-design` skill** — load it before writing the HTML (required by the
  Artifact tool).
- The Hermes source (branch `rbdev`) for any re-verification (Section 6.10).

## 9. First-iteration scope & open questions

- **First iteration = get the skeleton + one full dry-run request working**, in the
  agreed style, so the user can vet the look-and-feel and the step-reveal mechanic
  over SSH quickly. Then flesh out all 5 requests and Part 1.
- Keep numbers **small and self-consistent**; don't invent a hash output — either
  read `process_*` for the exact fold (Section 6.4) or present the index abstractly
  ("hash → slot 42") the way slide 27 does, and be explicit which you chose.
- Open questions to confirm with the user during build: (a) how much of the exact
  hashing to show vs. abstract "→ slot N"; (b) whether to include the DDRP action
  visually or keep the focus on predict/train; (c) deck length target.
