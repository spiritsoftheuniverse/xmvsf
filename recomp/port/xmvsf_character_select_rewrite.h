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

// 87.519: character-name command IDs A0..B3 and C0..D3 were captured
// in the guest's ORIGINAL FF2648 sound ring. They are NOT proved to be
// the byte layout used at 618000 by the sound CPU. Do NOT bypass the
// game's native sound-request transfer with LeaveSoundRequest.
//
// Instead rewrite the one-byte character-name command when the ROM
// WRITES it into a 16-byte FF2648 ring entry, before the ROM's own
// transfer routine consumes it. The source command family tells us
// which selecting side produced the request, never which recording to
// play. Both sides are rewritten to the SAME canonical A-family code
// obtained from their selected ROM character ID.
inline std::uint16_t RewriteSelectionNameRingWrite(
    std::uint32_t address, std::uint16_t value, unsigned bytes,
    const std::uint8_t *ram, std::size_t ramSize,
    const std::array<std::uint16_t, 2> &selected) {
    if (!ram || ramSize < 0x10000u ||
        (bytes != 1u && bytes != 2u)) return value;
    address &= 0xFFFFFFu;
    if (address < 0xFF2648u || address >= 0xFF2A48u)
        return value;
    const unsigned offset = address - 0xFF2648u;
    if (bytes == 1u && (offset & 15u) != 3u) return value;
    if (bytes == 2u && (offset & 15u) != 2u) return value;
    const unsigned native = value & 0xFFu;
    // Native first/second P1 sound sources can include B3 even when
    // the custom grid contains no B3 recording. The source is not an
    // authority for the resulting character voice.
    const unsigned side =
        native >= 0xA0u && native <= 0xB3u ? 0u :
        native >= 0xC0u && native <= 0xD3u ? 1u : 2u;
    if (side >= 2u) return value;
    // The high byte belongs to the 16-bit sound code. 03BB and
    // similar character-owned sprite commands are NOT announcements.
    if (bytes == 2u) {
        if ((value & 0xFF00u) != 0u) return value;
    } else if (ram[0x2648u + offset - 1u] != 0u) {
        return value;
    }
    const std::uint16_t name = NativeCharacterVoice(selected[side]);
    if (name == 0xFFFFu) return value;
    return static_cast<std::uint16_t>((value & 0xFF00u) | name);
}


// The byte-write filter alone cannot see every queued voice: the guest may
// assemble or copy a complete request in RAM before advancing FF2642.
// Inspect COMPLETED 16-byte requests at the native queue commit boundary,
// before the ROM consumes them. The original A/B or C/D-family code still
// identifies the player; only then is it replaced with that player's
// custom selector's canonical voice. Never rescan a rewritten entry as if
// its new A-family command were a fresh P1 request.
inline unsigned RewriteQueuedSelectionNames(
    std::uint16_t before, std::uint16_t after,
    std::uint8_t *ram, std::size_t ramSize,
    const std::array<std::uint16_t, 2> &selected) {
    if (!ram || ramSize < 0x10000u ||
        before > 0x3F0u || after > 0x3F0u ||
        (before & 15u) || (after & 15u)) return 0u;
    const unsigned distance = (static_cast<unsigned>(after) + 0x400u -
                               static_cast<unsigned>(before)) & 0x3F0u;
    // A queue-pointer byte update can briefly look like an enormous
    // forward jump (e.g. 03F0 -> 00F0 before the low byte reaches 0000).
    // Never reinterpret stale ring slots as fresh announcements.
    if (distance > 0x80u) return 0u;
    unsigned corrected = 0u;
    for (unsigned delta = 0u; delta < distance; delta += 16u) {
        const unsigned offset = (static_cast<unsigned>(before) + delta) & 0x3F0u;
        std::uint8_t *const entry = ram + 0x2648u + offset;
        if (!SelectionSoundEnvelope(entry, 0x10u)) continue;
        const std::uint16_t native = static_cast<std::uint16_t>(
            (static_cast<unsigned>(entry[2]) << 8u) | entry[3]);
        const std::uint16_t replacement = RewriteSelectionNameRingWrite(
            0xFF264Au + offset, native, 2u, ram, ramSize, selected);
        if (replacement == native) continue;
        entry[2] = static_cast<std::uint8_t>(replacement >> 8u);
        entry[3] = static_cast<std::uint8_t>(replacement);
        ++corrected;
    }
    return corrected;
}

// The ROM's actual FF2648 ring is now the only producer of name speech.
// The Z80 shared-RAM stage is reserved for the ROM's own conversion.
// Only mute separately queued character-owned 0A00 sprite speech here.
// Never block names A?/C? at the sound transport: we just corrected
// their source in the ring, and their transfer format is ROM-owned.
inline bool AllowNativeSelectionSound(const std::uint8_t *request) {
    if (!request) return false;
    if (SelectionSoundEnvelope(request, 0x0Au))
        return false;
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
