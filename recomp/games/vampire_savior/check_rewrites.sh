#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Checks the game's rewritten routines (src/) against the code they replace: every call of each, over the
# game's movies, run both ways and compared (docs/READABLE.md). Inner rewrites are only called by
# outer ones, so each movie is run again with the outer ones left out. Run from the repository's top.
GAME=vampire_savior
port=${B:-build/$GAME}/$GAME
movies=games/$GAME/movies
status=0
. tools/at-once.sh # (the runs at once, a few at a time; what each wrote printed in order)
check() { # movie, frames, features, rewrites to leave out, more of the port's options (the movie's own)
    out=$(CPS2RECOMP_SKIP_REWRITES="$4" $port --rom "${ROM:-rom/$GAME}" --headless --frames "$2" --movie "$movies/$1.txt" --features "$3" $5 --check-rewrites 2>&1)
    failed=$?
    echo "== $1${4:+ (without $4)}"
    echo "$out" | grep -E "^rewrite|did not" | grep -v " 0 calls"
    return $failed
}
run() {
    at_once check "$@"
}
inPicture="DrawnIfInWide,DrawnIfInPicture,ThrownGone,PieceByCamera,KeepIn,KeepBothIn,AllPieces"
scripts="PiecePlain,PieceFlickering,PieceDancers,PieceLady,PieceThrower"
for skip in "" "AllHitTests,CameraAcross,CameraUp,$inPicture" "$inPicture,Drawn,$scripts" "AllHitTests,PlayersAgainstPlayers,ThrownAgainstPlayers,GuardRange" \
    "AllHitTests,PlayersAgainstPlayers,ThrownAgainstPlayers,HitTest,AttackAgainst"; do
    run versus_normals 3000 free_play "$skip"
    run versus_attacks 2600 free_play "$skip"
    run versus 6000 "" "$skip"
    run ladder 12000 free_play "$skip"
done
# The computer player (computer.cpp): the movies a computer plays in, and the attract mode's fights (the computer
# against itself) with its level held low, middling and high, and each tier of its tactics. Then with the outer
# routines left out, for the ones they use.
computer="ComputerPlays,AnswerNow,BrokenOff"
# (in the tag style and the ring out the computer plays the modes' tactics of the port's, not the game's alone: the
# movies of those modes are checked with no side playing them, CPS2RECOMP_COACH=0; as each movie says, the battle's
# numbers at random from seed 1)
export CPS2RECOMP_COACH=0 CPS2RECOMP_BATTLE_SEED=1
check_attract() { # frames, bytes held (--poke), rewrites to leave out
    out=$(CPS2RECOMP_SKIP_REWRITES="$3" $port --rom "${ROM:-rom/$GAME}" --headless --frames "$1" ${2:+--poke $2} --check-rewrites 2>&1)
    failed=$?
    echo "== the attract mode${2:+ ($2)}${3:+ (without $3)}"
    echo "$out" | grep -E "^rewrite|did not" | grep -v " 0 calls"
    return $failed
}
computers="ComputerPlays|AnswerNow|BrokenOff|Reactions|UnpackMove|UnpackChainMove|Chance|AttackComing|JumpMoveNow|SecondMoveNow|GuardPose|SetGuardsWithMove|AtEdgeJump|GuardAnyway|NextChainMove"
check_mode() { # as check: a movie of a mode, whose own changes to the round's rules differ from the game's, as they are
    # meant to (only the computer's rewrites are looked at)
    out=$(check "$@")
    echo "$out" | grep -E "^==|did not" ; out=$(echo "$out" | grep -E "^rewrite ($computers)( |$)")
    echo "$out"
    echo "$out" | grep -E "differs|calls, [1-9][0-9]* differed" >/dev/null && return 1
    echo "$out" | grep -q "^rewrite ComputerPlays\|^rewrite Chance" || return 1
    return 0
}
for skip in "" "$computer"; do
    at_once check_mode team_tag_computer 8000 free_play,team_style=0,team_size=1,team_opponent=0,stage=4 "$skip" "--keys 60:cteam_start"
    at_once check_mode ringout_computer 8000 free_play "$skip" "--keys 60:cringout_start"
    # (FF87B3, FF8BB3: each player's level; FF860A, FF8A0A: their tiers, 40 in the attract mode's own fights)
    for held in "FF87B3=00,FF8BB3=00" "FF87B3=0C,FF8BB3=0C,FF860A=20,FF8A0A=20" "FF87B3=1F,FF8BB3=1F,FF860A=00,FF8A0A=00"; do
        at_once check_attract 40000 "$held" "$skip"
    done
done
run ladder 12000 free_play "$computer"
# The stage's code (camera.cpp, planes.cpp, rows.cpp, picture.cpp, edges.cpp, pieces.cpp): each stage has its own choice of those routines, so every stage
# is visited: a training session on it, the players moved by made-up controls (no movie with them: a movie's
# controls would be the only ones, and the camera has to go from end to end). The outer rewrites, then
# with them left out the ones they use.
check_stage() { # stage (1-12), rewrites to leave out, more of the port's options
    out=$(CPS2RECOMP_SKIP_REWRITES="$2" $port --rom "${ROM:-rom/$GAME}" --headless --frames 6000 --auto 7:1500 \
        --keys "5:fstage=$1,10:ctraining_start" $3 --check-rewrites 2>&1)
    echo "== stage $1${2:+ (without the outer ones)}${3:+ ($3)}"
    # (training's own changes to the round's rules differ from the game's, as they are meant to: only the
    # stage's rewrites are looked at)
    out=$(echo "$out" | grep -E "^rewrite|did not" | grep -E "Camera|Players(Across|Up)|Back8|Far32|Rows|Drawn|Piece|Script[0-9]|Plane(Of|AndRow)|PlaceOf|Animat|GiveBack|ThrownGone|KeepIn|KeepBothIn|did not" | grep -v " 0 calls")
    echo "$out"
    echo "$out" | grep -q "CameraPlaced" || return 1
    echo "$out" | grep -E "differs|did not|calls, [1-9][0-9]* differed" >/dev/null && return 1
    return 0
}
stage() {
    at_once check_stage "$@"
}
outer=$(grep -oE '"(Back8|Far32)(Across|Up)[A-Za-z0-9]*"' games/$GAME/src/planes.cpp | tr -d '"' | paste -sd, -)
for n in 1 2 3 4 5 6 7 8 9 10 11 12; do
    stage $n ""
    stage $n "CameraAcross,CameraUp,$outer,$inPicture"
    stage $n "CameraAcross,CameraUp,$outer,$inPicture,$scripts,Drawn,SetStageAnimation"
    # (many pieces' scripts do something else once a round is decided, which a training session's never
    # is: the game's flag for it held on, for the game's code and the rewrites alike)
    stage $n "CameraAcross,CameraUp,$outer,$inPicture" "--poke FF8107=01"
done
at_once_wait || status=1
[ $status -eq 0 ] && echo "ok: every rewrite does what the game's code does" || echo "FAILED"
exit $status
