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
    uint16_t g_committed[2][2] = {{0xFFFFu, 0xFFFFu}, {0xFFFFu, 0xFFFFu}};
    bool g_commitPending = false; // one native match handoff, not every arcade round
    // 87.506: purely local, frame-scoped voice correction state. This is
    // never used for character selection, cursor movement or team commits.
    bool g_voiceConfirm[2] = {false, false};
    uint16_t g_voiceQueueFrameStart = 0xFFFFu;

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
        const uint32_t at = address & 0xFFFFFFu;
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

    // 0787E0 is the native 16-byte sound-request enqueuer (documented in
    // xmvsf.h / PROGRESS.md). Its hook runs BEFORE the request currently
    // being inserted. The F10-confirmed 0xA4,0x82 pair means that when
    // it is called for the following request, the previous voice request
    // is already in the queue, but has not been handed to the sound CPU.
    // Patch ONLY the previous request's character ID, keeping the rest of
    // the native request and its queue timing completely unmodified.
    bool SelectionVoiceQueued(M68K &) {
        if (!g_layoutRunning || !Ram8(kPlaying) ||
            Ram16(kGameStep) != kGameChoosing ||
            g_voiceQueueFrameStart == 0xFFFFu) return false;
        const uint16_t next = Ram16(kSoundQueueEnd);
        if (next > kSoundQueueMask || (next & 15u) != 0u) return false;
        const unsigned fresh = (static_cast<unsigned>(next) + 0x400u -
                                g_voiceQueueFrameStart) & kSoundQueueMask;
        // A newly queued request must have been added *this* CPU frame.
        // Prevent accidentally rewriting last frame's queued voice.
        if (fresh < 16u || fresh > 128u) return false;
        unsigned side = 2u;
        for (unsigned n = 0u; n < 2u; ++n) {
            if (!g_voiceConfirm[n] || g_accepted[n] >= 2u) continue;
            // Do not guess ownership if both players confirmed together.
            if (side != 2u) return false;
            side = n;
        }
        if (side >= 2u) return false;
        const uint32_t previous = kSoundQueue +
            ((static_cast<uint32_t>(next) + 0x400u - 16u) & kSoundQueueMask);
        // The recorded QSound announcer request is
        // 00 00 00 A4 00 00 10 00 ... (Gambit), not a generic SFX.
        if (Ram16(previous) != 0u || Ram8(previous + 2u) != 0u ||
            Ram16(previous + 4u) != 0u ||
            Ram16(previous + 6u) != 0x1000u) return false;
        const uint8_t nativeVoice = Ram8(previous + 3u);
        if (nativeVoice < 0xA0u || nativeVoice > 0xB2u) return false;
        const uint16_t nativeRom =
            static_cast<uint16_t>((nativeVoice - 0xA0u) * 2u);
        if (port::xmvsf_select::Find(nativeRom) < 0) return false;
        const uint16_t selectedRom = IntendedNativeId(side);
        const uint16_t wantedVoice =
            port::xmvsf_select::CharacterVoiceCommand(selectedRom);
        if (wantedVoice == 0xFFFFu ||
            wantedVoice == nativeVoice) return false;
        SetRam8(previous + 3u, static_cast<uint8_t>(wantedVoice));
        std::fprintf(stderr,
            "XMVSF 87.506 VOICE P%u PICK%u: native=%02X corrected=%02X character=%04X\n",
            side + 1u, static_cast<unsigned>(g_accepted[side]) + 1u,
            nativeVoice, wantedVoice, selectedRom);
        return false; // preserve the ROM's actual enqueuer
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
        g_voiceConfirm[0] = g_voiceConfirm[1] = false;
        g_voiceQueueFrameStart = 0xFFFFu;
        return;
    }
    if (!g_layoutRunning) {
        g_layout.Reset();
        g_committed[0][0] = g_committed[0][1] = 0xFFFFu;
        g_committed[1][0] = g_committed[1][1] = 0xFFFFu;
        g_commitPending = false;
        g_layoutRunning = true;
    }
    // Keep this active for the ROM's actual writes during RunFrame.
    // F10's separate RAM write observer remains undisturbed.
    machine.selectionRamWriteOverride = &RemapSelectionWrite;
    // The original native sound queue can receive multiple requests in a
    // frame. Track its starting write index to distinguish a freshly queued
    // announcer from an old ring slot. Never mutate the index itself.
    g_voiceQueueFrameStart = Ram16(kSoundQueueEnd);
    constexpr uint16_t kConfirmButtons = kPunch1 | kPunch2 | kPunch3 |
                                         kKick1 | kKick2 | kKick3;
    for (int n = 0; n < 2; ++n) {
        const uint32_t record = kChoosing + static_cast<uint32_t>(n) * kChoosingSize;
        // The native 03->04 transition means the second character was
        // accepted. Steps 04..06 are the game's MODE/SPEED choices and
        // MUST retain real directional input (87.477 regression lesson).
        const bool active = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;
        g_voiceConfirm[n] = active && (machine.players[n] & kConfirmButtons) != 0u;
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
    // 87.504 F10-only observation of the exact custom positions sent to
    // native selection this frame. No navigation/ROM/audio changes.
    xmvsf_f10::Capture::Get().CustomGridBeforeCpu(
        machine, g_layout.index[0], g_layout.index[1]);
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
                const uint16_t committed = hidden ? native : target;
                g_committed[n][pick] = committed;
                if (native != committed) SetRam16(pickAt, committed);
                std::fprintf(stderr,
                    "XMVSF 87.500 native select P%d PICK%d: native=%04X mapped=%04X count=%u\n",
                    n + 1, pick + 1, native, committed, accepted);
            }
            // The second character defaults to the next cell across,
            // as native Cyclops->Wolverine / Ryu->Ken did.
            if (prior == 0u && accepted == 1u) {
                g_layout.index[n] = port::xmvsf_select::Navigate(g_layout.index[n], 1u);
            }
        }
        if (accepted == 2u && g_accepted[n] < 2u) g_commitPending = true;
        g_accepted[n] = accepted;
        // Write AFTER the ROM's original navigation, not only before it.
        // Do not overwrite native mode/speed (steps 4+) or completed picks.
        if (accepted < 2u && Ram8(record) <= 3u) {
            const unsigned preview = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[preview].rom);
        }
    }
}

// 87.505: Render identity from the same custom slot that is committed
// to the native team, not the ROM's transient FF518C/FF51AC cursor word.
// The F10 87.504 capture showed native cursor=0008 (Gambit) while the
// custom second slot was 0006 (Rogue), subsequently committed at FF50B2.
// Read-only: this does NOT change navigation, phase, audio or accepted picks.
uint16_t ChoosingVisualCharacterId(unsigned side) {
    if (side >= 2u) return 0xFFFFu;
    const uint32_t record = kChoosing + static_cast<uint32_t>(side) * kChoosingSize;
    if (!g_layoutRunning) return Ram16(record + kCursor);
    if (Ram8(record + 3u) >= 2u)
        return Ram16(kPicks + static_cast<uint32_t>(side) * 4u + 2u);
    const unsigned slot = static_cast<unsigned>(g_layout.index[side]);
    return port::xmvsf_select::kSlots[slot].rom;
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
    AddHook(HookAt(0x0787E0), "SelectionVoiceQueued", SelectionVoiceQueued);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden character with no code: hold Start as you choose Ryu for Akuma. (Start held as you "
                                  "choose Chun-Li gives her Street Fighter Alpha look, as on the board.)"});
}

} // namespace cps2recomp
