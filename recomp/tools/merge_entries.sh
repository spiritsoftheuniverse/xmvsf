#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Adds entries files to one: every address in any of them, sorted, once each, under the one comment line the
# port writes (port/main.cpp). The file is written only when it gains an address, so that its time says when it
# last grew (build.mk remakes a graft's map from it). Files that are not there are passed over.
#
#   merge_entries.sh FILE OTHER_FILE ...      (says how many FILE had and has, when it grew; -q: not)
quiet=; [ "${1:-}" = -q ] && { quiet=1; shift; }
main=${1:?usage: merge_entries.sh [-q] FILE OTHER_FILE ...}
shift
before=$(cat "$main" 2>/dev/null | grep -v '^#' | grep -c .)
cat "$main" "$@" 2>/dev/null | grep -v '^#' | grep . | sort -u > "$main.new"
after=$(wc -l < "$main.new" | tr -d ' ')
if [ "$after" -gt "$before" ] || [ ! -f "$main" ] && [ "$after" -gt 0 ]; then
    { echo "# Places the game's code was seen to reach in ways the translator cannot follow"; cat "$main.new"; } > "$main"
    [ -n "$quiet" ] || echo "$main: $before -> $after"
fi
rm -f "$main.new"
