#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Street Fighter Alpha 3's address index from HSFA (https://github.com/Rickow/HSFA, MIT: re/connaissances/
# sfa3_arcade.tsv, about 9,000 addresses with their role and how each was found, in French), put at our set's
# addresses and checked against our program, as a local reference for games/sfa3. HSFA's reference set is sfa3ud (USA
# 980904, Phoenix Edition: a deciphered copy), ours sfa3u (USA 980904). The two are one build: every word of sfa3ud's
# program is our deciphered program's (instructions, and what is read through the pc) or its raw one (other data), but
# for the Phoenix Edition's own changes (the vectors, the start, its code from 0FF000), so an address of the one is the
# same address of the other and nothing is moved. What the index has of the Phoenix changes, and of HSFA's own
# translation (their image, their emulator core's additions), is marked so, not dropped.
#
# The result is a list made from the game's program, so it is kept out of the repository: work/sfa3/hsfa_index.tsv
# (work/ is not committed). Run from the repository's top after `make GAME=sfa3`:
#   tools/hsfa_index.py HSFA/re/connaissances/sfa3_arcade.tsv
#   tools/hsfa_index.py TSV --sfa3ud DIR    also check, against sfa3ud's program ROMs (sz3ud.03c, sz3ud.04c,
#                                           sz3.05c-sz3.10b, in DIR or a zip), that the two programs are still one
#                                           build and where they differ (else the built-in list of the Phoenix changes)
#   tools/hsfa_index.py TSV --out FILE      somewhere else than work/sfa3/hsfa_index.tsv
#
# Columns of the result: address (ours; a range LOW-HIGH), check (same: our program has it at the same address, and an
# instruction of our listing begins there if it is a routine; phoenix: sfa3ud's own change, not in sfa3u; theirs:
# HSFA's translation or emulator, not the board's program; unlisted: a routine of theirs where our listing, which has
# only the code the translation has reached, has nothing; mid-instruction: a routine of theirs inside one of our
# listing's instructions, which is not a place the game's code goes to: most are where their MIPS translation was cut
# up, a few wrong),
# kind, name, role, cautions, how established (mesuré measured, déduit deduced, supposé assumed), their source.
import argparse
import bisect
import csv
import os
import re
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import m68k  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORK = os.path.join(ROOT, "work", "sfa3")

# Where sfa3ud's program is neither our deciphered program nor our raw one (found with --sfa3ud): the Phoenix
# Edition's vectors and start, and its own code at the end of the first megabyte
PHOENIX = [(0x000008, 0x000030), (0x000922, 0x000926), (0x000932, 0x000956), (0x0009FC, 0x0009FE), (0x000BCC, 0x000BF0),
           (0x000D66, 0x000D68), (0x00105A, 0x001060), (0x0011EA, 0x001206), (0x00143E, 0x001442), (0x0018C6, 0x0018C8),
           (0x0019DE, 0x0019E2), (0x01094C, 0x010950), (0x010B3A, 0x010B46), (0x010B54, 0x010B64), (0x0FF000, 0x0FFD38),
           (0x0FFE00, 0x0FFE1C)]

SFA3UD = ["sz3ud.03c", "sz3ud.04c", "sz3.05c", "sz3.06c", "sz3.07c", "sz3.08c", "sz3.09c", "sz3.10b"]

# HSFA's names for what belongs to their translation and emulator rather than to the board's program (other names of
# theirs, max_ or recomp_ say, are of arcade code too)
THEIRS_NAMES = re.compile(r"^(image|fork|hsfa|greffe|portage|decor_sfa2|decors_ws|chevauchement|"
                          r"sauvegardes_registres_portage|paires_s16|ram_zone_basse_relogee)")


def read_rom_files(where):
    """sfa3ud's program, in the 68000's byte order (MAME loads its ROMs word-swapped)."""
    out = bytearray()
    zf = zipfile.ZipFile(where) if where.endswith(".zip") else None
    for name in SFA3UD:
        if zf:
            data = zf.read(name)
        else:
            with open(os.path.join(where, name), "rb") as f:
                data = f.read()
        for i in range(0, len(data), 2):
            out += bytes((data[i + 1], data[i]))
    return bytes(out)


