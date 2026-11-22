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
#include "../../../port/xmvsf_character_select_layout.h" // fixed 18-slot visual/ROM map
#include "../../../port/xmvsf_character_select_rewrite.h" // in-frame native bus selection writes
#include <array>
#include <cstdio>

namespace cps2recomp {

namespace {
    using namespace xmvsf;

    constexpr uint16_t kRyu = 0x12, kAkuma = 0x1E;
    constexpr uint32_t kCursor = 0x0C; // word of the choosing record: the character under the player's cursor
    bool g_start[2] = {false, false};  // each player's Start held this frame
    port::xmvsf_select::Navigation g_layout;
    bool g_layoutRunning = false;
    uint8_t g_accepted[2] = {0, 0}; // real FF5183 / FF51A3 native accepted counts
    bool g_secondCandidateReady[2] = {false, false}; // advance once at native phase 3
    uint16_t g_committed[2][2] = {{0xFFFFu, 0xFFFFu}, {0xFFFFu, 0xFFFFu}};
    bool g_commitPending = false; // one native match handoff, not every arcade round

    void EnterSecondCandidate(unsigned n) {
        if (n >= 2u || g_secondCandidateReady[n]) return;
        const int prior = g_layout.index[n];
        g_layout.index[n] = port::xmvsf_select::Navigate(prior, 1u);
        g_secondCandidateReady[n] = true;
        std::fprintf(stderr,
            "XMVSF 87.501 SECOND_READY P%u: grid=%d->%d id=%04X\n",
            n + 1u, prior, g_layout.index[n],
            port::xmvsf_select::kSlots[
                static_cast<unsigned>(g_layout.index[n])].rom);
    }

    uint16_t IntendedNativeId(unsigned n) {
        const uint16_t raw =
            port::xmvsf_select::kSlots[static_cast<unsigned>(g_layout.index[n])].rom;
        // The original board's Start variations are applied at acceptance.
        // Preserve them even when the native pick write is remapped.
        if (g_start[n]) {
            if (raw == kRyu && FeatureOn("hidden_characters")) return kAkuma;
            if (raw == 0x16u) return 0x24u; // Chun-Li -> Alpha Chun-Li
        }
        return raw;
    }

    void RemapSelectionWrite(Machine &, uint32_t address,
                             uint16_t &value, unsigned bytes) {
        // Follow the NATIVE phase-03 entry, not the first accepted-count
        // increment (which can precede the end of the first voice/flash).
        // Do it at the bus write, before the ROM's second-candidate sound
        // routine reads the cursor; AfterFrame is only a fallback.
        const uint32_t at = address & 0xFFFFFFu;
        for (unsigned n = 0; n < 2u; ++n) {
            const uint32_t record = kChoosing + n * kChoosingSize;
            const bool phaseWrite = at == record &&
                ((bytes == 1u && value == 3u) ||
                 (bytes == 2u && (value >> 8u) == 3u));
            if (phaseWrite && Ram8(record + 3u) == 1u)
                EnterSecondCandidate(n);
        }
        const std::array<uint16_t, 2> selected{{
            IntendedNativeId(0u), IntendedNativeId(1u)
        }};
        const std::array<uint8_t, 2> accepted{{g_accepted[0], g_accepted[1]}};
        value = port::xmvsf_select::RewriteNativeSelectionWrite(
            address, value, bytes, selected, accepted);

        // FF5183 / FF51A3 are the ROM's actual accepted-character
        // counters. Capture the ID AT THE RAM WRITE: the ROM can leave
        // character selection on the same frame as pick two, meaning a
        // post-frame check may otherwise never see the second accept.
        for (unsigned n = 0; n < 2u; ++n) {
            const uint32_t counter = 0xFF5183u + n * 0x20u;
            const bool wroteCounter = (bytes == 1u && at == counter) ||
                                      (bytes == 2u && at == counter - 1u);
            if (!wroteCounter) continue;
            const uint8_t count = static_cast<uint8_t>(value & 0xFFu);
            if (count > g_accepted[n] && count <= 2u) {
                const unsigned pick = count - 1u;
                g_committed[n][pick] = IntendedNativeId(n);
                if (count == 2u) g_commitPending = true;
                std::fprintf(stderr,
                    "XMVSF 87.500 BUS_ACCEPT P%u PICK%u: id=%04X count=%u\n",
                    n + 1u, pick + 1u, g_committed[n][pick], count);
            }
        }
    }

