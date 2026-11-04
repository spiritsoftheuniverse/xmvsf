#!/usr/bin/env python3
"""Read-only XMVSF F9 v2 ROM/input cross-check.

Run after VERIFY_REPLAY_V2.py (also invoked here). This is an evidence
validator, NOT a claim that action 2E identifies a super command.
Optional --replay-results compares a fresh replay's ROM action trace.
No savestate, ROM, input, or recording file is modified.
"""
import argparse
import csv
import io
import json
from pathlib import Path
import struct
import sys

from VERIFY_REPLAY_V2 import SessionSource, verify, ReplayError


def require(value, why):
    if not value:
        raise ReplayError(why)


def rows(blob):
    return list(csv.DictReader(io.StringIO(blob.decode("utf-8-sig")), delimiter="\t"))


def load_capture(path):
    given = Path(path)
    if given.is_dir() and not (given / "manifest.tsv").is_file():
        possible = sorted(given.glob("replays/session_*/manifest.tsv"))
        require(len(possible) == 1, "Expected exactly one replay session in TEST directory")
        session = possible[0].parent
    else:
        session = given
    source = SessionSource(session)
    try:
        integrity = verify(source)
        manifest = source.read("manifest.tsv").decode("utf-8")
        prefix = source.prefix if source.zip else ""
        if source.zip:
            root_prefix = prefix.rsplit("replays/", 1)[0]
            def evidence(name):
                return source.zip.read(root_prefix + name)
        else:
            root_dir = source.path.parent.parent
            def evidence(name):
                return (root_dir / name).read_bytes()
        observed = rows(evidence("rom_super_observed.tsv"))
        catalog = rows(evidence("rom_super_catalog.tsv"))
        discovery = json.loads(evidence("native_super_reverse_engineering.json"))
        segments, frames = [], []
        for line in manifest.splitlines()[1:]:
            fields = line.split("\t")
            if fields[0] == "SEG":
                segments.append({"session": int(fields[1]), "index": int(fields[2]),
                                 "file": fields[5], "start": len(frames)})
            elif fields[0] == "DONE":
                seg = segments[-1]
                number = int(fields[3])
                raw = source.read(seg["file"])
                count = struct.unpack_from("<I", raw, 8)[0]
                widths = raw[45:44 + count * 2:2]
                require(count == 29 and all(x == 1 for x in widths),
                        "Unexpected native CPS2 input layout")
                start = 44 + count * 2
                stride = 4 + sum(widths)
                require(len(raw) - start == number * stride,
                        "Bad segment payload " + seg["file"])
                for k in range(number):
                    location = start + k * stride
                    frames.append(raw[location + 4:location + stride])
                seg["end"] = len(frames)
        require(len(frames) == integrity["frames"], "Input frame total mismatch")
        return integrity, segments, frames, observed, catalog, discovery
    finally:
        source.close()


FIELDS = ("frame", "movie_input_frame", "side", "fighter_id", "reserve_id",
          "action", "selector", "native_script", "table_script", "animation",
          "rom_match", "last_input_frame", "last_input_mask",
          "last_input_chord", "motion_hint", "input_origin",
          "classification", "motion_path")


