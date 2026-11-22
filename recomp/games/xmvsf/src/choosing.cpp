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

    // 87.519: NO extra per-player/pick voice queue and NO direct QSound
    // injection. The game's own FF2648 sound queue is rewritten at the
    // byte/word bus write. Voice code comes from kSlots[...].nameVoice.
    uint8_t g_soundAccepted[2] = {0, 0};
    unsigned g_soundTailFrames = 0u;
    // 87.509: beginning-of-frame native ring pointer and custom grid.
    // Used only to fill missing ROM 0x84 ticks on *actual* grid movement.
    uint16_t g_cursorSoundBegin = 0xFFFFu;
    uint16_t g_nameQueueHead = 0xFFFFu; // next uncorrected FF2648 request
    int g_gridBeforeFrame[2] = {-1, -1};
    // A (gamepad South / default kick 1) is the ONLY native select
    // authorization. B (East / kick 2) undoes the last accepted pick.
    // Both are edge-triggered, and the other fighting buttons remain free.
    bool g_aHeld[2] = {false, false};
    bool g_bHeld[2] = {false, false};
    bool g_confirmQueued[2] = {false, false};
    unsigned g_confirmPulse[2] = {0u, 0u};

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

    void CancelTeamPick(unsigned n) {
        if (n >= 2u) return;
        const uint32_t record = kChoosing + static_cast<uint32_t>(n) * kChoosingSize;
        const unsigned count = Ram8(record + 3u);
        if (count == 0u || count > 2u || Ram8(record) > 4u) return;
        const unsigned undone = count - 1u;
        const std::uint16_t old = g_committed[n][undone];
        const int at = port::xmvsf_select::Find(old);
        if (at >= 0) g_layout.index[n] = at;
        g_committed[n][undone] = 0xFFFFu;
        g_accepted[n] = static_cast<uint8_t>(undone);
        g_soundAccepted[n] = static_cast<uint8_t>(undone);
        g_confirmQueued[n] = false;
        g_confirmPulse[n] = 0u;
        g_commitPending = false;
        SetRam8(record, static_cast<uint8_t>(undone ? 3u : 1u));
        SetRam8(record + 3u, static_cast<uint8_t>(undone));
        SetRam16(kPicks + static_cast<uint32_t>(n * 4u + undone * 2u), 0u);
        SetRam16(record + kCursor,
                 port::xmvsf_select::kSlots[static_cast<unsigned>(g_layout.index[n])].rom);
        if (!RunningAhead())
            std::fprintf(stderr, "XMVSF 87.521 B_CANCEL P%u PICK%u\\n",
                         n + 1u, count);
    }

    bool AuthoritativeSelectionSound(Machine &, uint8_t *request) {
        return port::xmvsf_select::AllowNativeSelectionSound(request);
    }


    // Complete native requests, not transient native cursor positions, own
    // voice delivery. Preserve source A/B (P1) vs C/D (P2) until each
    // entry has been rewritten exactly once.
    void CorrectNativeQueue(Machine &machine, uint16_t after) {
        if (!g_layoutRunning || g_nameQueueHead == 0xFFFFu ||
            g_nameQueueHead > 0x3F0u || after > 0x3F0u ||
            (g_nameQueueHead & 15u) || (after & 15u)) return;
        const unsigned distance =
            (static_cast<unsigned>(after) + 0x400u -
             static_cast<unsigned>(g_nameQueueHead)) & 0x3F0u;
        if (distance > 0x80u) return; // pointer byte still being written
        const std::array<uint16_t, 2> selected{{
            IntendedNativeId(0u), IntendedNativeId(1u)
        }};
        const unsigned changed = port::xmvsf_select::RewriteQueuedSelectionNames(
            g_nameQueueHead, after, machine.ram.data(),
            machine.ram.size(), selected);
        if (changed && !RunningAhead())
            std::fprintf(stderr,
                "XMVSF 87.520 ANNOUNCER_QUEUE corrected=%u "
                "P1=%04X P2=%04X frame=%llu\n",
                changed, selected[0], selected[1],
                static_cast<unsigned long long>(machine.frame));
        g_nameQueueHead = after;
    }

    void RewriteNativeQueuedNames(Machine &machine) {
        // Scanline fallback if the guest copied a full entry directly.
        CorrectNativeQueue(machine, Ram16(kSoundQueueEnd));
    }

    void RemapSelectionWrite(Machine &machine, uint32_t address,
                             uint16_t &value, unsigned bytes) {
        const std::array<uint16_t, 2> selected{{
            IntendedNativeId(0u), IntendedNativeId(1u)
        }};
        const std::array<uint8_t, 2> accepted{{g_accepted[0], g_accepted[1]}};
        // FF2642 is the producer's commit pointer. Correct completed
        // 16-byte names here, before the ROM transfers them to QSound.
        const uint32_t at = address & 0xFFFFFFu;
        if (at == 0xFF2642u || at == 0xFF2643u) {
            const uint16_t prior = Ram16(kSoundQueueEnd);
            const uint16_t updated =
                bytes == 2u && at == 0xFF2642u ? value :
                bytes == 1u && at == 0xFF2642u
                    ? static_cast<uint16_t>(((value & 0xFFu) << 8u) | (prior & 0xFFu)) :
                bytes == 1u && at == 0xFF2643u
                    ? static_cast<uint16_t>((prior & 0xFF00u) | (value & 0xFFu)) :
                    prior;
            CorrectNativeQueue(machine, updated);
        }
        value = port::xmvsf_select::RewriteNativeSelectionWrite(
            address, value, bytes, selected, accepted);

        // The original countdown can still run internally, but it has
        // NO permission to accept a character or leave the selection
        // phase. Only a real A press can authorize native acceptance.
        for (unsigned n = 0u; n < 2u; ++n) {
            const uint32_t record = kChoosing + n * kChoosingSize;
            if (at == record && (bytes == 1u || bytes == 2u)) {
                const unsigned next = bytes == 1u ? value & 0xFFu : value >> 8u;
                const unsigned count = Ram8(record + 3u);
                const bool mayAdvanceFirst = count > 0u || g_confirmQueued[n];
                const bool mayAdvanceSecond = count > 1u ||
                    (count == 1u && g_confirmQueued[n]);
                if ((next >= 2u && !mayAdvanceFirst) ||
                    (next >= 4u && !mayAdvanceSecond)) {
                    const uint16_t prior = Ram8(record);
                    value = bytes == 1u
                        ? prior
                        : static_cast<uint16_t>((prior << 8u) | (value & 0xFFu));
                }
            }
        }

        // FF5183 / FF51A3 are the ROM's actual accepted-character
        // counters. Capture the ID AT THE RAM WRITE: the ROM can leave
        // character selection on the same frame as pick two, meaning a
        // post-frame check may otherwise never see the second accept.
        for (unsigned n = 0; n < 2u; ++n) {
            const uint32_t counter = 0xFF5183u + n * 0x20u;
            const bool wroteCounter = (bytes == 1u && at == counter) ||
                                      (bytes == 2u && at == counter - 1u);
            if (!wroteCounter) continue;
            uint8_t count = static_cast<uint8_t>(value & 0xFFu);
            // Reject native timeout and all non-A character confirms.
            // A grant stays armed until the ROM truly advances FF5183.
            if (count > g_accepted[n] && !g_confirmQueued[n]) {
                count = g_accepted[n];
                value = bytes == 2u
                    ? static_cast<uint16_t>((value & 0xFF00u) | count)
                    : count;
            }
            if (count > g_accepted[n] && count <= 2u) {
                g_confirmQueued[n] = false;
                g_confirmPulse[n] = 0u;
            }
            // The native accepted counter is the only authority for
            // an audible selection. Not raw button presses, cursor
            // animation frames, or a guessed announcer source ID.
            // Sound grants must be issued in the committed timeline,
            // not during speculative run-ahead frames (unlike RAM, the
            // source-only ticket counters are not save-state serialized).
            if (!RunningAhead() && count > g_soundAccepted[n] && count <= 2u) {
                g_soundAccepted[n] = count;
                const unsigned pick = count - 1u;
                const std::uint16_t selected = IntendedNativeId(n);
                g_committed[n][pick] = selected;
                std::fprintf(stderr,
                    "XMVSF 87.519 ACCEPT P%u PICK%u rom=%04X name=%02X frame=%llu\n",
                    n + 1u, count, static_cast<unsigned>(selected),
                    static_cast<unsigned>(
                        port::xmvsf_select::NativeCharacterVoice(selected)),
                    static_cast<unsigned long long>(machine.frame));
            }
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
        // The second accepted pick can leave choosing in the SAME frame.
        // Keep filtering native sound requests for a bounded drain tail
        // so a delayed B3/C-family name cannot leak into the VS handoff.
        if (g_layoutRunning) g_soundTailFrames = 30u;
        machine.selectionSoundRequestFilter = g_soundTailFrames > 0u
            ? &AuthoritativeSelectionSound : nullptr;
        machine.selectionVoiceQueueBeforeSound = nullptr;
        g_nameQueueHead = 0xFFFFu;
        if (g_soundTailFrames > 0u) {
            --g_soundTailFrames;
        } else {
            g_soundAccepted[0] = g_soundAccepted[1] = 0u;
        }
        g_cursorSoundBegin = 0xFFFFu;
        g_gridBeforeFrame[0] = g_gridBeforeFrame[1] = -1;
        if (g_layoutRunning) g_layout.Reset();
        for (unsigned n = 0u; n < 2u; ++n) {
            g_aHeld[n] = g_bHeld[n] = g_confirmQueued[n] = false;
            g_confirmPulse[n] = 0u;
        }
        g_layoutRunning = false;
        g_accepted[0] = g_accepted[1] = 0;
        return;
    }
    if (!g_layoutRunning) {
        g_layout.Reset();
        g_committed[0][0] = g_committed[0][1] = 0xFFFFu;
        g_committed[1][0] = g_committed[1][1] = 0xFFFFu;
        g_commitPending = false;
        g_soundAccepted[0] = g_soundAccepted[1] = 0u;
        for (unsigned n = 0u; n < 2u; ++n) {
            g_aHeld[n] = g_bHeld[n] = g_confirmQueued[n] = false;
            g_confirmPulse[n] = 0u;
        }
        g_layoutRunning = true;
    }
    g_soundTailFrames = 0u; // fresh selection owns its own audio
    // Keep this active for the ROM's actual writes during RunFrame.
    // F10's separate RAM write observer remains undisturbed.
    machine.selectionRamWriteOverride = &RemapSelectionWrite;
    // The native sound ROM owns queue delivery and correct QSound format.
    // We alter ONLY its announcer byte at FF2648 when the ROM writes it.
    machine.selectionVoiceQueueBeforeSound = &RewriteNativeQueuedNames;
    machine.selectionSoundRequestFilter = &AuthoritativeSelectionSound;
    g_cursorSoundBegin = Ram16(kSoundQueueEnd);
    // Reset each frame so speculative run-ahead cannot desynchronize us.
    g_nameQueueHead = g_cursorSoundBegin;
    g_gridBeforeFrame[0] = g_layout.index[0];
    g_gridBeforeFrame[1] = g_layout.index[1];

    for (int n = 0; n < 2; ++n) {
        const uint32_t record = kChoosing + static_cast<uint32_t>(n) * kChoosingSize;
        // The native 03->04 transition means the second character was
        // accepted. Steps 04..06 are the game's MODE/SPEED choices and
        // MUST retain real directional input (87.477 regression lesson).
        const unsigned phase = Ram8(record);
        const unsigned count = Ram8(record + 3u);
        const bool active = phase <= 3u && count < 2u;
        const uint16_t physical = machine.players[n];
        const bool a = (physical & kKick1) != 0u;
        const bool b = (physical & kKick2) != 0u;
        const bool aEdge = a && !g_aHeld[n];
        const bool bEdge = b && !g_bHeld[n];
        g_aHeld[n] = a;
        g_bHeld[n] = b;
        // Character selection is explicitly A/B, not all six attacks.
        // Keep the original directional bits for native cursor sounds.
        if (phase <= 4u && count <= 2u) {
            machine.players[n] &= static_cast<uint16_t>(
                ~(kPunch1 | kPunch2 | kPunch3 |
                  kKick1 | kKick2 | kKick3));
        }
        if (bEdge && phase <= 4u && count != 0u) {
            CancelTeamPick(static_cast<unsigned>(n));
        }
        const bool selecting = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;
        const unsigned direction = physical & (kRight | kLeft | kDown | kUp);
        const int before = g_layout.index[n];
        g_layout.Advance(n, direction, selecting);
        if (selecting && Ram8(record + 3u) == 1u) {
            // The first pick is no longer a valid cursor destination.
            g_layout.index[n] = port::xmvsf_select::SkipTeamIndex(
                before, g_layout.index[n], direction, g_committed[n][0]);
        }
        if (selecting) {
            const unsigned slot = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[slot].rom);
            // One rising A press = at most one native confirmation.
            // A held across the first->second handoff cannot choose twice.
            if (aEdge && !b && !g_confirmQueued[n] &&
                (Ram8(record + 3u) == 0u ||
                 !port::xmvsf_select::SameTeamCharacter(
                      g_committed[n][0], IntendedNativeId(static_cast<unsigned>(n))))) {
                g_confirmQueued[n] = true;
                g_confirmPulse[n] = 0u;
            }
            // The first confirmation can enter native phase 02 for
            // its animation. Hold the grant until phase 03 is ready.
            if (g_confirmQueued[n] && (phase == 1u || phase == 3u)) {
                if (g_confirmPulse[n]++ % 8u == 0u)
                    machine.players[n] |= kPunch1;
            }
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
                // Preserve the EXACT ID captured at the native accept
                // write. Do not recompute from a cursor that may have
                // already moved to the following character.
                const uint16_t fixed = g_committed[n][pick] != 0xFFFFu
                    ? g_committed[n][pick] : committed;
                g_committed[n][pick] = fixed;
                if (native != fixed) SetRam16(pickAt, fixed);
                std::fprintf(stderr,
                    "XMVSF 87.500 native select P%d PICK%d: native=%04X mapped=%04X count=%u\n",
                    n + 1, pick + 1, native, fixed, accepted);
            }
            // The second character defaults to the next cell across,
            // as native Cyclops->Wolverine / Ryu->Ken did.
            if (prior == 0u && accepted == 1u) {
                g_layout.index[n] = port::xmvsf_select::NextTeamIndex(g_layout.index[n]);
            }
        }
        if (accepted == 2u && g_accepted[n] < 2u) g_commitPending = true;
        g_accepted[n] = accepted;
        // Selection no longer times out. Keep native phase aligned with
        // the number of A-confirmed picks even if its clock reaches zero.
        if (accepted == 0u && Ram8(record) >= 2u)
            SetRam8(record, 1u);
        if (accepted == 1u && Ram8(record) >= 4u)
            SetRam8(record, 3u);
        // Write AFTER the ROM's original navigation, not only before it.
        // Do not overwrite native mode/speed (steps 4+) or completed picks.
        if (accepted < 2u && Ram8(record) <= 3u) {
            const unsigned preview = static_cast<unsigned>(g_layout.index[n]);
            SetRam16(record + kCursor, port::xmvsf_select::kSlots[preview].rom);
        }
    }
    // The original ROM already plays 0x84 when it handles movement.
    // During the first->second pick animation our custom grid can move
    // while its native movement sound is temporarily gated. Compare the
    // grid and this frame's native sound ring, and fill ONLY a missing
    // movement tick. Includes the ROM-style automatic next-cell hop
    // after first confirmation, without re-enqueuing the confirm voice.
    // F10 87.511: the ROM uses distinct native sound IDs:
    // P1 0x84, P2 0x85. Injecting 0x84 for P2 caused audible
    // double-ticks on top of P2's native 0x85 movement.
    const bool p1Moved = g_gridBeforeFrame[0] >= 0 &&
                         g_gridBeforeFrame[0] != g_layout.index[0];
    const bool p2Moved = g_gridBeforeFrame[1] >= 0 &&
                         g_gridBeforeFrame[1] != g_layout.index[1];
    if ((p1Moved || p2Moved) && g_cursorSoundBegin != 0xFFFFu &&
        !RunningAhead() && machine.ram.size() >= 0x10000u) {
        // Both sides can move on one frame; scan for the native ID
        // belonging to each side. The ring helper never doubles an ID
        // already queued by the ROM in that frame.
        for (unsigned side = 0; side < 2u; ++side) {
            if (!(side == 0u ? p1Moved : p2Moved)) continue;
            const uint16_t after = Ram16(kSoundQueueEnd);
            const unsigned soundCode = side == 0u ? 0x84u : 0x85u;
            if (port::xmvsf_select::QueueMissingCursorMovementSound(
                    machine.ram.data(), machine.ram.size(),
                    g_cursorSoundBegin, after, soundCode)) {
                std::fprintf(stderr,
                    "XMVSF 87.511 cursor tick: P%u native omitted %02X frame=%llu\n",
                    side + 1u, soundCode,
                    static_cast<unsigned long long>(machine.frame));
            }
        }
    }
}

