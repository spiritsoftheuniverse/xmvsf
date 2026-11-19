#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Checks the game's rewritten routines (src/) against the code they replace: every call of each, over the game's
# movies, run both ways and compared (docs/READABLE.md). The rewrites (the camera's place, keeping the players inside
# the picture: widescreen.cpp) do not call one another through the game's code, so each movie is run once. Run from the
# repository's top.
GAME=sfa
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
done
# The computer player (computer.cpp): the attract mode's fights (the computer against itself), and the endless
# battle with both sides the game's (CPS2RECOMP_BATTLE_CPU), every character on one side or the other, each with the
# level, the tier of its tactics and its plan held (FF84C7, FF861A, FF8605; player 2 FF88C7, FF8A1A, FF8A05), the
# gauge full for the super combos (FF84BE; FF88BE).
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
gauge="FF84BE=00,FF84BF=90,FF88BE=00,FF88BF=90"
for held in "FF84C7=00,FF88C7=00" "FF84C7=0C,FF88C7=0C,FF861A=20,FF8A1A=20,FF8605=02,FF8A05=04" \
    "FF84C7=1F,FF88C7=1F,FF861A=40,FF8A1A=40,FF8605=06,FF8A05=00"; do
    at_once check_computer "the attract mode" 30000 "$held,$gauge"
done
# (the endless battle's fighters by their number on its page: 1 Ryu to 13 Dan)
for pair in 1:2 3:4 5:6 7:8 9:10 11:12 13:1; do
    one=${pair%:*} two=${pair#*:}
    at_once check_computer "the endless battle, $one against $two" 30000 \
        "FF84C7=1F,FF88C7=14,FF861A=40,FF8A1A=20,FF8605=0$(( one % 4 * 2 )),FF8A05=0$(( two % 4 * 2 )),$gauge" \
        "--features free_play --keys 5:fendless_one=$one,5:fendless_two=$two,10:cendless_start"
done
at_once_wait || status=1
[ $status -eq 0 ] && echo "ok: every rewrite does what the game's code does" || echo "FAILED"
exit $status
