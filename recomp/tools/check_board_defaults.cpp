// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 sunkpoet-lang and contributors. Part of cps2recomp.
// ROM-free regression: classic selection must not change existing CPS-1 QSound DIP reads.
#include "machine/romset.h"
#include <iostream>
#include <stdexcept>

static void Require(bool value) {
    if (!value) throw std::runtime_error("board/default DIP regression");
}
int main() {
    auto q = cps2recomp::ParseRomList("", "", "", "", "", "test", "test", "cps1", "");
    Require(q.board.cps1 && q.board.qsound);
    for (auto value : q.board.dip) Require(value == 0xFF);
    auto c = cps2recomp::ParseRomList("", "", "", "", "", "test", "test", "cps1-classic", "");
    Require(c.board.cps1 && !c.board.qsound && c.board.programPairs == 0);
    auto cp = cps2recomp::ParseRomList("", "", "", "", "", "test", "test", "cps1-classic pairs=2", "");
    Require(cp.board.cps1 && !cp.board.qsound && cp.board.programPairs == 2);
    auto qp = cps2recomp::ParseRomList("", "", "", "", "", "test", "test", "cps1 pairs=4", "");
    Require(qp.board.cps1 && qp.board.qsound && qp.board.programPairs == 4);
    auto p = cps2recomp::ParseRomList("", "", "", "", "", "test", "test", "", "");
    Require(!p.board.cps1 && p.board.qsound);
    std::cout << "PASS: board selection, pairs=N and existing CPS-1 QSound FF DIP defaults\n";
}