// 87.505: Render identity from the same custom slot that is committed
// to the native team, not the ROM's transient FF518C/FF51AC cursor word.
// The F10 87.504 capture showed native cursor=0008 (Gambit) while the
// custom second slot was 0006 (Rogue), subsequently committed at FF50B2.
// Read-only: this does NOT change navigation, phase, audio or accepted picks.
// Read only the custom-confirmed identity for an accepted team slot.
// The ROM can temporarily report a different ID (or even an off-grid
// value) at FF50B0/FF50B2 during the selection animation. Never use
// that transient value to place a frozen custom cursor.
uint16_t ChoosingVisualCommittedId(unsigned side, unsigned pick) {
    if (side >= 2u || pick >= 2u || !g_layoutRunning)
        return 0xFFFFu;
    if (g_soundAccepted[side] <= pick && g_accepted[side] <= pick)
        return 0xFFFFu;
    return g_committed[side][pick];
}

uint16_t ChoosingVisualCharacterId(unsigned side) {
    if (side >= 2u) return 0xFFFFu;
    const uint32_t record = kChoosing + static_cast<uint32_t>(side) * kChoosingSize;
    if (!g_layoutRunning) return Ram16(record + kCursor);
    if (g_soundAccepted[side] >= 2u || Ram8(record + 3u) >= 2u) {
        // Completed selectors belong at the second custom pick, never
        // at a stale/incomplete native FF50B2 word.
        const uint16_t committed = g_committed[side][1];
        if (port::xmvsf_select::Find(committed) >= 0) return committed;
    }
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
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden character with no code: hold Start as you choose Ryu for Akuma. (Start held as you "
                                  "choose Chun-Li gives her Street Fighter Alpha look, as on the board.)"});
}

} // namespace cps2recomp
