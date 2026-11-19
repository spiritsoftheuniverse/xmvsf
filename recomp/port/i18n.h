// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The toolkit's words in the player's language (recomp-common/i18n.h): its menu's rows, its features, commands and
// pages, its launcher's pages, the online play's lines. recomp-common's tables have the shared menu's and launcher's
// own; these the rest. A text neither has stays in English. The games' own words (on the board's screen) are never
// changed.
#pragma once

#include "../recomp-common/i18n.h"
#include "i18n_es.h"
#include "i18n_pt_br.h"

namespace port {

inline void AddTranslations() {
    static const bool added = [] {
        recomp::i18n::Add("pt-BR", kPortuguese);
        recomp::i18n::Add("es", kSpanish);
        return true;
    }();
    (void)added;
}

} // namespace port
