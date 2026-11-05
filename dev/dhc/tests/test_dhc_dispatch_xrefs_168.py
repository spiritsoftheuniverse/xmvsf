#!/usr/bin/env python3
"""ROM-free regression for native 68000 dispatcher xref signatures."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from TRACE_68000_DISPATCH_CALLERS import (
    opcode_branch_target, scan, DEFAULT_TARGETS,
)


def main():
    code = bytearray(0x20000)

    def put(pc, op):
        code[pc:pc + len(op)] = bytes.fromhex(op)

    # Exact real disassembly fixture from 011B1A -> 01038E.
    put(0x11B1A, "4efae872")
    # A direct 68000 absolute JMP.
    put(0x10500, "4ef90001038e")
    # A hypothetical known-action preparation near a real dispatcher call.
    put(0x10800, "702e")
    put(0x10804, "720c")
    put(0x10810, "4eb90001038e")
    # Another legit native action entry with a different implementation PC.
    put(0x10C00, "4eb9000103c6")
    assert opcode_branch_target(code, 0x11B1A) == (
        "JMP_PC_W", 0x01038E, 4)
    assert opcode_branch_target(code, 0x10810) == (
        "JSR_ABS_L", 0x01038E, 6)

    result = scan(bytes(code))
    assert result["target_pcs"] == [
        f"{p:06X}" for p in DEFAULT_TARGETS]
    assert result["all_static_dispatch_xrefs"] == 4, result
    assert result["near_action_2e_immediate"] == 1, result
    ranked = result["candidates"][0]
    assert ranked["pc"] == "010810"
    assert ranked["recent_action_2e_immediates"][0]["pc"] == "010800"
    assert ranked["status"] == "STATIC_XREF_CANDIDATE_NOT_EXECUTION_PROOF"

    # Must handle a one-instruction long PC-relative BRA to 01038E.
    put(0x11B1A, "6000e872")
    assert opcode_branch_target(code, 0x11B1A) == (
        "BRA_W", 0x01038E, 4)
    print("PASS: native 68000 xrefs, action 2E hints, PC-rel branches")


if __name__ == "__main__":
    main()
