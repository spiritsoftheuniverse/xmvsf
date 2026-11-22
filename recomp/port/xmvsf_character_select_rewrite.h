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
// The expected voice for Rogue 06 is therefore 0xA3.
// This helper is called only from the optional between-CPU/sound scanline
// callback while a real native confirmation button is held.
inline constexpr std::uint16_t NativeCharacterVoice(std::uint16_t id) {
    return (Find(id) >= 0 && (id & 1u) == 0u)
        ? static_cast<std::uint16_t>(0xA0u + id / 2u)
        : 0xFFFFu;
}

// Rewrites just byte +3 of one already-enqueued native character-voice
// message. The cursor, queue pointers, sound transport, and native
// 0x82 confirmation SFX are never changed. Returns true for a recognized
// voice request even if it was already correct; false for other sounds.
inline bool CorrectQueuedCharacterVoice(std::uint8_t *ram, std::size_t size,
                                        unsigned queueOffset,
                                        std::uint16_t chosenRomId) {
    if (!ram || size < 0x10000u || queueOffset > 0x3F0u ||
        (queueOffset & 15u) != 0u) return false;
    const std::uint16_t wanted = NativeCharacterVoice(chosenRomId);
    if (wanted == 0xFFFFu) return false;
    std::uint8_t *p = ram + 0x2648u + queueOffset;
    // All 15 other bytes must match the observed native announcer form.
    if (p[0] || p[1] || p[2] || p[4] || p[5] ||
        p[6] != 0x10u || p[7]) return false;
    for (unsigned i = 8; i < 16; ++i)
        if (p[i]) return false;
    if (p[3] < 0xA0u || p[3] > 0xB2u ||
        Find(static_cast<std::uint16_t>((p[3] - 0xA0u) * 2u)) < 0)
        return false;
    p[3] = static_cast<std::uint8_t>(wanted);
    return true;
}

} // namespace port::xmvsf_select
