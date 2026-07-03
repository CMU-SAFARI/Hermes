#!/usr/bin/env python3
"""Generate the next round's exp.yml + manifest.json for campaign1.

Usage: gen_round.py <campaign_dir>

Reads protocol.yml + state.json.
  state.round == 0  -> round 1: all 56 singleton configs + nopf/pythia
                       baselines, each ALSO duplicated at the full window
                       (window-fidelity check).
  state.round == N  -> round N+1: every beam member extended by every
                       candidate token not already in the set, deduped by
                       canonical key against the memo and within the round.

Writes rounds/roundNN/{exp.yml,manifest.json}; prints a launch summary.
Refuses to write when: address_space is unconfirmed, the job count exceeds
per_launch_cap, or the total budget would be exceeded.
"""
import json
import os
import sys

import yaml

BASE_ARGS = (
    "--warmup_instructions={warm} --simulation_instructions={sim} "
    "--llc_replacement_type=ship "
    "--config=$(SIM_HOME_IN_CLUSTER)/config/nopref.ini "
    "--num_rob_partitions=3 --rob_partition_size=64,128,320 "
    "--rob_frontal_partition_ids=0 --rob_dorsal_partition_ids=2"
)
PYTHIA = (
    "--l2c_prefetcher_types=scooby "
    "--config=$(SIM_HOME_IN_CLUSTER)/config/pythia.ini "
    "--scooby_enable_direct_pref_issue=true --scooby_pref_at_lower_level=true "
    "--scooby_dyn_degrees_type2=1,1,2,4"
)
OCP_UNCORE = (
    "--config=$(SIM_HOME_IN_CLUSTER)/config/ocp_hermes.ini "
    "--offchip_pred_location=uncore"
)
HERMES = "--config=$(SIM_HOME_IN_CLUSTER)/config/hermes_base.ini --ddrp_req_latency=1"

NONREGION_SIZE_FILL = 21  # value carried in the region-size vector for non-region slots


def tok_str(feat, size):
    return f"f{feat}" if size is None else f"f{feat}@{size}"


def tok_parse(s):
    if "@" in s:
        f, z = s.split("@")
        return (int(f[1:]), int(z))
    return (int(s[1:]), None)


def canonical(tokens):
    """tokens: list of (feat, size|None) -> sorted unique canonical key."""
    ts = sorted(set(tokens), key=lambda t: (t[0], -1 if t[1] is None else t[1]))
    return "+".join(tok_str(f, z) for f, z in ts), ts


def candidate_pool(proto):
    c = proto["candidates"]
    pool = [(f, None) for f in c["non_region_features"]]
    pool += [(f, z) for f in c["region_features"] for z in c["region_size_log2s"]]
    return pool


def config_args(tokens, proto, window):
    """Build the experiment arg string for a canonical token list."""
    n = len(tokens)
    thr = proto["thresholds_by_n"][n]
    fixed = proto["fixed_knobs"]
    feats = ",".join(str(f) for f, _ in tokens)
    wts = ",".join(str(fixed["weight_array_size_per_feature"]) for _ in tokens)
    hsh = ",".join(str(fixed["feature_hash_type"]) for _ in tokens)
    rsz = ",".join(str(NONREGION_SIZE_FILL if z is None else z) for _, z in tokens)
    base = "$(TBASE)" if window == "tuning" else "$(WBASE)"
    phys = (" --ocp_perc_use_physical_address=true"
            if proto["system"]["address_space"] == "physical" else "")
    return (f"{base} $(PYTHIA) $(OCP_UNCORE){phys} "
            f"--ocp_perc_activated_features={feats} "
            f"--ocp_perc_weight_array_sizes={wts} "
            f"--ocp_perc_feature_hash_types={hsh} "
            f"--ocp_perc_feature_region_size_log2s={rsz} "
            f"--ocp_perc_activation_threshold={thr[0]} "
            f"--ocp_perc_pos_train_thresh={thr[1]} "
            f"--ocp_perc_neg_train_thresh={thr[2]} "
            f"$(HERMES)")


