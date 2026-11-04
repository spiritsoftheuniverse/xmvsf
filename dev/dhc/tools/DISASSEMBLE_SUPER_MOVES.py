#!/usr/bin/env python3
"""Read-only native CPS2 super inventory and optional 68000 disassembly.

This script reads PRIVATE locally captured ROM/data bytes from 87.160.
The decrypted opcode dump is *host-word-swapped* by FBNeo; reverse each
16-bit byte pair before sending it to a big-endian M68K disassembler.

ROM action 2E tables give native action/script addresses, NOT inputs.
Recorded controller correlations are evidence hints; only decoded native
command recognition plus corroborating actual inputs can certify a move.
Do not generate game-control patches from this report automatically.
"""
from __future__ import annotations
import argparse
import csv
import json
from pathlib import Path

KNOWN = {
    (0x12, 0x08, 0x0D3070): ("RYU SHINKUU HADOKEN", "QCF+PP"),
    (0x12, 0x00, 0x0D2CFC): ("RYU SHINKUU TATSUMAKI", "QCB+KK"),
    (0x14, 0x00, 0x0D6722): ("KEN SHORYUREPPA", "QCF+PP"),
    (0x14, 0x02, 0x0D6976): ("KEN SHINRYUKEN", "QCF+KK"),
}
EXCLUSIONS = {(0x14, 0x06, 0x0D6B3E): "KEN VARIABLE COMBINATION"}

def w(blob: bytes, p: int) -> int:
    if p < 0 or p + 2 > len(blob):
        return 0
    return blob[p] | blob[p + 1] << 8

def l(blob: bytes, p: int) -> int:
    return (w(blob, p) << 16) | w(blob, p + 2)

def native_candidates(blob: bytes) -> list[dict]:
    rows = []
    if len(blob) < 0x8AA08:
        return rows
    for fighter in range(0, 0x42, 2):
        table = l(blob, 0x8A946 + fighter * 2)
        if table <= 0 or table + 0x38 >= len(blob):
            continue
        delta = w(blob, table + 0x2E)
        section = table + delta
        if not delta or section >= len(blob):
            continue
        nearby = []
        for a in range(0x30, 0x36, 2):
            d = w(blob, table + a)
            if d and table + d > section and table + d < len(blob):
                nearby.append(table + d)
        bound = (min(nearby) - section) & ~1 if nearby else 0x50
        for selector in range(0, min(bound, 0x50), 2):
            sd = w(blob, section + selector)
            script = section + sd
            if not sd or script <= section or script + 16 > len(blob):
                continue
            duration = w(blob, script + 2)
            if not (0 < duration < 0x8000):
                continue
            key = (fighter, selector, script)
            label, command = KNOWN.get(key, ("UNKNOWN", "UNDECODED"))
            status = ("EXCLUDE_VARIABLE_COMBINATION" if key in EXCLUSIONS
                      else "KNOWN_CAPTURE" if key in KNOWN else
                      "UNVERIFIED_2E_ROM_DATA")
            rows.append(dict(fighter_id=f"{fighter:02X}",
                             action="2E", selector=f"{selector:02X}",
                             script=f"{script:06X}",
                             animation=f"{w(blob, script + 6):04X}",
                             duration=duration, status=status,
                             known_name=EXCLUSIONS.get(key, label),
                             verified_input=command))
    return rows

def byte_unswap(raw: bytes) -> bytes:
    swapped = bytearray(raw)
    for p in range(0, len(swapped) - 1, 2):
        swapped[p], swapped[p + 1] = swapped[p + 1], swapped[p]
    return bytes(swapped)

def disassemble(codepath: Path, addresses: list[int], count: int) -> dict:
    try:
        from capstone import Cs, CS_ARCH_M68K, CS_MODE_M68K_000, CS_MODE_BIG_ENDIAN
    except (ImportError, AttributeError):
        return {
            "status": "Capstone M68K unavailable; install 'capstone' locally",
            "points": {},
        }
    raw = codepath.read_bytes()
    raw = byte_unswap(raw)
    decoder = Cs(CS_ARCH_M68K, CS_MODE_M68K_000 | CS_MODE_BIG_ENDIAN)
    result = {}
    for pc in addresses:
        if pc < 0 or pc >= len(raw):
            result[f"{pc:06X}"] = {"status": "PC_OUTSIDE_DUMP"}
            continue
        # These are already native instruction-boundary PCs established in
        # the project. Do not invent a handler identity from its address.
        insns = []
        for op in decoder.disasm(raw[pc:pc + 240], pc, count=count):
            insns.append(dict(pc=f"{op.address:06X}",
                              bytes=op.bytes.hex(),
                              mnemonic=op.mnemonic, operands=op.op_str))
        result[f"{pc:06X}"] = insns or {"status": "NO_INSTRUCTIONS_DECODED"}
    return {"status": "DECODED_OPCODE_WINDOWS_NOT_YET_INPUT_XREFS",
            "points": result}

def observations(path: Path) -> list[dict]:
    if not path.is_file():
        return []
    with path.open(newline="", encoding="utf-8-sig") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True,
                        help="Private FBNeo host-word-swapped CpsRom data dump")
    parser.add_argument("--opcodes", type=Path,
                        help="Private decrypted host-word-swapped CpsCode")
    parser.add_argument("--observations", type=Path,
                        help="87.160 rom_super_observed.tsv")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--pc", action="append", default=[],
                        help="Native instruction PC (hex), repeatable")
    parser.add_argument("--count", type=int, default=24)
    args = parser.parse_args()
    if not args.rom.is_file():
        parser.error("Private CpsRom file not found; run 87.160 TEST first")
    rom = args.rom.read_bytes()
    candidates = native_candidates(rom)
    observed = observations(args.observations) if args.observations else []
    # Establish these PCs only as ROM dispatch / tag starting landmarks.
    # They are NOT assumed to be all fighters' command recognizers.
    pc_values = [int(p, 16) for p in args.pc] if args.pc else [
        0x01038E, 0x013B8E, 0x00C4E6, 0x011B00
    ]
    decoded = (disassemble(args.opcodes, pc_values, args.count)
               if args.opcodes and args.opcodes.is_file() else
               {"status": "NO_DECRYPTED_OPCODE_DUMP", "points": {}})
    report = {
        "purpose": "Evidence-first, read-only native super discovery",
        "warning": "Animation/script records do NOT prove command inputs",
        "data_bytes": len(rom),
        "candidates": candidates,
        "live_2e_events": observed,
        "m68k_disassembly": decoded,
        "not_enabled_for_gameplay": True,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"ROM 2E candidate scripts: {len(candidates)}")
    print(f"Live native 2E observations: {len(observed)}")
    print(f"M68K opcode decoding: {decoded['status']}")
    print(f"Read-only evidence report: {args.out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
