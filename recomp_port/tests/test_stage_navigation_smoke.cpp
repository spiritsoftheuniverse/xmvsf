// XMVSF 87.487: actual pure directional edges and bounded thumbnail movement.
// No SDL, hardware, generated assets or emulation necessary.
#include "../xmvsf_stage_nav.h"
#include <cassert>
using namespace port::xmvsf_home::stage_nav;
int main() {
    // Horizontal ends do not wrap to an apparent FOUR-panel wrong-way hop.
    assert(Move(0, Left, 18) == 0);
    assert(Move(0, Right, 18) == 1);
    assert(Move(4, Right, 18) == 4);
    assert(Move(5, Left, 18) == 5);
    assert(Move(9, Right, 18) == 9);
    assert(Move(9, Left, 18) == 8);
    assert(Move(0, Up, 18) == 0);
    assert(Move(0, Down, 18) == 5);
    assert(Move(5, Up, 18) == 0);
    assert(Move(5, Down, 18) == 5);
    // Second page has eight entries, with three in the bottom row.
    assert(Move(14, Down, 18) == 17);
    assert(Move(17, Right, 18) == 17);
    assert(Move(17, Up, 18) == 12);
    assert(Move(10, Left, 18) == 10);
    assert(Move(10, Right, 18) == 11);
    assert(Move(0, Left | Right, 18) == 0);
    assert(Move(0, Right | Down, 18) == 0);
    assert(Page(0, false, true, 18) == 10);
    assert(Page(10, true, false, 18) == 0);
    assert(Page(9, false, true, 18) == 17);
    assert(Page(9, true, true, 18) == 9);

    Edges keys;
    // HOME's held Right at stage entry never leaks into stage selection.
    assert(keys.Press(Right, false) == 0);
    assert(keys.Press(Right, true) == 0);
    assert(keys.Press(0, true) == 0);
    assert(keys.Press(Right, true) == Right);
    assert(keys.Press(Right, true) == 0); // no accidental repeat
    assert(keys.Press(Right | Left, true) == 0); // conflict neutral
    assert(keys.Press(Left, true) == Left); // rearm after conflict
    assert(keys.Press(Left | Down, true) == 0); // diagonal neutral
    assert(keys.Press(Down, true) == Down); // direction switches without neutral
    assert(keys.Press(0, false) == 0);
    assert(keys.Press(Down, true) == 0); // held while reopening is not an edge
    assert(keys.Press(0, true) == 0);
    assert(keys.Press(Down, true) == Down);
    return 0;
}
