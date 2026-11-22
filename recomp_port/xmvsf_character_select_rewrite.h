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

// 87.518: One character, one voice. The canonical command is IN THE
// SAME RECORD as its character ROM ID and portrait, never computed from
// P1/P2 ownership, team slot, original ROM cursor or sound source bank.
inline constexpr std::uint16_t NativeCharacterVoice(std::uint16_t id) {
    const int slot = Find(id);
    return slot < 0 ? 0xFFFFu :
        kSlots[static_cast<unsigned>(slot)].nameVoice;
}

// An alternate Chun-Li portrait speaks Chun-Li's recorded name; both
// records explicitly contain AB. This helper remains for legacy checks.
inline constexpr std::uint16_t CharacterVoiceIdentity(std::uint16_t id) {
    return id == 0x24u ? 0x16u : id;
}

inline bool SelectionSoundEnvelope(const std::uint8_t *p, unsigned kind) {
    if (!p || p[0] || p[1] || p[4] || p[5] ||
        p[6] != kind || p[7]) return false;
    for (unsigned i = 8u; i < 16u; ++i)
        if (p[i]) return false;
    return true;
}

// This is the exact unmarked command format already used by QSound.
// It goes DIRECTLY to the Machine's shared sound transport; no private
// metadata is stored in the guest ROM queue and no M68K copy logic is
// expected to preserve such metadata.
inline bool MakeSelectionSoundRequest(std::uint8_t *request, std::size_t size,
                                     std::uint16_t code) {
    if (!request || size < 16u || code > 0xFFu) return false;
    for (unsigned i = 0u; i < 16u; ++i) request[i] = 0u;
    request[3] = static_cast<std::uint8_t>(code);
    request[6] = 0x10u;
    return true;
}

// All character announcements in the unmodified ROM are suppressed
// at the completed 61801E sound request. The ONE authoritative voice
// for each accepted pick is sent separately as an ordinary request by
// Choosing's scanline callback. The caller's explicit-send flag is
// required to bypass this filter for that request only.
// Keep the real 84/85 navigation and all unrelated UI/music commands.
inline bool AllowNativeSelectionSound(std::uint8_t *request) {
    if (!request) return false;
    if (SelectionSoundEnvelope(request, 0x10u) && request[2] == 0u) {
        const unsigned code = request[3];
        if (code == 0x82u || code == 0x83u ||
            (code >= 0xA0u && code <= 0xDFu))
            return false;
    }
    if (SelectionSoundEnvelope(request, 0x0Au))
        return false; // native character sprite speech
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
