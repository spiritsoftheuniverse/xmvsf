#!/usr/bin/env python3
"""Read-only integrity verifier for native XMVSF F9 host-aware recordings.

Authoritative schema: REPLAY_DATA_MODEL_MANIFESTO.md.
Never writes, repairs, converts, decodes proprietary ROM data, or mutates a
savestate. Handles a session directory, its manifest.tsv, or a handoff ZIP.

Examples:
    python dev/dhc/tools/VERIFY_REPLAY_V2.py test/87.167/replays/session_0001
    python dev/dhc/tools/VERIFY_REPLAY_V2.py 87.166.zip --allow-unsealed
    python dev/dhc/tools/VERIFY_REPLAY_V2.py --self-test
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
import tempfile
import zipfile


class ReplayError(Exception):
    pass


def required(ok, message):
    if not ok:
        raise ReplayError(message)


class SessionSource:
    def __init__(self, path):
        self.path = Path(path)
        self.zip = None
        self.prefix = ""
        if self.path.suffix.lower() == ".zip":
            self.zip = zipfile.ZipFile(self.path, "r")
            manifests = sorted(
                x for x in self.zip.namelist()
                if x == "manifest.tsv" or x.endswith("/manifest.tsv")
            )
            required(len(manifests) == 1,
                     f"Expected exactly one manifest.tsv in ZIP, found {len(manifests)}")
            self.prefix = manifests[0][:-len("manifest.tsv")]
        else:
            self.path = self.path.parent if self.path.name == "manifest.tsv" else self.path
            required(self.path.is_dir(), f"Not a replay directory: {self.path}")

    def read(self, leaf):
        required(leaf != "" and leaf not in (".", "..") and
                 "/" not in leaf and "\\" not in leaf and ".." not in leaf,
                 f"Unsafe manifest filename: {leaf!r}")
        if self.zip is not None:
            try:
                return self.zip.read(self.prefix + leaf)
            except KeyError as e:
                raise ReplayError(f"Referenced file missing from ZIP: {leaf}") from e
        try:
            return (self.path / leaf).read_bytes()
        except OSError as e:
            raise ReplayError(f"Referenced file missing/unreadable: {leaf}: {e}") from e

    def close(self):
        if self.zip is not None:
            self.zip.close()


def verify(source: SessionSource, *, allow_unsealed=False):
    try:
        manifest = source.read("manifest.tsv").decode("utf-8")
    except UnicodeDecodeError as e:
        raise ReplayError("Manifest must be UTF-8/ASCII") from e
    lines = manifest.splitlines()
    required(lines and lines[0] == "XM8764V2\t1",
             "Bad or unsupported manifest signature")
    segments = []
    closed = set()
    events = []
    footer = None
    for n, line in enumerate(lines[1:], 2):
        if not line:
            continue
        row = line.split("\t")
        kind = row[0]
        required(footer is None, f"Record after FINAL at line {n}")
        if kind == "EVENT":
            required(len(row) == 6, f"Malformed EVENT line {n}")
            try:
                sid, absolute = int(row[1]), int(row[2])
            except ValueError as e:
                raise ReplayError(f"Bad EVENT indices at line {n}") from e
            required(sid >= 0 and absolute >= 0,
                     f"Negative EVENT indices at line {n}")
            required(bool(row[3]), f"Missing EVENT verb at line {n}")
            events.append((sid, absolute, row[3]))
        elif kind == "SEG":
            required(len(row) == 26, f"SEG requires 19 host fields at line {n}")
            try:
                sid, index, mode = map(int, row[1:4])
                host = tuple(map(int, row[7:]))
            except ValueError as e:
                raise ReplayError(f"Non-integer SEG fields at line {n}") from e
            required(sid >= 1 and index >= 0 and mode in (0, 1, 2),
                     f"Invalid session/segment/mode at line {n}")
            required(bool(row[4]), f"Missing checkpoint reason at line {n}")
            required(not segments or (sid == segments[-1]["session"] and
                                      index == segments[-1]["index"] + 1) or
                     (sid == segments[-1]["session"] + 1 and index == 0),
                     f"Noncontiguous session/segment at line {n}")
            required(segments or (sid == 1 and index == 0),
                     f"First segment must be session 1/segment 0 at line {n}")
            required((sid, index) not in closed,
                     f"Duplicate SEG at line {n}")
            segments.append({
                "session": sid, "index": index, "mode": mode,
                "reason": row[4], "inputs": row[5], "state": row[6],
                "host": host, "frames": None,
            })
        elif kind == "DONE":
            required(len(row) == 4, f"Malformed DONE at line {n}")
            try:
                sid, index, frames = map(int, row[1:])
            except ValueError as e:
                raise ReplayError(f"Non-integer DONE at line {n}") from e
            required(frames > 0, f"Empty segment DONE at line {n}")
            required(segments and
                     (segments[-1]["session"], segments[-1]["index"]) ==
                     (sid, index), f"DONE does not close last SEG at line {n}")
            required((sid, index) not in closed,
                     f"Duplicate DONE at line {n}")
            segments[-1]["frames"] = frames
            closed.add((sid, index))
        elif kind == "FINAL":
            required(len(row) == 5, f"Malformed FINAL at line {n}")
            try:
                final_frames, final_sessions, final_segments = map(int, row[1:4])
            except ValueError as e:
                raise ReplayError(f"Non-integer FINAL counters at line {n}") from e
            required(row[4] in ("F9_STOP", "APPLICATION_EXIT"),
                     f"Unknown FINAL stop reason at line {n}")
            footer = (final_frames, final_sessions, final_segments, row[4])
        else:
            raise ReplayError(f"Unknown manifest record {kind!r} at line {n}")
    required(segments, "Manifest has no recorded segments")
    required(all(s["frames"] is not None for s in segments),
             "Manifest contains an uncommitted SEG without DONE")
    sessions = len({s["session"] for s in segments})
    total_frames = sum(s["frames"] for s in segments)
    if footer is not None:
        required(footer[:3] == (total_frames, sessions, len(segments)),
                 f"FINAL totals mismatch: {footer[:3]} vs "
                 f"{(total_frames, sessions, len(segments))}")
    else:
        required(allow_unsealed,
                 "UNSEALED PREFIX: missing FINAL. Explicit --allow-unsealed "
                 "required; never publish as latest successful recording.")
    last_event_frame = -1
    for sid, absolute, verb in events:
        required(absolute >= last_event_frame,
                 f"Nonmonotonic EVENT at frame {absolute}: {verb}")
        required(absolute <= total_frames,
                 f"EVENT points beyond captured movie: {absolute} ({verb})")
        last_event_frame = absolute

    rom = None
    slot_layout = None
    for s in segments:
        data = source.read(s["inputs"])
        state = source.read(s["state"])
        required(len(state) > 8 and state.startswith(b"FB1 "),
                 f"Invalid native FBNeo full state {s['state']}")
        required(len(data) >= 44 and data[:8] == b"XM8761R1",
                 f"Invalid native input header {s['inputs']}")
        count = struct.unpack_from("<I", data, 8)[0]
        required(0 < count <= 1024, f"Input slot count out of range in {s['inputs']}")
        rom_name = data[12:44].split(b"\x00", 1)[0]
        required(rom_name == b"xmvsfu",
                 f"Recording is not the expected xmvsfu ROM: {rom_name!r}")
        header_end = 44 + count * 2
        required(len(data) >= header_end,
                 f"Truncated input slot layout {s['inputs']}")
        layout = data[44:header_end]
        widths = layout[1::2]
        required(all(width in (1, 2) for width in widths),
                 f"Unsupported input width in {s['inputs']}")
        stride = 4 + sum(widths)
        required(len(data) - header_end == s["frames"] * stride,
                 f"Payload size mismatch in {s['inputs']}; expected "
                 f"{s['frames']} x {stride} after header, "
                 f"got {len(data) - header_end} bytes")
        for i in range(s["frames"]):
            offset = header_end + i * stride
            frame = struct.unpack_from("<I", data, offset)[0]
            required(frame == i,
                     f"Noncontiguous input index {frame} at frame {i} "
                     f"in {s['inputs']}")
        if slot_layout is None:
            slot_layout = layout
            rom = rom_name.decode("ascii")
        else:
            required(layout == slot_layout,
                     f"Input slot layout changed at {s['inputs']}")
    return {
        "status": "SEALED" if footer else "UNSEALED_PREFIX",
        "rom": rom,
        "sessions": sessions,
        "segments": len(segments),
        "frames": total_frames,
        "segment_frames": [s["frames"] for s in segments],
        "host_events": len(events),
        "input_slots": len(slot_layout) // 2,
        "stop_reason": footer[3] if footer else None,
        "files_verified": len(segments) * 2 + 1,
        "warning": None if footer else
        "Every SEG has a valid DONE and matching native payload, but the "
        "original capture never recorded FINAL. No missing data was invented.",
    }


def self_test():
    with tempfile.TemporaryDirectory(prefix="xmvsf_v2_verify_") as root:
        root = Path(root)
        layout = bytes((1, 1, 1, 1))
        state = b"FB1 FS1 \x00\x00\x00\x00"
        rom = b"xmvsfu" + bytes(26)
        inputs = b"XM8761R1" + struct.pack("<I", 2) + rom + layout
        inputs += struct.pack("<I", 0) + bytes((0x13, 0x25))
        (root / "s0001_seg0000_inputs.bin").write_bytes(inputs)
        (root / "s0001_seg0000_start.fs").write_bytes(state)
        manifest = "\n".join((
            "XM8764V2\t1",
            "EVENT\t1\t0\tGAME_BEGIN\t1\t0",
            "SEG\t1\t0\t1\tSESSION_START\t"
            "s0001_seg0000_inputs.bin\ts0001_seg0000_start.fs\t" +
            "\t".join("0" for _ in range(19)),
            "DONE\t1\t0\t1",
            "FINAL\t1\t1\t1\tF9_STOP",
        )) + "\n"
        (root / "manifest.tsv").write_text(manifest)
        reader = SessionSource(root)
        try:
            assert verify(reader)["status"] == "SEALED"
            (root / "manifest.tsv").write_text(
                manifest.replace("FINAL\t1\t1\t1\tF9_STOP\n", "")
            )
            try:
                verify(reader)
                raise AssertionError("Unsealed capture was accepted implicitly")
            except ReplayError as e:
                assert "UNSEALED PREFIX" in str(e)
            assert verify(reader, allow_unsealed=True)["status"] == "UNSEALED_PREFIX"
            (root / "s0001_seg0000_inputs.bin").write_bytes(inputs[:-1])
            try:
                verify(reader, allow_unsealed=True)
                raise AssertionError("Truncated input was not detected")
            except ReplayError as e:
                assert "Payload size mismatch" in str(e)
        finally:
            reader.close()
    print("REPLAY V2 SELFTEST PASS: SEALED / UNSEALED PREFIX / TRUNCATED")


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("recording", nargs="?", help="session folder, manifest.tsv, or ZIP")
    p.add_argument("--allow-unsealed", action="store_true",
                   help="explicitly inspect a complete-frame unsealed legacy prefix")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    p.error("recording is required unless --self-test" if not args.recording else "")
    src = SessionSource(args.recording)
    try:
        report = verify(src, allow_unsealed=args.allow_unsealed)
        report["recording"] = str(args.recording)
        print(json.dumps(report, indent=2))
        return 0
    finally:
        src.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, ReplayError, zipfile.BadZipFile) as exc:
        print("REPLAY_V2_VALIDATION_FAILED:", exc, file=sys.stderr)
        raise SystemExit(2)
