#!/usr/bin/env python3
"""ROM-free regression for precise original CPU action-writer PC attribution."""
import sys
from pathlib import Path
from tempfile import TemporaryDirectory
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ANALYZE_NATIVE_PC_ARBITRATION_182 import parse

COLUMNS = (
    "frame phase ordinal region PPC fromPPC PC_next A6 D0 D1 cycles ordinalCPU changeMask "
    "RyuAction RyuA0 RyuD6 controller256 controller259 C0 C2 C6 CA"
).split()

def row(frame, seq, pc, flags, action, mode, d6, c2, ca):
    d = {
        "frame": str(frame), "phase":"2", "ordinal":str(seq),
        "region":"RYU_COMMAND_DISPATCH", "PPC":f"{pc:06X}",
        "fromPPC":f"{0x020906:06X}", "D1":"00000004",
        "PC_next":f"{pc+2:06X}", "A6":"FF4000", "D0":"0000002C",
        "cycles":"12", "ordinalCPU":str(seq), "changeMask":f"{flags:02X}",
        "RyuAction":f"{action:04X}", "RyuA0":f"{mode:04X}",
        "RyuD6":f"{d6:02X}", "controller256":"00", "controller259":"08",
        "C0":"0031", "C2":f"{c2:04X}", "C6":"0032",
        "CA":f"{ca:04X}",
    }
    return "\t".join(d[k] for k in COLUMNS)

def run():
    with TemporaryDirectory() as d:
        path=Path(d)/"pc_trace.tsv"
        path.write_text("\n".join([
            "# XM87.182 NATIVE PC SESSION: first",
            "\t".join(COLUMNS),
            "# DHC_FRAME frame=7016 watched=3 dropped=0 allInstructions=10000",
            row(7016, 1, 0x01042C, 0, 0x2E08, 0x002A, 0x0A, 0x0001, 0),
            row(7016, 2, 0x010448, 64, 0x2E08, 0x002A, 0x0A, 0x0001, 0x0060),
            row(7016, 3, 0x033000, 3, 0x0002, 0x0200, 0x0A, 0x0061, 0),
            "# DHC_FRAME frame=7264 watched=2 dropped=1 allInstructions=9000",
            row(7264, 1, 0x033000, 0, 0x2E08, 0x002A, 0x0A, 0x0041, 0),
            row(7264, 2, 0x123456, 7, 0x2C04, 0x2A00, 0, 0x0041, 0x0030),
            "# XM87.182 NATIVE PC SESSION: second",
            "\t".join(COLUMNS),
            "# DHC_FRAME frame=7580 watched=1 dropped=0 allInstructions=8000",
            row(7580, 1, 0x033D92, 2, 0x2E08, 0x2A00, 0x0A, 0x0001, 0x0030),
        ])+"\n", encoding="utf-8")
        result=parse(path)
        c=result["counts"]
        assert c["sessions"]==2, c
        assert c["traced_frames"]==3, c
        assert c["native_action_after_0002"]==1, c
        assert c["native_action_after_2C04"]==1, c
        assert c["original_wrong_action_writer_pcs_captured"]==2, c
        assert c["dropped_filtered_instruction_rows"]==1, c
        frames=result["frame_summaries_first_1500"]
        assert len(frames)==3, frames
        assert frames[0]["action_changes"][0]["executed_pc"]=="033000"
        assert frames[1]["action_changes"][0]["executed_pc"]=="123456"
        assert frames[1]["action_changes"][0]["previous_executed_pc"]=="020906"
        assert frames[1]["trace_truncated"] is True
        assert frames[2]["action_changes"]==[]
        print("PASS 87.182: exact native wrong-action writer PCs and bounded trace diagnostics")

if __name__=="__main__":
    run()
