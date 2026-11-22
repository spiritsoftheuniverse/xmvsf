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
inline constexpr std::uint16_t NativeCharacterVoice(std::uint16_t id,
                                                    unsigned side = 0u) {
    return (side < 2u && Find(id) >= 0 && (id & 1u) == 0u)
        ? static_cast<std::uint16_t>(0xA0u + side * 0x20u + id / 2u)
        : 0xFFFFu;
}

// Rewrites just byte +3 of one already-enqueued native character-voice
// message. P1 uses A0..B2; P2 uses C0..D2. The cursor, queue pointers,
// sound transport, and native 0x82 (P1) / 0x83 (P2) confirmation SFX
// are never changed. Returns true for a recognized
// voice request even if it was already correct; false for other sounds.
inline bool CorrectQueuedCharacterVoice(std::uint8_t *ram, std::size_t size,
                                        unsigned queueOffset,
                                        std::uint16_t chosenRomId,
                                        unsigned side = 0u) {
    if (!ram || size < 0x10000u || queueOffset > 0x3F0u ||
        (queueOffset & 15u) != 0u) return false;
    const std::uint16_t wanted = NativeCharacterVoice(chosenRomId, side);
    if (wanted == 0xFFFFu) return false;
    std::uint8_t *p = ram + 0x2648u + queueOffset;
    // All 15 other bytes must match the observed native announcer form.
    if (p[0] || p[1] || p[2] || p[4] || p[5] ||
        p[6] != 0x10u || p[7]) return false;
    for (unsigned i = 8; i < 16; ++i)
        if (p[i]) return false;
    const unsigned base = 0xA0u + side * 0x20u;
    if (p[3] < base || p[3] > base + 0x12u ||
        Find(static_cast<std::uint16_t>((p[3] - base) * 2u)) < 0)
        return false;
    p[3] = static_cast<std::uint8_t>(wanted);
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
