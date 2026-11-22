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
// 0x82 companion request are not changed by THIS announcer helper.
// Returns true for a recognized voice request even if already correct;
// false for all other sounds.
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

// 87.510: the native selection routine pairs every character-specific
// announcer command A0..B2 with one trailing 0x82 request. F10 confirmed
// Magneto A7,82 (frame 648) and wrong Gambit A4,82 (frame 884).
// 0x82 is the candidate for the unwanted original sprite voice when the
// native sprite and the custom selection do not agree.
//
// Only during a native character confirmation (the caller owns the gate),
// after the announcer was recognized/corrected, remove that exact trailing
// 0x82 message BEFORE the sound CPU processes it. Rewind the producer by
// one entry; do not enqueue a zero sound command, adjust the consumer, or
// touch the neighboring announcer and 0x84 cursor-movement commands.
// A request must be immediately adjacent to the corrected announcer and
// the final queued request; otherwise leave the native queue untouched.
inline bool SuppressTrailingSelectionSpriteVoice(std::uint8_t *ram, std::size_t size,
                                                 unsigned announcerOffset,
                                                 unsigned companionOffset,
                                                 unsigned producerNext,
                                                 std::uint16_t selectedRomId) {
    if (!ram || size < 0x10000u ||
        announcerOffset > 0x3F0u || companionOffset > 0x3F0u ||
        producerNext > 0x3F0u ||
        (announcerOffset & 15u) || (companionOffset & 15u) ||
        (producerNext & 15u)) return false;
    if (((announcerOffset + 16u) & 0x3F0u) != companionOffset ||
        ((companionOffset + 16u) & 0x3F0u) != producerNext)
        return false;
    // Never remove a second request that does not follow the expected
    // character-name announcer for this very same confirmation.
    const std::uint16_t announcer = NativeCharacterVoice(selectedRomId);
    if (announcer == 0xFFFFu) return false;
    const std::uint8_t *voice = ram + 0x2648u + announcerOffset;
    const std::uint8_t *companion = ram + 0x2648u + companionOffset;
    const auto matches = [](const std::uint8_t *entry, unsigned id) {
        if (entry[0] || entry[1] || entry[2] || entry[3] != id ||
            entry[4] || entry[5] || entry[6] != 0x10u || entry[7])
            return false;
        for (unsigned i = 8u; i < 16u; ++i)
            if (entry[i]) return false;
        return true;
    };
    if (!matches(voice, announcer) || !matches(companion, 0x82u))
        return false;
    // This entry was most recently produced by the main CPU in the current
    // scanline; the native sound CPU has not executed its next slice yet.
    // Unpublishing the tail is preferable to a fabricated no-op command.
    for (unsigned i = 0u; i < 16u; ++i)
        ram[0x2648u + companionOffset + i] = 0u;
    ram[0x2642u] = static_cast<std::uint8_t>(companionOffset >> 8u);
    ram[0x2643u] = static_cast<std::uint8_t>(companionOffset);
    return true;
}

// 87.509: preserve the board's own character-grid movement tick (0x84).
// F10 confirmed this exact 16-byte request on native movement:
// 00 00 00 84 00 00 10 00 00 00 00 00 00 00 00 00.
// If a custom grid movement occurs while the ROM is in its inter-pick
// animation, the ROM may not queue that tick itself. Append ONLY when no
// identical native request was produced during this CPU frame.
//
// Sound requests are queued as the original game queues them; playback
// occurs on the following native sound processing, not via a new ROM hook.
inline bool QueueMissingCursorMovementSound(std::uint8_t *ram, std::size_t size,
                                             std::uint16_t before,
                                             std::uint16_t after) {
    if (!ram || size < 0x10000u ||
        before > 0x3F0u || after > 0x3F0u ||
        (before & 15u) || (after & 15u)) return false;
    const unsigned distance = (static_cast<unsigned>(after) + 0x400u -
                               static_cast<unsigned>(before)) & 0x3F0u;
    // Never guess if the queue moved implausibly far in one native
    // frame or has wrapped so far that its contents might be overwritten.
    if (distance > 128u) return false;
    const auto nativeMoveTick = [](const std::uint8_t *p) {
        if (p[0] || p[1] || p[2] || p[3] != 0x84u ||
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
    entry[3] = 0x84u;
    entry[6] = 0x10u;
    const std::uint16_t next = static_cast<std::uint16_t>(
        (static_cast<unsigned>(after) + 16u) & 0x3F0u);
    ram[0x2642u] = static_cast<std::uint8_t>(next >> 8);
    ram[0x2643u] = static_cast<std::uint8_t>(next);
    return true;
}

} // namespace port::xmvsf_select