    // 00B1E8: a player's choosing record (A4) takes the character under its cursor
    bool Chosen(M68K &c) {
        const uint32_t record = c.a[4] & 0xFFFFFF;
        if (record != kChoosing && record != kChoosing + kChoosingSize) {
            return false;
        }
        const int n = record == kChoosing ? 0 : 1;
        // Real M68K hook entry, before any P1/P2 selection changes.
        // The custom cursor's ID must be in the ORIGINAL record before
        // the ROM's subsequent accept logic executes. Unlike a cosmetic
        // post-frame rewrite, the bus filter also covers its pick writes.
        if (g_layoutRunning && Ram8(record + 3u) < 2u) {
            const unsigned slot = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[slot].rom);
        }
        // This does not assert that a character was actually committed.
        xmvsf_f10::Capture::Get().AcceptHook(
            c, n, record, Ram8(record), Ram16(record + kCursor));
        if (FeatureOn("hidden_characters") && g_start[n] && Ram16(record + kCursor) == kRyu) {
            SetRam16(record + kCursor, kAkuma);
        }
        // F10 proved this hook does not fire for P1's second team pick.
        // It is logged; Training's handoff comes from native phase 03->04.
        session::CharacterAccepted(n);
        return false;
    }
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    // 87.479: draw the ROM's original selector frames and flicker.
    // A palette-27 blanket OAM filter hid real P1/P2 cursor pixels.
    machine.video.hideP2Selector = false;
    for (int n = 0; n < 2; n++) {
        g_start[n] = (machine.players[n] & kStart) != 0;
    }
}

// The game keeps its own accept/animation/sound/mode routines. Only input
// navigation and the cursor's source word are replaced; buttons still go
// through the unmodified native accept routine. Called AFTER Training P1->P2
// ownership routing so the active dummy receives the same new grid.
void ChoosingLayoutBeforeCpu(Machine &machine) {
    // Only the native XMVSF custom frontend owns this layout. Do not change
    // arcade-replay/headless/online default ROM selection when it is off.
    const bool choosing = machine.video.hudShock &&
                          Ram8(kPlaying) != 0 &&
                          Ram16(kGameStep) == kGameChoosing;
    if (!choosing) {
        machine.selectionRamWriteOverride = nullptr;
        if (g_layoutRunning) g_layout.Reset();
        g_layoutRunning = false;
        g_accepted[0] = g_accepted[1] = 0;
        g_secondCandidateReady[0] = g_secondCandidateReady[1] = false;
        return;
    }
    if (!g_layoutRunning) {
        g_layout.Reset();
        g_committed[0][0] = g_committed[0][1] = 0xFFFFu;
        g_committed[1][0] = g_committed[1][1] = 0xFFFFu;
        g_secondCandidateReady[0] = g_secondCandidateReady[1] = false;
        g_commitPending = false;
        g_layoutRunning = true;
    }
    // Keep this active for the ROM's actual writes during RunFrame.
    // F10's separate RAM write observer remains undisturbed.
    machine.selectionRamWriteOverride = &RemapSelectionWrite;
    for (int n = 0; n < 2; ++n) {
        const uint32_t record = kChoosing + static_cast<uint32_t>(n) * kChoosingSize;
        // The native 03->04 transition means the second character was
        // accepted. Steps 04..06 are the game's MODE/SPEED choices and
        // MUST retain real directional input (87.477 regression lesson).
        const uint8_t phase = Ram8(record);
        const uint8_t accepted = Ram8(record + 3u);
        // Native 02 is first-pick input; 03 is second-pick input.
        // Do not move the visual cursor during 00/01 entry or the
        // acceptance animation between the two picks.
        const bool active = (phase == 2u && accepted == 0u) ||
                            (phase == 3u && accepted == 1u && g_secondCandidateReady[n]);
        const unsigned direction = machine.players[n] & (kRight | kLeft | kDown | kUp);
        g_layout.Advance(n, direction, active);
        if (active) {
            const unsigned slot = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[slot].rom);
            // KEEP the physical direction bits: the native movement
            // sound/effects only run when the original ROM sees them.
            // Its cursor and pick RAM writes are now remapped in-frame,
            // rather than suppressing input and repairing afterwards.
        }
    }
}

