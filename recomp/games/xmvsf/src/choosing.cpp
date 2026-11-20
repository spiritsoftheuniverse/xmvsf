// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: the hidden character, Akuma, chosen with no code (the Game page's "Hidden characters"):
// Start held as a player chooses Ryu gives Akuma. The choice is the game's own (a button pressed on the cursor's
// character takes it, 00B1E8 on: the game itself gives Alpha Chun-Li for Chun-Li chosen with Start held, 00B1DA), with
// the character changed as the game takes it.
#include "xmvsf.h"
#include "session.h"
#include "alternative.h"
#include "../../../port/xmvsf_m68k_capture.h" // native accept-hook event, not inferred selection

namespace cps2recomp {

namespace {
    using namespace xmvsf;

    constexpr uint16_t kRyu = 0x12, kAkuma = 0x1E;
    constexpr uint32_t kCursor = 0x0C; // word of the choosing record: the character under the player's cursor
    bool g_start[2] = {false, false};  // each player's Start held this frame

    // 00B1E8: a player's choosing record (A4) takes the character under its cursor
    bool Chosen(M68K &c) {
        const uint32_t record = c.a[4] & 0xFFFFFF;
        if (record != kChoosing && record != kChoosing + kChoosingSize) {
            return false;
        }
        const int n = record == kChoosing ? 0 : 1;
        // Real M68K hook entry, before any P1/P2 selection changes.
        // This does not assert that a character was actually committed.
        xmvsf_f10::Capture::Get().AcceptHook(
            c, n, record, Ram8(record), Ram16(record + kCursor));
        if (FeatureOn("hidden_characters") && g_start[n] && Ram16(record + kCursor) == kRyu) {
            SetRam16(record + kCursor, kAkuma);
        }
        // The actual ROM routine accepted this P1/P2 character. Training
        // switches controller ownership after the second confirmed P1 pick.
        session::CharacterAccepted(n);
        return false;
    }
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    // Preserve the native P1 selector. Suppress only P2's original
    // palette-27 pen 10-12 tiles while Training chooses both teams.
    // This is a renderer setting, not selection RAM or a guessed rectangle.
    machine.video.hideP2Selector = session::InSession() && modes::Choosing();
    for (int n = 0; n < 2; n++) {
        g_start[n] = (machine.players[n] & kStart) != 0;
    }
}

void RegisterChoosing() {
    AddHook(HookAt(0x00B1E8), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden character with no code: hold Start as you choose Ryu for Akuma. (Start held as you "
                                  "choose Chun-Li gives her Street Fighter Alpha look, as on the board.)"});
}

} // namespace cps2recomp
