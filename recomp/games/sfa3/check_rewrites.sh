#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Checks the game's rewritten routines (src/) against the code they replace: every call of each, over the game's
# movies, run both ways and compared (docs/READABLE.md). The rewrites (the camera's place, keeping the players inside
# the picture: widescreen.cpp) do not call one another through the game's code, so each movie is run once (and again
# without the camera's place across, as Alpha 1's script does). Run from the repository's top.
GAME=sfa3
port=${B:-build/$GAME}/$GAME
movies=games/$GAME/movies
status=0
. tools/at-once.sh
check() { # movie, frames, features, rewrites to leave out, more of the port's options
    out=$(CPS2RECOMP_SKIP_REWRITES="$4" $port --rom "${ROM:-rom/$GAME}" --headless --frames "$2" --movie "$movies/$1.txt" --features "$3" $5 --check-rewrites 2>&1)
    failed=$?
    echo "== $1${4:+ (without $4)}${5:+ ($5)}"
    echo "$out" | grep -E "^rewrite|did not" | grep -v " 0 calls"
    echo "$out" | grep -E "differs|calls, [1-9][0-9]* differed" > /dev/null && return 1
    return $failed
}
run() {
    at_once check "$@"
}
for skip in "" "CameraAcross"; do
    run versus 12000 free_play "$skip"
    run ladder 40000 free_play,no_damage,one_hit "$skip"
    # (two players, made up: they walk into each other, jump and throw)
    run versus 20000 free_play "$skip" "--auto 7:1400"
    run versus 20000 free_play "$skip" "--auto 13:1400"
    # (training: a string of attacks, and throws in the corner: the players kept in together)
    run training_attacks 4400 free_play "$skip" "--keys 5:ctraining_start"
    run throws 3200 free_play "$skip" "--keys 5:ctraining_start"
done
# The computer player (computer.cpp): the attract mode's fights (the computer against itself), and the endless
# battle with both sides the game's (CPS2RECOMP_BATTLE_CPU), each with the level, the tier of its tactics and the ism
# held (FF8527, FF8613, FF8532; player 2 FF8927, FF8A13, FF8932: 0 the A-ism, FF the X-ism, 1 the V-ism), the gauge
# full for the isms' super combos. In the tag style and the ring out the computer plays the modes' tactics of the
# port's, not the game's alone: those are measured by play (PROGRESS.md), not checked here, and nobody plays them in
# these (CPS2RECOMP_COACH=0).
computers="ComputerThinks|AnswerNow|BrokenOff|Reactions|Chance"
check_computer() { # name, frames, bytes held (--poke), more of the port's options
    # (only the computer's rewrites are looked at: the endless battle's camera differs from the game's, as it is meant to)
    out=$(CPS2RECOMP_COACH=0 CPS2RECOMP_BATTLE_CPU=1 CPS2RECOMP_BATTLE_SEED=1 $port --rom "${ROM:-rom/$GAME}" --headless --frames "$2" --poke "$3" $4 --check-rewrites 2>&1)
    echo "== $1 ($3)"
    out=$(echo "$out" | grep -E "^rewrite ($computers)( |$)|did not")
    echo "$out"
    echo "$out" | grep -E "differs|calls, [1-9][0-9]* differed|did not" > /dev/null && return 1
    echo "$out" | grep -q "^rewrite ComputerThinks" || return 1
    return 0
}
for held in "FF8527=00,FF8927=00,FF8532=00,FF8932=00" "FF8527=0C,FF8927=0C,FF8613=20,FF8A13=20,FF8532=FF,FF8932=FF" \
    "FF8527=1F,FF8927=1F,FF8613=40,FF8A13=40,FF8532=01,FF8932=01"; do
    at_once check_computer "the attract mode" 30000 "$held"
    at_once check_computer "the endless battle" 30000 "$held,FF851E=00,FF851F=90,FF891E=00,FF891F=90" \
        "--features free_play --keys 5:fendless_one=0,5:fendless_two=0,10:cendless_start"
done
at_once_wait || status=1
[ $status -eq 0 ] && echo "ok: every rewrite does what the game's code does" || echo "FAILED"
exit $status
