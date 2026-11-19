// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program
// (driver 1.05A) keeps a table of its songs at 0x9006, four bytes a song, as Marvel Super Heroes' does: the word at
// 0x9000 is how many (0x400), and a song whose data starts with a 0 is music.
#include "game.h"
#include "nwarr.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = nwarr::family::kSongCount;
    for (const auto &song : nwarr::family::kSongs) {
        TheGame().songs.push_back({song.number, song.name, song.title, song.once});
    }
}

} // namespace cps2recomp
