// 87.194 regression: Sabretooth-style passive native entry uses actual
// ROM-owned descent and ground landing. Only X target is steered; Y,
// velocity, actor action/script and physical buttons are never owned by
// this pure helper. No home-baked duration, no snap to floor at spawn.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_194_native_passive_jump_entry.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h"
using namespace XM87194Jump;
int main(){
    Arrival a;
    assert(!a.begin(2u,0x14u,1754u,304,408,472,740,true));
    assert(!a.active);
    assert(!a.begin(0u,0x14u,1754u,304,472,472,740,true));
    assert(!a.begin(0u,0x14u,1754u,304,408,120,740,true));
    // 87.193 Ken arrived at x=304, Y=408, floor=472 while the
    // existing DHC destination was foeX-48=692. Never snap on entry.
    assert(a.begin(0u,0x14u,1754u,304,408,472,740,true));
    assert(a.startX==304 && a.startY==408);
    assert(a.floorY==472 && a.targetX==692);
    assert(a.position(1754u,408)==304);
    assert(!a.landed && !a.ready(0x14u,408));
    // Native ROM may first rise; do not fabricate horizontal motion
    // until the ROM's actual Y enters the descending segment.
    assert(a.position(1755u,400)==304);
    assert(a.position(1756u,409)>304);
    assert(a.descended);
    const int p=a.position(1757u,420);
    assert(p>304 && p<692);
    const int q=a.position(1758u,449);
    assert(q>p && q<692);
    assert(!a.landed);
    assert(a.position(1759u,472)==692);
    assert(a.ready(0x14u,472));
    assert(!a.ready(0x12u,472));
    assert(a.landingFrame==1759u);
    assert(a.position(1760u,472)==692);
    // Engine does not infer landing from a jump start and an
    // instantaneous Y=floor on the very next frame.
    Arrival noFlight;
    assert(noFlight.begin(0u,0x14u,1u,304,408,472,740,true));
    assert(noFlight.position(2u,472)==692);
    assert(!noFlight.landed);
    assert(!noFlight.ready(0x14u,472));
    // Mirror target on the other side; clamp only to the existing
    // Stage-05 bounds, never to a guessed animation-timer endpoint.
    Arrival right;
    assert(right.begin(1u,0x12u,100u,704,384,472,256,false));
    assert(right.targetX==304);
    assert(right.position(101u,394)<704);
    assert(right.position(102u,415)<right.position(101u,394));
    assert(right.position(103u,472)==304);
    assert(right.ready(0x12u,472));
    Arrival clamped;
    assert(clamped.begin(0u,0x14u,1u,280,408,472,192,true));
    assert(clamped.targetX==192);
    // Physical input remains neutral throughout native entry; it
    // belongs to the reserve only after landing / DELIVER_NATIVE_INPUT.
    assert(XM87176Input::neutralize(12u,0x14u,0x12u,
                                  true,true,1758u,0u));
    assert(!XM87176Input::neutralize(12u,0x12u,0x14u,
                                   true,true,1758u,0u));
    assert(!XM87176Input::neutralize(12u,0x14u,0x12u,
                                   false,true,1758u,0u));
    a.reset();assert(!a.active && !a.landed);
    std::puts("PASS 87.194: actual ROM descent drives passive jump to opponent-relative target, release only on native landing");
}
