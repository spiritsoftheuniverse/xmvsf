#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Where a stage's art ends, on each scroll screen, and how much nearer the middle a wide picture's camera must stop so
# that the picture never shows past it. Reads two --layer-log files of the same stage (port/main.cpp), each with the
# players put in the left corner and in the right corner in turn (training's "Start over"): one of the board's picture,
# one of the wide picture played in (the camera stopping `margin` short of each end).
#
# "The art" of a scroll screen is what the board itself shows of it: on each line of its map, the columns from its place
# with the camera at its left end to its place at the right end and the 384 beyond. Past that the map holds either
# nothing (a gap) or art the board never shows. A scroll screen that moves slower than the camera (a far background:
# Street Fighter Alpha's third goes at half its speed) is where the wide picture first goes past it, so the camera must
# stop further in for it: by what it went past, over its speed.
#
#   tools/stage_ends.py BOARD WIDE [--from FRAME] [--layers 2,3] [--reach MOST:2]
#
# --reach: the camera stops at most MOST further in than the wide picture's margin for the scroll screens named (the
# stage's own plane): their lines that would need more (the far edge of a perspective floor, near its horizon) are not
# looked at.
# Only what is seen counts: a screen behind another's painted pixels may go past its art unseen. Lines of a screen
# that the board shows less of than the wide picture's width (a far plane that barely moves) are left out: no camera
# can keep the wide picture inside those.
#
# Prints, for each scroll screen, how far the wide picture went past its art at each end, and the camera's ends that
# would keep it in; the last line is "ends LEFT RIGHT" (how much nearer the middle than the board's, at each end, for
# all the screens), and the exit status is 1 if the wide picture went past the art at all.
import collections
import math
import struct
import sys

H = 224                       # lines of the picture
RECORD = 8 + 3 * H * 12       # a frame: number, word, width, then Video::LayerLine for each scroll screen's lines
MAP = [511, 1023, 2047]       # the maps' heights, less 1 (8, 16 and 32-pixel tiles)
SETTLED = 4                   # frames the camera stays put before a frame is counted (the picture is a frame or two behind)


def frames(path):
    data = open(path, 'rb').read()
    for at in range(0, len(data) - RECORD + 1, RECORD):
        frame, camera, width = struct.unpack_from('<IHH', data, at)
        lines = [[struct.unpack_from('<iihh', data, at + 8 + (w * H + y) * 12) for y in range(H)] for w in range(3)]
        yield frame, camera - 65536 if camera >= 32768 else camera, width, lines


class Corners:
    """A log's two corners: per scroll screen and line of its map, the screen's place at the picture's left edge with
    the camera at its left end and at its right end, whether it is seen anywhere on the line, and its first column seen
    at the left end and its last at the right (each as a place and how far from it)"""

    def __init__(self, path, first):
        kept = [(f, c, w, l) for f, c, w, l in frames(path) if f >= first]
        if not kept:
            sys.exit('%s: no frames from %d' % (path, first))
        # (before the fight the camera's word holds what it held: from its first change on)
        moved = next((n for n in range(1, len(kept)) if kept[n][1] != kept[0][1]), 0)
        kept = kept[moved:]
        self.width = kept[-1][2]
        self.ends = (min(c for _, c, _, _ in kept), max(c for _, c, _, _ in kept))
        self.place = [[collections.defaultdict(set), collections.defaultdict(set)] for _ in range(3)]
        self.seen = [[collections.defaultdict(set), collections.defaultdict(set)] for _ in range(3)]
        still = 0
        for n, (frame, camera, width, lines) in enumerate(kept):
            still = still + 1 if n and kept[n - 1][1] == camera else 0
            if still < SETTLED or camera not in self.ends:
                continue
            end = self.ends.index(camera)
            for w in range(3):
                for row, left, first_seen, last_seen in lines[w]:
                    if row < 0:
                        continue
                    self.place[w][end][row & MAP[w]].add(left)
                    if first_seen >= 0:
                        self.seen[w][end][row & MAP[w]].add((left, first_seen if end == 0 else last_seen))


def near(value, to, wide):
    """value, a place in a map `wide` across that repeats, taken as the one nearest `to`"""
    return value + round((to - value) / wide) * wide


