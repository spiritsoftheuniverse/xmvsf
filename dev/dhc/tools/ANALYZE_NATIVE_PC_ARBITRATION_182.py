#!/usr/bin/env python3
"""Read-only original 68000 executed-PC audit for Stage 87.182.

The source TSV logs a PPC immediately AFTER the corresponding instruction
executes. A new original-ROM action observed at changeMask bit 0 is a
precise candidate for the native writer. The host-installed 3200 bridge
does not execute in Musashi and must not be misidentified as ROM-written.
No private original ROM/opcode bytes are copied to the JSON result.
"""
import argparse
import csv
import json
from collections import Counter
from pathlib import Path

HEADER = {"frame", "PPC", "fromPPC", "PC_next", "A6", "D0", "D1", "ordinalCPU",
          "changeMask", "RyuAction", "RyuA0", "RyuD6",
          "controller256", "controller259", "C0", "C2", "C6", "CA"}
FLAG_NAMES = {
    1: "RYU_ACTION_CHANGED",
    2: "RYU_MODE_CHANGED",
    4: "RYU_D6_CHANGED",
    8: "CONTROLLER_256_CHANGED",
    16: "CONTROLLER_259_CHANGED",
    32: "INPUT_C2_CHANGED",
    64: "PUNCH_EDGE_CA_CHANGED",
}


def parse(path):
    sessions = 0
    current_rows = []
    session_rows = []
    summaries = []
    errors = Counter()
    header = None
    frames = {}

    def flush_frame():
        nonlocal current_rows
        if not current_rows:
            return
        rows = sorted(current_rows, key=lambda r: int(r["ordinalCPU"]))
        first = rows[0]
        action_changes = []
        input_changes = []
        dispatcher_calls = []
        for r in rows:
            if r["PPC"] == "01038E":
                dispatcher_calls.append({
                    "ordinal": int(r["ordinalCPU"]),
                    "caller_last_executed_pc": r["fromPPC"],
                    "d0_action_family": r["D0"],
                    "d1_selector": r["D1"],
                    "meaning": "ORIGINAL_ROM_ENTERED_SHARED_ACTION_DISPATCHER",
                })
        for r in rows:
            mask = int(r["changeMask"], 16)
            if mask & 1:
                # The shared dispatcher at 01038E writes action from
                # D0/D1; the immediately preceding executed PPC is often
                # the original JMP/JSR that requested the move.
                nearest = next((d for d in reversed(dispatcher_calls)
                          if d["ordinal"] <= int(r["ordinalCPU"])
                          and int(r["ordinalCPU"]) - d["ordinal"] <= 18), None)
                action_changes.append({
                    "executed_pc": r["PPC"],
                    "previous_executed_pc": r["fromPPC"],
                    "next_pc": r["PC_next"],
                    "D0_action_family": r["D0"],
                    "D1_selector": r["D1"],
                    "nearest_dispatch_request": nearest,
                    "actor_a6": r["A6"], "native_action_after": r["RyuAction"],
                    "native_mode_after": r["RyuA0"],
                    "native_command_d6_after": r["RyuD6"],
                    "input_c2": r["C2"], "input_ca": r["CA"],
                    "ordinal": int(r["ordinalCPU"]),
                    "source": "EXECUTED_INSTRUCTION_STATE_DELTA",
                })
            if mask & (32|64):
                input_changes.append({
                    "executed_pc": r["PPC"], "native_c2": r["C2"],
                    "native_ca": r["CA"], "ordinal": int(r["ordinalCPU"]),
                })
        if len(summaries) < 1500:
            summaries.append({
                "session": sessions,
                "frame": first["frame"],
                "tracked_68000_instructions": len(rows),
                "action_changes": action_changes[:36],
                "dispatcher_entries_first_36": dispatcher_calls[:36],
                "input_history_changes": input_changes[:36],
                "trace_truncated": frames.get((sessions, first["frame"]), {}).get("dropped", 0) > 0,
                "last_rom_action_sample": rows[-1]["RyuAction"],
            })
        current_rows = []

    with path.open(encoding="utf-8-sig", errors="replace") as handle:
        for raw in handle:
            line = raw.rstrip("\r\n")
            if line.startswith("# XM87.182 NATIVE PC SESSION"):
                flush_frame()
                sessions += 1
                errors["sessions"] += 1
                header = None
                continue
            if line.startswith("# DHC_FRAME "):
                fields = {}
                for item in line[2:].split():
                    if "=" in item:
                        k, v = item.split("=", 1)
                        fields[k] = v
                if "frame" in fields:
                    frames[(sessions, fields["frame"])] = {
                        "watched": int(fields.get("watched", "0")),
                        "dropped": int(fields.get("dropped", "0")),
                        "all_instructions": int(fields.get("allInstructions", "0")),
                    }
                errors["traced_frames"] += 1
                continue
            if line.startswith("frame\tphase\tordinal\tregion\t"):
                header = line.split("\t")
                if not HEADER.issubset(header):
                    header = None
                    errors["invalid_headers"] += 1
                continue
            if not header or not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) != len(header):
                errors["malformed_rows"] += 1
                continue
            row = dict(zip(header, parts))
            try:
                _ = int(row["frame"])
                _ = int(row["ordinalCPU"])
                _ = int(row["changeMask"], 16)
            except ValueError:
                errors["malformed_numeric_rows"] += 1
                continue
            if current_rows and current_rows[0]["frame"] != row["frame"]:
                flush_frame()
            current_rows.append(row)
            errors["watched_rows"] += 1
    flush_frame()

    verdict = Counter()
    for frame in summaries:
        for transition in frame["action_changes"]:
            after = transition["native_action_after"].upper().zfill(4)
            verdict["native_action_after_" + after] += 1
            if after in {"0002", "2C02", "2C04"}:
                verdict["original_wrong_action_writer_pcs_captured"] += 1
    truncated = sum(v["dropped"] for v in frames.values())
    return {
        "schema": "XMVSF_NATIVE_PC_ARBITRATION_87_182_V1",
        "status": "READ_ONLY_ORIGINAL_ROM_EXECUTED_PC_EVIDENCE",
        "counts": {**dict(errors), **dict(verdict),
                   "dropped_filtered_instruction_rows": truncated,
                   "frames_with_rows": len(summaries)},
        "frame_summaries_first_1500": summaries,
        "limits": "Executed PPC at each post-instruction native RAM delta "
                  "pinpoints an instruction, but this is not a proof of "
                  "why a preceding branch was taken. It does not include "
                  "host-only changes made after the 68000 CPU returns. "
                  "No super registry or native actor state is modified.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    if not args.input.is_file():
        parser.error("Native PC trace missing: " + str(args.input))
    report = parse(args.input)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    counts = report["counts"]
    print("87.182 traced controller-8 CPU frames:",counts.get("traced_frames",0))
    print("Native action writer PCs:",sum(v for k,v in counts.items()
          if k.startswith("native_action_after_")))
    print("Wrong-action writer PCs:",counts.get("original_wrong_action_writer_pcs_captured",0))
    print("Dropped watched PC rows:",counts.get("dropped_filtered_instruction_rows",0))
    print("PC evidence:",args.out)


if __name__ == "__main__":
    main()
