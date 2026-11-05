#!/usr/bin/env python3
"""Read-only summary of Stage 87.180 CPS2 subframe DHC input-history trace.

Never labels CA as an accepted native super; classification is observational.
Handles multiple append sessions and host frame-counter resets.
"""
import argparse
import csv
import io
import json
from collections import Counter
from pathlib import Path

REQUIRED = {"frame", "stage", "dhc_phase", "action", "modeA0",
            "C0", "C2", "C4", "C6", "C8", "CA", "ctrl256", "ctrl259"}
POINTS = {"BEFORE_CPU", "AFTER_PRE_DISPLAY_CPU", "AFTER_SCAN_0",
          "AFTER_SCAN_1", "AFTER_SCAN_2", "AFTER_VBLANK_CPU",
          "AFTER_DHC_HOST"}

def summarize(path):
    count = Counter()
    events = []
    session = 0
    header = None
    frame_rows = []
    previous_frame = None

    def flush():
        if not frame_rows:
            return
        count["frames_sampled"] += 1
        cpu = [r for r in frame_rows if r["stage"] != "AFTER_DHC_HOST"]
        if not cpu:
            return
        values = []
        for r in cpu:
            try:
                ca = int(r["CA"], 16)
            except ValueError:
                count["bad_CA"] += 1
                return
            values.append(ca & 0x0070)
        first = next((x for x in values if x), None)
        last = values[-1]
        frame = frame_rows[0]["frame"]
        action_changes = [(a["stage"], a["action"]) for a in cpu]
        changed_actions = len({v for _, v in action_changes}) > 1
        if first and not last:
            count["press_edges_lost_within_cpu_frame"] += 1
            kind = "PRESS_EDGE_APPEARED_THEN_CLEARED"
        elif first:
            count["press_edges_still_visible_after_cpu"] += 1
            kind = "PRESS_EDGE_VISIBLE"
        else:
            count["no_press_edge_in_sampled_cpu_slices"] += 1
            kind = None
        if changed_actions:
            count["native_action_changed_within_cpu_frame"] += 1
        if kind or changed_actions:
            if len(events) < 512:
                events.append({
                    "session": session, "frame": frame,
                    "kind": kind or "NATIVE_ACTION_CHANGED",
                    "input_edges_by_cpu_slice": [
                        {"stage": r["stage"], "CA_punch": v}
                        for r, v in zip(cpu, values)
                    ],
                    "native_actions": action_changes,
                    "host_phase": frame_rows[0]["dhc_phase"],
                })
        if any(
            (int(r["ctrl259"], 16) == 8 and
             int(r["ctrl256"], 16) == 0 and
             r["action"] in ("2C02", "2C04", "0002"))
            for r in frame_rows
        ):
            count["sampled_stale_tag_latch_wrong_action_frames"] += 1

    with path.open(encoding="utf-8-sig", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\r\n")
            if line.startswith("# XM87.180 SUBFRAME SESSION"):
                flush()
                frame_rows.clear()
                session += 1
                count["sessions"] += 1
                header = None
                previous_frame = None
                continue
            if line.startswith("frame\tcycles\tstage\t"):
                header = line.split("\t")
                if not REQUIRED.issubset(header):
                    header = None
                    count["invalid_headers"] += 1
                continue
            if not line or line.startswith("#") or header is None:
                continue
            fields = line.split("\t")
            if len(fields) != len(header):
                count["malformed_rows"] += 1
                continue
            r = dict(zip(header, fields))
            if r["stage"] not in POINTS:
                count["unknown_stages"] += 1
                continue
            try:
                frame = int(r["frame"])
            except ValueError:
                count["bad_frames"] += 1
                continue
            count["sample_rows"] += 1
            if previous_frame is not None and frame < previous_frame:
                flush()
                frame_rows.clear()
                count["frame_counter_resets"] += 1
                session += 1
            elif previous_frame is not None and frame != previous_frame:
                flush()
                frame_rows.clear()
            frame_rows.append(r)
            previous_frame = frame
    flush()
    return {
        "schema": "XMVSF_87_180_SUBFRAME_TRACE_SUMMARY_V1",
        "source": str(path),
        "counts": dict(count),
        "examples_first_512": events,
        "limits": "CA is a sampled input edge, not proof that native 68000 "
                  "controller 8 or action 2E was accepted. Output is read-only.",
    }

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    a = ap.parse_args()
    if not a.input.is_file():
        ap.error("Cannot find 87.180 subframe input: " + str(a.input))
    result = summarize(a.input)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print("87.180 CPU subframe slices:", result["counts"].get("sample_rows", 0))
    print("Press edges cleared within CPU frame:",
          result["counts"].get("press_edges_lost_within_cpu_frame", 0))
    print("Wrong-action frames with stale host latch:",
          result["counts"].get("sampled_stale_tag_latch_wrong_action_frames", 0))
    print("Summary:", a.out)

if __name__ == "__main__":
    main()
