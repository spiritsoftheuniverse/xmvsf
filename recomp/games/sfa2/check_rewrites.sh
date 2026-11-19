#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Checks the game's rewritten routines (src/) against the code they replace: every call of each, over the
# game's movies, run both ways and compared (docs/READABLE.md). Inner rewrites are only called by
# outer ones, so each movie is run again with the outer ones left out. Run from the repository's top. A game made
# from Alpha 2's program that builds these sources too (Zero 2 Alpha) runs it with its own name: check_rewrites.sh GAME
GAME=${1:-sfa2}
port=${B:-build/$GAME}/$GAME
movies=games/sfa2/movies
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
for skip in "" $(sh games/sfa2/layers.sh); do
    run versus 3000 free_play "$skip"
    run versus_attacks 7800 free_play "$skip"
    run versus_attacks 7800 free_play,full_gauge,full_gauge_2 "$skip"
    run clash 3600 free_play "$skip"
    run air_guard 12700 free_play "$skip"
    run custom_combo 4200 free_play,full_gauge "$skip"
    run crouch 10800 free_play "$skip"
    run stun 11000 free_play "$skip"
    run throw_tech 5000 free_play "$skip"
    run versus_attacks 4000 free_play "$skip" "--poke FF8482=0A"
    run wall_jump 6000 free_play "$skip" "--poke FF8482=04"
    # (Rose, Guy, Charlie: their hits' reactions; and the characters a game made from this program adds, which its
    # check names in CHARACTERS)
    for who in 09 07 03 $CHARACTERS; do
        run versus_attacks 7800 free_play,full_gauge "$skip" "--poke FF8482=$who"
    done
    run ladder 30000 free_play "$skip"
done
# The computer player (computer.cpp): the movies a computer plays in, and the attract mode's fights (the computer
# against itself) with its level held low, middling and high, and each tier of its tactics. The players' routines
# are left out (layers.sh's last line), so that the game's own code calls the computer's (PlayerInControl calls
# ComputerThinks, the action routines Chance and BrokenOff).
deep=$(sh games/sfa2/layers.sh | tail -1)
computers="ComputerThinks|AnswerNow|BrokenOff|Reactions|Chance"
# (in the tag style and the ring out the computer plays the modes' tactics of the port's, not the game's alone: the
# movies of those modes are checked with no side playing them, CPS2RECOMP_COACH=0; as each movie says, the battle's
# numbers at random from seed 1)
export CPS2RECOMP_COACH=0 CPS2RECOMP_BATTLE_SEED=1
check_attract() { # frames, bytes held (--poke), rewrites to leave out
    out=$(CPS2RECOMP_SKIP_REWRITES="$3" $port --rom "${ROM:-rom/$GAME}" --headless --frames "$1" ${2:+--poke $2} --check-rewrites 2>&1)
    failed=$?
    echo "== the attract mode${2:+ ($2)}${3:+ (without the outer ones)}"
    echo "$out" | grep -E "^rewrite|did not" | grep -v " 0 calls"
    echo "$out" | grep -E "differs|calls, [1-9][0-9]* differed" > /dev/null && return 1
    return $failed
}
check_mode() { # as check: a movie of a mode, whose own changes to the round's rules differ from the game's, as they are
    # meant to (only the computer's rewrites are looked at)
    out=$(check "$@")
    echo "$out" | grep -E "^==|did not" ; out=$(echo "$out" | grep -E "^rewrite ($computers)( |$)")
    echo "$out"
    echo "$out" | grep -E "differs|calls, [1-9][0-9]* differed" >/dev/null && return 1
    echo "$out" | grep -q "^rewrite ComputerThinks" || return 1
    return 0
}
at_once check_mode team_tag_computer 8000 free_play,team_style=0,team_size=1,team_opponent=0,stage=4 "$deep" "--keys 60:cteam_start"
at_once check_mode ringout_computer 8000 free_play "$deep" "--keys 60:cringout_start"
# (FF84A7, FF88A7: each player's level; FF861A, FF8A1A: their tiers)
for held in "FF84A7=00,FF88A7=00" "FF84A7=0C,FF88A7=0C,FF861A=20,FF8A1A=20" "FF84A7=1F,FF88A7=1F,FF861A=40,FF8A1A=40"; do
    at_once check_attract 30000 "$held" "$deep"
done
unset CPS2RECOMP_COACH CPS2RECOMP_BATTLE_SEED
# A training session (player 1 sweeps; the dummy's quick rise): the training mode's own lines act inside the round's
# clock and the damage (and what encloses them), so those are left out, and the players' routines are checked
run training_rise 5500 free_play,dummy_getup=1 "RoundFrame,PlayersStep,PlayerFighting,RoundFighting,AllHitTests,PlayersAgainstPlayers,HitTest,TakeDamage" "--keys 3:fstage=1,5:ctraining_start"
at_once_wait || status=1
[ $status -eq 0 ] && echo "ok: every rewrite does what $GAME's code does" || echo "FAILED"
exit $status
