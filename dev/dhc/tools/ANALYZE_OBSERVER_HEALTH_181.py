#!/usr/bin/env python3
"""87.181 read-only launch/observer triage; never treats exit code 0 as a trace.

The launcher writes RUN_*_START and RUN_*_EXIT. The CPS2 health hook writes
CPS2_FIRST_FRAME, XMVSF_FIRST_FRAME, XMVSF_FIRST_COMBAT and heartbeat rows.
Old 87.175/87.180 TSV files are append-only, so only their LATEST session's
rows are counted. This tool never mutates the input logs.
"""
import argparse
import json
from pathlib import Path


def latest_observer_session(path, prefix):
    if not path.is_file():
        return {"exists": False, "sessions": 0, "latest_rows": 0, "size": 0}
    sessions = 0
    rows = 0
    with path.open("r", encoding="utf-8-sig", errors="replace") as source:
        for line in source:
            if line.startswith(prefix):
                sessions += 1
                rows = 0
            elif sessions > 0 and line and not line.startswith("#") and (
                line[0].isdigit()
            ) and "\t" in line:
                rows += 1
    return {
        "exists": True,
        "sessions": sessions,
        "latest_rows": rows,
        "size": path.stat().st_size,
    }


def read_latest_launch(path):
    if not path.is_file():
        return []
    rows = []
    with path.open("r", encoding="utf-8-sig", errors="replace") as source:
        for raw in source:
            line = raw.strip()
            if line.startswith(("RUN_TEST_START", "RUN_REPLAY_START")):
                rows = [line]
            elif rows:
                rows.append(line)
    return rows


def summarize(health, native, subframe):
    events = read_latest_launch(health)
    markers = {
        "launcher_started": any(s.startswith(("RUN_TEST_START", "RUN_REPLAY_START")) for s in events),
        "engine_hook": any(s.startswith("ENGINE_HEALTH_INITIALIZED\t") for s in events),
        "cps2_frame": any(s.startswith("CPS2_FIRST_FRAME\t") for s in events),
        "xmvsf_frame": any(s.startswith("XMVSF_FIRST_FRAME\t") for s in events),
        "xmvsf_combat": any(s.startswith("XMVSF_FIRST_COMBAT\t") for s in events),
        "launcher_exited": any(s.startswith(("RUN_TEST_EXIT", "RUN_REPLAY_EXIT")) for s in events),
    }
    old = latest_observer_session(native, "# XM87.175 OBSERVER SESSION")
    new = latest_observer_session(subframe, "# XM87.180 SUBFRAME SESSION")
    # Engine-start events include key/value readiness fields.
    ready = []
    for line in events:
        if line.startswith(("XMVSF_FIRST_FRAME\t", "XMVSF_FIRST_COMBAT\t")):
            fields = {}
            for entry in line.split("\t")[1:]:
                if "=" in entry:
                    k, v = entry.split("=", 1)
                    fields[k] = v
            ready.append(fields)
    if not markers["launcher_started"]:
        status = "NO_LAUNCH_MARKER"
        message = "Launcher health preflight was not written."
    elif not markers["cps2_frame"]:
        status = "NO_CPS2_FRAME"
        message = "Executable exited without entering the CPS2 frame hook (app menu/boot/driver not reached)."
    elif not markers["xmvsf_frame"]:
        status = "NO_XMVSF_DRIVER"
        message = "A CPS2 frame ran, but no XMVSF CPS2 driver frame was observed."
    elif not old["exists"] or not new["exists"]:
        status = "OBSERVER_FILE_NOT_CREATED"
        message = "XMVSF frame ran but at least one observer TSV did not open; check observerReady and file-open stderr."
    elif any(
        fields.get("oldObserverReady") == "0" or fields.get("subframeObserverReady") == "0"
        for fields in ready
    ):
        status = "OBSERVER_NOT_READY"
        message = "The engine reached XMVSF but the runtime observer was not enabled/open."
    elif not markers["xmvsf_combat"]:
        status = "NO_COMBAT_FRAME"
        message = "XMVSF started and observer headers were saved, but native combat mode 8 was never reached."
    elif old["latest_rows"] == 0:
        status = "NO_NATIVE_OBSERVER_ROWS"
        message = "Combat was entered but the native post-CPU observer generated no actor rows."
    else:
        status = "NATIVE_OBSERVER_CAPTURED"
        message = "The latest native observer session contains actor rows; inspect the summary for DHC events."
    return {
        "schema": "XMVSF_OBSERVER_HEALTH_87_181_V1",
        "status": status, "explanation": message,
        "engine_markers": markers,
        "readiness_events": ready[-3:],
        "native_87_175": old, "subframe_87_180": new,
        "last_health_events": events[-12:],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--health", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--subframe", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    a = parser.parse_args()
    report = summarize(a.health, a.native, a.subframe)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("XMVSF OBSERVER HEALTH: " + report["status"])
    print(report["explanation"])
    print("Last native actor rows:", report["native_87_175"]["latest_rows"])
    print("Last subframe rows:", report["subframe_87_180"]["latest_rows"])
    print("Health summary:", a.out)
    # Report an incomplete capture in output, but don't replace game return code.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
