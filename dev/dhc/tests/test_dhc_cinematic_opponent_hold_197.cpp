// 87.197: selective DHC freeze of the opposing native actor.
// This TEST holds P2 during the genuine Ryu3200 -> KenROM2E bridge;
// Ken's actor, original native fall, meter, partner and the CPU continue.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_197_cinematic_opponent_hold.h"
using namespace XM87197Hold;
static void ww(std::uint8_t* r,unsigned a,unsigned v){
    wb(r,a,v>>8u);wb(r,a+1u,v);
}
int main(){
    std::vector<std::uint8_t> ram(65536u,0u);
    wb(ram.data(),0x5000u,8u);
    const unsigned enemy=0x4400u,ken=0x4000u;
    for(unsigned i=0u;i<0x400u;++i)
        ram[enemy+i]=std::uint8_t((i*37u+11u)&255u);
    ww(ram.data(),enemy+0x52u,0x20u);
    ww(ram.data(),enemy+0x60u,0x2C02u);
    ww(ram.data(),enemy+0x0Cu,650u);
    ww(ram.data(),enemy+0x70u,0x11u);
    wb(ram.data(),enemy+0x82u,1u);
    ww(ram.data(),ken+0x52u,0x14u);
    ww(ram.data(),ken+0x0Cu,300u);
    const std::vector<std::uint8_t> baseline(
        ram.begin()+enemy,ram.begin()+enemy+0x400u);
    Freeze freeze;
    assert(!freeze.capture(ram.data(),0u,100u,2u,0x12u,0x14u));
    assert(!freeze.capture(ram.data(),0u,100u,3u,0x12u,0x12u));
    assert(!freeze.capture(ram.data(),2u,100u,3u,0x12u,0x14u));
    assert(!freeze.active);
    assert(freeze.capture(ram.data(),0u,100u,3u,0x12u,0x14u));
    assert(freeze.active && freeze.fighter==0x20u &&
           freeze.base==enemy && freeze.start==100u);
    assert(rb(ram.data(),enemy+0x70u)==0u);
    assert(rb(ram.data(),enemy+0x82u)==0u);
    assert(Freeze::phaseHolds(3u));
    assert(Freeze::phaseHolds(4u));
    assert(Freeze::phaseHolds(5u));
    assert(Freeze::phaseHolds(8u));
    assert(Freeze::phaseHolds(12u));
    assert(Freeze::phaseHolds(6u)); // 87.198 holds actual 2E startup too
    assert(!Freeze::phaseHolds(0u));
    assert(!Freeze::phaseHolds(7u));
    // The original native 68000 mutates the opponent during the frame.
    // The DHC host restores ONLY P2's actor record after the CPU runs.
    for(unsigned frame=101u;frame<=145u;++frame){
        const unsigned phase=frame<115u?3u:frame<119u?4u:
                             frame<135u?12u:8u;
        ww(ram.data(),enemy+0x0Cu,800u+frame);
        ww(ram.data(),enemy+0x60u,0x2C30u);
        wb(ram.data(),enemy+0x82u,4u);
        // P1 Ken genuinely advances his X/Y and launches a super;
        // the freeze must never even look at his native actor bytes.
        ww(ram.data(),ken+0x0Cu,frame+250u);
        ww(ram.data(),ken+0x10u,frame+150u);
        const std::uint8_t kenBefore[0x400u]={};
        (void)kenBefore;
        assert(freeze.hold(ram.data(),frame,phase));
        assert(rw(ram.data(),enemy+0x0Cu)==650u);
        assert(rw(ram.data(),enemy+0x60u)==0x2C02u);
        assert(rb(ram.data(),enemy+0x82u)==0u);
        assert(rw(ram.data(),ken+0x0Cu)==frame+250u);
        assert(rw(ram.data(),ken+0x10u)==frame+150u);
        wb(ram.data(),enemy+0x82u,8u);
        freeze.beforeCPU(ram.data(),phase);
        assert(rb(ram.data(),enemy+0x82u)==0u);
    }
    assert(freeze.held==45u);
    assert(freeze.hold(ram.data(),146u,6u)); // 87.198: wait until first ROM live offensive box
    // Only the originally recorded opponent actor resumes with its
    // exact saved original offense, action, animation and health.
    assert(freeze.release(ram.data()));
    assert(!freeze.active);
    assert(std::memcmp(ram.data()+enemy,
                       baseline.data(),baseline.size())==0);
    assert(rw(ram.data(),ken+0x0Cu)==395u);
    // No new capture during a normal tag, and a valid zero fighter ID
    // (Wolverine) must not be mistaken for an empty actor.
    ww(ram.data(),enemy+0x52u,0u);
    assert(freeze.capture(ram.data(),0u,200u,3u,0x12u,0x14u));
    assert(freeze.fighter==0u);
    assert(freeze.release(ram.data()));
    // Actor ownership change while held must release without replacing
    // the new native fighter's RAM (no cross-slot corruption).
    ww(ram.data(),enemy+0x52u,0x22u);
    assert(freeze.capture(ram.data(),0u,300u,3u,0x12u,0x14u));
    ww(ram.data(),enemy+0x52u,0x20u);
    assert(!freeze.hold(ram.data(),301u,3u));
    assert(!freeze.release(ram.data()));
    assert(rw(ram.data(),enemy+0x52u)==0x20u);
    // A stalled tag must not keep the opponent locked forever:
    // the watchdog is a RELEASE ONLY, never a fabricated Ken action.
    assert(freeze.capture(ram.data(),0u,400u,3u,0x12u,0x14u));
    assert(!freeze.hold(ram.data(),581u,12u));
    assert(freeze.release(ram.data()));
    // Mirrored P2 DHC transaction maps to original P1 opponent slot.
    ww(ram.data(),ken+0x52u,0x1Cu);
    assert(freeze.capture(ram.data(),1u,600u,3u,0x12u,0x14u));
    assert(freeze.base==0x4000u && freeze.fighter==0x1Cu);
    assert(freeze.release(ram.data()));
    std::puts("PASS 87.197 selective opponent freeze: native 3200->2E, Ken still moves, reversible P1/P2, abort watchdog");
}
