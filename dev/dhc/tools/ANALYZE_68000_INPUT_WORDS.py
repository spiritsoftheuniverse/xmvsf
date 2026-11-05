#!/usr/bin/env python3
"""Stage 87.172: read-only proof of original XMVSF held/edge input semantics.

Inputs are the user's PRIVATE FBNeo word-swapped decrypted opcode dump and
host-swapped program-data dump. No ROM bytes are exported or committed.
This proves selected exact 68000 instruction sequences, not a complete
execution trace, nor a particular super's command acceptance.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

SITES = {
    "00C31A": ("4eba40e6", "BSR.W native input updater 010402"),
    "00C342": ("70003d4000c03d4000c43d4000c23d4000c63d4000ca3d4000c8", "reset all six input tracking words"),
    "01042C": ("3010322e00c03d4100c23d4000c03d4100c83d4000c64641c0413d4000c43d4000ca", "read current port, preserve prior word, compute held and press edges"),
    "01044E": ("4a2e00a36724", "test A3 facing orientation; skip conversion if zero"),
    "010454": ("302e00c4610000683d4000ca302e00c06100005c3d4000c6302e00c2610000503d4000c84e75", "normalize C4/C0/C2 using shared conversion into CA/C6/C8"),
    "010482": ("4a2e00a36726302e00c4610000343d4000ca302e00c0610000283d4000c6302e00c26100001c3d4000c84e75", "second native input-update path calls same facing converter"),
    "0104AE": ("3d6e00c400ca3d6e00c000c63d6e00c200c84e75", "direct, unnormalized aliases when no facing conversion"),
    "0104C2": ("32000240077c08010000670400400002080100016704004000014e75", "preserve bits 2..10; swap bit 0/bit 1 for horizontal facing"),
    "0109F0": ("70003d4000c03d4000c43d4000c23d4000c63d4000ca3d4000c8", "secondary input-word reset path"),
    "013CE0": ("322e00c60241000f", "canonical 013C30 routine reads held C6 and masks directional nibble"),
    "013FF2": ("322e00cac252", "alternate 013F76 reads new-press CA with descriptor mask"),
    "0140D2": ("322e00c6c252", "alternate 014096 reads held C6 with descriptor mask"),
    "014426": ("322e00ca0241077fb252", "alternate 014392 reads new-press CA and applies 077F"),
    "0145C8": ("302e00c6", "alternate 01458E classifies held C6"),
    "03298A": ("4eb900013c30674a", "Ryu matcher return must be nonzero to pass this actor branch"),
    "0412CA": ("4eb900013c3067000156", "Akuma matcher return must be nonzero to pass this actor branch"),
}


def unswap(raw: bytes) -> bytes:
    if len(raw) & 1:
        raise ValueError("Original 68000 opcode file must be even length")
    out = bytearray(raw)
    for i in range(0, len(out), 2):
        out[i], out[i + 1] = out[i + 1], out[i]
    return bytes(out)


def mirror_low_directions(mask: int) -> int:
    """Literal 0104C2 68000 routine, NOT arbitrary 16-bit bit-swap."""
    return ((mask & 0x077C)
            | (0x0002 if mask & 0x0001 else 0)
            | (0x0001 if mask & 0x0002 else 0))


def input_tick(now: int, previous: int, facing_swap: bool) -> dict:
    """Byte-derived non-destructive model of 01042C..01047A."""
    current_raw = now & 0xFFFF
    prev_raw = previous & 0xFFFF
    edge_raw = current_raw & (~prev_raw & 0xFFFF)
    convert = mirror_low_directions if facing_swap else lambda value: value
    return {
        "C0_raw_held": current_raw,
        "C2_raw_previous": prev_raw,
        "C4_raw_new_press": edge_raw,
        "C6_facing_relative_held": convert(current_raw),
        "C8_facing_relative_previous": convert(prev_raw),
        "CA_facing_relative_new_press": convert(edge_raw),
    }


def verify_opcode_sites(code: bytes) -> list[dict]:
    evidence = []
    for pc_hex, (opcode_hex, interpretation) in SITES.items():
        pc = int(pc_hex, 16)
        expected = bytes.fromhex(opcode_hex)
        if code[pc:pc + len(expected)] != expected:
            raise ValueError(f"68000 opcode signature changed at {pc_hex}, refuse interpretation")
        evidence.append({"pc": pc_hex, "length_bytes": len(expected),
                         "meaning": interpretation,
                         "verification": "EXACT_ORIGINAL_OPCODE_MATCH"})
    # Branch displacement semantics in the original native code, checked
    # independently of a disassembly library.
    raw = int.from_bytes(code[0xC31C:0xC31E], "big", signed=True)
    if 0xC31C + raw != 0x10402:
        raise ValueError("Input updater BSR target no longer matches 010402")
    for call,expected in ((0x10458,0x104C2),(0x10464,0x104C2),(0x10470,0x104C2),
                          (0x1048C,0x104C2),(0x10498,0x104C2),(0x104A4,0x104C2)):
        if code[call:call+2] != b"\x61\x00":
            raise ValueError(f"Missing BSR.W at {call:06X}")
        delta=int.from_bytes(code[call+2:call+4],"big",signed=True)
        if call + 2 + delta != expected:
            raise ValueError(f"Input mirror target mismatch at {call:06X}")
    return evidence


def direction_evidence(canonical_records: list[dict]) -> dict:
    """Independent QCF/QCB concordance, not an automated super mapping."""
    ryu = [e for e in canonical_records if e.get("fighter_id") == "12"]
    fwd = [e for e in ryu if e["step_tokens"] == ["0004", "0006", "0002"]]
    back = [e for e in ryu if e["step_tokens"] == ["0004", "0005", "0001"]]
    if not fwd or not back:
        raise ValueError("Recorded Ryu QCF/QCB descriptor shapes absent")
    return {
        "observed_ryu_forward_motion_tokens": ["0004","0006","0002"],
        "observed_ryu_backward_motion_tokens": ["0004","0005","0001"],
        "fwd_record_count": len(fwd), "back_record_count": len(back),
        "native_held_nibble": "000F",
        "bit_0004": "DOWN_CORROBORATED_BY_QCF_QCB_RECORDS",
        "bit_0002": "FORWARD_IN_FACING_RELATIVE_INPUT",
        "bit_0001": "BACK_IN_FACING_RELATIVE_INPUT",
        "bit_0008": "REMAINING_DIRECTION_BIT_PROBABLY_UP_NOT_PROVEN_BY_QCF_QCB_ALONE",
        "button_word_0070": "PUNCH_GROUP_CONCORDANT_WITH_PRIOR_RYU_QCF_PP",
        "button_word_0700": "KICK_GROUP_CONCORDANT_WITH_PRIOR_RYU_QCB_KK",
        "caution": "Bit meanings cross-supported by recorded command motions but not direct per-frame native RAM C6/CA traces. Do not assign individual punch/kick bit IDs without more evidence."
    }


def analyze(opcodes:bytes, rom:bytes, canonical_records:list[dict]) -> dict:
    code=unswap(opcodes)
    verified=verify_opcode_sites(code)
    # Nontrivial examples exercising both words and the exact 077C bit gate.
    checks = {
        "press_forward": input_tick(0x0002, 0, False),
        "hold_forward": input_tick(0x0002, 0x0002, False),
        "press_forward_facing_flipped": input_tick(0x0002, 0, True),
        "release_forward_facing_flipped": input_tick(0, 0x0002, True),
        "press_two_buttons": input_tick(0x0030, 0, False),
        "press_combo_facing_flipped": input_tick(0x0034, 0, True),
    }
    assert checks["hold_forward"]["CA_facing_relative_new_press"] == 0
    assert checks["press_forward_facing_flipped"]["CA_facing_relative_new_press"] == 1
    assert checks["release_forward_facing_flipped"]["CA_facing_relative_new_press"] == 0
    return {
        "schema": "xmvsf-68000-held-vs-press-input-words-v1",
        "opcodes_sha256": hashlib.sha256(opcodes).hexdigest(),
        "rom_data_sha256": hashlib.sha256(rom).hexdigest(),
        "opcode_signature_checks":len(verified),
        "opcode_evidence":verified,
        "producer_entry": "010402",
        "raw_current_word":"C0", "raw_previous_word":"C2",
        "raw_press_edge_word":"C4",
        "facing_relative_current_word":"C6",
        "facing_relative_previous_word":"C8",
        "facing_relative_press_edge_word":"CA",
        "edge_formula":"C4 = C0 & (~C2 & 0xFFFF)",
        "flip_condition":"TST.B 00A3(A6); if nonzero normalize C4,C0,C2 to CA,C6,C8",
        "orientation_transform":"bits_0_and_1_swap; output = (input & 077C) | ((input & 1)<<1) | ((input & 2)>>1)",
        "high_bits":"DROPPED_BY_FACING_CONVERSION_MASK_077C_WHEN_ENABLED",
        "consumers":{
            "013C30":"C6 held input",
            "013F76":"CA new-press edge",
            "014096":"C6 held input",
            "014392":"CA new-press edge",
            "01458E":"C6 held input"},
        "direction_bit_evidence":direction_evidence(canonical_records),
        "synthetic_tick_checks":checks,
        "limitations":[
            "The model is derived from exact opcode sequences; no runtime CPU instruction log was supplied.",
            "The record flags and exact named-super input acceptance are not established.",
            "Command descriptors may represent special moves, supers, or related actions; do not auto-enable DHC.",
            "No assumptions about encrypted data-bus indirect table reads are made.",
        ],
        "raw_rom_exported":False,
        "gameplay_changed":False,
    }


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--opcodes", type=Path, required=True)
    p.add_argument("--rom",type=Path,required=True)
    p.add_argument("--canonical-json",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    args=p.parse_args()
    can=json.loads(args.canonical_json.read_text("utf-8"))
    result=analyze(args.opcodes.read_bytes(),args.rom.read_bytes(),can["entries"])
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("PASS:",result["opcode_signature_checks"],"original opcode-site checks")
    print("C6 = facing-held; CA = facing-new-press")
    print("Derived report:",args.out)


if __name__ == "__main__":main()
