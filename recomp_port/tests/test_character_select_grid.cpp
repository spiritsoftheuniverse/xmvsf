#include "../../recomp/port/xmvsf_character_select_layout.h"
#include "../../recomp/port/xmvsf_character_select_rewrite.h"
#include <array>
#include <cassert>
#include <set>
using namespace port::xmvsf_select;
static_assert(kSlots.size() == 18);
static_assert(X(0) == 43 && X(8) == 307 && X(9) == 43 && X(17) == 307);
static_assert(Y(0) == 136 && Y(9) == 175);
static_assert(Find(0x02) == 0 && Find(0x00) == 1);
static_assert(Find(0x12) == 9 && Find(0x14) == 10);
static_assert(Find(0x1e) == 8 && Find(0x24) == 14);
static_assert(Find(0x10) == -1);
// 87.508 F10 native voice mapping. Full announcer request format is
// validated before any queued byte is changed (no M68K execution hook).
static_assert(NativeCharacterVoice(0x0E) == 0xA7); // Magneto, captured
static_assert(NativeCharacterVoice(0x08) == 0xA4); // Gambit, captured
static_assert(NativeCharacterVoice(0x06) == 0xA3); // Rogue, intended
static_assert(NativeCharacterVoice(0x10) == 0xFFFF); // no Apocalypse slot
static_assert(NativeCharacterVoice(0x25) == 0xFFFF);

