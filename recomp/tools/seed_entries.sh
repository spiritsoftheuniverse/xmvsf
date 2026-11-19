#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Gives a fresh tree the code another tree found (its work/<id>/entries.txt and entries.txt.z80), added to what
# this tree has, so that the next 'make' translates from those without a make explore. The places are in the ROM
# set's program, which no change here moves; a later explore only adds to them. (The repository tracks each game's
# lists too, games/<id>/entries.txt and entries.txt.z80, which every build adds to work/'s; this is for finds
# another tree has not committed yet. 'make GAME=<id> explore' or 'record' then adds them to the tracked lists.)
#
#   tools/seed_entries.sh OTHER_TREE [game ...]   (every game the other tree has found code for, if none named)
set -eu
from=${1:?usage: seed_entries.sh OTHER_TREE [game ...]}
shift
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
[ $# -gt 0 ] || set -- $(cd "$from/work" 2>/dev/null && for f in */entries.txt; do [ -f "$f" ] && printf '%s ' "${f%/entries.txt}"; done)
for game in "$@"; do
    mkdir -p "work/$game"
    tools/merge_entries.sh "work/$game/entries.txt" "$from/work/$game/entries.txt"
    tools/merge_entries.sh "work/$game/entries.txt.z80" "$from/work/$game/entries.txt.z80"
done
