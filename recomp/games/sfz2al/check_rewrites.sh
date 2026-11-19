#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy
#
# Checks Alpha 2's rewritten routines (games/sfa2/src), which this game builds too, against this game's own code:
# Alpha 2's check, its movies played on this game (games/sfa2/check_rewrites.sh). Run from the repository's top.
# (with this game's other forms too: the classic Chun-Li, the EX forms, Evil Ryu, and row 28's Chun-Li, which its
# code tests by number)
CHARACTERS="12 15 16 17 18 19 1A 1B 1C" exec sh games/sfa2/check_rewrites.sh sfz2al