static_assert(Navigate(0, 1) == 1 && Navigate(8, 1) == 0);
static_assert(Navigate(9, 1) == 10 && Navigate(9, 8) == 0);
int main() {
    // F10 frame 884: wrong Gambit voice when Rogue was confirmed.
    std::array<std::uint8_t, 65536> ram{};
    constexpr unsigned ringBase = 0x2648u;
    const auto voiceRequest = [&](unsigned offset, std::uint8_t voice) {
        auto *p = ram.data() + ringBase + offset;
        for (unsigned n = 0; n < 16u; ++n) p[n] = 0u;
        p[3] = voice;
        p[6] = 0x10u;
    };
    voiceRequest(0u, 0xA4u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0u, 0x06u));
    assert(ram[ringBase + 3u] == 0xA3u); // Rogue
    assert(ram[ringBase + 6u] == 0x10u); // native envelope preserved
    // Correct Magneto voice remains untouched.
    voiceRequest(16u, 0xA7u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 16u, 0x0Eu));
    assert(ram[ringBase + 16u + 3u] == 0xA7u);
    // The common post-announcement SFX 0x82 and all ordinary SFX
    // remain completely untouched.
    voiceRequest(32u, 0x82u);
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 32u, 0x06u));
    assert(ram[ringBase + 32u + 3u] == 0x82u);
    // Invalid envelope/ID/pointers must not be edited.
    voiceRequest(48u, 0xA4u);
    ram[ringBase + 48u + 7u] = 1u;
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 48u, 0x06u));
    assert(ram[ringBase + 48u + 3u] == 0xA4u);
    voiceRequest(64u, 0xA8u); // Apocalypse has no slot
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 64u, 0x06u));
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0x400u, 0x06u));
    assert(!CorrectQueuedCharacterVoice(ram.data(), 0x100u, 0u, 0x06u));
    // Last entry of ring is still 16 bytes within the 0x400 region.
    voiceRequest(0x3F0u, 0xA4u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0x3F0u, 0x06u));
    assert(ram[ringBase + 0x3F0u + 3u] == 0xA3u);

    // 87.510: eliminate only the 0x82 companion to a confirmed/corrected
    // native character announcer; preserve the correct voice and any
    // subsequent 0x84 movement. Never erase unrelated queued sound.
    std::array<std::uint8_t, 65536> spriteQueue{};
    auto &sp = spriteQueue;
    const auto spriteRequest = [&](unsigned at, unsigned id) {
        auto *entry = sp.data() + 0x2648u + at;
        for (unsigned i = 0u; i < 16u; ++i) entry[i] = 0u;
        entry[3] = static_cast<std::uint8_t>(id);
        entry[6] = 0x10u;
    };
    spriteRequest(0u, 0xA4u); // native incorrect Gambit announcer
    spriteRequest(16u, 0x82u); // native sprite voice companion
    sp[0x2643u] = 32u;
    assert(CorrectQueuedCharacterVoice(sp.data(), sp.size(), 0u, 0x06u));
    assert(SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 0u, 16u, 32u, 0x06u));
    assert(sp[0x2648u + 3u] == 0xA3u); // corrected Rogue announcer retained
    assert(sp[0x2643u] == 16u); // producer rewound exactly one entry
    for (unsigned i = 0u; i < 16u; ++i)
        assert(sp[0x2648u + 16u + i] == 0u);
    // 87.509 handoff movement is still playable after unpublishing the
    // native sprite voice; it occupies the newly freed slot (not A3).
    assert(QueueMissingCursorMovementSound(sp.data(), sp.size(), 0u, 16u));
    assert(sp[0x2648u + 3u] == 0xA3u);
    assert(sp[0x2648u + 16u + 3u] == 0x84u);
    assert(sp[0x2643u] == 32u);
    // Already-correct Magneto must work the same way.
    spriteRequest(32u, 0xA7u);
    spriteRequest(48u, 0x82u);
    sp[0x2643u] = 64u;
    assert(SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 32u, 48u, 64u, 0x0Eu));
    assert(sp[0x2648u + 32u + 3u] == 0xA7u);
    assert(sp[0x2643u] == 48u);
    // A stray 0x82 cannot be deleted without a matching announcer.
    spriteRequest(64u, 0x84u);
    spriteRequest(80u, 0x82u);
    sp[0x2643u] = 96u;
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 64u, 80u, 96u, 0x06u));
    assert(sp[0x2648u + 80u + 3u] == 0x82u && sp[0x2643u] == 96u);
    // Do not rewind when another native event is queued afterward.
    spriteRequest(96u, 0xA3u);
    spriteRequest(112u, 0x82u);
    spriteRequest(128u, 0x84u);
    sp[0x2643u] = 144u;
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 96u, 112u, 144u, 0x06u));
    assert(sp[0x2648u + 128u + 3u] == 0x84u);
    assert(sp[0x2643u] == 144u);
    // If the name did not match the confirmed selection, do not erase
    // the native companion before the announcer correction is validated.
    spriteRequest(160u, 0xA4u);
    spriteRequest(176u, 0x82u);
    sp[0x2643u] = 192u;
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 160u, 176u, 192u, 0x06u));
    assert(sp[0x2643u] == 192u);
    // Corrupt companion format must be left alone; never guess by ID.
    spriteRequest(208u, 0xA3u);
    spriteRequest(224u, 0x82u);
    sp[0x2648u + 224u + 7u] = 1u;
    sp[0x2643u] = 240u;
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 208u, 224u, 240u, 0x06u));
    assert(sp[0x2643u] == 240u);
    // Ring-wrap pair: voice at 3F0, companion at 000, next at 010.
    spriteRequest(0x3F0u, 0xA3u);
    spriteRequest(0u, 0x82u);
    sp[0x2642u] = 0u;
    sp[0x2643u] = 16u;
    assert(SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 0x3F0u, 0u, 16u, 0x06u));
    assert(sp[0x2642u] == 0 && sp[0x2643u] == 0);
    assert(sp[0x2648u + 0x3F0u + 3u] == 0xA3u);
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 1u, 0u, 16u, 0x06u));
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), 256u, 0x3F0u, 0u, 16u, 0x06u));
    assert(!SuppressTrailingSelectionSpriteVoice(
        sp.data(), sp.size(), 0x3F0u, 0x400u, 16u, 0x06u));

    // 87.509: reproduce frame 648 first confirmation: A7 voice and
    // 82 confirmation SFX already enqueued, but the automatic 1-slot
    // visual handoff did NOT enqueue the usual 84 movement tick.
    std::array<std::uint8_t, 65536> cursorRing{};
    auto &q = cursorRing;
    const auto command = [&](unsigned at, unsigned id) {
        auto *entry = q.data() + 0x2648u + at;
        for (unsigned i = 0u; i < 16u; ++i) entry[i] = 0;
        entry[3] = static_cast<std::uint8_t>(id);
        entry[6] = 0x10u;
    };
    command(0u, 0xA7u); // Magneto, first voice confirmation
    command(16u, 0x82u); // ROM's ordinary confirmation SFX
    q[0x2643u] = 32u;
    assert(QueueMissingCursorMovementSound(q.data(), q.size(), 0u, 32u));
    assert(q[0x2643u] == 48u);
    assert(q[0x2648u + 3u] == 0xA7u); // voice untouched
    assert(q[0x2648u + 16u + 3u] == 0x82u); // confirmation untouched
    assert(q[0x2648u + 32u + 3u] == 0x84u); // missing move tick
    assert(q[0x2648u + 32u + 6u] == 0x10u);
    // Normal native movement already supplies 84: do NOT double-play it.
    command(48u, 0x84u);
    q[0x2643u] = 64u;
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(), 48u, 64u));
    assert(q[0x2643u] == 64u);
    // No native sound requests at all during a muted movement: append.
    assert(QueueMissingCursorMovementSound(q.data(), q.size(), 64u, 64u));
    assert(q[0x2643u] == 80u);
    assert(q[0x2648u + 64u + 3u] == 0x84u);
    // Validate the last ring entry and wrapped next pointer.
    command(0x3F0u, 0x82u);
    q[0x2642u] = 0;
    q[0x2643u] = 0;
    assert(QueueMissingCursorMovementSound(q.data(), q.size(), 0x3F0u, 0u));
    assert(q[0x2642u] == 0 && q[0x2643u] == 16u);
    assert(q[0x2648u + 3u] == 0x84u);
    // An already-native 84 at the wrap boundary must also deduplicate.
    command(0x3F0u, 0x84u);
    q[0x2642u] = q[0x2643u] = 0;
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(), 0x3F0u, 0u));
    assert(q[0x2642u] == 0 && q[0x2643u] == 0);
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(), 0x3F1u, 0u));
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(), 0u, 0x400u));
    assert(!QueueMissingCursorMovementSound(q.data(), 100u, 0u, 0u));
    // Too many native enqueues is ambiguous: do not add anything.
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(), 0u, 0x90u));

    Navigation n;
    assert(n.index[0] == 9 && n.index[1] == 8);
    n.Advance(0, 1, true);
    assert(n.index[0] == 10);
    n.Advance(0, 1, true);
    assert(n.index[0] == 10);
    n.Advance(0, 8, true);
    assert(n.index[0] == 1);
    n.Advance(1, 2, true);
    assert(n.index[1] == 7);
    n.Reset();
    assert(n.index[0] == 9 && n.index[1] == 8);
    std::set<unsigned> ids;
    for (const auto &slot : kSlots) assert(ids.insert(slot.rom).second);
    // Every native ID must be selectable at the original ROM write point:
    // two players, two team slots, in both byte and word write paths.
    for (const Slot &slot : kSlots) {
        std::array<std::uint16_t,2> selected{{slot.rom, slot.rom}};
        for (unsigned side = 0; side < 2u; ++side) {
            const std::uint32_t cursor = 0xFF518Cu + side * 0x20u;
            for (unsigned pick = 0; pick < 2u; ++pick) {
                std::array<std::uint8_t,2> accepted{{2u,2u}};
                accepted[side] = static_cast<std::uint8_t>(pick);
                const std::uint32_t address = 0xFF50B0u + side * 4u + pick * 2u;
                assert(RewriteNativeSelectionWrite(cursor, 0xFFFF, 2, selected, accepted) == slot.rom);
                assert(RewriteNativeSelectionWrite(address, 0xFFFF, 2, selected, accepted) == slot.rom);
                assert(RewriteNativeSelectionWrite(address, 0xFF, 1, selected, accepted) == (slot.rom >> 8u));
                assert(RewriteNativeSelectionWrite(address + 1, 0xFF, 1, selected, accepted) == (slot.rom & 0xFFu));
                assert(RewriteNativeSelectionWrite(0xFF50B0u + side * 4u + (1u - pick) * 2u, 0xAAAA, 2, selected, accepted) == 0xAAAA);
                // Ordinary memory and completed picks cannot be changed.
                assert(RewriteNativeSelectionWrite(0xFF5104, 0xABCD, 2, selected, accepted) == 0xABCD);
                accepted[side] = 2;
                assert(RewriteNativeSelectionWrite(cursor, 0xAAAA, 2, selected, accepted) == 0xAAAA);
                assert(RewriteNativeSelectionWrite(address, 0xAAAA, 2, selected, accepted) == 0xAAAA);
            }
        }
    }
}
