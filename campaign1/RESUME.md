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

## The loop (one iteration ≈ one wake-up, every 30–60 min)

```bash
C=/home/rahbera/thesis/runs/tuning/campaign1
ORCH=/home/rahbera/thesis/champsim-infra/scripts/cluster_run.py   # python3.12
REPO=/home/rahbera/thesis/Hermes
```

1. Read `state.json`. If `status` is `awaiting-green-signal` or `hold`: do nothing.
2. If a batch is in flight (`batches[round]` set, round not yet collected):
   `ssh kratos2 "squeue -u rahbera -h | wc -l"` — if jobs remain, sleep again.
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
   g. Publish (worktree, never the main checkout):
      `rsync -a --delete $C/ /home/rahbera/thesis/Hermes-tuning/campaign1/ &&
       git -C /home/rahbera/thesis/Hermes-tuning add campaign1 &&
       git -C /home/rahbera/thesis/Hermes-tuning commit -m "campaign1: round NN" &&
       git -C /home/rahbera/thesis/Hermes-tuning push origin tuning`
      Then notify the owner: SendUserFile of report.md with status=proactive.
4. Failed jobs (`sacct` state FAILED/TIMEOUT): retry once by resubmitting a
   pruned exp/tlist for just those pairs; if still failing, note in
   decision.md — collect.py's common-trace filter keeps scoring fair.

## Escalation → HOLD

On any protocol `escalation` trigger: set status `hold` + log entry, write
`rounds/roundNN/HOLD.md` with what happened and what decision is needed,
push + notify owner, and idle (keep checking for owner replies). Never
improvise around the protocol.

## Green-signal checklist (owner, before departure)

1. ~~Confirm address space~~ RESOLVED: physical (owner, 2026-07-03).
2. Say "go": status -> `running`, generate + launch round 1, start the loop.

## Open decisions log

- 2026-07-03: address_space — RESOLVED by owner: **physical**
  (`ocp_perc_use_physical_address=true`) — "we are designing for uncore so we
  would only get physical address." Note for the thesis: prior pcless9
  batches ran virtual; round-1 singletons are the fresh physical baseline.
- 2026-07-03: publish channel — RESOLVED by owner: branch `tuning` on the
  Hermes GitHub origin (deletable later); NOT the thesis GitLab repo.
