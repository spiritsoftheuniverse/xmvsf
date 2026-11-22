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