def main():
    cdir = os.path.abspath(sys.argv[1])
    proto = yaml.safe_load(open(os.path.join(cdir, "protocol.yml")))
    state = json.load(open(os.path.join(cdir, "state.json")))

    if not proto["system"].get("address_space_confirmed", False):
        sys.exit("REFUSING: system.address_space_confirmed is false in protocol.yml")

    rnd = state["round"] + 1
    memo = set(state["memo"].keys())

    # --- build the round's config list -------------------------------------
    entries = []  # (exp_name, tokens, window, role)
    if rnd == 1:
        sets = [canonical([t]) for t in candidate_pool(proto)]
        for i, (key, ts) in enumerate(sets):
            entries.append((f"r01_c{i:03d}", ts, "tuning", "config"))
            entries.append((f"r01_c{i:03d}_wf", ts, "full", "config"))
        for nm, role in (("nopf", "baseline_nopf"), ("pythia", "baseline_pythia")):
            entries.append((f"r01_{nm}", [], "tuning", role))
            entries.append((f"r01_{nm}_wf", [], "full", role))
    else:
        if not state["beam"]:
            sys.exit("REFUSING: no beam in state.json for an extension round")
        cap = proto["stop"]["max_features"]
        emitted, i = set(), 0
        for member in state["beam"]:
            parent = [tok_parse(t) for t in member["tokens"]]
            if len(parent) >= cap:
                continue
            for cand in candidate_pool(proto):
                if cand in parent:
                    continue
                key, ts = canonical(parent + [cand])
                if key in memo or key in emitted:
                    continue
                emitted.add(key)
                entries.append((f"r{rnd:02d}_c{i:03d}", ts, "tuning", "config"))
                i += 1

    # --- budget gates --------------------------------------------------------
    traces = yaml.safe_load(open(proto["system"]["tlist"]))
    ntraces = len(next(iter(traces.values())))
    njobs = len(entries) * ntraces
    if njobs > proto["stop"]["per_launch_cap"]:
        sys.exit(f"REFUSING: {njobs} jobs exceeds per_launch_cap")
    if state["jobs_total"] + njobs > proto["stop"]["total_job_budget"]:
        sys.exit(f"REFUSING: {njobs} jobs would exceed total_job_budget")

    # --- write exp.yml + manifest.json ---------------------------------------
    rdir = os.path.join(cdir, "rounds", f"round{rnd:02d}")
    os.makedirs(rdir, exist_ok=True)
    wt, wf = proto["system"]["window_tuning"], proto["system"]["window_full"]
    lines = [
        "---",
        f"# campaign1 round {rnd} — generated by gen_round.py (DO NOT EDIT BY HAND)",
        "definitions:",
        f'  - TBASE: "{BASE_ARGS.format(warm=wt["warmup"], sim=wt["sim"])}"',
        f'  - WBASE: "{BASE_ARGS.format(warm=wf["warmup"], sim=wf["sim"])}"',
        f'  - PYTHIA: "{PYTHIA}"',
        f'  - OCP_UNCORE: "{OCP_UNCORE}"',
        f'  - HERMES: "{HERMES}"',
        "experiments:",
    ]
    manifest = {}
    for name, ts, window, role in entries:
        if role == "baseline_nopf":
            base = "$(TBASE)" if window == "tuning" else "$(WBASE)"
            args = base
        elif role == "baseline_pythia":
            base = "$(TBASE)" if window == "tuning" else "$(WBASE)"
            args = f"{base} $(PYTHIA)"
        else:
            args = config_args(ts, proto, window)
        lines.append(f"  - {name} : {args}")
        manifest[name] = {
            "key": canonical(ts)[0] if ts else role,
            "tokens": [tok_str(f, z) for f, z in ts],
            "window": window,
            "role": role,
        }
    with open(os.path.join(rdir, "exp.yml"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(rdir, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)

    ncfg = sum(1 for e in entries if e[3] == "config" and e[2] == "tuning")
    print(f"round={rnd} configs={ncfg} experiments={len(entries)} "
          f"traces={ntraces} jobs={njobs}")
    print(f"exp={os.path.join(rdir, 'exp.yml')}")


if __name__ == "__main__":
    main()
