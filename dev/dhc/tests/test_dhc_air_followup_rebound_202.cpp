#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_202_air_followup_rebound.h"
using namespace XM87202Air;
int main(){
    Evidence e;
    // 87.201 frames 6233/6234: original-ROM PC 010392 rebound.
    e.phase=2;e.point=0x12;e.reserve=0x14;e.outgoing=0x12;e.incoming=0x14;
    e.capturedAction=0x2E0E;e.capturedMode=0x2A1C;
    e.capturedStep=6;e.capturedScript=0xD322C;e.capturedD6=0x0A;
    e.captured259=0;
    e.action=0x2E0C;e.mode=0x2A00;e.step=2;
    e.script=0xD31A8;e.d6=0x0A;e.controller256=0;e.controller259=8;
    e.started=6233;e.now=6234;
    assert(exactNativeAirRebound(e));
    // 87.201 frame 12107: later 2E0E script rebounds identically.
    e.capturedScript=0xD327C;e.started=12106;e.now=12107;
    assert(exactNativeAirRebound(e));
    e.now=12108;assert(!exactNativeAirRebound(e));e.now=12107;
    e.controller259=0;assert(!exactNativeAirRebound(e));e.controller259=8;
    e.controller256=8;assert(!exactNativeAirRebound(e));e.controller256=0;
    e.captured259=8;assert(!exactNativeAirRebound(e));e.captured259=0;
    e.point=0x14;assert(!exactNativeAirRebound(e));e.point=0x12;
    e.d6=0;assert(!exactNativeAirRebound(e));e.d6=0x0A;
    e.script=0xD31B8;assert(!exactNativeAirRebound(e));e.script=0xD31A8;
    e.step=4;assert(!exactNativeAirRebound(e));e.step=2;
    e.capturedAction=0x2E0C;assert(!exactNativeAirRebound(e));e.capturedAction=0x2E0E;
    // Never synthesize a tag from genuine normal airborne actions.
    e.action=0x0212;e.mode=0x1C00;e.script=0xD0084;
    assert(!exactNativeAirRebound(e));
    e.action=0x0214;e.mode=0x1C02;e.script=0xD0118;
    assert(!exactNativeAirRebound(e));
    e.action=0x2C02;e.mode=0x0200;
    assert(!exactNativeAirRebound(e));
    std::puts("PASS 87.202: 2E0E PC010392 rebound only; wrong action, owner and stale-latch rejected");
}
