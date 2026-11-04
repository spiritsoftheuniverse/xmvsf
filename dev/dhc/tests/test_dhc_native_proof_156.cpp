// 87.156: proof path through the ROM's REAL native inputs.
// After natural Ryu super completion, submit HP+HK to trigger normal tag,
// wait for the verified Ken ownership swap/neutral, then QCF+2P or QCF+2K.
// Synthetic ROM *address* cells only; this cannot verify live CPU acceptance.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
using namespace XM87144;
static void romW(std::vector<std::uint8_t>& v,unsigned a,unsigned n){
    v[a]=std::uint8_t(n);v[a+1]=std::uint8_t(n>>8);
}
static void romL(std::vector<std::uint8_t>& v,unsigned a,unsigned n){
    romW(v,a,n>>16);romW(v,a+2,n);
}
static void makeKen(std::vector<std::uint8_t>& v){
    const unsigned fighter=0x14u,table=0xD378Eu,section=0xD3966u;
    romL(v,0x8A946u+fighter*2u,table);
    romW(v,table+0x2Eu,section-table);
    romW(v,section+0u,0xD6722u-section);
    romW(v,section+2u,0xD6976u-section);
    romL(v,0x8AA04u+fighter*2u,0x9D0BAu);
    for(unsigned a: {0xD6722u,0xD6976u}){
        romW(v,a+0u,0u);
        romW(v,a+2u,3u);
        romL(v,a+4u,0x0026A5C2u);
        romL(v,a+8u,0x30008000u);
        romW(v,a+12u,0u);
        romW(v,a+14u,0u);
    }
}
static void exercise(XM87143Queue::Command requested,unsigned action,unsigned script,
                     unsigned mask){
    std::vector<std::uint8_t> rom(0xE1000u,0u),ram(65536u,0u);
    makeKen(rom);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x12u);ww(ram.data(),0x4852u,0x14u);
    ww(ram.data(),0x4060u,0x2E0Au);ww(ram.data(),0x40A0u,0x2A02u);
    State s;
    s.init();
    assert(s.enabled && s.proofNative);
    s.phase[0]=WAIT_RYU_SUPER_END;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    s.command[0]=requested;
    s.started[0]=100u;
    const auto ryuSuper=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,1u);
    assert(s.phase[0]==WAIT_RYU_SUPER_END && ram==ryuSuper);
    // Let the ROM naturally exit the first super.
    ww(ram.data(),0x4060u,0u);ww(ram.data(),0x40A0u,0x002Au);
    const auto ryuIdle=ram;
    for(unsigned t=102u;t<=104u;++t)s.frame(ram.data(),rom.data(),unsigned(rom.size()),t,1u);
    assert(s.phase[0]==SEND_ROM_TAG_INPUT);
    assert(s.inputBegin[0]==106u);
    assert(ram==ryuIdle);
    auto input=s.inputForFrame(ram.data(),106u);
    assert(input.active && input.attacks==0x24u && input.directions==0u);
    input=s.inputForFrame(ram.data(),109u);
    assert(input.active && input.attacks==0u);
    input=s.inputForFrame(ram.data(),112u);
    assert(!input.active);
    assert(ram==ryuIdle); // input submission never writes fighter RAM
    // Simulate actual ROM normal-tag acceptance one frame after HP+HK.
    ww(ram.data(),0x4060u,0x3200u);ww(ram.data(),0x40A0u,0x2A00u);
    const auto tag=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),107u,1u);
    assert(s.phase[0]==WAIT_NATIVE_SWAP && ram==tag);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),108u,1u);
    assert(s.phase[0]==WAIT_NATIVE_SWAP && ram==tag);
    // The ROM is solely responsible for the ownership swap and kick/taunt.
    ww(ram.data(),0x4052u,0x14u);ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4060u,0x2A0Cu);ww(ram.data(),0x40A0u,0x2A00u);
    const auto kenIncoming=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),121u,1u);
    assert(s.phase[0]==WAIT_LANDING && ram==kenIncoming);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),122u,1u);
    assert(s.phase[0]==WAIT_LANDING && ram==kenIncoming);
    ww(ram.data(),0x4060u,0x0002u);ww(ram.data(),0x40A0u,0x020Eu);
    wb(ram.data(),0x404Bu,1u); // Ken facing right for directional QCF
    const auto ready=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),140u,1u);
    assert(s.phase[0]==WAIT_KEN_EXIT && ram==ready);
    for(unsigned t=141u;t<=143u;++t)
        s.frame(ram.data(),rom.data(),unsigned(rom.size()),t,1u);
    assert(s.phase[0]==DELIVER_NATIVE_INPUT && ram==ready);
    assert(s.kenExpectedAction[0]==action);
    assert(s.kenExpectedScript[0]==script);
    assert(s.inputBegin[0]==145u);
    input=s.inputForFrame(ram.data(),147u);
    assert(input.active && input.directions==4u && !input.attacks);
    input=s.inputForFrame(ram.data(),151u);
    assert(input.active && input.directions==5u && !input.attacks);
    input=s.inputForFrame(ram.data(),158u);
    assert(input.active && input.directions==1u && input.attacks==mask);
    assert(ram==ready);
    ww(ram.data(),0x4060u,action);ww(ram.data(),0x40A0u,0x2A02u);
    wl(ram.data(),0x4034u,script);
    const auto superRAM=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),159u,1u);
    assert(s.phase[0]==SUPER_STARTED && s.kenSuperConfirmed[0]);
    assert(ram==superRAM); // no forced native super state
}
static void noNativeTagNoMutation(){
    std::vector<std::uint8_t> rom(0xE1000u,0),ram(65536u,0);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x12u);ww(ram.data(),0x4852u,0x14u);
    ww(ram.data(),0x4060u,0x0000u);ww(ram.data(),0x40A0u,0x002Au);
    State s;s.init();s.proofNative=true;
    s.phase[0]=SEND_ROM_TAG_INPUT;s.inputBegin[0]=102u;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    const auto original=ram;
    for(unsigned t=103u;t<=123u;++t)
        s.frame(ram.data(),rom.data(),unsigned(rom.size()),t,1u);
    assert(s.phase[0]==IDLE && ram==original);
}
int main(){
    assert(setenv("XM87144_DHC_NATIVE_TEST","1",1)==0);
    assert(setenv("XM87156_PROOF_NATIVE","1",1)==0);
    exercise(XM87143Queue::QCF_PP,0x2E00u,0xD6722u,0x03u);
    exercise(XM87143Queue::QCF_KK,0x2E02u,0xD6976u,0x18u);
    noNativeTagNoMutation();
    return 0;
}
