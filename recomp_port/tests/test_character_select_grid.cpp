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
    // 87.515: the ROM's exact accepted ID, not its transient native
    // cursor, determines which ONE character-name recording can pass.
    // The name is explicitly queued in the same native 16-byte format.
    std::array<std::uint8_t, 65536> ring{};
    constexpr unsigned ringBase = 0x2648u;
    assert(QueueExplicitSelectionSound(ring.data(), ring.size(), 0xAFu));
    assert(ring[0x2643u] == 16u);
    assert(ring[ringBase + 3u] == 0xAFu);  // Akuma
    assert(ring[ringBase + 6u] == 0x10u); // native announcement envelope
    assert(QueueExplicitSelectionSound(ring.data(), ring.size(), 0x82u));
    assert(ring[0x2643u] == 32u);
    assert(ring[ringBase + 16u + 3u] == 0x82u); // confirmation restored
    assert(!QueueExplicitSelectionSound(ring.data(), 100u, 0xA0u));
    assert(!QueueExplicitSelectionSound(ring.data(), ring.size(), 0x100u));
    // Every one of the 18 ROM IDs in the actual screen table must
    // produce an explicit, non-ambiguous P1 and P2 announcer address.
    // Both picks use that same ID; the native cursor's wrong source
    // or whether it was the second pick cannot change the spoken name.
    for (const Slot &slot : kSlots) {
        const std::uint16_t p1Name = NativeCharacterVoice(slot.rom, 0u);
        const std::uint16_t p2Name = NativeCharacterVoice(slot.rom, 1u);
        assert(p1Name != 0xFFFFu && p2Name != 0xFFFFu);
        assert(p1Name >= 0xA0u && p1Name <= 0xB1u);
        assert(p2Name == p1Name + 0x20u);
    }
    // The 0x400-byte native ring wraps cleanly.
    ring[0x2642u] = 3u;
    ring[0x2643u] = 0xF0u;
    assert(QueueExplicitSelectionSound(ring.data(), ring.size(), 0xCBu));
    assert(ring[0x2642u] == 0u && ring[0x2643u] == 0u);
    assert(ring[ringBase + 0x3F0u + 3u] == 0xCBu);
    ring[0x2642u] = 0u;
    ring[0x2643u] = 1u;
    assert(!QueueExplicitSelectionSound(ring.data(), ring.size(), 0xA7u));

    std::array<std::uint8_t, 16> sound{};
    auto request = [&](unsigned code, unsigned mode = 0x10u) {
        sound.fill(0u);
        sound[2] = static_cast<std::uint8_t>(code >> 8u);
        sound[3] = static_cast<std::uint8_t>(code);
        sound[6] = static_cast<std::uint8_t>(mode);
    };
    std::array<std::uint16_t, 2> expected{{0xAFu, 0xCBu}};
    std::array<unsigned, 2> nameTickets{{1u, 1u}};
    std::array<unsigned, 2> confirmTickets{{1u, 1u}};
    request(0xB1u); // Cammy: native is unrelated to selected Akuma
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0xB3u); // native off-grid second-pick announcer
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x03BBu, 0x0Au); // Cammy's native character-owned speech
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x037Bu, 0x0Au); // no guessed "Akuma sprite voice" substitution
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    // Keep side-specific movement clicks without doubling or muting.
    request(0x84u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x85u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    // First acceptance: name and confirmation play exactly once.
    request(0xAFu);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    assert(nameTickets[0] == 0u);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x82u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    assert(confirmTickets[0] == 0u);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    // Second player is entirely independent, including name family C?.
    request(0xABu); // P1 Chun-Li code is NOT P2 Chun-Li
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0xCBu);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    assert(nameTickets[1] == 0u);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x83u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    assert(confirmTickets[1] == 0u);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    // Without a real accepted-counter edge, no original native speech,
    // announcer, or confirmation is authorized.
    request(0xA7u);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x03BBu, 0x0Au);
    assert(!AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    // Non-character UI and music requests (including a 0x10 envelope)
    // must not be muted just because selection is visible.
    request(0x40u, 0x10u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    request(0x40u, 0x20u);
    assert(AllowAuthoritativeSelectionSound(sound.data(), expected, nameTickets, confirmTickets));
    assert(!AllowAuthoritativeSelectionSound(nullptr, expected, nameTickets, confirmTickets));

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
