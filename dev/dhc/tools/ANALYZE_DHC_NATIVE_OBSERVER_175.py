#!/usr/bin/env python3
"""Summarize append-only 87.175 native observation; read-only, no game state writes.

Accepts repeated session markers and headers. Never treats post-CPU action
snapshots as proof of a specific 68000 PC/branch being executed.
"""
import argparse
import json
from collections import Counter
from pathlib import Path

REQUIRED = ("frame", "movie_frame", "stage", "side", "slot", "fighter",
            "phase", "inputSynthetic", "d6", "modeA0", "action",
            "script34", "c0", "c2", "c4", "c6", "c8", "ca")


def consume(path):
    counts = Counter()
    candidate_events = []
    prior_cpu = {}
    prior_host = {}
    session = 0
    last_frame = None
    header = None
    for line_number, raw in enumerate(path.open(encoding="utf-8-sig", errors="replace"), 1):
        line = raw.rstrip("\r\n")
        if line.startswith("# XM87.175 OBSERVER SESSION"):
            session += 1
            prior_cpu.clear()
            prior_host.clear()
            last_frame = None
            header = None
            counts["sessions"] += 1
            continue
        if line.startswith("frame\tmovie_frame\tstage\t"):
            header = line.split("\t")
            if any(k not in header for k in REQUIRED):
                counts["invalid_headers"] += 1
                header = None
            continue
        if not line or line.startswith("#"):
            continue
        if not header:
            counts["unparsed_rows"] += 1
            continue
        parts = line.split("\t")
        if len(parts) != len(header):
            counts["malformed_rows"] += 1
            continue
        r = dict(zip(header, parts))
        counts["rows"] += 1
        stage = r["stage"]
        counts[stage] += 1
        if r["inputSynthetic"] == "1" and stage == "POST_CPU_PRE_DHC":
            counts["synthetic_input_actor_rows"] += 1
        if stage == "POST_HOST_DHC_CHANGED":
            counts["host_changed_actor_rows"] += 1
            prior_host[(session, r["side"], r["slot"])] = {
                k: r[k] for k in ("fighter", "d6", "modeA0", "action", "script34")
            }
        if stage == "POST_HOST_DHC_PHASE_ONLY":
            counts["phase_only_actor_rows"] += 1
        if stage != "POST_CPU_PRE_DHC":
            continue
        try:
            observed_frame = int(r["frame"])
        except ValueError:
            counts["malformed_frames"] += 1
            continue
        if last_frame is not None and observed_frame < last_frame:
            prior_cpu.clear()
            prior_host.clear()
            counts["frame_resets_within_session"] += 1
        last_frame = observed_frame
        key = (session, r["side"], r["slot"])
        current = {k: r[k] for k in ("fighter", "d6", "modeA0", "action", "script34")}
        prior = prior_cpu.get(key)
        host = prior_host.pop(key, None)
        # A sampled post-CPU state is not guaranteed to have been caused by a
        # particular code PC. The prior *host* snapshot is not a CPU trace.
        try:
            action = int(r["action"], 16)
        except ValueError:
            counts["malformed_actions"] += 1
            continue
        event = None
        if action >> 8 == 0x2E and (prior is None or prior["action"] != r["action"]):
            if host is not None and host["action"] == r["action"]:
                counts["post_cpu_2e_echo_of_host_write"] += 1
                event = "POST_CPU_2E_ECHOES_PRIOR_HOST_WRITE"
            else:
                counts["post_cpu_2e_action_transitions"] += 1
                event = "POST_CPU_2E_STATE_NOT_PC_PROOF"
        elif prior and prior["d6"] != r["d6"]:
            counts["post_cpu_d6_changes"] += 1
            event = "POST_CPU_D6_CHANGED"
        elif prior and prior["modeA0"] != r["modeA0"]:
            counts["post_cpu_mode_changes"] += 1
            event = "POST_CPU_MODE_CHANGED"
        if event and len(candidate_events) < 256:
            candidate_events.append({
                "line": line_number, "session": session,
                "frame": r["frame"], "movie_frame": r["movie_frame"],
                "fighter": r["fighter"], "side": r["side"], "slot": r["slot"],
                "phase": r["phase"], "synthetic": r["inputSynthetic"],
                "d6": r["d6"], "modeA0": r["modeA0"],
                "action": r["action"], "script": r["script34"],
                "held_c6": r["c6"], "press_ca": r["ca"],
                "event": event,
            })
        prior_cpu[key] = current
    return {
        "schema": "XMVSF_NATIVE_DHC_OBSERVER_87_175_SUMMARY_V1",
        "read_only": True,
        "counts": dict(counts),
        "candidate_events_first_256": candidate_events,
        "limitation": "Post-CPU and post-host snapshots do not prove that a specific "
                      "68000 instruction branch ran. No move is newly mapped or enabled.",
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    if not args.input.is_file():
        ap.error("Observer TSV missing: " + str(args.input))
    result = consume(args.input)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("87.175 observer sessions:", result["counts"].get("sessions", 0))
    print("Actor rows:", result["counts"].get("rows", 0))
    print("After-CPU action 2E transitions:",
          result["counts"].get("post_cpu_2e_action_transitions", 0))
    print("Host changed rows:", result["counts"].get("host_changed_actor_rows", 0))
    print("Summary:", args.out)


if __name__ == "__main__":
    main()
