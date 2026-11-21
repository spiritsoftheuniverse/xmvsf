// XMVSF 87.493: executed pixel-ownership tint regression, no SDL/ROM.
#include "../../recomp/port/xmvsf_source_tint.h"
#include <cassert>
#include <cstdint>
#include <vector>

using cps2recomp::native_video::VisibleOamInk;
namespace tint = port::xmvsf_home::source_tint;
int main() {
    constexpr int w = 16, h = 8;
    VisibleOamInk owners;
    owners.Begin(w, h, w);
    assert(owners.Valid());
    assert(owners.Note(7u, 2, 3));
    assert(owners.Note(7u, 2, 4));
    assert(owners.Note(8u, 3, 3)); // unrelated visible sprite inside bounds
    assert(owners.Note(7u, 5, 4)); // selected source, outside selected bounds
    assert(!owners.Note(7u, 3, 3)); // first renderer's front owner wins
    assert(owners.OwnerAt(2, 3) == 7);
    assert(owners.OwnerAt(3, 3) == 8);
    assert(owners.OwnerAt(4, 3) == -1);
    assert(owners.OwnerAt(-1, 0) == -1);
    assert(owners.OwnerAt(16, 3) == -1);
    constexpr std::uint32_t colour = 0xA5224466u;
    std::vector<std::uint32_t> original(w * h, colour);
    auto frame = original;
    assert(tint::Paint(frame, w, h, owners, 7u, 1, 2, 4, 5) == 2u);
    assert(frame[3 * w + 2] == tint::Magenta(colour));
    assert(frame[4 * w + 2] == tint::Magenta(colour));
    assert((frame[3 * w + 2] & 0xFF000000u) == (colour & 0xFF000000u));
    assert(frame[3 * w + 3] == colour); // other object's pixel untouched
    assert(frame[4 * w + 5] == colour); // outside measured bounds untouched
    assert(frame[3 * w + 4] == colour); // scroll/backdrop untouched
    assert(tint::Paint(frame, w, h, owners, 99u, 0, 0, 15, 7) == 0u);
    assert(tint::Paint(frame, w, h, owners, 7u, 6, 6, 2, 2) == 0u);
    owners.Begin(w, h, w); // next game frame starts fresh
    frame = original;
    assert(tint::Paint(frame, w, h, owners, 7u, 0, 0, 15, 7) == 0u);
    assert(frame == original);
    owners.Disable();
    assert(!owners.Valid() && owners.OwnerAt(2, 3) == -1);
    assert(tint::Paint(frame, w, h, owners, 7u, 0, 0, 15, 7) == 0u);
    assert(frame == original);
    return 0;
}