def phoenix_ranges(ud, code, data):
    """Runs of words where sfa3ud is neither of our views (joined across a few bytes)."""
    runs, start = [], None
    for i in range(0, min(len(ud), len(code), len(data)), 2):
        w = ud[i:i + 2]
        bad = w != code[i:i + 2] and w != data[i:i + 2]
        if bad and start is None:
            start = i
        elif not bad and start is not None:
            runs.append([start, i])
            start = None
    if start is not None:
        runs.append([start, len(ud)])
    joined = []
    for a, b in runs:
        if joined and a - joined[-1][1] <= 8:
            joined[-1][1] = b
        else:
            joined.append([a, b])
    return [tuple(r) for r in joined]


def instruction_starts(listing):
    """Addresses where our listing (work/sfa3/program.lst) has an instruction, in order."""
    starts = []
    with open(listing, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"[> ]([0-9A-F]{6}) ", line)
            if m:
                starts.append(int(m.group(1), 16))
    return sorted(set(starts))


def place_in_listing(address, starts, code):
    """same, mid-instruction or unlisted (see above)"""
    i = bisect.bisect_right(starts, address) - 1
    if i >= 0 and starts[i] == address:
        return "same"
    if i >= 0 and address < starts[i] + m68k.decode(code, starts[i]).length:
        return "mid-instruction"
    return "unlisted"


def parse_address(text):
    m = re.match(r"\$([0-9A-Fa-f]+)(?:-\$([0-9A-Fa-f]+))?$", text.strip())
    if not m:
        return None, None
    low = int(m.group(1), 16)
    return low, int(m.group(2), 16) if m.group(2) else low


def main():
    ap = argparse.ArgumentParser(description="HSFA's Alpha 3 address index at our addresses (work/sfa3/hsfa_index.tsv)")
    ap.add_argument("tsv", help="HSFA's re/connaissances/sfa3_arcade.tsv")
    ap.add_argument("--program", default=os.path.join(WORK, "program.bin"),
                    help="our deciphered program (program.bin.data, its raw view, beside it)")
    ap.add_argument("--listing", default=os.path.join(WORK, "program.lst"))
    ap.add_argument("--sfa3ud", help="sfa3ud's ROM files (a folder or its zip), to check the two are one build")
    ap.add_argument("--out", default=os.path.join(WORK, "hsfa_index.tsv"))
    args = ap.parse_args()

    with open(args.program, "rb") as f:
        code = f.read()
    with open(args.program + ".data", "rb") as f:
        data = f.read()
    patches = PHOENIX
    if args.sfa3ud:
        ud = read_rom_files(args.sfa3ud)
        patches = phoenix_ranges(ud, code, data)
        differing = sum(b - a for a, b in patches)
        print("sfa3ud against our program: %d bytes neither view's, in %d places:" % (differing, len(patches)))
        for a, b in patches:
            print("  %06X-%06X" % (a, b - 1))
        if differing > 0x4000:
            sys.exit("the programs are not one build: their addresses would have to be lined up (as tools/ cannot yet)")
    starts = instruction_starts(args.listing) if os.path.exists(args.listing) else None

    with open(args.tsv, encoding="utf-8", newline="") as f:
        rows = list(csv.reader(f, delimiter="\t", quoting=csv.QUOTE_NONE))
    head, rows = rows[0], rows[1:]
    col = {name: n for n, name in enumerate(head)}

    def field(row, name):
        n = col.get(name)
        return row[n] if n is not None and n < len(row) else ""

    counts = {}
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="") as out:
        w = csv.writer(out, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_NONE, escapechar="\\")
        w.writerow(["address", "check", "kind", "name", "role", "cautions", "established", "source"])
        for row in rows:
            low, high = parse_address(field(row, "adresse"))
            if low is None:
                continue
            name, kind = field(row, "nom"), field(row, "genre")
            role = field(row, "role")
            if THEIRS_NAMES.match(name) or 0x940000 <= low < 0xFF0000 or \
                    re.search(r"[Ii]mage traduite, pas|AJOUTÉS? par", role):
                check = "theirs"
            elif any(a <= low < b for a, b in patches) or name.startswith("phoenix"):
                check = "phoenix"
            elif kind == "fonction" and low < 0x100000 and starts is not None:
                check = place_in_listing(low, starts, code)
            else:
                check = "same"
            counts[check] = counts.get(check, 0) + 1
            address = "%06X" % low if high == low else "%06X-%06X" % (low, high)
            w.writerow([address, check, kind, name, role, field(row, "mises_en_garde"), field(row, "confiance"),
                        field(row, "source")])
    print("%s: %d entries (%s)" % (os.path.relpath(args.out, ROOT), sum(counts.values()),
                                   ", ".join("%d %s" % (n, c) for c, n in sorted(counts.items()))))


if __name__ == "__main__":
    main()
