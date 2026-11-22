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
};
inline constexpr std::array<Slot, 18> kSlots{{
    {"Cyclops",       "layer_0014_0x02_Cyclops.png",       0x02},
    {"Wolverine",     "layer_0016_0x00_Wolverine.png",     0x00},
    {"Gambit",        "layer_0012_0x08_Gambit.png",        0x08},
    {"Rogue",         "layer_0018_0x06_Rogue.png",         0x06},
    {"Sabretooth",    "layer_0007_0x0A_Sabertooth.png",    0x0A},
    {"Storm",         "layer_0008_0x04_Storm.png",         0x04},
    {"Magneto",       "layer_0005_0x0E_Magneto.png",       0x0E},
    {"Juggernaut",    "layer_0002_0x0C_Juggernaut.png",    0x0C},
    {"Akuma",         "layer_0004_0x1E_Akuma.png",         0x1E},
    {"Ryu",           "layer_0013_0x12_Ryu.png",           0x12},
    {"Ken",           "layer_0015_0x14_Ken.png",           0x14},
    {"Cammy",         "layer_0017_0x22_Cammy.png",         0x22},
    {"Charlie",       "layer_0000_0x20_Charlie.png",       0x20},
    {"Chun-Li",       "layer_0010_0x16_Chun-Li.png",       0x16},
    {"Alpha Chun-Li", "layer_0009_0x24_Alpha Chun-Li.png", 0x24},
    {"Zangief",       "layer_0011_0x1A_Zangief.png",       0x1A},
    {"Dhalsim",      "layer_0003_0x18_Dhalsim.png",       0x18},
    {"M. Bison",      "layer_0001_0x1C_M. Bison.png",      0x1C}
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
