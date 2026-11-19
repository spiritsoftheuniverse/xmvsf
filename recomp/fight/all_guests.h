// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Every game whose stages another can show (fight/guest_stages.h; each described in its own folder's guest.h), in one
// order: a game that shows them takes them all but its own, in this order, after its own stages. (The order keeps the
// numbers the stages already had: each game's guests until now come first.) A game's game.cfg names them too (its
// guests line), so that their sets are found.
#pragma once

#include "../games/cybots/guest.h"
#include "../games/dstlk/guest.h"
#include "../games/msh/guest.h"
#include "../games/mshvsf/guest.h"
#include "../games/mvsc/guest.h"
#include "../games/nwarr/guest.h"
#include "../games/pocket_fighter/guest.h"
#include "../games/sfa/guest.h"
#include "../games/sfa2/guest.h"
#include "../games/sfa3/guest.h"
#include "../games/ssf2t/guest.h"
#include "../games/vampire_savior/guest.h"
#include "../games/xmcota/guest.h"
#include "../games/xmvsf/guest.h"

#include <cstring>
#include <vector>

namespace cps2recomp::guests {

struct Entry {
    const char *id;
    guest_stages::Guest (*description)();
    const char *const *names;
    size_t stages;
};

template <typename Names> constexpr Entry Make(const char *id, guest_stages::Guest (*description)(), const Names &names) {
    return {id, description, names.data(), names.size()};
}

inline const std::vector<Entry> &All() {
    static const std::vector<Entry> all = {
        Make("dstlk", dstlk_guest::Description, dstlk_guest::kStageNames),
        Make("vampire_savior", vampire_savior_guest::Description, vampire_savior_guest::kStageNames),
        Make("sfa2", sfa2_guest::Description, sfa2_guest::kStageNames),
        Make("sfa3", sfa3_guest::Description, sfa3_guest::kStageNames),
        Make("ssf2t", ssf2t_guest::Description, ssf2t_guest::kStageNames),
        Make("sfa", sfa_guest::Description, sfa_guest::kStageNames),
        Make("nwarr", nwarr_guest::Description, nwarr_guest::kStageNames),
        Make("msh", msh_guest::Description, msh_guest::kStageNames),
        Make("xmvsf", xmvsf_guest::Description, xmvsf_guest::kStageNames),
        Make("mshvsf", mshvsf_guest::Description, mshvsf_guest::kStageNames),
        Make("mvsc", mvsc_guest::Description, mvsc_guest::kStageNames),
        Make("xmcota", xmcota_guest::Description, xmcota_guest::kStageNames),
        Make("pocket_fighter", pocket_fighter_guest::Description, pocket_fighter_guest::kStageNames),
        Make("cybots", cybots_guest::Description, cybots_guest::kStageNames),
    };
    return all;
}

// The guests of a game (all but the one whose stages it has: its own id, or the game it is made from)
inline std::vector<guest_stages::Guest> AllBut(const char *own) {
    std::vector<guest_stages::Guest> out;
    for (const Entry &e : All()) {
        if (std::strcmp(e.id, own) != 0) {
            out.push_back(e.description());
        }
    }
    return out;
}
// ... and their stages' names, in the same order
inline std::vector<const char *> NamesBut(const char *own) {
    std::vector<const char *> out;
    for (const Entry &e : All()) {
        if (std::strcmp(e.id, own) != 0) {
            out.insert(out.end(), e.names, e.names + e.stages);
        }
    }
    return out;
}

} // namespace cps2recomp::guests