// The ROM can write its original cursor after the beforeFrame hook. Keep the
// new horizontal mapping authoritative at the end of the native CPU frame,
// where the SDL composition reads FF518C / FF51AC. Native accept processing
// still runs untouched; commit only on the two real FF5183/FF51A3 edges.
void ChoosingLayoutAfterFrame(Machine &machine) {
    if (!g_layoutRunning || !machine.video.hudShock ||
        Ram16(kGameStep) != kGameChoosing || !Ram8(kPlaying)) return;
    for (int n = 0; n < 2; ++n) {
        const uint32_t record = kChoosing + static_cast<uint32_t>(n) * kChoosingSize;
        const uint8_t accepted = Ram8(record + 3u);
        const uint8_t prior = g_accepted[n];
        const unsigned slot = static_cast<unsigned>(g_layout.index[n]);
        if (accepted > prior && accepted <= 2u) {
            for (unsigned pick = prior; pick < accepted; ++pick) {
                const uint32_t pickAt = kPicks + static_cast<uint32_t>(n * 4u + pick * 2u);
                const uint16_t target = port::xmvsf_select::kSlots[slot].rom;
                const uint16_t native = Ram16(pickAt);
                // Preserve the ROM's Start+Ryu/Akuma or Start+Chun-Li/Alpha
                // variation if that feature is enabled on this machine.
                const bool hidden = FeatureOn("hidden_characters") && g_start[n] &&
                    ((target == 0x12u && native == kAkuma) ||
                     (target == 0x16u && native == 0x24u));
                // The bus accepted-count write is the precise commit edge;
                // do not overwrite its ID using the NEXT character's index.
                const uint16_t committed = g_committed[n][pick] != 0xFFFFu
                    ? g_committed[n][pick] : (hidden ? native : target);
                g_committed[n][pick] = committed;
                if (native != committed) SetRam16(pickAt, committed);
                std::fprintf(stderr,
                    "XMVSF 87.500 native select P%d PICK%d: native=%04X mapped=%04X count=%u\n",
                    n + 1, pick + 1, native, committed, accepted);
            }
         }
        if (accepted == 2u && g_accepted[n] < 2u) g_commitPending = true;
        g_accepted[n] = accepted;
        // The original ROM enters phase 03 when the SECOND character is
        // actually ready. Advance only on that edge, never immediately
        // when FF5183/FF51A3 first becomes 1 (which is still the first
        // character's confirmation/voice animation).
        if (accepted == 1u && Ram8(record) == 3u)
            EnterSecondCandidate(static_cast<unsigned>(n));
        // Write AFTER the ROM's original navigation, not only before it.
        // Do not overwrite native mode/speed (steps 4+) or completed picks.
        if (accepted < 2u && Ram8(record) <= 3u) {
            const unsigned preview = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[preview].rom);
        }
    }
}

// Single read-only source of displayed character identity. The original
// cursor word is NO LONGER authoritative after a second team pick: the ROM
// can replace it while finalizing its native selection bookkeeping.
// This gives the art and names the same IDs we actually commit to the team.
uint16_t ChoosingVisualCharacterId(unsigned side) {
    if (side >= 2u) return 0xFFFFu;
    const uint32_t record = kChoosing + side * kChoosingSize;
    if (!g_layoutRunning) return Ram16(record + kCursor);
    if (Ram8(record + 3u) >= 2u) {
        const uint16_t committed = g_committed[side][1];
        if (committed != 0xFFFFu) return committed;
        return Ram16(kPicks + side * 4u + 2u);
    }
    return port::xmvsf_select::kSlots[
        static_cast<unsigned>(g_layout.index[side])].rom;
}

// The native 00BAEA stage hook is before the ROM constructs its final
// match records. Apply each accepted player team there ONE time. Never
// replace Arcade CPU opponents on later matches with stale selector data.
void ApplyCustomTeamPicksBeforeMatch() {
    if (!g_commitPending) return;
    for (int n = 0; n < 2; ++n) {
        if (n == 1 && (Ram8(kPlayersIn) & 2u) == 0u) continue;
        if (g_committed[n][0] == 0xFFFFu || g_committed[n][1] == 0xFFFFu)
            continue;
        for (unsigned pick = 0; pick < 2u; ++pick) {
            SetRam16(kPicks + static_cast<uint32_t>(n * 4u + pick * 2u),
                     g_committed[n][pick]);
        }
    }
    g_commitPending = false;
}

void RegisterChoosing() {
    AddHook(HookAt(0x00B1E8), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden character with no code: hold Start as you choose Ryu for Akuma. (Start held as you "
                                  "choose Chun-Li gives her Street Fighter Alpha look, as on the board.)"});
}

} // namespace cps2recomp
