#!/usr/bin/env python3
"""Return the original F9 v2 ROM driver from its binary input header.

The launch BAT uses this rather than guessing that every recording is XMVSF.
No ROM payloads are read, installed or copied.
"""
from pathlib import Path
import re
import struct
import sys

ALLOWED = re.compile(r"(?:xmvsfu|mvsc[a-z0-9]*)\Z", re.ASCII)

def get_driver(folder: Path) -> str:
    manifest = folder / "manifest.tsv"
    if not manifest.is_file():
        raise ValueError("missing F9 v2 manifest")
    drivers = set()
    segments = 0
    for line in manifest.read_text(encoding="utf-8").splitlines():
        fields = line.split("\t")
        if fields[0] != "SEG":
            continue
        if len(fields) < 7:
            raise ValueError("malformed F9 segment")
        rel = Path(fields[5])
        if rel.is_absolute() or ".." in rel.parts or len(rel.parts) != 1:
            raise ValueError("unsafe F9 input filename")
        path = folder / rel
        with path.open("rb") as stream:
            head = stream.read(44)
        if len(head) != 44 or head[:8] != b"XM8761R1":
            raise ValueError("F9 input header signature/truncation")
        slot_count = struct.unpack("<I", head[8:12])[0]
        if slot_count == 0 or slot_count > 1024:
            raise ValueError("impossible F9 native input count")
        rom = head[12:44].split(b"\x00", 1)[0].decode("ascii")
        if not ALLOWED.fullmatch(rom):
            raise ValueError("unexpected ROM driver in F9 input header: " + rom)
        drivers.add(rom)
        segments += 1
    if segments == 0 or len(drivers) != 1:
        raise ValueError("F9 session is empty or contains multiple ROM drivers")
    return drivers.pop()

def main(argv):
    if len(argv) != 2:
        print("usage: DETECT_REPLAY_DRIVER.py <F9-session-directory>", file=sys.stderr)
        return 2
    try:
        print(get_driver(Path(argv[1]).resolve()))
        return 0
    except (ValueError, OSError, UnicodeError) as exc:
        print("F9 ROM DETECTOR ERROR:", exc, file=sys.stderr)
        return 2

if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
