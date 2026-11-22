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

// 87.521: first teammate's index is skipped for both players.
static_assert(NextTeamIndex(0) == 1);
static_assert(NextTeamIndex(8) == 7);  // right-hand corner
static_assert(NextTeamIndex(9) == 10);
static_assert(NextTeamIndex(17) == 16); // lower-right corner
static_assert(SameTeamCharacter(0x08u, 0x08u));
static_assert(SameTeamCharacter(0x16u, 0x24u)); // both Chun-Li forms
static_assert(!SameTeamCharacter(0x08u, 0x06u));
static_assert(SkipTeamIndex(1, 2, 1u, 0x08u) == 3); // Gambit skipped going right
static_assert(SkipTeamIndex(3, 2, 2u, 0x08u) == 1); // skipped going left
static_assert(SkipTeamIndex(11, 2, 8u, 0x08u) == 11); // vertical blocked
// The timer never authorizes native acceptance. One A press can grant
// only the next pick, and repeated writes of the same count are harmless.
static_assert(!NativePickAllowed(1, 0, 0, false));
static_assert(NativePickAllowed(1, 0, 0, true));
static_assert(!NativePickAllowed(2, 0, 0, true)); // cannot select two with one A
static_assert(NativePickAllowed(1, 0, 1, false)); // same native count repeated
static_assert(!NativePickAllowed(2, 1, 1, false));
static_assert(NativePickAllowed(2, 1, 1, true));
static_assert(NativePickAllowed(0, 0, 0, false)); // cancelled slot
static_assert(Navigate(0, 1) == 1 && Navigate(8, 1) == 0);
static_assert(Navigate(9, 1) == 10 && Navigate(9, 8) == 0);
int main() {
    // 87.519: captured NAME commands are native FF2648 ring bytes.
    // Never copy a ring-format request directly to QSound shared RAM.
    // The ROM's own sound transfer supplies that conversion.
    constexpr std::array<std::uint16_t, 18> expectedVoices{{
        0xA1u, 0xA0u, 0xA4u, 0xA3u, 0xA5u, 0xA2u,
        0xA7u, 0xA6u, 0xAFu, 0xA9u, 0xAAu, 0xB1u,
        0xB0u, 0xABu, 0xABu, 0xADu, 0xACu, 0xAEu
    }};
    std::array<std::uint8_t, 65536> ringRam{};
    for (std::size_t slot = 0; slot < kSlots.size(); ++slot) {
        assert(kSlots[slot].nameVoice == expectedVoices[slot]);
        assert(NativeCharacterVoice(kSlots[slot].rom) == expectedVoices[slot]);
        // Four independently confirmed slots, SAME character voice.
        for (unsigned side = 0u; side < 2u; ++side) {
            for (unsigned pick = 0u; pick < 2u; ++pick) {
                const std::array<std::uint16_t, 2> selected{{
                    kSlots[slot].rom, kSlots[slot].rom
                }};
                const unsigned offset = 16u * pick + side * 32u;
                const std::uint32_t addr = 0xFF264Au + offset;
                const unsigned nativeCode = side ? 0xC4u : 0xA4u;
                assert(RewriteSelectionNameRingWrite(
                    addr, static_cast<std::uint16_t>(nativeCode), 2u,
                    ringRam.data(), ringRam.size(), selected) ==
                        expectedVoices[slot]);
                // RAM byte and word writes must both produce same name.
                assert(RewriteSelectionNameRingWrite(
                    addr + 1u, static_cast<std::uint16_t>(nativeCode), 1u,
                    ringRam.data(), ringRam.size(), selected) ==
                        expectedVoices[slot]);
            }
        }
    }
    // 87.520: exercise the PRODUCTION completed-queue path, not only a
    // hypothetical individual byte/word write at FF264B. Each native
    // announcement must become the SAME voice for all four pick slots.
    for (const Slot &slot : kSlots) {
        for (unsigned side = 0u; side < 2u; ++side) {
            for (unsigned pick = 0u; pick < 2u; ++pick) {
                std::array<std::uint8_t, 65536> ram{};
                const unsigned at = 16u * (pick * 2u + side);
                auto *name = ram.data() + 0x2648u + at;
                name[3] = static_cast<std::uint8_t>(side ? 0xD3u : 0xB3u);
                name[6] = 0x10u;
                auto *confirm = name + 16u;
                confirm[3] = static_cast<std::uint8_t>(side ? 0x83u : 0x82u);
                confirm[6] = 0x10u;
                const std::array<std::uint16_t, 2> selected{{slot.rom, slot.rom}};
                assert(RewriteQueuedSelectionNames(
                    static_cast<std::uint16_t>(at),
                    static_cast<std::uint16_t>(at + 32u),
                    ram.data(), ram.size(), selected) == 1u);
                assert(name[3] == slot.nameVoice);
                assert(name[6] == 0x10u);
                assert(confirm[3] == (side ? 0x83u : 0x82u));
                // Scanner state must advance even when no bytes changed:
                // NEVER revisit a canonicalized P2 name as a P1 entry.
                assert(RewriteQueuedSelectionNames(
                    static_cast<std::uint16_t>(at + 32u),
                    static_cast<std::uint16_t>(at + 32u),
                    ram.data(), ram.size(), selected) == 0u);
            }
        }
    }
    // Two simultaneous accepts: the original native C-family byte
    // identifies P2, while P1's A-family byte identifies P1.
    std::array<std::uint8_t, 65536> paired{};
    paired[0x2648u + 3u] = 0xA4u;
    paired[0x2648u + 6u] = 0x10u;
    paired[0x2648u + 16u + 3u] = 0xC4u;
    paired[0x2648u + 16u + 6u] = 0x10u;
    const std::array<std::uint16_t,2> both{{0x06u, 0x08u}};
    assert(RewriteQueuedSelectionNames(
        0u, 32u, paired.data(), paired.size(), both) == 2u);
    assert(paired[0x2648u + 3u] == 0xA3u); // P1 Rogue
    assert(paired[0x2648u + 16u + 3u] == 0xA4u); // P2 Gambit
    // Ring wrap, partial-pointer jumps and non-announcer voices.
    std::array<std::uint8_t, 65536> wrap{};
    wrap[0x2648u + 0x3F0u + 3u] = 0xD0u;
    wrap[0x2648u + 0x3F0u + 6u] = 0x10u;
    assert(RewriteQueuedSelectionNames(
        0x3F0u, 0u, wrap.data(), wrap.size(), both) == 1u);
    assert(wrap[0x2648u + 0x3F0u + 3u] == 0xA4u);
    assert(RewriteQueuedSelectionNames(
        0x3F0u, 0x00F0u, wrap.data(), wrap.size(), both) == 0u);
    wrap[0x2648u + 3u] = 0xD0u;
    wrap[0x2648u + 6u] = 0x0Au; // sprite voice, never an announcement
    assert(RewriteQueuedSelectionNames(
        0u, 16u, wrap.data(), wrap.size(), both) == 0u);
    assert(wrap[0x2648u + 3u] == 0xD0u);

    // Exact F10 regression: P1 picked Rogue (06) second, but the
    // original ROM queued Gambit A4 at FF2648. Rewrite A4 -> A3 at
    // the native ring BEFORE the M68K transfers it to QSound.
    const std::array<std::uint16_t, 2> p1Rogue{{0x06u, 0x1Eu}};
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x00A4u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A3u);
    // Other captured case: source B3 (native Akuma) while P1 picks
    // Storm; MUST be A2, not B3.
    const std::array<std::uint16_t, 2> p1Storm{{0x04u, 0x1Eu}};
    assert(RewriteSelectionNameRingWrite(
        0xFF265Au, 0x00B3u, 2u, ringRam.data(), ringRam.size(),
        p1Storm) == 0x00A2u);
    // P2 picking Gambit retains A4, never native C4.
    const std::array<std::uint16_t, 2> p2Gambit{{0x06u, 0x08u}};
    assert(RewriteSelectionNameRingWrite(
        0xFF266Au, 0x00C4u, 2u, ringRam.data(), ringRam.size(),
        p2Gambit) == 0x00A4u);
    // P1 *second* Gambit remains A4 even when source is B3.
    const std::array<std::uint16_t, 2> p1Gambit{{0x08u, 0x04u}};
    assert(RewriteSelectionNameRingWrite(
        0xFF267Au, 0x00B3u, 2u, ringRam.data(), ringRam.size(),
        p1Gambit) == 0x00A4u);
    // CPS2 longword/word/byte ranges, queue bounds, absent character.
    assert(RewriteSelectionNameRingWrite(
        0xFF2A3Au, 0x00A4u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A3u);
    assert(RewriteSelectionNameRingWrite(
        0xFF2A4Au, 0x00A4u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A4u);
    assert(RewriteSelectionNameRingWrite(
        0xFF2648u, 0x00A4u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A4u);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x038Bu, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x038Bu);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x03BBu, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x03BBu);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x0082u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x0082u);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x0084u, 2u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x0084u);
    const std::array<std::uint16_t, 2> noChar{{0x10u, 0x08u}};
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x00A4u, 2u, ringRam.data(), ringRam.size(),
        noChar) == 0x00A4u);
    ringRam[0x2648u + 2u] = 0x03u;
    assert(RewriteSelectionNameRingWrite(
        0xFF264Bu, 0x00A4u, 1u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A4u);
    ringRam[0x2648u + 2u] = 0x00u;
    assert(RewriteSelectionNameRingWrite(
        0xFF264Bu, 0x00A4u, 1u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A3u);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x00A4u, 2u, nullptr, ringRam.size(),
        p1Rogue) == 0x00A4u);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x00A4u, 2u, ringRam.data(), 0x100u,
        p1Rogue) == 0x00A4u);
    assert(RewriteSelectionNameRingWrite(
        0xFF264Au, 0x00A4u, 4u, ringRam.data(), ringRam.size(),
        p1Rogue) == 0x00A4u);

    // The sound latch no longer suppresses native corrected names.
    // Only original 0A00 character sprite speech is muted there.
    std::array<std::uint8_t, 16> sound{};
    const auto request = [&](unsigned code, unsigned kind) {
        sound.fill(0u);
        sound[2] = static_cast<std::uint8_t>(code >> 8u);
        sound[3] = static_cast<std::uint8_t>(code);
        sound[6] = static_cast<std::uint8_t>(kind);
    };
    for (const unsigned code : {0xA3u, 0xA4u, 0xAFu, 0xC4u,
                                0x82u, 0x83u, 0x84u, 0x85u}) {
        request(code, 0x10u);
        assert(AllowNativeSelectionSound(sound.data()));
    }
    request(0x03BBu, 0x0Au);
    assert(SelectionSoundEnvelope(sound.data(), 0x0Au));
    assert(!AllowNativeSelectionSound(sound.data()));
    request(0x40u, 0x20u);
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
