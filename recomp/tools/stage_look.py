#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Another game's stage (fight/guest_stages.h), as a game shows it, against the same stage in its own game: what a person
# sees at once when it is wrong. Both are recorded from the stage viewer (--stage-look FILE), the camera moved with the
# stick: held still where the round puts it, then at the stage's left end, then at its right (MOVIE, below).
#   tools/stage_look.py sweep --host sfa2 --rom rom/sfa2 --guest sfa3=rom/sfa3 --guest msh=rom/msh ... [--only msh]
#     [--build build] [--jobs 2] [--pictures DIR]
# records every stage of the guests named, each in its own game (built in build/<id>/<id>) and in the host, and compares
# them; or two recordings made otherwise:
#   tools/stage_look.py own.slk shown.slk [--name NAME] [--pictures PREFIX]
# Where both held the camera at the same place (all three scroll screens at the same places), every one of the own game's
# frames there is against each shown there; at each pixel the shown colour is to be one the own game has there in one of
# them (its pieces move: any step of theirs). What is checked:
#   - missing or wrong: the share of the picture whose colour the own game never has there (a piece left out, a scroll
#     screen not drawn, wrong colours)
#   - its pieces moving: how much of the picture changes from a frame to the next while the camera stays put, against the
#     own game's (not moving at all, or faster)
#   - its camera: how far it goes in a frame, against the own game's with the stick held (much faster: it jumps)
#   - in front of the fighters: in how many frames any of its pixels are (Guest::fightersDrawn)
# Each says ok or what is wrong; the exit status is 1 if anything is. With --pictures, PREFIX-worst.png: the own game's,
# the shown, and where they differ (red), at the frame that differs most. A failure is to be looked at: the own game's
# viewer is not always right either (a stage whose pieces go by the round's clock, one that changes by itself, a viewer
# that shows more than the stage at the round's start).
# Needs numpy (pip install numpy).
import argparse
import os
import struct
import subprocess
import sys
import tempfile
import zlib
from concurrent.futures import ThreadPoolExecutor

try:
    import numpy as np
except ImportError:
    sys.exit("stage_look.py needs numpy: pip install numpy")

MISSING_MOST = 0.02  # (a share of the picture: more differs, at the frame that differs least of a place's, than this)
MOVING_LEAST, MOVING_MOST = 0.35, 2.5  # (the shown stage's pieces' changes a frame, against the own game's)
STEP_MOST = 2.5  # (the shown camera's steps against the own game's)
HELD = 40  # (frames with the camera still at a place, in each, for the place to be compared)
NEAR = 24  # (a colour this near, red + green + blue, is the same: the two palettes' rounding)


def read(path):
    with open(path, "rb") as f:
        data = f.read()
    frames = []
    at = 0
    while at + 24 <= len(data):
        if data[at:at + 4] != b"SLK1":
            sys.exit(f"{path}: not a --stage-look file (at byte {at})")
        number, = struct.unpack_from("<I", data, at + 4)
        width, height = struct.unpack_from("<HH", data, at + 8)
        key = struct.unpack_from("<6H", data, at + 12)
        pixels = width * height
        rgb = np.frombuffer(data, np.uint8, pixels * 3, at + 24).reshape(height, width, 3)
        front = np.frombuffer(data, np.uint8, pixels, at + 24 + pixels * 3).reshape(height, width)
        frames.append((number, key, rgb, front))
        at += 24 + pixels * 4
    return frames


def write_png(path, rgb):
    height, width, _ = rgb.shape
    raw = b"".join(b"\0" + rgb[y].tobytes() for y in range(height))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
                chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def place(key, layer):
    return key[layer * 2], key[layer * 2 + 1]


def moving(frames):
    # The share of the picture that changes from a frame to the next while the camera stays put (all three screens'
    # places the same), on average; a change of most of the picture (a fade, a flash) left out
    shares = []
    for (n0, k0, a, _), (n1, k1, b, _) in zip(frames, frames[1:]):
        if n1 != n0 + 1 or k0 != k1:
            continue
        share = float((np.abs(a.astype(np.int16) - b.astype(np.int16)).sum(axis=2) > NEAR).mean())
        if share < 0.5:
            shares.append(share)
    return len(shares), (sum(shares) / len(shares) if shares else 0.0)


