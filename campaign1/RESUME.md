# Campaign 1 — autonomous beam search over PC-less OCP features (uncore)

**If you are a fresh Claude session: this file + `protocol.yml` + `state.json`
are the complete truth. The conversation that created them is disposable.**

Owner: Rahul Bera (traveling, reachable async). Authorizations, budgets,
convergence rules: see `protocol.yml` (FROZEN — do not edit except the
`address_space_confirmed` flag on the owner's green signal).

## Standing constraints

- **No simulator code changes, no rebuilds.** Before every submit verify:
  `git -C /home/rahbera/thesis/Hermes status --short` shows nothing but `?? docs/`,
  and `git rev-parse HEAD` == `code.head_commit` in protocol.yml.
- ≤ 15,000 jobs per launch; 120,000 total (gen_round.py enforces both).
- Publish campaign artifacts on branch `tuning` of the Hermes GitHub origin,
  via the dedicated worktree `/home/rahbera/thesis/Hermes-tuning` — NEVER from
  the main checkout. The main Hermes checkout (rbdev) stays frozen at
  `head_commit`; never commit or switch branches there during the campaign.
  (Do NOT use the thesis repo's ETH GitLab remote — owner ruled it out.)

## The loop (one iteration ≈ one wake-up; self-paced, no owner tuning needed)

Wake-up cadence policy: 60 min while a batch has many jobs queued/running
(rounds are day-scale; hourly checks cost <5% added latency); tighten to
20–30 min once the batch is nearly drained so the round transition starts
promptly; no sleeping during a transition (rollup -> decision -> launch is
one continuous wake-up); hourly heartbeat on hold/converged.

```bash
C=/home/rahbera/thesis/runs/tuning/campaign1
ORCH=/home/rahbera/thesis/champsim-infra/scripts/cluster_run.py   # python3.12
REPO=/home/rahbera/thesis/Hermes
```

1. Read `state.json`. If `status` is `awaiting-green-signal` or `hold`: do nothing.
2. If a batch is in flight (`batches[round]` set, round not yet collected):
   `ssh kratos2 "squeue -u rahbera -h -o %j" | grep -c "_r<NN>_"` — if jobs
   remain, sleep again. **squeue ONLY — never sacct** (owner directive: sacct
   hits the accounting DB and is far heavier; squeue is the cheap poll).
   Filter by the round's `_rNN_` exp-name pattern: unrelated batches (owner's
   or pre-campaign ones) may share the queue, so a bare user-wide count lies.
3. When the round's jobs are done:
   a. `python3.12 $ORCH rollup --repo $REPO --batch <batch_id>` → fetches
      `.cluster-run/runs/<batch>/stats.csv`.
   b. `python3 $C/scripts/collect.py $C <round> <stats.csv>` → rollup.csv,
      top5.json, report.md.
   c. **Judgment step** (you, not a script): read report.md — sanity of
      baselines vs earlier rounds, flags, sibling degeneracy (≥4 of top-5
      sharing one parent → consider capping children at 3, see protocol).
      Write `rounds/roundNN/decision.md`: the chosen top-5 + why (or why you
      deviated from top5.json). Retry/failure notes go here too.
   d. Update `state.json` (atomically: write tmp, `os.replace`): round=N,
      beam=chosen 5 (tokens+score), fold rollup.csv into `memo`
      (key -> geo_nopf), append `log` entry, add batch job count to
      `jobs_total`, record `best_geomean_by_round[N]`.
   e. **Convergence check** (protocol `stop`): best geomean improved < 0.2%
      relative vs previous round, OR all extensions regressed, OR next round
      would exceed 8 features / job budget → write `FINAL_REPORT.md`
      (winner, full beam history, greedy curve, proposals), set status
      `converged`, commit+push, notify owner, stop looping.
   f. Otherwise: `python3 $C/scripts/gen_round.py $C` → next round's exp.yml.
      Verify code freeze (above). Launch **in background** (build+smoke take
      minutes):
      `python3.12 $ORCH submit --repo $REPO --tlist <protocol tlist> \
         --exp $C/rounds/roundNN/exp.yml --mfile $C/mfile.yml \
         --label campaign1_rNN`
      Record batch_id in state.json.
      **Then right-size walltime immediately** (learned r01: the submit tool's
      24h default blocks backfill behind higher-priority users — 8h starvation
      with 620 CPUs idle; scontrol TimeLimit -> 4:00:00 unblocked it in
      seconds). One ssh, server-side:
      `squeue -u rahbera -h -o "%i %j" | awk '$2 ~ /_rNN_/ {print $1}' |
       while read i; do scontrol update jobid=$i TimeLimit=4:00:00; done`
      **4h is the FLOOR — never trim below it.** Measured r01: the slowest
      suite trace (853.ns3 tcp_validation, IPC 0.11) needs ~3h05m for
      50M+200M; a 3h trim killed 71 jobs minutes before completion (retried
      at 5h). Only round 1 had 12h _wf twins.
   g. Publish (worktree, never the main checkout):
      `rsync -a --delete $C/ /home/rahbera/thesis/Hermes-tuning/campaign1/ &&
       git -C /home/rahbera/thesis/Hermes-tuning add campaign1 &&
       git -C /home/rahbera/thesis/Hermes-tuning commit -m "campaign1: round NN" &&
       git -C /home/rahbera/thesis/Hermes-tuning push origin tuning`
      Then notify the owner: SendUserFile of report.md with status=proactive.
4. Failed jobs: detect from the rollup OUTPUTS, not Slurm accounting — the
   infra rollup's `--report-json` / stats.csv Filter column marks every
   (trace,exp) whose .out is bad. Retry once by resubmitting a pruned
   exp/tlist for just those pairs; if still failing, note in decision.md —
   collect.py's common-trace filter keeps scoring fair.

## Escalation → HOLD

On any protocol `escalation` trigger: set status `hold` + log entry, write
`rounds/roundNN/HOLD.md` with what happened and what decision is needed,
push + notify owner, and idle (keep checking for owner replies). Never
improvise around the protocol.

## Green-signal checklist (owner, before departure)

1. ~~Confirm address space~~ RESOLVED: physical (owner, 2026-07-03).
2. Say "go": status -> `running`, generate + launch round 1, start the loop.

## Phase 2: precision sweeps (owner-approved 2026-07-06, post-convergence)

The beam search converged at round 2 (f21+f22, +0.171% < eps). The owner then
amended the goal based on the bandwidth analysis (per-trace precision<->IPC
correlation +0.95 on fotonik3d/graph500-3313B while suite-wide only +0.36 —
ample DRAM bandwidth masks precision in the geomean): **the selection
criterion for the final config is now precision-first with a justifiable
recall trade-off**, evaluated on precision-recall CURVES (threshold-swept),
not fixed-threshold points.

Mechanics (all reusable autonomously):
- `scripts/gen_sweep.py <campaign> <sweepNN>` reads `rounds/<sweepNN>/spec.json`
  (entries = token-set + list of activation thresholds; pos/neg train stay at
  the N-rule) and writes exp.yml + manifest.json. Keys are `<set>|act=<A>`.
- Launch/collect/publish exactly like a beam round (same orchestrator, same
  collect.py, baselines from baselines.json). Walltime right-size: **6h**
  (sweep contains triples; r02 lesson).
- Judgment for sweeps: overlay P-R curves per token-set; decision rule =
  does any probe triple's curve dominate f21+f22's curve at precision >= ~82%?
  If no: feature set CLOSED at f21+f22, pick the operating point (owner call
  on the precision floor). If yes: the dominating third feature earns a
  follow-up sweep. Report per-trace fotonik3d/graph500-3313B columns too —
  they are the bandwidth-sensitive canaries.
- sweep01 (30 configs, 1500 jobs; revised per owner 2026-07-06): f21+f22 act
  {-15..+7 step 2} (12 pts — owner dropped +9, added -15); probe triples
  +f20/+f18/+f27@17/+f30@20 at act {-10,-4,+2}; f20+f21 at {-13,-9,-5,-1,+3,+7}
  (6 pts — owner expanded). AWAITING OWNER GO — do not submit until the owner
  says go.

**OVERNIGHT AUTHORIZATION (owner, 2026-07-06 evening):** while the owner is
offline (back in the morning), the operator MAY launch further exploration
sweeps at own judgment to use the free cluster — each gated on the evidence
before it, within all standing caps. Planned ladder: (1) val02 = sweep02
configs x 147 traces once sweep02 scores; (2) sweep03 = pos/neg
train-threshold grid on the leading triple at its 2 best act points;
(3) sweep04 = weight-table-size sweep on the triple; (4) val03 = full-window
(100M+500M) confirmation of the leading operating points on 147 traces.
gen_sweep.py now supports per-entry `pos_train`/`neg_train`/`weight_sizes`
overrides (sizes written in entry token order, sorted canonically with the
tokens; keys gain |tr=… / |w=… suffixes).

Queued follow-up sweeps (owner-declared 2026-07-06, run AFTER the feature set
is closed via sweep01, each as its own sweepNN with owner vet of the spec —
SUPERSEDED for tonight by the overnight authorization above):
1. **pos/neg train-threshold sweep** on the chosen set at its chosen
   activation point (the train thresholds set weight saturation/adaptivity;
   only act was swept so far).
2. **weight-table-size sweep** on the chosen set (owner expects low
   sensitivity for f21 — its natural index space is 2^10 so 65536 is
   oversized — but f22's 28-bit signature may care; this sweep sizes the
   actual hardware budget).

## Open decisions log

- 2026-07-03: address_space — RESOLVED by owner: **physical**
  (`ocp_perc_use_physical_address=true`) — "we are designing for uncore so we
  would only get physical address." Note for the thesis: prior pcless9
  batches ran virtual; round-1 singletons are the fresh physical baseline.
- 2026-07-03: publish channel — RESOLVED by owner: branch `tuning` on the
  Hermes GitHub origin (deletable later); NOT the thesis GitLab repo.
