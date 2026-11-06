// 87.196: verify the original Ken fall state that native CPS2 ROM
// selected exactly 67 frames after 2A08 in all five 87.195 captures.
// No proprietary ROM data: synthetic one-cell action lookup.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
using namespace XM87144;
static void romW(std::vector<std::uint8_t>&r,unsigned a,unsigned v){
    r[a]=std::uint8_t(v);r[a+1u]=std::uint8_t(v>>8u);
}
static void romL(std::vector<std::uint8_t>&r,unsigned a,unsigned v){
    romW(r,a,v>>16u);romW(r,a+2u,v);
}
int main(){
    std::vector<std::uint8_t> rom(0xE0000u,0u), ram(65536u,0u);
    const unsigned fighter=0x14u,table=0xD378Eu,
                   section=0xD3922u,script=0xD3D76u;
    romL(rom,0x8A946u+fighter*2u,table);
    romW(rom,table+0x02u,section-table);
    romW(rom,section+0x12u,script-section);
    romW(rom,script,0u);romW(rom,script+2u,3u);
    romL(rom,script+4u,0x0026A5C2u);
    romL(rom,script+8u,0x30008000u);
    romW(rom,script+12u,0u);romW(rom,script+14u,0u);
    romL(rom,0x8AA04u+fighter*2u,0x9D0BAu);
    const auto resolved=XM87142::resolve(rom.data(),
                       unsigned(rom.size()),fighter,0x02u,0x12u);
    assert(resolved.valid && resolved.script==0xD3D76u);
    using XM87196Fall::choose;
    const auto valid=choose(fighter,0x2A0Cu,0x10u,408u,472u,true,
                             resolved.script);
    assert(valid.valid && valid.action==0x02u &&
           valid.selector==0x12u && valid.script==0xD3D76u &&
           valid.mode==0x0600u && valid.d6==0x12u);
    assert(valid.verticalVelocity==0u && valid.gravity==0x6000u);
    assert(choose(fighter,0x2A0Eu,0x10u,118u,472u,true,
                  resolved.script).valid); // opposite-side/high entry
    // Explicitly refuse all unsafe substitutions:
    assert(!choose(0x0Au,0x2A0Cu,0x10u,408u,472u,true,
                   resolved.script).valid); // Sabretooth unverified
    assert(!choose(fighter,0x0000u,0x10u,408u,472u,true,
                   resolved.script).valid); // ordinary neutral
    assert(!choose(fighter,0x2A0Cu,0x12u,408u,472u,true,
                   resolved.script).valid); // not active tag controller
    assert(!choose(fighter,0x2A0Cu,0x10u,472u,472u,true,
                   resolved.script).valid); // already landed
    assert(!choose(fighter,0x2A0Cu,0x10u,408u,472u,false,
                   resolved.script).valid); // TEST opt-out
    assert(!choose(fighter,0x2A0Cu,0x10u,408u,472u,true,
                   0xD567Eu).valid); // old 2A08 pose is NOT fall
    assert(!choose(fighter,0x2A0Cu,0x10u,408u,472u,true,
                   0u).valid);
    // ROM script init must never mutate x/y/fractional positioning,
    // motion longs, opponent, partner or meter. The caller then applies
    // the trace-proven physical fall controller 0600+D6=12.
    wl(ram.data(),0x400Cu,0x01300088u);
    wl(ram.data(),0x4010u,0x01980010u);
    wl(ram.data(),0x4014u,0x00120000u);
    wl(ram.data(),0x401Cu,0x00002000u);
    wl(ram.data(),0x4020u,0x00006000u);
    ww(ram.data(),0x4452u,0x20u);
    ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4214u,3u);
    const unsigned origX=rl(ram.data(),0x400Cu);
    const unsigned origY=rl(ram.data(),0x4010u);
    assert(setNativeScript(ram.data(),rom.data(),unsigned(rom.size()),
                           0x4000u,fighter,valid.action,valid.selector));
    assert(rw(ram.data(),0x4060u)==0x0212u);
    assert(rl(ram.data(),0x4034u)==0xD3D76u);
    assert(rl(ram.data(),0x400Cu)==origX);
    assert(rl(ram.data(),0x4010u)==origY);
    assert(rl(ram.data(),0x4014u)==0x00120000u);
    assert(rl(ram.data(),0x401Cu)==0x00002000u);
    assert(rl(ram.data(),0x4020u)==0x00006000u);
    ww(ram.data(),0x40A0u,valid.mode);
    wb(ram.data(),0x40D6u,valid.d6);
    wl(ram.data(),0x4014u,0u);wl(ram.data(),0x4018u,0u);
    wl(ram.data(),0x401Cu,valid.verticalVelocity);
    wl(ram.data(),0x4020u,valid.gravity);
    assert(rw(ram.data(),0x40A0u)==0x0600u);
    assert(rb(ram.data(),0x40D6u)==0x12u);
    assert(rl(ram.data(),0x401Cu)==0u);
    assert(rl(ram.data(),0x4020u)==0x6000u);
    assert(rl(ram.data(),0x400Cu)==origX);
    assert(rl(ram.data(),0x4010u)==origY);
    assert(rw(ram.data(),0x4452u)==0x20u);
    assert(rw(ram.data(),0x4852u)==0x12u);
    assert(rw(ram.data(),0x4214u)==3u);
    std::puts("PASS 87.196 original 0212/0600/D3D76 entry: no 2A08 lag, native gravity, no snapped X/Y/opponent/meter");
}
