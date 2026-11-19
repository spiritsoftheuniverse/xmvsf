#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Where each stage's art ends, against where the wide picture's camera stops (tools/stage_ends.py says how). For every
# stage a training session is started (the game's movies/corners.txt up to its first walk), and the players are put in
# the left corner and the right corner in turn by "Start over", player 1 jumping now and then (for the lines above);
# once with the board's picture and once with widescreen. Run from the repository's top:
#
#   tools/stage_ends.sh GAME STAGES CAMERA [DIR]
#
# STAGES: how many; CAMERA: the camera's place across, a word of work RAM (hex, as the game's names have it), or one
# for each stage, with commas between (where the camera's record is the stage's own: the Marvel games). Prints
# tools/stage_ends.py's lines for each stage, and exits 1 if the wide picture went past a stage's art anywhere. The
# logs are left in DIR if given (else a folder that is removed). B, ROM, CORNERS (another game's corners movie, for a
# game built from its sources), START (the ticks the session's stage is chosen and it is started; 3,5), FEATURES (more
# features for both runs:
# widescreen_walls for invisible walls), LAYERS (the scroll screens looked at, as 1,2,3), REACH (tools/stage_ends.py's
# --reach), JOBS, ONLY (the stages to look at, as "3 7"); QUIET=1 says only the
# stages that went past their art.
GAME=$1
STAGES=$2
CAMERA=$3
port=${B:-build/$GAME}/$GAME
home=$(echo "$GAME" | tr a-z A-Z)_HOME
dir=${4:-$(mktemp -d)}
mkdir -p "$dir"
corners=${CORNERS:-games/$GAME/movies/corners.txt}
# (the session begins: the frame of the corners movie's first walk)
walk=$(awk '!/^#/ && ($2 == "0001" || $2 == "0002") { print $1; exit }' "$corners" 2>/dev/null)
if [ -z "$walk" ]; then
    echo "stage ends: no corners movie with a walk in it ($corners)"
    exit 1
fi
end=$((walk + 1800))
{
    awk -v walk=$walk '/^#/ || $1 < walk' $corners
    f=$walk
    while [ $f -lt $end ]; do
        echo "$f 0000 0000"
        echo "$((f + 50)) 0008 0000"
        echo "$((f + 54)) 0000 0000"
        f=$((f + 100))
    done
} > "$dir/movie.txt"
# (Start over at the left corner, then the right, three times each: the keys count the port's ticks, which run ahead of
# the frames while the title is skipped)
start=${START:-3,5}
keys="${start%,*}:fstage=STAGE,${start#*,}:ctraining_start"
for k in 0 400 800; do
    keys="$keys,$((walk + k)):frestart_at=1,$((walk + k + 1)):k1,$((walk + k + 200)):frestart_at=2,$((walk + k + 201)):k1"
done
jobs=${JOBS:-8}
n=0
stages=${ONLY:-$(seq 1 "$STAGES")}
for stage in $stages; do
    for run in board wide; do
        features=free_play${FEATURES:+,$FEATURES}
        [ $run = wide ] && features=$features,widescreen
        camera=$(echo "$CAMERA" | tr , '\n' | sed -n "${stage}p")
        [ -z "$camera" ] && camera=$CAMERA
        env $home="$dir/home_${run}_$stage" $port --rom "${ROM:-rom/$GAME}" --headless --frames $end --features $features \
            --movie "$dir/movie.txt" --keys "$(echo "$keys" | sed "s/STAGE/$stage/")" \
            --layer-log "$walk:$camera:$dir/${run}_$stage.bin" > /dev/null 2>&1 &
        n=$((n + 1))
        [ $((n % jobs)) -eq 0 ] && wait
    done
done
wait
status=0
past=""
for stage in $stages; do
    if [ ! -s "$dir/board_$stage.bin" ] || [ ! -s "$dir/wide_$stage.bin" ]; then
        status=1
        past="$past $stage"
        echo "stage $stage: no log (the run failed)"
    elif out=$(python3 tools/stage_ends.py "$dir/board_$stage.bin" "$dir/wide_$stage.bin" ${LAYERS:+--layers $LAYERS} ${REACH:+--reach $REACH}); then
        [ -z "$QUIET" ] && { echo "stage $stage:"; echo "$out" | sed 's/^/    /'; }
    else
        status=1
        past="$past $stage"
        echo "stage $stage:"
        echo "$out" | sed 's/^/    /'
    fi
done
mode="played in the wide picture"
case ",$FEATURES," in *,widescreen_walls,*) mode="with invisible walls" ;; esac
if [ $status -eq 0 ]; then
    echo "stage ends ($mode): nothing shown past the art on any of the $STAGES stages"
else
    echo "stage ends ($mode): shown past the art on stages$past"
fi
[ -z "$4" ] && rm -rf "$dir"
exit $status
