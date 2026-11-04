// 87.157: preserve 87.150 frame-accurate early tag/kick suppression,
// but use the 87.156 CONFIRMED Ken ROM neutral command dispatcher.
// No old forced 2C00 Hadoken, 2C32 dash or fake 2E action is allowed.
// Synthetic ROM with only enough pointer records for isolated unit tests.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
using namespace XM87144;
static void w16(std::vector<std::uint8_t>& v,unsigned a,unsigned n){
    v[a]=std::uint8_t(n);v[a+1u]=std::uint8_t(n>>8);
}
static void w32(std::vector<std::uint8_t>& v,unsigned a,unsigned n){
    w16(v,a,n>>16);w16(v,a+2u,n);
}
static void entry(std::vector<std::uint8_t>& rom,unsigned action,
                  unsigned selector,unsigned section,unsigned script){
    const unsigned table=0xD378Eu;
    w32(rom,0x8A946u+0x14u*2u,table);
    w16(rom,table+action,section-table);
    w16(rom,section+selector,script-section);
    w16(rom,script,0u);
    w16(rom,script+2u,3u);
    w32(rom,script+4u,0x0026A5C2u);
    w32(rom,script+8u,0x30008000u);
    w16(rom,script+12u,0u);
    w16(rom,script+14u,0u);
    w32(rom,0x8AA04u+0x14u*2u,0x9D0BAu);
}
static void testEarly(XM87143Queue::Command command,
                      unsigned expectedAction,unsigned expectedScript,
                      unsigned mask){
    std::vector<std::uint8_t> rom(0xE1000u,0u),ram(65536u,0u);
    entry(rom,0x00u,0x00u,0xD38E0u,0xD39FAu);
    entry(rom,0x2Au,0x08u,0xD3910u,0xD567Eu);
    entry(rom,0x2Eu,0x00u,0xD3966u,0xD6722u);
    entry(rom,0x2Eu,0x02u,0xD3966u,0xD6976u);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x14u);ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4060u,0x2A08u);
    ww(ram.data(),0x40A0u,0x2A00u);
    wl(ram.data(),0x4034u,0xD567Eu);
    wb(ram.data(),0x40D6u,0x12u);
    wb(ram.data(),0x404Bu,1u); // Ken right-facing
    State s;s.init();
    assert(s.enabled && s.earlyCancel && !s.proofNative);
    s.phase[0]=WAIT_KEN_EXIT;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    s.command[0]=command;
    s.kenExitObserved[0]=true;s.kenExitAt[0]=100u;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,1u);
    assert(s.phase[0]==DELIVER_NATIVE_INPUT);
    assert(s.inputBegin[0]==102u);
    assert(s.kenExpectedAction[0]==expectedAction);
    assert(s.kenExpectedScript[0]==expectedScript);
    assert(rw(ram.data(),0x4060u)==0x0000u);
    assert(rw(ram.data(),0x40A0u)==0x002Au);
    assert(rb(ram.data(),0x40D6u)==0x10u);
    assert(rl(ram.data(),0x4034u)==0xD39FAu);
    const auto idleState=ram;
    auto in=s.inputForFrame(ram.data(),102u);
    assert(in.active && in.directions==4u && !in.attacks);
    in=s.inputForFrame(ram.data(),104u);
    assert(in.active && in.directions==5u && !in.attacks);
    in=s.inputForFrame(ram.data(),107u);
    assert(in.active && in.directions==1u && in.attacks==mask);
    in=s.inputForFrame(ram.data(),109u);
    assert(in.active && !in.attacks);
    in=s.inputForFrame(ram.data(),111u);
    assert(!in.active);
    assert(ram==idleState); // input routing doesn't write fighter state
    // Only the ROM command decoder can transition to one of the two 2E
    // super actions. Explicitly never prime a 2C move or fake 2E.
    ww(ram.data(),0x4060u,expectedAction);
    ww(ram.data(),0x40A0u,0x2A02u);
    wl(ram.data(),0x4034u,expectedScript);
    const auto romSuper=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),108u,1u);
    assert(s.phase[0]==SUPER_STARTED && s.kenSuperConfirmed[0]);
    assert(ram==romSuper);
}
static void guardWrongExit(){
    std::vector<std::uint8_t> rom(0xE1000u,0u),ram(65536u,0u);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x14u);ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4060u,0x2A08u);
    wb(ram.data(),0x40D6u,0x10u); // not verified D6=12
    wl(ram.data(),0x4034u,0xD567Eu);
    State s;s.init();s.phase[0]=WAIT_KEN_EXIT;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    s.kenExitObserved[0]=true;s.kenExitAt[0]=50u;
    const auto original=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),51u,1u);
    assert(s.phase[0]==IDLE && ram==original);
}
int main(){
    assert(setenv("XM87144_DHC_NATIVE_TEST","1",1)==0);
    assert(setenv("XM87157_EARLY_CANCEL","1",1)==0);
    testEarly(XM87143Queue::QCF_PP,0x2E00u,0xD6722u,0x03u);
    testEarly(XM87143Queue::QCF_KK,0x2E02u,0xD6976u,0x18u);
    guardWrongExit();
    return 0;
}
