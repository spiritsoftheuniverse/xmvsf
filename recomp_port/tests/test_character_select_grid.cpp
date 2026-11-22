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
// 87.516: player ownership NEVER changes a character's name sample.
// The native P2 C-family commands are filtered; canonical names use A?.
static_assert(NativeCharacterVoice(0x08) == 0xA4); // Gambit for either side
static_assert(NativeCharacterVoice(0x00) == 0xA0); // Wolverine for either side
static_assert(NativeCharacterVoice(0x22) == 0xB1); // Cammy for either side
// 87.513: an alternate Chun-Li costume shares the real Chun-Li voice,
// not the unverified B2/D2 sequence-table entry.
static_assert(CharacterVoiceIdentity(0x24) == 0x16);
static_assert(NativeCharacterVoice(0x24) == 0xAB); // Alpha Chun-Li = Chun-Li

static_assert(Navigate(0, 1) == 1 && Navigate(8, 1) == 0);
static_assert(Navigate(9, 1) == 10 && Navigate(9, 8) == 0);
int main() {
    // 87.518: the actual character table contains the literal AUDIO
    // assignment for every one of the 18 portraits. Absolutely no
    // P1/P2 or first/second transformation can be applied to this ID.
    constexpr std::array<std::uint16_t, 18> expectedVoices{{
        0xA1u, 0xA0u, 0xA4u, 0xA3u, 0xA5u, 0xA2u,
        0xA7u, 0xA6u, 0xAFu, 0xA9u, 0xAAu, 0xB1u,
        0xB0u, 0xABu, 0xABu, 0xADu, 0xACu, 0xAEu
    }};
    for (std::size_t slot = 0u; slot < kSlots.size(); ++slot) {
        assert(kSlots[slot].nameVoice == expectedVoices[slot]);
        assert(NativeCharacterVoice(kSlots[slot].rom) == expectedVoices[slot]);
        for (unsigned side = 0u; side < 2u; ++side) {
            for (unsigned pick = 0u; pick < 2u; ++pick) {
                (void)side; (void)pick;
                const auto sound = NativeCharacterVoice(kSlots[slot].rom);
                assert(sound == expectedVoices[slot]);
            }
        }
    }
    // Explicit cross-character and selection-order regressions:
    // Gambit A4 for P1 pick 1, P1 pick 2, P2 pick 1 and P2 pick 2.
    for (unsigned side = 0u; side < 2u; ++side)
        for (unsigned pick = 0u; pick < 2u; ++pick) {
            (void)side; (void)pick;
            assert(NativeCharacterVoice(0x08u) == 0xA4u);
            assert(NativeCharacterVoice(0x04u) == 0xA2u);
            assert(NativeCharacterVoice(0x1Eu) == 0xAFu);
            assert(NativeCharacterVoice(0x0Cu) == 0xA6u);
        }
    assert(NativeCharacterVoice(0x10u) == 0xFFFFu); // no Apocalypse
    assert(NativeCharacterVoice(0x25u) == 0xFFFFu);

    // The injection request always uses the original QSound envelope,
    // without ROM ring provenance tags or guessed sprite sound blocks.
    std::array<std::uint8_t, 16> sound{};
    assert(MakeSelectionSoundRequest(sound.data(), sound.size(), 0xA4u));
    assert(sound[3] == 0xA4u);
    assert(sound[6] == 0x10u);
    for (unsigned i = 0u; i < sound.size(); ++i) {
        if (i != 3u && i != 6u) assert(sound[i] == 0u);
    }
    assert(SelectionSoundEnvelope(sound.data(), 0x10u));
    assert(!MakeSelectionSoundRequest(nullptr, sound.size(), 0xA4u));
    assert(!MakeSelectionSoundRequest(sound.data(), 15u, 0xA4u));
    assert(!MakeSelectionSoundRequest(sound.data(), sound.size(), 0x100u));

    // Native announcements from either side can NEVER supply our
    // character voice. The separate, direct, original-format request
    // is what the injected QSound path plays.
    for (const unsigned code : {0xA2u, 0xA4u, 0xAFu, 0xB3u,
                                0xC2u, 0xC4u, 0xCFu, 0xD3u,
                                0x82u, 0x83u}) {
        assert(MakeSelectionSoundRequest(sound.data(), sound.size(), code));
        assert(!AllowNativeSelectionSound(sound.data()));
    }
    for (const unsigned code : {0x84u, 0x85u, 0x40u}) {
        assert(MakeSelectionSoundRequest(sound.data(), sound.size(), code));
        assert(AllowNativeSelectionSound(sound.data()));
    }
    sound.fill(0u);
    sound[2] = 0x03u;
    sound[3] = 0xBBu; // native Cammy sprite speech
    sound[6] = 0x0Au;
    assert(SelectionSoundEnvelope(sound.data(), 0x0Au));
    assert(!AllowNativeSelectionSound(sound.data()));
    sound.fill(0u);
    sound[3] = 0x40u; // unrelated sound control request
    sound[6] = 0x20u;
    assert(AllowNativeSelectionSound(sound.data()));
    assert(!AllowNativeSelectionSound(nullptr));

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
