#!/usr/bin/env python3
"""Read-only 87.186 native beginning-gate trace summary.

Separates actual Ryu Shinkuu action phases, recorded press/command
transitions, and *observed* controller-8 arming. No inference that
missing startup attempts were ROM-rejected. Append sessions isolated.
"""
import argparse
import csv
import json
from pathlib import Path


def analyze(path):
    sessions=0
    episodes={}
    frames=0
    changes=0
    for raw in path.read_text(encoding="utf-8-sig",
                              errors="replace").splitlines():
        if raw.startswith("# XM87.186 OPENING SESSION"):
            sessions+=1
            continue
        if not raw or raw.startswith("#") or raw.startswith("frame\t"):
            if raw.startswith("frame\t"):
                header=raw.split("\t")
            continue
        parts=raw.split("\t")
        if "header" not in locals() or len(parts)!=len(header):
            continue
        r=dict(zip(header,parts))
        try:
            frame=int(r["frame"])
            epoch=int(r["epoch"])
            age=int(r["age_from_root"])
            attack_age=int(r["age_from_attack"])
            observed=int(r["PPC_requested"])
            punch=int(r["C0"],16)&0x0070
            pressed=int(r["CA"],16)&0x0070
            controller=int(r["ctrl256"],16)
        except (ValueError,KeyError):
            continue
        if epoch==0: continue
        key=(sessions,epoch)
        episode=episodes.setdefault(key, {
            "session":sessions,"epoch":epoch,"root_frame":None,
            "first_attack_frame":None,"first_recovery_frame":None,
            "exit_frame":None,"first_observed_punch_age":None,
            "first_observed_controller8_age":None,
            "opening_frames_requested_pc":[],"host_tag_interrupt":False})
        if r["event"]=="ACTUAL_NATIVE_ROOT" or (
            episode["root_frame"] is None and age==0):
            episode["root_frame"]=frame
        if r["event"]=="ACTUAL_ATTACK_START":
            episode["first_attack_frame"]=frame
        if r["event"]=="ACTUAL_RECOVERY_START":
            episode["first_recovery_frame"]=frame
        if r["event"]=="ORIGINAL_ACTION_EXIT_OR_HOST_INTERRUPT":
            episode["exit_frame"]=frame
            episode["host_tag_interrupt"]=(r["actor_action"]=="3200")
        if (punch or pressed) and episode["first_observed_punch_age"] is None:
            episode["first_observed_punch_age"]=age
        if controller==8 and episode["first_observed_controller8_age"] is None:
            episode["first_observed_controller8_age"]=age
        if r["boundary"]=="PRE_CPU" and observed:
            episode["opening_frames_requested_pc"].append(
                {"frame":frame,"super_age":age,"attack_age":attack_age,
                 "native_phase":r["phase"]})
            changes+=1
        frames+=1
    cases=list(episodes.values())
    return {
        "schema":"XMVSF_87_186_OPENING_LIFECYCLE_V1",
        "sessions":sessions,"actor_rows":frames,
        "original_supers":len(cases),
        "opening_pc_frames_requested":changes,
        "episodes":cases,
        "important_limit":
          "Controller-8 is the host bridge, not original ROM tag success. "
          "An unattempted startup offset remains UNTESTED. A real Ken "
          "2E00 is required to prove a completed DHC. The uploaded "
          "87.185 recording had 14/14 armed Ken successes.",
    }


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--input",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    a=ap.parse_args()
    if not a.input.is_file():
        ap.error("Missing native 87.186 opening trace: "+str(a.input))
    result=analyze(a.input)
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("87.186 original super episodes:",result["original_supers"])
    print("Opening PC probe frames:",result["opening_pc_frames_requested"])
    print("Lifecycle summary:",a.out)


if __name__=="__main__":
    main()
