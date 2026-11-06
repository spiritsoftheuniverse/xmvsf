#!/usr/bin/env python3
"""Read-only original ROM Shinkuu action lifetime audit from native F10 TSV.

Tracks Ryu (12) action 2E08 startup/attack, action 2E0A recovery,
and original action exit. DHC-interrupted actions are NOT natural lifetimes.
The game engine never depends on durations calculated by this analyzer.
"""
import argparse
import csv
from pathlib import Path

FIELDS = ["start","attack_start","recovery_start","end","startup_frames",
          "attack_frames","recovery_frames","total_frames","end_action","status"]


def analyze(path):
    frames = {}
    with path.open(encoding="utf-8-sig", errors="replace", newline="") as src:
        for r in csv.DictReader(src, delimiter="\t"):
            if r.get("role") != "0" or r.get("fighterId") != "0012":
                continue
            try: frame = int(r["frame"])
            except (TypeError, ValueError): continue
            frames[frame] = r  # last observation of each frame
    records, current = [], None
    for frame, r in sorted(frames.items()):
        action = r["action60"].upper()
        step = r["step08"].upper()
        script = r["script34"].upper()
        mode = r["modeA0"].upper()
        root = action=="2E08" and step=="0002" and \
               script=="000D3070" and mode in ("2A28","2A02")
        startup = action=="2E08" and step=="0002" and \
                  "000D3070"<=script<="000D30A0"
        attack = action=="2E08" and step=="0004" and \
                 "000D30B0"<=script<="000D30E0"
        recovery = action=="2E0A" and step=="0006" and \
                   "000D30F4"<=script<="000D3194"
        if current is None:
            if root:
                current={"start":frame,"attack_start":None,
                         "recovery_start":None,"end":None}
            continue
        if attack and current["attack_start"] is None:
            current["attack_start"] = frame
        if recovery and current["recovery_start"] is None:
            current["recovery_start"] = frame
        if root and frame!=current["start"] and \
           current["attack_start"] is not None:
            current["end"]=frame
            current["end_action"]="NEW_ROOT"
            current["status"]="INTERRUPTED_BY_NEW_ROOT"
        elif not (startup or attack or recovery):
            current["end"]=frame
            current["end_action"]=action
            current["status"] = (
                "NATURAL_EXIT"
                if current["recovery_start"] is not None and action!="3200"
                else "INTERRUPTED_OR_INCOMPLETE")
        else:
            continue
        a=current["attack_start"]; b=current["recovery_start"]
        end=current["end"]
        current["startup_frames"]=a-current["start"] if a is not None else ""
        current["attack_frames"]=b-a if a is not None and b is not None else ""
        current["recovery_frames"]=end-b if b is not None else ""
        current["total_frames"]=end-current["start"]
        records.append(current)
        current=None
        if root:
            current={"start":frame,"attack_start":None,
                     "recovery_start":None,"end":None}
    return records


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--probe",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    a=ap.parse_args()
    records=analyze(a.probe)
    a.out.parent.mkdir(parents=True,exist_ok=True)
    with a.out.open("w",encoding="utf-8",newline="") as dst:
        w=csv.DictWriter(dst,fieldnames=FIELDS)
        w.writeheader();w.writerows(records)
    natural=[r for r in records if r["status"]=="NATURAL_EXIT"]
    print("Complete natural Shinkuu lifetimes:",len(natural))
    print("DHC-interrupted Shinkuu actions:",len(records)-len(natural))
    print("Natural frame spans:",[r["total_frames"] for r in natural])
    print("Output:",a.out)

if __name__=="__main__":
    main()
