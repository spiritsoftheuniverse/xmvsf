// 87.158 regression against ACTUAL 87.151 Ken Hadoken arrival trace:
// frame 1857 Ryu+4E=0x1C controller8, frame 1858 ROM A0=2A00
// native outgoing 3200, frame1872 real Ken swap, frame1873 kickless
// cancel 2A0C->2A08. This must remain the entry route for Ken's SUPER.
// The separate delayed normal tag / variable combo does NOT qualify.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
using namespace XM87144;
static void putW(std::vector<std::uint8_t>& v,unsigned p,unsigned n){
    v[p]=std::uint8_t(n);v[p+1u]=std::uint8_t(n>>8u);
}
static void putL(std::vector<std::uint8_t>& v,unsigned p,unsigned n){
    putW(v,p,n>>16u);putW(v,p+2u,n);
}
static void entry(std::vector<std::uint8_t>& rom,unsigned fighter,
                  unsigned act,unsigned sel,unsigned script){
    const unsigned root=fighter==0x12u?0xCFAD4u:0xD378Eu;
    const unsigned section=fighter==0x12u?0xCFCCAu:0xD3966u;
    assert(script>section);
    putL(rom,0x8A946u+fighter*2u,root);
    putW(rom,root+act,section-root);
    putW(rom,section+sel,script-section);
    putW(rom,script,0u);
    putW(rom,script+2u,3u);
    putL(rom,script+4u,0x0026A5C2u);
    putL(rom,script+8u,0x30008000u);
    putW(rom,script+12u,0u);putW(rom,script+14u,0u);
    putL(rom,0x8AA04u+fighter*2u,fighter==0x12u?0x9CA18u:0x9D0BAu);
}
int main(){
    assert(setenv("XM87144_DHC_NATIVE_TEST","1",1)==0);
    assert(setenv("XM87157_EARLY_CANCEL","1",1)==0);
    assert(setenv("XM87156_PROOF_NATIVE","0",1)==0);
    std::vector<std::uint8_t> ram(65536u,0u),rom(0xE1000u,0u);
    entry(rom,0x12u,0x32u,0u,0xD1DACu);
    entry(rom,0x14u,0x2Eu,0u,0xD6722u);
    entry(rom,0x14u,0x2Eu,2u,0xD6976u);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x12u);ww(ram.data(),0x4852u,0x14u);
    ww(ram.data(),0x4452u,0x20u);ww(ram.data(),0x4C52u,0x22u);
    ww(ram.data(),0x4060u,0x2E08u);ww(ram.data(),0x40A0u,0x2A28u);
    wb(ram.data(),0x40D6u,0x0Au);
    ww(ram.data(),0x404Eu,0x25u); // 87.157 input came at 37 frames
    ww(ram.data(),0x4214u,3u);
    assert(XM87143Queue::state().prepare(ram.data(),rom.data(),
                                        unsigned(rom.size()),99u));
    assert(XM87143Queue::state().select(
               0u,0x14u,XM87143Queue::QCF_PP,100u));
    State s;s.frame(ram.data(),rom.data(),unsigned(rom.size()),100u,1u);
    assert(s.enabled && s.earlyCancel && !s.proofNative);
    assert(s.phase[0]==WAIT_RYU_SUPER_END); // not old timer 8
    assert(rb(ram.data(),0x4256u)==0u);
    // Verified handoff begins when +4E becomes 28, not 8.
    ww(ram.data(),0x404Eu,0x001Cu);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,1u);
    assert(s.phase[0]==WAIT_TAG_CONTROLLER);
    assert(rb(ram.data(),0x4256u)==8u);
    assert(rb(ram.data(),0x4259u)==8u);
    assert(rw(ram.data(),0x40A0u)==0x002Au);
    assert(rw(ram.data(),0x404Eu)==0x001Cu);
    // Native emulator invokes controller 8 exactly as in 87.151's trace.
    wb(ram.data(),0x4256u,0u);
    ww(ram.data(),0x40A0u,0x2A00u);
    // +259 is ROM-written, but the OLD successful 87.151 bridge did NOT
    // need any additional synthetic byte equality check to recognize it.
    wb(ram.data(),0x4259u,0u);
    const unsigned beforeStock=rw(ram.data(),0x4214u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),102u,1u);
    assert(s.phase[0]==WAIT_NATIVE_SWAP);
    assert(rw(ram.data(),0x4060u)==0x3200u);
    assert(rw(ram.data(),0x40A0u)==0x2A00u);
    assert(rl(ram.data(),0x4034u)==0xD1DACu);
    assert(rb(ram.data(),0x40D6u)==0x14u);
    assert(rw(ram.data(),0x404Eu)==0x001Eu);
    assert(rw(ram.data(),0x4214u)==beforeStock);
    assert(rw(ram.data(),0x4452u)==0x20u);
    return 0;
}
