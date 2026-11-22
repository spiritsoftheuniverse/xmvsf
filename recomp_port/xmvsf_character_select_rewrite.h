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
// The user sound-tested the A-family as the actual correct spoken names.
// 87.516: Character ID, NOT player side, determines the name. The ROM
// sometimes emits C-family codes for P2, but they are native transport
// behavior and NOT the canonical sound assignments for our custom grid.
// Alpha Chun-Li is a costume/form of Chun-Li, not another separately
// recorded spoken character. Share her native selection sound identity.
inline constexpr std::uint16_t CharacterVoiceIdentity(std::uint16_t id) {
    return id == 0x24u ? 0x16u : id;
}
inline constexpr std::uint16_t NativeCharacterVoice(std::uint16_t id) {
    const std::uint16_t voiceId = CharacterVoiceIdentity(id);
    return (Find(voiceId) >= 0 && (voiceId & 1u) == 0u)
        ? static_cast<std::uint16_t>(0xA0u + voiceId / 2u)
        : 0xFFFFu;
}

// 87.515: Selection audio owns its OWN character identity. The ROM's
// transient selection cursor cannot choose the samples played by QSound.
// The authoritative ID is from kSlots/IntendedNativeId at the real
// FF5183/FF51A3 accepted-count write. BOTH players speak the exact
// same A-family code (e.g. Gambit A4 for P1 OR P2). Alpha Chun-Li
// shares Chun-Li's name recording until independently sound-tested.
//
// The shared sound transport delivers a *complete* 16-byte QSound
// request when the M68K writes 61801E. Filter that request BEFORE it
// can reach the Z80, not by guessing when the ROM enqueues it.
//
// Preserve native cursor ticks 84/85. Names and confirmation sounds
// 82/83 are explicitly tagged per *player AND pick*, then restored to
// native 16-byte format before the sound CPU sees them. Drop all original
// A/C-family names and 0A00 sprite speech; pass unrelated music/UI unchanged.
inline bool SelectionSoundEnvelope(const std::uint8_t *p, unsigned kind) {
    if (!p || p[0] || p[1] || p[4] || p[5] ||
        p[6] != kind || p[7]) return false;
    for (unsigned i = 8u; i < 16u; ++i)
        if (p[i]) return false;
    return true;
}

// Each explicit name/confirmation carries the pick identity that produced
// it. The ROM uses all-zero bytes 8..15 for these 16-byte native envelopes.
// Mark bytes 8..11 only while in the M68K request ring; the delivery gate
// strips them BEFORE the Z80 or sound-driver receives the command.
// This prevents an old P1 pick 1 sound from consuming P1 pick 2's ticket.
inline constexpr std::uint8_t kSelectTagA = 0x51u, kSelectTagB = 0x5Eu;
using SelectionNames = std::array<std::array<std::uint16_t, 2>, 2>;
using SelectionTickets = std::array<std::array<unsigned, 2>, 2>;

inline bool QueueExplicitSelectionSound(std::uint8_t *ram, std::size_t size,
                                        std::uint16_t command,
                                        unsigned side = 0u, unsigned pick = 0u) {
    if (!ram || size < 0x10000u || command > 0xFFu ||
        side >= 2u || pick >= 2u) return false;
    const unsigned at = (static_cast<unsigned>(ram[0x2642u]) << 8u) |
                         ram[0x2643u];
    if (at > 0x3F0u || (at & 15u)) return false;
    std::uint8_t *p = ram + 0x2648u + at;
    for (unsigned i = 0u; i < 16u; ++i) p[i] = 0u;
    p[3] = static_cast<std::uint8_t>(command);
    p[6] = 0x10u;
    p[8] = kSelectTagA;
    p[9] = kSelectTagB;
    p[10] = static_cast<std::uint8_t>(side);
    p[11] = static_cast<std::uint8_t>(pick);
    const unsigned next = (at + 16u) & 0x3F0u;
    ram[0x2642u] = static_cast<std::uint8_t>(next >> 8u);
    ram[0x2643u] = static_cast<std::uint8_t>(next);
    return true;
}

// The ROM is free to produce its old character voice, but NONE of those
// unmarked A/C-family announcements or 0A00 sprite voices can get through.
// Only our explicitly tagged, exact-name request for an accepted pick
// is allowed. Confirmation uses the side-specific 82/83 command but is
// likewise authorized once per actual pick, not once per input repeat.
inline bool AllowAuthoritativeSelectionSound(
    std::uint8_t *request, const SelectionNames &expectedNames,
    SelectionTickets &nameTickets, SelectionTickets &confirmTickets) {
    if (!request) return false;
    if (request[0] == 0u && request[1] == 0u && request[2] == 0u &&
        request[4] == 0u && request[5] == 0u &&
        request[6] == 0x10u && request[7] == 0u &&
        request[8] == kSelectTagA && request[9] == kSelectTagB) {
        for (unsigned n = 12u; n < 16u; ++n)
            if (request[n] != 0u) return false;
        const unsigned side = request[10], pick = request[11];
        if (side >= 2u || pick >= 2u) return false;
        const unsigned code = request[3];
        if (code == expectedNames[side][pick] &&
            nameTickets[side][pick] != 0u) {
            --nameTickets[side][pick];
        } else if (code == 0x82u + side &&
                   confirmTickets[side][pick] != 0u) {
            --confirmTickets[side][pick];
        } else {
            return false;
        }
        // The sound CPU must see the game's original native format.
        request[8] = request[9] = request[10] = request[11] = 0u;
        return true;
    }
    if (SelectionSoundEnvelope(request, 0x10u) && request[2] == 0u) {
        const unsigned code = request[3];
        // Player-specific navigation is unrelated to speech.
        if (code == 0x84u || code == 0x85u) return true;
        // The ROM's original confirmations and A/C voice families are
        // suppressed: only the exact tagged versions above play.
        if (code == 0x82u || code == 0x83u ||
            (code >= 0xA0u && code <= 0xDFu)) return false;
    }
    if (SelectionSoundEnvelope(request, 0x0Au))
        return false; // native sprite speech during character selection
    // Pass non-character sounds, soundtrack control, etc., unchanged.
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
