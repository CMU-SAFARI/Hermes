# Final single-core projection run — staging (awaiting owner GO)

Goal (owner, 2026-07-09): finalize Hermes@uncore for the full SPEC suites
and hand results to the architects — Hermes vs Intel XPT on performance
AND prediction quality (precision/recall), standalone and with Pythia.

- Repo/binary: /home/rahbera/thesis/Hermes @ rbdev fc49264 (b18be0c
  semantics + frozen version inis), existing 1-core cluster path.
- Traces: runs/cluster/spec26.rate-int.yml (144) + spec26.speed-int.yml
  (260) = 404, all v2. Two batches: projection_rate, projection_speed.
- Window: 100M warmup + 500M sim (full). Walltime 12h.
- 10 experiments (exp_proj.yml): nopf | pythia | lite/normal/big
  (no Pythia) | lite/normal/big + Pythia | xpt@uncore(physical) |
  xpt + Pythia. DDRP action enabled on all predictor rows.
- Jobs: 10 x 404 = 4,040.
- mfile: campaign-1 mfile (ipc + LLC_offchip_pred precision/recall).
- Scoring: per-suite and combined; geomean IPC speedups vs proj_nopf and
  vs proj_pythia (both columns for every row); mean precision/recall for
  the 8 predictor rows. Headline contrasts: Hermes-X vs XPT (standalone),
  Hermes-X+Pythia vs XPT+Pythia, per version.
- Smokes done: hermes-without-pythia + xpt@uncore+physical both run and
  report predictor stats on the frozen binary.
