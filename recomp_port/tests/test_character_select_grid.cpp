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
static_assert(NativeCharacterVoice(0x08, 1u) == 0xC4); // F10 P2 Gambit
static_assert(NativeCharacterVoice(0x00, 1u) == 0xC0); // F10 P2 Wolverine
static_assert(NativeCharacterVoice(0x22, 1u) == 0xD1); // P2 Cammy
static_assert(NativeCharacterVoice(0x22, 0u) == 0xB1); // P1 Cammy
static_assert(NativeCharacterVoice(0x08, 2u) == 0xFFFF); // no third player
// 87.513: an alternate Chun-Li costume shares the real Chun-Li voice,
// not the unverified B2/D2 sequence-table entry.
static_assert(CharacterVoiceIdentity(0x24) == 0x16);
static_assert(NativeCharacterVoice(0x24, 0u) == 0xAB);
static_assert(NativeCharacterVoice(0x24, 1u) == 0xCB);

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
    // 87.511: P2 uses a DIFFERENT character-voice family C0..D2.
    // F10 proves Gambit=C4 and Wolverine=C0 on native P2 picks.
    voiceRequest(0x80u, 0xC4u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0x80u, 0x22u, 1u));
    assert(ram[ringBase + 0x80u + 3u] == 0xD1u); // Cammy in P2
    assert(ram[ringBase + 0x80u + 6u] == 0x10u);
    // Don't accidentally rewrite a P1 request using the P2 family.
    voiceRequest(0x90u, 0xA4u);
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0x90u, 0x22u, 1u));
    assert(ram[ringBase + 0x90u + 3u] == 0xA4u);
    voiceRequest(0xA0u, 0xC4u);
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xA0u, 0x22u, 0u));
    assert(ram[ringBase + 0xA0u + 3u] == 0xC4u);
    // Native side-specific confirmation sounds MUST remain (87.510
    // incorrectly attempted to remove P1 82).
    voiceRequest(0xB0u, 0x82u);
    voiceRequest(0xC0u, 0x83u);
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xB0u, 0x06u, 0u));
    assert(!CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xC0u, 0x06u, 1u));
    assert(ram[ringBase + 0xB0u + 3u] == 0x82u);
    assert(ram[ringBase + 0xC0u + 3u] == 0x83u);

    // 87.513 F10: second-pick native announcements can land outside the
    // nominal grid source. Capture #3: Storm 0x04 vs native B3 (0x26).
    voiceRequest(0xD0u, 0xB3u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xD0u, 0x04u, 0u));
    assert(ram[ringBase + 0xD0u + 3u] == 0xA2u);
    // Capture #5: Alpha Chun-Li selected, native B2 spoke wrong;
    // the alternate must use Chun-Li's stock AB voice.
    voiceRequest(0xE0u, 0xB2u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xE0u, 0x24u, 0u));
    assert(ram[ringBase + 0xE0u + 3u] == 0xABu);
    // Akuma's native AF when custom Juggernaut is chosen: A6, not AF.
    voiceRequest(0xF0u, 0xAFu);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0xF0u, 0x0Cu, 0u));
    assert(ram[ringBase + 0xF0u + 3u] == 0xA6u);
    // P2 side uses D3 source and C-family target, not A-family.
    voiceRequest(0x100u, 0xD3u);
    assert(CorrectQueuedCharacterVoice(ram.data(), ram.size(), 0x100u, 0x0Cu, 1u));
    assert(ram[ringBase + 0x100u + 3u] == 0xC6u);

    // The F10 Akuma case has an extra native character-owned 03BB
    // command immediately after AF,82, while the native pick is Cammy.
    // Its 0x20-wide sound block maps Cammy 0x3A0 -> Akuma 0x360,
    // retaining the observed selection-speech slot 0x1B.
    std::array<std::uint8_t, 65536> spriteRam{};
    const auto spriteCommand = [&](unsigned offset, unsigned code) {
        auto *p = spriteRam.data() + ringBase + offset;
        for (unsigned i = 0u; i < 16u; ++i) p[i] = 0u;
        p[2] = static_cast<std::uint8_t>(code >> 8u);
        p[3] = static_cast<std::uint8_t>(code);
        p[6] = 0x0Au;
    };
    spriteCommand(0u, 0x03BBu);
    assert(CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 0u, 0x1Eu));
    assert(spriteRam[ringBase + 2u] == 0x03u);
    assert(spriteRam[ringBase + 3u] == 0x7Bu); // Akuma 037B
    assert(spriteRam[ringBase + 6u] == 0x0Au);
    spriteCommand(16u, 0x037Bu);
    assert(CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 16u, 0x0Cu));
    assert(spriteRam[ringBase + 16u + 2u] == 0x02u);
    assert(spriteRam[ringBase + 16u + 3u] == 0x5Bu); // Juggernaut 025B
    spriteCommand(32u, 0x03BBu);
    assert(CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 32u, 0x24u));
    assert(spriteRam[ringBase + 32u + 2u] == 0x02u);
    assert(spriteRam[ringBase + 32u + 3u] == 0xFBu); // Chun-Li 02FB
    // A correctly-owned voice must remain byte-identical.
    spriteCommand(48u, 0x037Bu);
    assert(CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 48u, 0x1Eu));
    assert(spriteRam[ringBase + 48u + 3u] == 0x7Bu);
    // 82/83 confirm SFX, 84/85 movement and other character sound
    // slots must never be treated as the selected sprite's voice.
    spriteCommand(64u, 0x0082u);
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 64u, 0x1Eu));
    spriteCommand(80u, 0x0084u);
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 80u, 0x1Eu));
    spriteCommand(96u, 0x03BAu);
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 96u, 0x1Eu));
    spriteCommand(112u, 0x03BBu);
    spriteRam[ringBase + 112u + 7u] = 1u;
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 112u, 0x1Eu));
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), spriteRam.size(), 0x400u, 0x1Eu));
    assert(!CorrectQueuedCharacterSpriteVoice(
        spriteRam.data(), 0x100u, 0u, 0x1Eu));

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
    // 87.511: P2 already produces 0x85 on movement, not 0x84.
    // Emitting P1's 0x84 on top of it caused the louder/additive sound.
    command(0x80u, 0x85u);
    q[0x2643u] = 0x90u;
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(),
                                            0x80u, 0x90u, 0x85u));
    assert(q[0x2643u] == 0x90u);
    // If P2's native tick is genuinely absent, add one native 0x85.
    assert(QueueMissingCursorMovementSound(q.data(), q.size(),
                                           0x90u, 0x90u, 0x85u));
    assert(q[0x2643u] == 0xA0u);
    assert(q[0x2648u + 0x90u + 3u] == 0x85u);
    // P1's tick remains distinct, so neither sounds twice.
    command(0xA0u, 0x84u);
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(),
                                            0xA0u, 0xB0u, 0x84u));
    assert(!QueueMissingCursorMovementSound(q.data(), q.size(),
                                            0xA0u, 0xB0u, 0x86u));
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
