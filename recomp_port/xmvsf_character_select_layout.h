// XMVSF character-select 384x224 artwork and native-ROM ID mapping.
// No screen-pixel inference, no alphabetical reordering.
#pragma once
#include <array>
#include <cstdint>

namespace port::xmvsf_select {
struct Slot {
    const char *name;
    const char *portrait;
    std::uint16_t rom;
    // Fixed QSound character-name command, independent of player/pick.
    std::uint8_t nameVoice;
};
inline constexpr std::array<Slot, 18> kSlots{{
    {"Cyclops",       "layer_0014_0x02_Cyclops.png",       0x02, 0xA1},
    {"Wolverine",     "layer_0016_0x00_Wolverine.png",     0x00, 0xA0},
    {"Gambit",        "layer_0012_0x08_Gambit.png",        0x08, 0xA4},
    {"Rogue",         "layer_0018_0x06_Rogue.png",         0x06, 0xA3},
    {"Sabretooth",    "layer_0007_0x0A_Sabertooth.png",    0x0A, 0xA5},
    {"Storm",         "layer_0008_0x04_Storm.png",         0x04, 0xA2},
    {"Magneto",       "layer_0005_0x0E_Magneto.png",       0x0E, 0xA7},
    {"Juggernaut",    "layer_0002_0x0C_Juggernaut.png",    0x0C, 0xA6},
    {"Akuma",         "layer_0004_0x1E_Akuma.png",         0x1E, 0xAF},
    {"Ryu",           "layer_0013_0x12_Ryu.png",           0x12, 0xA9},
    {"Ken",           "layer_0015_0x14_Ken.png",           0x14, 0xAA},
    {"Cammy",         "layer_0017_0x22_Cammy.png",         0x22, 0xB1},
    {"Charlie",       "layer_0000_0x20_Charlie.png",       0x20, 0xB0},
    {"Chun-Li",       "layer_0010_0x16_Chun-Li.png",       0x16, 0xAB},
    {"Alpha Chun-Li", "layer_0009_0x24_Alpha Chun-Li.png", 0x24, 0xAB},
    {"Zangief",       "layer_0011_0x1A_Zangief.png",       0x1A, 0xAD},
    {"Dhalsim",      "layer_0003_0x18_Dhalsim.png",       0x18, 0xAC},
    {"M. Bison",      "layer_0001_0x1C_M. Bison.png",      0x1C, 0xAE}
}};
inline constexpr int kColumns = 9;
inline constexpr int kX0 = 43, kY0 = 136, kStepX = 33, kStepY = 39;
inline constexpr int X(int index) { return kX0 + index % kColumns * kStepX; }
inline constexpr int Y(int index) { return kY0 + index / kColumns * kStepY; }
inline constexpr int Find(std::uint16_t rom) {
    for (int i = 0; i < static_cast<int>(kSlots.size()); ++i)
        if (kSlots[static_cast<unsigned>(i)].rom == rom) return i;
    return -1; // Apocalypse deliberately has no grid cell.
}
inline constexpr int Navigate(int index, unsigned pressed) {
    if (index < 0 || index >= 18) return index;
    const int col = index % 9, row = index / 9;
    // Only one direction per move; opposing directions cannot skip slots.
    if (pressed == 1u) return row * 9 + (col + 1) % 9; // right
    if (pressed == 2u) return row * 9 + (col + 8) % 9; // left
    if (pressed == 4u || pressed == 8u) return (1 - row) * 9 + col;
    return index;
}
// One selectable character per team. Alpha Chun-Li is the same fighter
// as Chun-Li for duplicate detection, even though the art/ROM IDs differ.
inline constexpr std::uint16_t TeamIdentity(std::uint16_t rom) {
    return rom == 0x24u ? 0x16u : rom;
}
inline constexpr bool SameTeamCharacter(std::uint16_t first, std::uint16_t second) {
    return first != 0xFFFFu && second != 0xFFFFu &&
           TeamIdentity(first) == TeamIdentity(second);
}
// After the first pick, move one cell right; at a right-hand corner,
// move LEFT instead. Never leave the cursor sitting on its own first pick.
inline constexpr int NextTeamIndex(int index) {
    if (index < 0 || index >= static_cast<int>(kSlots.size())) return index;
    const int left = index - index % kColumns;
    const int right = left + kColumns - 1;
    const int step = index == right ? -1 : 1;
    int candidate = index + step;
    // Chun-Li and Alpha Chun-Li are separate portraits but the same
    // team identity: never auto-hop onto the other version.
    while (candidate >= left && candidate <= right &&
           SameTeamCharacter(kSlots[static_cast<unsigned>(index)].rom,
                             kSlots[static_cast<unsigned>(candidate)].rom))
        candidate += step;
    return candidate >= left && candidate <= right ? candidate : index;
}
// Directionally skip the first team member. If a vertical move would
// land on the forbidden cell, remain where the cursor was.
inline constexpr int SkipTeamIndex(int from, int candidate,
                                    unsigned direction, std::uint16_t forbidden) {
    if (candidate < 0 || candidate >= static_cast<int>(kSlots.size()) ||
        !SameTeamCharacter(forbidden, kSlots[static_cast<unsigned>(candidate)].rom))
        return candidate;
    if (direction == 4u || direction == 8u) {
        // A two-row grid cannot continue vertically past the locked
        // first character; slide into its neighbor instead.
        const int neighbor = NextTeamIndex(candidate);
        return SameTeamCharacter(
            forbidden, kSlots[static_cast<unsigned>(neighbor)].rom)
            ? from : neighbor;
    }
    if (direction == 1u || direction == 2u) {
        // At an outer corner there is no further cell in this row.
        // Stop instead of wrapping across the entire grid.
        const int col = candidate % kColumns;
        if ((direction == 1u && col == kColumns - 1) ||
            (direction == 2u && col == 0)) return from;
        int next = candidate + (direction == 1u ? 1 : -1);
        const int left = candidate - col, right = left + kColumns - 1;
        while (next >= left && next <= right &&
               SameTeamCharacter(forbidden,
                                 kSlots[static_cast<unsigned>(next)].rom))
            next += direction == 1u ? 1 : -1;
        if (next >= left && next <= right) return next;
    }
    return from;
}
// Only a current A press may advance the native accepted count.
// A repeated native write is allowed ONLY if it was already recorded at
// the actual accepted-count bus write following an A press. The raw RAM
// value is never permission to accept a new character.
inline constexpr bool NativePickAllowed(unsigned proposed, unsigned accepted,
                                        unsigned confirmedByA, bool aAuthorized) {
    return proposed <= accepted || proposed <= confirmedByA ||
           (aAuthorized && proposed == accepted + 1u);
}
// Accept a fresh A only when the native chooser is READY for this pick.
// Never buffer A through phase 02 animation or phase 04 mode/speed.
inline constexpr bool FreshPickPress(bool aEdge, bool bHeld,
                                      unsigned phase, unsigned count,
                                      bool duplicate) {
    return aEdge && !bHeld && !duplicate &&
           ((count == 0u && phase == 1u) ||
            (count == 1u && phase == 3u));
}
struct Navigation {
    int index[2] = {9, 8}; // literal artwork: P1 Ryu (lower-left), P2 Akuma (upper-right).
    unsigned held[2] = {};
    unsigned frames[2] = {};
    void Reset() { *this = Navigation{}; }
    void Advance(int side, unsigned direction, bool active) {
        if (side < 0 || side > 1) return;
        if (!active || direction == 0u || (direction & (direction - 1u))) {
            held[side] = direction;
            frames[side] = 0;
            return;
        }
        if (held[side] != direction) {
            held[side] = direction;
            frames[side] = 0;
            index[side] = Navigate(index[side], direction);
        } else if (++frames[side] >= 15u && (frames[side] - 15u) % 5u == 0u) {
            index[side] = Navigate(index[side], direction);
        }
    }
};
} // namespace port::xmvsf_select
