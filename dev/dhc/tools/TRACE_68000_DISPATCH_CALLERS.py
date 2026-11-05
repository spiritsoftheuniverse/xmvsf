#!/usr/bin/env python3
"""Read-only static search for original XMVSF 68000 action-dispatch callers.

Runs against the locally exported PRIVATE decrypted CpsCode, host word-swapped.
No ROM bytes are written to disk or committed. A syntactically matching opcode
is only a candidate instruction boundary until executable control flow proves
it was reached; motion recognition is NOT automatically established.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path

from DISASSEMBLE_SUPER_MOVES import byte_unswap

DEFAULT_TARGETS = (0x01038E, 0x0103C6)
ACTION_IMMEDIATE_PATTERNS = {
    "MOVEQ_D0_2E": bytes.fromhex("702e"),
    "MOVE_W_IMMEDIATE_D0_2E": bytes.fromhex("303c002e"),
    "MOVE_B_IMMEDIATE_D0_2E": bytes.fromhex("103c002e"),
    "MOVE_B_IMMEDIATE_ACTION_A6_2E": bytes.fromhex("1d7c002e0060"),
}


def u16(code, p):
    return (code[p] << 8) | code[p + 1]


def u32(code, p):
    return (u16(code, p) << 16) | u16(code, p + 2)


def signed16(value):
    return value - 0x10000 if value >= 0x8000 else value


def signed8(value):
    return value - 0x100 if value >= 0x80 else value


def opcode_branch_target(code, pc):
    """Decode only unambiguous JSR/JMP and BRA/BSR encodings."""
    if pc + 2 > len(code):
        return None
    op = u16(code, pc)
    if op in (0x4EB9, 0x4EF9) and pc + 6 <= len(code):
        return ("JSR_ABS_L" if op == 0x4EB9 else "JMP_ABS_L",
                u32(code, pc + 2), 6)
    if op in (0x4EBA, 0x4EFA) and pc + 4 <= len(code):
        return ("JSR_PC_W" if op == 0x4EBA else "JMP_PC_W",
                pc + 2 + signed16(u16(code, pc + 2)), 4)
    if (op & 0xFF00) in (0x6000, 0x6100):
        kind = "BRA" if (op & 0xFF00) == 0x6000 else "BSR"
        if (op & 0xFF) == 0:
            if pc + 4 > len(code):
                return None
            return (kind + "_W",
                    pc + 2 + signed16(u16(code, pc + 2)), 4)
        return (kind + "_B", pc + 2 + signed8(op & 0xFF), 2)
    return None


def collect_action_immediates(code):
    """Find byte signatures, not yet proven instruction boundaries."""
    positions = {}
    for kind, pattern in ACTION_IMMEDIATE_PATTERNS.items():
        start = 0
        while True:
            pc = code.find(pattern, start)
            if pc == -1:
                break
            if pc % 2 == 0:
                positions.setdefault(pc, []).append(kind)
            start = pc + 2
    return positions


def scan(code, targets=DEFAULT_TARGETS, window=96, limit=512):
    if len(code) % 2:
        raise ValueError("Decrypted 68000 code length must be even")
    wanted = set(targets)
    immediate = collect_action_immediates(code)
    action_positions = sorted(immediate)
    # Extract exact synthetic opcode forms only; an even address alone does
    # NOT independently demonstrate an instruction boundary or execution.
    sites = []
    for pc in range(0, len(code) - 1, 2):
        info = opcode_branch_target(code, pc)
        if info is None or info[1] not in wanted:
            continue
        kind, target, length = info
        nearby = [
            {"pc": f"{at:06X}", "kind": form, "offset": pc - at}
            for at in action_positions if pc - window <= at <= pc
            for form in immediate[at]
        ]
        # Backward adjacency is a ranking hint, not a valid straight-line
        # dependency until the CPU control flow and caller registers are traced.
        sites.append({
            "pc": f"{pc:06X}", "target_pc": f"{target:06X}",
            "opcode_form": kind, "length": length,
            "recent_action_2e_immediates": nearby,
            "has_nearby_action_2e_hint": bool(nearby),
            "status": "STATIC_XREF_CANDIDATE_NOT_EXECUTION_PROOF",
        })
    # Prioritize potential super action setters; always also report complete
    # aggregate counts even when max emitted entries has been reached.
    sites.sort(key=lambda row: (
        not row["has_nearby_action_2e_hint"],
        min((hint["offset"] for hint in row["recent_action_2e_immediates"]),
            default=999999), int(row["pc"], 16)))
    selected = sites[:limit]
    # Report the immediate patterns close to an XREF, as the most useful
    # starting points for native input-read, branch and motion-buffer xrefs.
    return {
        "schema": "xmvsf-68000-dispatch-xref-v1",
        "source": "PRIVATE_DECRYPTED_OPCODE_DUMP_NOT_EMBEDDED",
        "target_pcs": [f"{pc:06X}" for pc in targets],
        "code_bytes": len(code),
        "all_static_dispatch_xrefs": len(sites),
        "near_action_2e_immediate": sum(x["has_nearby_action_2e_hint"]
                                        for x in sites),
        "action_2e_immediate_signature_hits": len(immediate),
        "xref_window_bytes": window,
        "emitted_static_xrefs": len(selected),
        "truncated": len(selected) < len(sites),
        "candidates": selected,
        "limitation": (
            "This is a syntactic candidate list, not validated code "
            "reachability. Does not cover register-indirect calls, calls "
            "through ROM pointers, or the native input recognizer itself. "
            "A command is only verified after actual branch/input tracing."
        ),
        "no_gameplay_change": True,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--opcodes", required=True, type=Path,
                        help="PRIVATE host-word-swapped decrypted opcode dump")
    parser.add_argument("--out", required=True, type=Path,
                        help="Derived analysis JSON only; NO ROM bytes")
    parser.add_argument("--target", action="append", default=[],
                        help="Native dispatcher PC as hexadecimal; repeatable")
    parser.add_argument("--window", type=int, default=96,
                        help="Prior bytes to inspect for action-2E immediates")
    parser.add_argument("--max-results", type=int, default=512)
    args = parser.parse_args()
    if args.window < 0 or args.window > 2048 or args.max_results < 1:
        parser.error("Invalid search window/result count")
    if not args.opcodes.is_file():
        parser.error("Private decrypted opcodes missing: " + str(args.opcodes))
    targets = tuple(int(x, 16) for x in args.target) if args.target \
        else DEFAULT_TARGETS
    code = byte_unswap(args.opcodes.read_bytes())
    result = scan(code, targets, args.window, args.max_results)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Static dispatch xrefs:", result["all_static_dispatch_xrefs"])
    print("Nearby action-2E signatures:", result["near_action_2e_immediate"])
    print("Report:", args.out)
    print("STATUS: STATIC XREFS ONLY, command recognizer not proven")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
