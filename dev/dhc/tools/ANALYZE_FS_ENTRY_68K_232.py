#!/usr/bin/env python3
"""87.232 read-only original XMVSF 68000 FS return control-flow windows.

Original source files are already retained locally under local_rom_dump/.
They are not copied, re-encrypted, modified, or committed by this utility.
The resulting TSV records real decrypted 68k 16-bit opcode words around
executed native PCs. Opcode *families* are hints, not a full disassembly.
"""
import argparse
import csv
import hashlib
from pathlib import Path

PCs = (
    (0x010392, "reset_action_before_bad_x"),
    (0x0109B4, "writer_of_b27c0700_world_x"),
    (0x012844, "native_controller_reset"),
    (0x01284A, "enable_ground_dash_1400"),
    (0x01038E, "native_action_dispatch_after_dash"),
    (0x01273E, "native_superjump_air_flag_test"),
    (0x012772, "native_superjump_mode_enter"),
    (0x0137B2, "air_dash_dispatch_reference"),
)

def word(buf: bytes, pc: int):
    if pc < 0 or pc+1 >= len(buf) or (pc & 1):
        return None
    return (buf[pc ^ 1] << 8) | buf[(pc + 1) ^ 1]

def family(w):
    if w is None:
        return "OUT_OF_RANGE"
    if w == 0x4E75:
        return "RTS"
    if w in (0x4EB9, 0x4EBA):
        return "JSR"
    if w in (0x4EF9, 0x4EFA):
        return "JMP"
    if (w & 0xFF00) == 0x6000:
        return "BRA"
    if (w & 0xFF00) == 0x6100:
        return "BSR"
    if (w & 0xF000) == 0x6000:
        return "Bcc"
    if (w & 0xF000) == 0x1000:
        return "MOVE.B"
    if (w & 0xF000) == 0x2000:
        return "MOVE.L"
    if (w & 0xF000) == 0x3000:
        return "MOVE.W"
    if (w & 0xF000) == 0xB000:
        return "CMP/EOR"
    if (w & 0xF000) == 0xD000:
        return "ADD"
    if (w & 0xF000) == 0x9000:
        return "SUB"
    return "OTHER"

def extract(code: bytes, data: bytes):
    for pc, role in PCs:
        opcode = word(code, pc)
        raw = word(data, pc)
        # The PPC in the native instruction callback names the previous
        # executed PC. Include context before and after to identify actual
        # addressing modes and extension words; DON'T assume one-word length.
        around = [word(code, addr) for addr in range(pc-24, pc+42, 2)]
        yield {
            "PC": f"{pc:06X}",
            "observed_87_231_role": role,
            "decrypted_opcode": f"{opcode:04X}" if opcode is not None else "",
            "source_data_word": f"{raw:04X}" if raw is not None else "",
            "opcode_family_hypothesis": family(opcode),
            "nearby_words_-24_through_+40": " ".join(
                f"{x:04X}" if x is not None else "----" for x in around
            ),
            "source": "xmvsfu_original_68000_word_swapped",
        }

def run(code_path: Path, data_path: Path, output: Path):
    code = code_path.read_bytes()
    data = data_path.read_bytes()
    if len(code) < 0x014000 or len(data) < 0x014000:
        raise ValueError("Source too small for verified 68000 dash PCs")
    rows = list(extract(code, data))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]), delimiter="\t")
        w.writeheader()
        w.writerows(rows)
    print("87.232 source-backed 68000 PC windows:", output)
    print("decrypted source SHA256:", hashlib.sha256(code).hexdigest())
    for row in rows:
        print(row["PC"], row["observed_87_231_role"],
              row["decrypted_opcode"], row["opcode_family_hypothesis"])

def main():
    project = Path(__file__).resolve().parents[3]
    p = argparse.ArgumentParser()
    p.add_argument("--code", type=Path, default=project / "local_rom_dump" /
                   "xmvsfu_decrypted_opcodes_host_swapped.bin")
    p.add_argument("--data", type=Path, default=project / "local_rom_dump" /
                   "xmvsfu_program_data_host_swapped.bin")
    p.add_argument("--output", type=Path, default=project / "test" /
                   "87.232" / "observer" / "fs_entry_68000_opcode_windows.tsv")
    a = p.parse_args()
    run(a.code, a.data, a.output)

if __name__ == "__main__":
    main()