def main():
    args = sys.argv[1:]
    first, layers = 0, (1, 2, 3)
    if '--from' in args:
        i = args.index('--from')
        first = int(args[i + 1])
        del args[i:i + 2]
    reach, reached = 10**6, ()
    if '--reach' in args:
        i = args.index('--reach')
        most, which = args[i + 1].split(':')
        reach, reached = int(most), tuple(int(x) for x in which.split(','))
        del args[i:i + 2]
    if '--layers' in args:
        i = args.index('--layers')
        layers = tuple(int(x) for x in args[i + 1].split(','))
        del args[i:i + 2]
    board, wide = Corners(args[0], first), Corners(args[1], first)
    side = (wide.width - board.width) // 2
    span = board.ends[1] - board.ends[0]
    # (how much further in than the margin the wide picture's camera stops already, at each end: --reach counts from
    # the margin)
    further = (wide.ends[0] - board.ends[0] - side, board.ends[1] - wide.ends[1] - side)
    print('camera: the board %d-%d, the wide picture %d-%d (%d columns more on each side)' % (
        board.ends[0], board.ends[1], wide.ends[0], wide.ends[1], side))
    past_any = False
    need = [0, 0]
    for w in range(3):
        if w + 1 not in layers:
            continue
        across = (MAP[w] + 1)  # (the map is as wide as it is high)
        rows = [r for r in board.seen[w][0] if board.place[w][0][r] and board.place[w][1][r]
                and wide.place[w][0][r] and wide.place[w][1][r]]
        if not rows or span <= 0:
            continue
        past, trims, speeds, places = [0, 0], [0, 0], collections.Counter(), collections.Counter()
        narrow = moving_alone = still_lines = 0
        beyond = set()
        for r in rows:
            ref = min(board.place[w][0][r]) % across
            b0 = min(near(v, ref, across) for v in board.place[w][0][r])
            # (a plane goes the camera's way, if at all: its place at the right end is taken as at or after the left's)
            b1 = max(b0 + (v - b0) % across for v in board.place[w][1][r])
            spread = max(max(board.place[w][end][r]) - min(board.place[w][end][r]) for end in range(2))
            if spread > 2:
                # (it moves while the camera stays put, on its own: clouds, a picture that goes round. Its art is
                # wherever it has gone: not looked at)
                moving_alone += 1
                continue
            if b1 == b0:
                # (a still line among moving ones: no camera can keep the picture inside it)
                still_lines += 1
                speeds[0] += 1
                continue
            if b0 < b1 and b1 - b0 + board.width < wide.width:
                # (the board shows less of it than the wide picture is wide: nothing can keep the picture inside it,
                # neither a camera nor a move; the hole is the game's to fill: Video::wide.fillHoles)
                narrow += 1
                speeds[round((b1 - b0) / span, 3)] += 1
                places[(b0, b1)] += 1
                continue
            # (what the wide picture shows of it, where nothing in front hides it)
            seen0 = [near(v, b0, across) + at for v, at in wide.seen[w][0][r]]
            seen1 = [near(v, b1, across) + at for v, at in wide.seen[w][1][r]]
            over = [b0 - min(seen0) if seen0 else 0, max(seen1) - (b1 + board.width - 1) if seen1 else 0]
            # (its speed against the camera's, from the board's two ends)
            speed = (b1 - b0) / span
            speeds[round(speed, 3)] += 1
            places[(b0, b1)] += 1
            for end in range(2):
                if over[end] > 0:
                    trim = math.ceil(over[end] / speed) if speed > 0.01 else 10**6
                    if w + 1 in reached and further[end] + trim > reach:
                        # (further in than the camera is let stop for it: --reach)
                        beyond.add(r)
                        continue
                    past[end] = max(past[end], over[end])
                    trims[end] = max(trims[end], trim)
        if not speeds:
            print('scroll %d: every line moves by itself: not looked at' % (w + 1))
            continue
        if set(speeds) == {0}:
            print('scroll %d: still (it does not move with the camera): not looked at' % (w + 1))
            continue
        print('scroll %d: %d lines of art%s%s; past it at the left end by %d, the right by %d; the camera %s' % (
            w + 1, len(rows), ' (%d of them narrower than the wide picture: not looked at)' % narrow if narrow else '',
            (' (%d of them moving by themselves: not looked at)' % moving_alone if moving_alone else '') +
            (' (%d of them beyond the camera\'s reach of %d: not looked at)' % (len(beyond), reach) if beyond else '') +
            (' (%d of them still: not looked at)' % still_lines if still_lines else ''),
            past[0], past[1],
            'stops in time' if not any(past) else 'must stop %s and %s nearer the middle' % (
                'never' if trims[0] >= 10**6 else trims[0], 'never' if trims[1] >= 10**6 else trims[1])))
        print('    its speeds against the camera (lines): %s; its places at the board\'s ends: %s' % (
            ', '.join('%g (%d)' % kv for kv in speeds.most_common(4)),
            ', '.join('%d-%d (%d)' % (a, b, n) for (a, b), n in places.most_common(3))))
        past_any = past_any or any(past)
        need = [max(need[0], trims[0]), max(need[1], trims[1])]
    margin = (wide.ends[0] - board.ends[0], board.ends[1] - wide.ends[1])
    print('ends %d %d' % (margin[0] + need[0], margin[1] + need[1]))
    return 1 if past_any else 0


if __name__ == '__main__':
    sys.exit(main())