def correlate(integrity, segments, frames, observed, catalog, discovery):
    catalog_map = {(r["fighter_id"], r["selector"], r["script"]): r for r in catalog}
    json_map = {(r["fighter_id"], r["selector"], r["script"]): r
                for r in discovery["candidates"]}
    physical = 0
    without_input = 0
    offsets = {}
    catalog_gaps = set()
    json_gaps = set()
    for event in observed:
        frame = int(event["movie_input_frame"])
        require(0 <= frame < len(frames), "ROM event beyond recorded movie")
        segment = next((s for s in segments if s["start"] <= frame < s["end"]), None)
        require(segment is not None, "ROM event outside segments")
        key = (event["fighter_id"], event["selector"], event["native_script"])
        require(event["action"] == "2E" and event["rom_match"] == "1"
                and event["native_script"] == event["table_script"],
                "Unmatched native ROM script at movie frame " + str(frame))
        if key not in catalog_map:
            catalog_gaps.add(key)
        else:
            require(catalog_map[key]["animation"] == event["animation"],
                    "ROM catalog animation disagreement: " + str(key))
        if key not in json_map:
            json_gaps.add(key)
        else:
            require(json_map[key]["animation"] == event["animation"],
                    "Reverse JSON animation disagreement: " + str(key))
        offsets.setdefault(str(segment["session"]), set()).add(
            frame - int(event["frame"]))
        if event["input_origin"] == "PHYSICAL_CANDIDATE":
            physical += 1
            lag = int(event["frame"]) - int(event["last_input_frame"])
            require(0 <= lag <= 64 and frame - lag >= segment["start"],
                    "Input correlation crossed a state boundary: " + str(frame))
            # The order is the native Cps2FightingInputList in d_cps2.cpp:
            # P1 LP/MP/HP/LK/MK/HK at slots 6-11, P2 at 18-23.
            side = int(event["side"])
            require(side in (1, 2), "Unrecognized player side")
            start = 6 if side == 1 else 18
            actual = sum(1 << i for i in range(6)
                         if frames[frame - lag][start + i])
            require(actual == int(event["last_input_mask"], 16),
                    "Physical input does not match native recording at movie "
                    + str(frame - lag))
        elif event["input_origin"] == "NONE":
            without_input += 1
        else:
            require(event["input_origin"] == "DHC_TEST_INJECTED",
                    "Unknown input origin: " + event["input_origin"])
    return {
        "status": "ROM_INPUT_CORRELATION_PASS",
        "frames": integrity["frames"],
        "sessions": integrity["sessions"],
        "segments": integrity["segments"],
        "sealed": integrity["status"] == "SEALED",
        "rom_2E_events": len(observed),
        "physical_button_chords_byte_matched": physical,
        "no_recent_physical_input": without_input,
        "unique_action_scripts": len(set(
            (r["fighter_id"], r["selector"], r["native_script"]) for r in observed)),
        "fighter_ids": sorted(set(r["fighter_id"] for r in observed)),
        "catalog_candidates": len(catalog_map),
        "json_candidates": len(json_map),
        "catalog_only_unverified_candidates": len(set(catalog_map) - set(json_map)),
        "observed_rom_catalog_gaps": [list(k) for k in sorted(catalog_gaps)],
        "observed_reverse_json_gaps": [list(k) for k in sorted(json_gaps)],
        "movie_minus_rom_frame_offsets_by_session":
            {k: sorted(v) for k, v in offsets.items()},
        "runtime_playback": "NOT_VERIFIED",
        "command_recognition": "NOT_VERIFIED",
    }


def compare_trace(reference, directory, expected_frames, expected_segments):
    # The permanent RUN_REPLAY.bat marks each run in a separate status log.
    # Require a complete, NEW replay before comparing event data.
    status_path = Path(directory) / "replay_v2_status.log"
    require(status_path.is_file(), "Missing fresh replay status: " + str(status_path))
    log_lines = status_path.read_text(encoding="utf-8", errors="replace").splitlines()
    starts = [i for i, line in enumerate(log_lines)
              if line.startswith("===== 87.167 HOST REPLAY START ")]
    require(starts, "Missing run marker; do not accept stale replay results")
    current = log_lines[starts[-1] + 1:]
    require(not any("REPLAY_DIVERGED" in line for line in current),
            "Runtime reported replay divergence")
    loaded = sum(line.startswith("CHECKPOINT_LOADED session=") for line in current)
    require(loaded == expected_segments,
            "Checkpoint load count mismatch: %d of %d" % (loaded, expected_segments))
    require(any(line == "PLAY_END complete_frames=" + str(expected_frames)
                for line in current),
            "No PLAY_END for all recorded input frames in the latest run")
    trace = Path(directory) / "rom_super_observed.tsv"
    require(trace.is_file(), "Replay ROM diagnostic missing: " + str(trace))
    replayed = rows(trace.read_bytes())
    require(len(reference) == len(replayed),
            "Replay ROM action count differs: recorded %d, replayed %d" %
            (len(reference), len(replayed)))
    for index, (before, after) in enumerate(zip(reference, replayed), 1):
        for name in FIELDS:
            require(before[name] == after[name],
                    "ROM trace divergence at event %d, %s: %s -> %s" %
                    (index, name, before[name], after[name]))
    return {"result": "ALL_ROM_ACTION_EVENTS_IDENTICAL",
            "matched_events": len(replayed),
            "limitation": "Action trace matching is not a CPU/RAM checksum proof"}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("capture", help="TEST version folder, session folder, or capture ZIP")
    p.add_argument("--replay-results",
                   help="Fresh RUN_REPLAY.bat replay_results folder")
    args = p.parse_args()
    data = load_capture(args.capture)
    report = correlate(*data)
    if args.replay_results:
        report["playback_comparison"] = compare_trace(
            data[3], args.replay_results, data[0]["frames"], data[0]["segments"])
        report["runtime_playback"] = "CHECKPOINT_LOADS_AND_ROM_TRACE_MATCHED; NO_CPU_HASH"
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (ReplayError, ValueError, OSError, KeyError, UnicodeError) as exc:
        print("DHC_ROM_CORRELATION_FAILED:", exc, file=sys.stderr)
        sys.exit(2)
