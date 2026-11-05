#!/usr/bin/env python3
"""Synthetic, ROM-free DHC evidence regression checks. Run with Python 3."""
import json
import struct
from pathlib import Path
import sys
from tempfile import TemporaryDirectory

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from VERIFY_DHC_ROM_ACTIONS import export_command_evidence
from DISASSEMBLE_SUPER_MOVES import audit_live_script_roots

def main():
    frames = [bytearray(29) for _ in range(65)]
    # Native CPS2 P1 slot 6 LP, 9 LK, 8 HP, slot 5 physical right.
    frames[20][6] = 1
    frames[30][6] = 1
    frames[40][5] = 1
    frames[45][9] = 1
    frames[60][8] = 1
    observed = [{
        "fighter_id": "1E", "selector": "0C", "action": "2E",
        "native_script": "0EB986", "rom_match": "1",
        "last_input_frame": "60", "last_input_mask": "04",
        "input_origin": "PHYSICAL_CANDIDATE",
        "movie_input_frame": "60", "frame": "60",
        "side": "1", "motion_hint": "UNKNOWN", "motion_path": "5>6>5"
    }]
    data = ({"frames":65}, [{"session":1, "index":0, "start":0, "end":65}],
            frames, observed, [], {"candidates":[]})
    with TemporaryDirectory(prefix="dhc_evidence_") as temp:
        info = export_command_evidence(data, Path(temp))
        result = json.loads((Path(temp) / "dhc_command_evidence.json").read_text())
        assert info["physical_events"] == 1
        assert info["roots"] == 1
        assert info["akuma_sequence_matches"] == 1
        assert result["events"][0]["lag_frames"] == 0
        assert not any(e.get("checkpoint_baseline") for e in
                       result["events"][0]["recent_edges"])
        # Verify the actual recorded five-input order; unlike a segment-start
        # snapshot, this window begins with a known prior frame.
        match = result["akuma_shun_goku_satsu_sequences"][0]["sequence_matches"][0]
        assert [button for frame, button in match] == [
            "LP", "LP", "R", "LK", "HP"]
        assert [frame for frame, button in match] == [20,30,40,45,60]

    # Synthetic private data ROM validates the exact original table arithmetic.
    rom = bytearray(0xF0000)
    table, section, script = 0xE8000, 0xE9000, 0xEB986
    def word(at, value):
        rom[at:at+2] = struct.pack("<H", value)
    word(0x8A946 + 0x1E*2, table >> 16)
    word(0x8A946 + 0x1E*2 + 2, table & 0xFFFF)
    word(table + 0x2E, section - table)
    word(section + 0x0C, script - section)
    word(script + 2, 3)
    word(script + 6, 0x1234)
    observed[0]["animation"] = "1234"
    audited = audit_live_script_roots(rom, [], observed)
    assert audited["matched_by_original_rom_table"] == 1
    assert len(audited["unindexed_live_roots"]) == 1
    assert not audited["rom_resolution_disagreements"]
    print("DHC evidence TEST PASS: same-frame, 5-step command, checkpoint, ROM pointer")

if __name__ == "__main__":
    main()