def steps(frames, layer):
    out = []
    for (n0, k0, _, _), (n1, k1, _, _) in zip(frames, frames[1:]):
        if n1 == n0 + 1:
            d = abs(((place(k1, layer)[0] - place(k0, layer)[0] + 0x8000) & 0xFFFF) - 0x8000)
            if d:
                out.append(d)
    return out


def compare(own_path, shown_path, name, pictures=""):
    own, shown = read(own_path), read(shown_path)
    if not own or not shown:
        return False, [f"{name}: FAIL: no frames ({len(own)} of the own game's, {len(shown)} shown)"]
    # (the scroll screen that moves most across in the own game's: where its camera is)
    spans = [max(place(k, n)[0] for _, k, _, _ in own) - min(place(k, n)[0] for _, k, _, _ in own) for n in range(3)]
    layer = max(range(3), key=lambda n: spans[n])
    # Where both held their camera still (all three screens' places the same for a while): every one of the own game's
    # frames there, its pieces in every step they take, against the shown frames there
    held_own, held_shown = {}, {}
    for i, (_, key, _, _) in enumerate(own):
        held_own.setdefault(key, []).append(i)
    for i, (_, key, _, _) in enumerate(shown):
        held_shown.setdefault(key, []).append(i)
    held = [k for k in held_shown if len(held_shown[k]) >= HELD and len(held_own.get(k, [])) >= HELD]
    worst, worst_share, problems, lines = None, -1.0, [], []
    for key in held:
        mine = held_own[key]
        theirs = np.stack([own[i][2] for i in mine[::max(1, len(mine) // 160)]]).astype(np.int16)
        shares = []
        for i in held_shown[key][::max(1, len(held_shown[key]) // 40)]:
            rgb = shown[i][2]
            near = (np.abs(theirs - rgb.astype(np.int16)[None]).sum(axis=3) <= NEAR).any(axis=0)
            share = 1.0 - float(near.mean())
            shares.append(share)
            if share > worst_share:
                worst_share, worst = share, (i, key, rgb, near, own[mine[0]][2])
        shares.sort()
        typical = shares[len(shares) // 2]
        lines.append(f"held at {place(key, layer)}: {len(held_shown[key])} frames, differing {typical:.1%} typically, {shares[-1]:.1%} at most")
        if typical > MISSING_MOST:
            problems.append(f"missing or wrong at {place(key, layer)}: {typical:.1%} of the picture typically is not as its own game has it")
    if not held:
        problems.append("its camera never held where the own game's did")
    own_pairs, own_rate = moving(own)
    pairs, rate = moving(shown)
    lines.append(f"pieces moving: {rate:.2%} of the picture changes a frame (own game {own_rate:.2%})")
    if own_rate > 0.0005 and own_pairs >= 30 and pairs >= 30:
        if rate < own_rate * MOVING_LEAST:
            problems.append("its pieces are not moving as in its own game")
        elif rate > own_rate * MOVING_MOST:
            problems.append("its pieces move too fast")
    own_steps, shown_steps = steps(own, layer), steps(shown, layer)
    if own_steps and shown_steps:
        own_step = sorted(own_steps)[len(own_steps) // 2]
        step = sorted(shown_steps)[len(shown_steps) // 2]
        lines.append(f"camera: {step} a frame typically, {max(shown_steps)} at most (own game {own_step}, {max(own_steps)})")
        if step > own_step * STEP_MOST or max(shown_steps) > max(own_steps) * STEP_MOST + 8:
            problems.append("its camera goes too fast")
    shown_front = sum(1 for _, _, _, f in shown if f.any())
    lines.append(f"in front of the fighters in {shown_front} of {len(shown)} frames")
    if pictures and worst:
        n, key, rgb, near, theirs = worst
        mark = rgb.copy()
        mark[~near] = (255, 0, 0)
        write_png(pictures + "-worst.png", np.concatenate([theirs, rgb, mark], axis=1))
    return not problems, [f"{name}: {'ok' if not problems else 'FAIL: ' + '; '.join(problems)}"] + ["  " + line for line in lines]


def stage_names(program, args):
    out = subprocess.run([program] + args + ["--list-features"], capture_output=True, text=True).stdout
    for line in out.splitlines():
        fields = line.split("\t")
        if fields[0] == "viewer_stage" and len(fields) > 5:
            return fields[5].split("|")
    return []


# The stick, in the viewer: still (the camera where the round puts it), then left to the stage's end and still there, then
# right to its other end and still there (the viewer's camera goes 1.5 pixels a frame with the stick held)
MOVIE = """# cps2recomp input movie: frame player1 player2 (pressed bits, hex)
0 0000 0000
2400 0002 0000
3300 0000 0000
4100 0001 0000
5000 0000 0000
"""
FRAMES = 5800


def record(program, args, stage, movie, path):
    subprocess.run(["nice", "-n", "20", program] + args +
                   ["--headless", "--frames", str(FRAMES), "--movie", movie, "--features", f"free_play,viewer_stage={stage}",
                    "--keys", "60:cviewer_start", "--stage-look", path], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)


def sweep(argv):
    ap = argparse.ArgumentParser(prog="stage_look.py sweep")
    ap.add_argument("--host", required=True)
    ap.add_argument("--rom", required=True)
    ap.add_argument("--rom2", default="")
    ap.add_argument("--guest", action="append", default=[], help="ID=ROMS, as the game takes it")
    ap.add_argument("--only", action="append", default=[])
    ap.add_argument("--build", default="build")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--pictures", default="")
    args = ap.parse_args(argv)
    host = os.path.join(args.build, args.host, args.host)
    host_args = ["--rom", args.rom] + (["--rom2", args.rom2] if args.rom2 else [])
    for guest in args.guest:
        host_args += ["--guest", guest]
    shown_names = stage_names(host, host_args)
    work = []
    for guest in args.guest:
        gid, roms = guest.split("=", 1)
        if args.only and gid not in args.only:
            continue
        own = os.path.join(args.build, gid, gid)
        own_names = [n for n in stage_names(own, ["--rom", roms]) if ": " not in n and n != "Infinite"]
        # (its stages in the other game's list are "<its name>: <stage>", "The" there "the": the name the most of its stages
        # are found under)
        prefixes = {}
        for name in own_names:
            for n in shown_names:
                if n.lower().endswith(": " + name.lower()):
                    prefixes[n[:-len(name)]] = prefixes.get(n[:-len(name)], 0) + 1
        prefix = max(prefixes, key=prefixes.get) if prefixes else ""
        for i, name in enumerate(own_names):
            at = [j for j, n in enumerate(shown_names) if n.lower() == (prefix + name).lower()]
            if len(at) != 1:
                print(f"{gid} {name}: not found once in {args.host}'s stages")
                continue
            work.append((gid, own, ["--rom", roms], i, at[0], name))
    temp = tempfile.mkdtemp(prefix="stage_look")
    movie = os.path.join(temp, "stick.txt")
    with open(movie, "w") as f:
        f.write(MOVIE)
    failed = 0

    def one(item):
        gid, own, own_args, i, j, name = item
        a, b = os.path.join(temp, f"{gid}-{i}-own.slk"), os.path.join(temp, f"{gid}-{i}-shown.slk")
        record(own, own_args, i, movie, a)
        record(host, host_args, j, movie, b)
        pictures = os.path.join(args.pictures, f"{gid}-{i + 1:02d}") if args.pictures else ""
        result = compare(a, b, f"{gid} {i + 1} {name} in {args.host}", pictures)
        for path in (a, b):
            if os.path.exists(path):
                os.remove(path)
        return result

    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for ok, lines in pool.map(one, work):
            failed += not ok
            print("\n".join(lines), flush=True)
    os.remove(movie)
    os.rmdir(temp)
    print(f"{len(work) - failed} of {len(work)} stages ok")
    return 1 if failed else 0


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "sweep":
        return sweep(sys.argv[2:])
    ap = argparse.ArgumentParser(description="another game's stage against the same in its own game (--stage-look files)")
    ap.add_argument("own")
    ap.add_argument("shown")
    ap.add_argument("--name", default="")
    ap.add_argument("--pictures", default="")
    args = ap.parse_args()
    ok, lines = compare(args.own, args.shown, args.name or args.shown, args.pictures)
    print("\n".join(lines))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
