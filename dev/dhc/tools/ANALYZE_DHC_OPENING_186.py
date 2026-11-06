#!/usr/bin/env python3
"""87.186 read-only original F10/DHC evidence to beginning-gate matrix.

No attempted input before age 91 in the uploaded 87.185 capture:
UNTESTED does not mean the ROM rejected a controller. No F9 edits.
"""
import argparse
import bisect
import csv
from pathlib import Path


def rows(path):
    with path.open(encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def analyze(probe, chat, handoff):
    frames = {int(r["frame"]): r for r in rows(probe)
              if r.get("frame", "").isdigit() and r.get("role") == "0"
              and r.get("fighterId") == "0012"}
    episodes = []
    for frame, r in sorted(frames.items()):
        root = (r["action60"] == "2E08" and r["step08"] == "0002"
                and r["script34"] == "000D3070")
        prev = frames.get(frame-1)
        previous_root = bool(prev and prev["action60"] == "2E08"
                             and prev["step08"] == "0002"
                             and prev["script34"] == "000D3070")
        if root and not previous_root:
            episodes.append({"root": frame, "attack": None,
                             "recovery": None, "exit": None})
    starts = [ep["root"] for ep in episodes]
    for i, ep in enumerate(episodes):
        stop = episodes[i+1]["root"] if i+1 < len(episodes) else max(frames)+1
        for frame in range(ep["root"], stop):
            r = frames.get(frame)
            if not r:
                continue
            action, step = r["action60"], r["step08"]
            if ep["attack"] is None and action == "2E08" and step == "0004":
                ep["attack"] = frame
            if ep["recovery"] is None and action == "2E0A" and step == "0006":
                ep["recovery"] = frame
            if ep["exit"] is None and action not in ("2E08", "2E0A"):
                ep["exit"] = frame
                ep["exit_kind"] = ("DHC_INTERRUPT" if action == "3200"
                                   else "NATURAL_OR_OTHER")
                break
    native = {}
    for r in rows(handoff):
        if r.get("frame", "").isdigit():
            native.setdefault(int(r["frame"]), []).append(r["decision"])
    attempts = []
    for r in rows(chat):
        if not r.get("frame", "").isdigit() or "P1 DHC QUEUED" not in r.get("message",""):
            continue
        frame = int(r["frame"])
        i = bisect.bisect_right(starts, frame)-1
        if i < 0:
            continue
        ep = episodes[i]
        events = [decision for f in range(frame,frame+45)
                  for decision in native.get(f, [])]
        if "DHC_KEN_REPPA_2E00_ROM_SEEN" in events:
            outcome = "CONFIRMED_NATIVE_KEN_2E00"
        elif "DHC_183_NATIVE_PUNCH_ALREADY_CONSUMED_SKIP_TAG" in events:
            outcome = "DECLINED_CONSUMED_PUNCH_HISTORY"
        else:
            outcome = "NO_CONFIRMED_NATIVE_KEN_2E00"
        phase = ("STARTUP" if ep["attack"] is None or frame<ep["attack"]
                 else "ATTACK" if ep["recovery"] is None or frame<ep["recovery"]
                 else "RECOVERY")
        attempts.append({"frame":frame,"root":ep["root"],
                         "offset":frame-ep["root"],"phase":phase,
                         "outcome":outcome})
    tested = {}
    for attempt in attempts:
        tested.setdefault(attempt["offset"],[]).append(attempt)
    matrix = []
    for age in range(165):
        stages = set()
        for ep in episodes:
            if ep["attack"] is None or age<ep["attack"]-ep["root"]:
                stages.add("STARTUP")
            elif ep["recovery"] is None or age<ep["recovery"]-ep["root"]:
                stages.add("ATTACK")
            else:
                stages.add("RECOVERY")
        actual = tested.get(age,[])
        matrix.append({
            "offset":age,"native_phase_observed":"/".join(sorted(stages)),
            "observed_attempts":len(actual),
            "native_ken_successes":sum(x["outcome"]=="CONFIRMED_NATIVE_KEN_2E00"
                                      for x in actual),
            "evidence":"TESTED" if actual else "UNTESTED",
            "required_next_probe":"NONE" if actual else "REPLAY_OR_NEW_INPUT_OFFSET",
        })
    return episodes,attempts,matrix


def write(path, data, fields):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w",encoding="utf-8",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=fields)
        writer.writeheader()
        writer.writerows(data)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--probe",type=Path,required=True)
    ap.add_argument("--chat",type=Path,required=True)
    ap.add_argument("--handoff",type=Path,required=True)
    ap.add_argument("--outdir",type=Path,required=True)
    a=ap.parse_args()
    eps,attempts,matrix=analyze(a.probe,a.chat,a.handoff)
    write(a.outdir/"native_lifetimes.csv",eps,
          ["root","attack","recovery","exit","exit_kind"])
    write(a.outdir/"observed_dhc_attempts.csv",attempts,
          ["frame","root","offset","phase","outcome"])
    write(a.outdir/"opening_offset_matrix.csv",matrix,
          ["offset","native_phase_observed","observed_attempts",
           "native_ken_successes","evidence","required_next_probe"])
    good=[a["offset"] for a in attempts
          if a["outcome"]=="CONFIRMED_NATIVE_KEN_2E00"]
    print("Original super roots:",len(eps))
    print("Queued DHC requests:",len(attempts))
    print("Confirmed native Ken supers:",len(good))
    print("Attempts before age 91:",sum(a["offset"]<91 for a in attempts))
    print("Earliest confirmed age:",min(good) if good else "NONE")
    print("Matrix:",a.outdir/"opening_offset_matrix.csv")


if __name__=="__main__":
    main()
