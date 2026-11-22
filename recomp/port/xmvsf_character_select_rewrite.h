// Native M68K selection RAM write translation for the new 18-slot layout.
// Operates at the original write boundary, BEFORE native logic reads its
// selected ID again; not a post-frame cosmetic correction.
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "xmvsf_character_select_layout.h"

namespace port::xmvsf_select {

inline constexpr uint16_t RewriteNativeSelectionWrite(
    uint32_t address, uint16_t value, unsigned bytes,
    const std::array<uint16_t, 2> &selected,
    const std::array<uint8_t, 2> &accepted) {
    if (bytes != 1u && bytes != 2u) return value;
    address &= 0xFFFFFFu;
    for (unsigned n = 0; n < 2u; ++n) {
        const uint32_t cursor = 0xFF518Cu + n * 0x20u;
        const uint32_t firstPick = 0xFF50B0u + n * 4u;
        const uint32_t pendingPick = firstPick + 2u * accepted[n];
        bool rewrite = (accepted[n] < 2u &&
                         (address == cursor || address == cursor + 1u));
        // P1 first/second and P2 first/second; never change a previously
        // committed partner again when the ROM refreshes its RAM.
        if (accepted[n] < 2u &&
            (address == pendingPick || address == pendingPick + 1u))
            rewrite = true;
        if (!rewrite) continue;
        if (bytes == 2u) return selected[n];
        return static_cast<uint16_t>((address & 1u) != 0u
            ? selected[n] & 0xFFu : selected[n] >> 8u);
    }
    return value;
}

// 87.508: no M68K execution hooks. The native confirmation voice request
// occupies a 16-byte entry in the FF2648 sound queue. F10 captured:
// Magneto 0E -> 000000A700001000... ; Gambit 08 -> 000000A400001000...
// The expected P1 voice for Rogue 06 is therefore 0xA3.
// 87.511 F10 (latest(2).zip) additionally confirms the P2 family:
// Gambit 08 -> 0xC4 and Wolverine 00 -> 0xC0.
// This helper is called only from the optional between-CPU/sound scanline
// callback while a real native confirmation button is held.
// Alpha Chun-Li is a costume/form of Chun-Li, not another separately
// recorded spoken character. Share her native selection sound identity.
inline constexpr std::uint16_t CharacterVoiceIdentity(std::uint16_t id) {
    return id == 0x24u ? 0x16u : id;
}
inline constexpr std::uint16_t NativeCharacterVoice(std::uint16_t id,
                                                    unsigned side = 0u) {
    const std::uint16_t voiceId = CharacterVoiceIdentity(id);
    return (side < 2u && Find(voiceId) >= 0 && (voiceId & 1u) == 0u)
        ? static_cast<std::uint16_t>(0xA0u + side * 0x20u + voiceId / 2u)
        : 0xFFFFu;
}

// 87.515: Selection audio owns its OWN character identity. The ROM's
// transient selection cursor cannot choose the samples played by QSound.
// The authoritative ID is from kSlots/IntendedNativeId at the real
// FF5183/FF51A3 accepted-count write. P1 speaks A0..B1, P2 C0..D1;
// Alpha Chun-Li shares Chun-Li's name recording.
//
// The shared sound transport delivers a *complete* 16-byte QSound
// request when the M68K writes 61801E. Filter that request BEFORE it
// can reach the Z80, not by guessing when the ROM enqueues it.
//
// Preserve native cursor ticks 84/85. Only explicitly authorized
// names and confirmation sounds 82/83 can pass during custom select.
// Drop other native selection-family commands and character-owned
// 0A00 speech, even when their source character differs from our grid.
// Non-selection sound transport and other game phases are untouched.
inline bool SelectionSoundEnvelope(const std::uint8_t *p, unsigned kind) {
    if (!p || p[0] || p[1] || p[4] || p[5] ||
        p[6] != kind || p[7]) return false;
    for (unsigned i = 8u; i < 16u; ++i)
        if (p[i]) return false;
    return true;
}

inline bool QueueExplicitSelectionSound(std::uint8_t *ram, std::size_t size,
                                        std::uint16_t command) {
    if (!ram || size < 0x10000u || command > 0xFFu) return false;
    const unsigned at = (static_cast<unsigned>(ram[0x2642u]) << 8u) |
                         ram[0x2643u];
    if (at > 0x3F0u || (at & 15u)) return false;
    std::uint8_t *p = ram + 0x2648u + at;
    for (unsigned i = 0u; i < 16u; ++i) p[i] = 0u;
    p[3] = static_cast<std::uint8_t>(command);
    p[6] = 0x10u; // exact envelope observed in the native F10 captures
    const unsigned next = (at + 16u) & 0x3F0u;
    ram[0x2642u] = static_cast<std::uint8_t>(next >> 8u);
    ram[0x2643u] = static_cast<std::uint8_t>(next);
    return true;
}

// Tickets are granted only when the ROM actually increments a player's
// accepted-character counter, once per pick. This prevents wrong native
// names, duplicate correct names, and duplicate confirmation sounds.
// A native packet with exactly the same correct command may consume a
// ticket first; the explicit duplicate is then discarded, so ONE plays.
inline bool AllowAuthoritativeSelectionSound(
    const std::uint8_t *request,
    const std::array<std::uint16_t, 2> &expectedNames,
    std::array<unsigned, 2> &nameTickets,
    std::array<unsigned, 2> &confirmTickets) {
    if (!request) return false;
    if (SelectionSoundEnvelope(request, 0x10u) && request[2] == 0u) {
        const unsigned code = request[3];
        // Character-select movement stays native and side-specific.
        if (code == 0x84u || code == 0x85u) return true;
        for (unsigned side = 0u; side < 2u; ++side) {
            const unsigned confirm = 0x82u + side;
            if (code == confirm) {
                if (confirmTickets[side] == 0u) return false;
                --confirmTickets[side];
                return true;
            }
            if (code == expectedNames[side] && nameTickets[side] != 0u) {
                --nameTickets[side];
                return true;
            }
        }
        // Native names in either family, including invalid source IDs,
        // cannot select a different recorded character. Other ordinary
        // UI/music requests are NOT character speech and stay untouched.
        if (code >= 0xA0u && code <= 0xDFu) return false;
        return true;
    }
    if (SelectionSoundEnvelope(request, 0x0Au)) {
        // Original sprite speech: this whole 0A00 request family is
        // suppressed during custom select. No guessed 20-byte block
        // remapping or substituted "Cammy as Akuma" voice.
        return false;
    }
    // Music and non-selection control requests are left alone.
    return true;
}

// 87.509/87.511: preserve the board's own per-player movement ticks.
// F10 confirms P1 uses 0x84, P2 uses 0x85, each in the native form:
// 00 00 00 XX 00 00 10 00 00 00 00 00 00 00 00 00.
// If a custom grid movement occurs while the ROM is in its inter-pick
// animation, the ROM may not queue that tick itself. Append ONLY when no
// identical native request was produced during this CPU frame.
//
// Sound requests are queued as the original game queues them; playback
// occurs on the following native sound processing, not via a new ROM hook.
inline bool QueueMissingCursorMovementSound(std::uint8_t *ram, std::size_t size,
                                             std::uint16_t before,
                                             std::uint16_t after,
                                             unsigned soundCode = 0x84u) {
    if ((soundCode != 0x84u && soundCode != 0x85u) ||
        !ram || size < 0x10000u ||
        before > 0x3F0u || after > 0x3F0u ||
        (before & 15u) || (after & 15u)) return false;
    const unsigned distance = (static_cast<unsigned>(after) + 0x400u -
                               static_cast<unsigned>(before)) & 0x3F0u;
    // Never guess if the queue moved implausibly far in one native
    // frame or has wrapped so far that its contents might be overwritten.
    if (distance > 128u) return false;
    const auto nativeMoveTick = [soundCode](const std::uint8_t *p) {
        if (p[0] || p[1] || p[2] || p[3] != soundCode ||
            p[4] || p[5] || p[6] != 0x10u || p[7]) return false;
        for (unsigned n = 8; n < 16; ++n)
            if (p[n]) return false;
        return true;
    };
    for (unsigned delta = 0u; delta < distance; delta += 16u) {
        const unsigned at = (static_cast<unsigned>(before) + delta) & 0x3F0u;
        if (nativeMoveTick(ram + 0x2648u + at)) return false;
    }
    // As in the original sound ring and the existing modes::PlaySound,
    // FF2642 is the next 16-byte request slot. Preserve all prior
    // entries (including character announcements and 0x82 SFX).
    std::uint8_t *entry = ram + 0x2648u + after;
    for (unsigned i = 0; i < 16u; ++i) entry[i] = 0u;
    entry[3] = static_cast<std::uint8_t>(soundCode);
    entry[6] = 0x10u;
    const std::uint16_t next = static_cast<std::uint16_t>(
        (static_cast<unsigned>(after) + 16u) & 0x3F0u);
    ram[0x2642u] = static_cast<std::uint8_t>(next >> 8);
    ram[0x2643u] = static_cast<std::uint8_t>(next);
    return true;
}

} // namespace port::xmvsf_select
