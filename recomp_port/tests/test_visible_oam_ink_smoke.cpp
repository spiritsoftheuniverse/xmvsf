// 87.492: executable, SDL/ROM-free exact native OAM pixel attribution.
#include "../../recomp/video/visible_oam_ink.h"
#include <cassert>
using cps2recomp::native_video::VisibleOamInk;
int main() {
    VisibleOamInk ink;
    assert(!ink.Valid());
    assert(!ink.Note(1u, 3, 3));
    ink.Begin(384, 224, 768);
    assert(ink.Valid());
    assert(ink.Note(16u, 144, 20));
    assert(!ink.Note(16u, 144, 20)); // two renderers, one screen pixel
    assert(!ink.Note(17u, 144, 20)); // front-most source owns it
    assert(ink.Note(16u, 145, 20));
    assert(ink.Note(16u, 144, 21));
    assert(ink.Note(17u, 146, 20));
    assert(ink.At(16u).pixels == 3u);
    assert(ink.At(16u).left == 144 && ink.At(16u).right == 145);
    assert(ink.At(16u).top == 20 && ink.At(16u).bottom == 21);
    assert(ink.At(17u).pixels == 1u);
    assert(ink.At(17u).left == 146 && ink.At(17u).right == 146);
    assert(ink.At(18u).pixels == 0u);
    assert(ink.At(18u).right == -1); // invisible source = no ink bounds
    assert(!ink.Note(1024u, 4, 4));
    assert(!ink.Note(20u, -1, 4));
    assert(!ink.Note(20u, 400, 4));
    assert(!ink.Note(20u, 4, 224));
    ink.Begin(384, 224, 768); // each frame's stats independent
    assert(ink.At(16u).pixels == 0u);
    assert(ink.Note(114u, 13, 17)); // shifted wall OAM slot is fine
    assert(ink.At(114u).pixels == 1u);
    ink.Disable();
    assert(!ink.Valid() && !ink.Note(114u, 14, 17));
    return 0;
}
