#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Checks the build with NO translated code (NO_GAME_CODE=1 in tools/release/lib.sh), which runs the ROM
# set's program in the port's interpreters: what CI builds, and what a checkout is before `make explore`.
# (A release holds the translated build, which `make check` and `check-blocks` cover.) Over the ladder movie,
# frame for frame:
#   1. two runs of it make the same pictures;
#   2. it makes the pictures the translated build makes with `--cpu interp` (the same program but for the
#      translated code: the same hooks, rewrites and features);
#   3. the translated build makes the pictures `--cpu blocks` makes (the interpreter, stopping where translated
#      code does): its translated code is the interpreter's, over the whole ladder.
# The interpreted build and the translated one are not compared: the interpreter takes interrupts between
# instructions, translated code between blocks, and in some games (Alpha, Alpha 2, Super Turbo) an interrupt a
# few instructions earlier changes how the game goes on (#190). Their count of differing frames is shown.
# The file of picture hashes it prints the checksum of is what a build for any other system must write too.
#
#   GAME=<id> check_release.sh [FRAMES]   (make GAME=<id> check-release: from the repository's top, after `make`; ROM=your set)
set -eu
GAME=${GAME:?which game: GAME=<id>}
ROM=${ROM:-rom/$GAME}
frames=${1:-40000}
# (the features the ladder is played with: the game's, or what lets player 1 win every match)
features=$(python3 tools/gamecfg.py release_features "$GAME")
features=${features:-free_play,no_damage,one_hit}
work=work/$GAME/check
build=build/$GAME-nocode
mkdir -p "$work" "$build/no-game-code/gen"
python3 tools/gamecfg.py --header "$build/no-game-code/gen/game_info.h" "$GAME"
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DGAME="$GAME" -DGAME_DIR="$PWD/games/$GAME" \
    -DWORK_DIR="$PWD/$build/no-game-code" > /dev/null
ninja -C "$build" "$GAME" > /dev/null
translated=build/$GAME/$GAME
run() { # name, program, port arguments
    name=$1 program=$2
    shift 2
    rm -f "$work/$name.txt" "$work/$name.stopped"
    "$program" --rom "$ROM" --headless --frames "$frames" --movie "games/$GAME/movies/ladder.txt" \
        --features "$features" --hash-picture --hash-log "$work/$name.txt" "$@" 2> "$work/$name.err" ||
        touch "$work/$name.stopped"
}
run release "$build/$GAME" & run release-again "$build/$GAME" & run interp "$translated" --cpu interp &
run translated "$translated" & run blocks "$translated" --cpu blocks & wait
status=0
for name in release release-again interp translated blocks; do
    if [ -e "$work/$name.stopped" ]; then
        echo "FAILED: the $name run stopped"; cat "$work/$name.err"; status=1
    fi
done
[ $status = 0 ] || exit 1
same() { # what, file, file, what it means
    if cmp -s "$2" "$3"; then
        echo "ok: $1 ($frames frames)"
    else
        echo "FAILED: $1 but in $(diff "$2" "$3" | grep -c '^<' || true) frames, the first: $(diff "$2" "$3" | grep -m1 '^<' | cut -c3- | cut -d' ' -f1)"
        status=1
    fi
}
same "two runs of the interpreted build alike" "$work/release.txt" "$work/release-again.txt"
same "the interpreted build's pictures are the translated build's run with --cpu interp" "$work/release.txt" "$work/interp.txt"
same "the translated build's pictures are --cpu blocks's" "$work/translated.txt" "$work/blocks.txt"
echo "(the interpreted build's pictures differ from the translated build's in $(diff "$work/release.txt" "$work/translated.txt" | grep -c '^<' || true) frames: interrupts between instructions, not blocks)"
if command -v sha256sum > /dev/null; then sum=sha256sum; else sum="shasum -a 256"; fi
echo "picture hashes: $($sum < "$work/release.txt" | cut -d' ' -f1)"
exit $status
