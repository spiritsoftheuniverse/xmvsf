// 87.471: compile + execute true CPS2 planar selector-ink discrimination.
#include "../../recomp/video/selector_pen.h"
#include <cassert>
#include <cstdint>
void fill(std::uint8_t* tile, unsigned pen) {
    for (unsigned y=0; y<16; ++y)
        for (unsigned group=0; group<2; ++group)
            for (unsigned plane=0; plane<4; ++plane)
                tile[y*8+group*4+plane]=(pen&(1u<<plane)) ? 0xffu : 0x00u;
}
int main() {
    std::uint8_t gfx[256]{};
    fill(gfx,10);
    using cps2recomp::native_video::IsP2SelectorTile;
    assert(IsP2SelectorTile(gfx,sizeof(gfx),0,27));
    assert(!IsP2SelectorTile(gfx,sizeof(gfx),0,26));
    assert(!IsP2SelectorTile(gfx,sizeof(gfx),2,27));
    fill(gfx,12);
    assert(IsP2SelectorTile(gfx,sizeof(gfx),0,27));
    fill(gfx,13);
    assert(!IsP2SelectorTile(gfx,sizeof(gfx),0,27)); // P1 survives
    fill(gfx,15);
    assert(!IsP2SelectorTile(gfx,sizeof(gfx),0,27));
    fill(gfx,10);
    gfx[1]=0;
    assert(!IsP2SelectorTile(gfx,sizeof(gfx),0,27)); // mixed pen art survives
}
