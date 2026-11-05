#!/usr/bin/env python3
"""ROM-only audit: original 68000 literal action requests targeting 01038E.

This produces address CANDIDATES; only 87.182's executed-PC trace can
establish which candidate is actually reached during a failed DHC.
Reads private decrypted host-word-swapped opcodes; never writes/ships ROM.
"""
import argparse
import csv
import hashlib
from pathlib import Path

CANDIDATES = [
    (0x2C, 0x02), (0x2C, 0x04), (0x00, 0x02)
]
TARGET = 0x01038E


def scan(opcodes):
    expected = [0x1D40, 0x0060, 0x1D41, 0x0061]
    target_bytes = b"".join(x.to_bytes(2, "little") for x in expected)
    if opcodes[TARGET:TARGET + len(target_bytes)] != target_bytes:
        raise ValueError("Original 01038E actor-action dispatcher opcode mismatch")
    results = []
    for d0, d1 in CANDIDATES:
        for kind, instruction in (("JMP", 0x4EF9), ("JSR", 0x4EB9)):
            sequence = [0x7000 | d0, 0x7200 | d1,
                        instruction, TARGET >> 16, TARGET & 0xFFFF]
            signature = b"".join(x.to_bytes(2, "little") for x in sequence)
            at = 0
            while True:
                at = opcodes.find(signature, at)
                if at < 0:
                    break
                results.append({
                    "action": f"{d0:02X}{d1:02X}",
                    "candidate_pc": f"{at:06X}",
                    "jump_instruction_pc": f"{at + 4:06X}",
                    "dispatcher": f"{TARGET:06X}",
                    "kind": kind,
                    "words": " ".join(f"{w:04X}" for w in sequence),
                    "proof_level": "STATIC_LITERAL_SIGNATURE_ONLY",
                })
                at += 2
    return sorted(results, key=lambda r: (r["action"], r["candidate_pc"]))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--opcodes", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args()
    b = a.opcodes.read_bytes()
    if len(b) < 0x100000:
        ap.error("Expected complete original decrypted opcode image")
    rows = scan(b)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with a.out.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "action", "candidate_pc", "jump_instruction_pc",
            "dispatcher", "kind", "words", "proof_level"])
        writer.writeheader()
        writer.writerows(rows)
    print("Opcode SHA256:", hashlib.sha256(b).hexdigest())
    print("Static candidate action requests:", len(rows))
    for action in ("0002", "2C02", "2C04"):
        print(action, sum(row["action"] == action for row in rows))
    print("Candidate catalog:", a.out)
    print("No candidate is considered executed until runtime PPC evidence exists.")


if __name__ == "__main__":
    main()
