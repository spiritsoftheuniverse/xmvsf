// 87.155: simultaneous ROM target verification with independently
// injectable Ken QCF+3P Shoryureppa and QCF+3K Shinryuken.
// Synthetic pointer table only; no copyrighted CPS-2 ROM bytes.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
using namespace XM87144;
static void romW(std::vector<std::uint8_t>& rom,unsigned at,unsigned n){
    rom[at]=std::uint8_t(n);rom[at+1u]=std::uint8_t(n>>8);
}
static void romL(std::vector<std::uint8_t>& rom,unsigned at,unsigned n){
    romW(rom,at,n>>16);romW(rom,at+2u,n);
}
static void setKenEntry(std::vector<std::uint8_t>& rom,unsigned sub,unsigned script){
    const unsigned table=0xD378Eu,section=0xD3966u,fighter=0x14u;
    romL(rom,0x8A946u+fighter*2u,table);
    romW(rom,table+0x2Eu,section-table);
    romW(rom,section+sub,script-section);
    romW(rom,script+2u,3u);
    romL(rom,script+4u,0x0026A5C2u);
    romL(rom,0x8AA04u+fighter*2u,0x9D0BAu);
}
static void trySuper(std::vector<std::uint8_t>& rom,
                     XM87143Queue::Command command,
                     unsigned expectedAction,unsigned expectedScript){
    std::vector<std::uint8_t> ram(65536u,0u);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x14u); // Ken is native point already
    ww(ram.data(),0x4852u,0x12u);
    ww(ram.data(),0x4060u,0x0002u);
    ww(ram.data(),0x40A0u,0x020Eu); // actual Ken input-ready state (87.151)
    wb(ram.data(),0x404Bu,1u); // facing RIGHT, so QCF => down/down-right/right
    State s;
    s.phase[0]=WAIT_KEN_EXIT;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    s.command[0]=command;
    s.kenExitObserved[0]=true;
    s.kenExitAt[0]=110u;
    for(unsigned t=116;t<=118;++t)
        s.frame(ram.data(),rom.data(),unsigned(rom.size()),t,1u);
    assert(s.phase[0]==DELIVER_NATIVE_INPUT);
    assert(s.kenExpectedAction[0]==expectedAction);
    assert(s.kenExpectedScript[0]==expectedScript);
    assert(s.inputBegin[0]==120u);
    const auto original=ram;
    auto m=s.inputForFrame(ram.data(),120u);
    assert(m.active && m.directions==4u && m.attacks==0u);
    m=s.inputForFrame(ram.data(),124u);
    assert(m.active && m.directions==5u && m.attacks==0u);
    m=s.inputForFrame(ram.data(),128u);
    assert(m.active && m.directions==1u && m.attacks==0u);
    m=s.inputForFrame(ram.data(),131u);
    assert(m.active && m.directions==1u &&
           m.attacks==(command==XM87143Queue::QCF_KK?0x18u:0x03u));
    assert(ram==original); // real native port injection, never ROM script hack
    // The ROM eventually confirms the correct Ken action; no 2Cxx fake.
    ww(ram.data(),0x4060u,expectedAction);
    ww(ram.data(),0x40A0u,0x2A02u);
    wl(ram.data(),0x4034u,expectedScript);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),132u,1u);
    assert(s.phase[0]==SUPER_STARTED);
    assert(s.kenSuperConfirmed[0] && s.chainUsed[0]);
    assert(!s.inputForFrame(ram.data(),133u).active);
}
static void wrongSuperOnlyUnlocks(){
    std::vector<std::uint8_t> rom(0xE1000u,0u);
    setKenEntry(rom,0x00u,0xD6722u);
    setKenEntry(rom,0x02u,0xD6976u);
    std::vector<std::uint8_t> ram(65536u,0u);
    wb(ram.data(),0x5000u,8u);
    ww(ram.data(),0x4052u,0x14u);ww(ram.data(),0x4852u,0x12u);
    State s;s.phase[0]=DELIVER_NATIVE_INPUT;
    s.outgoing[0]=0x12u;s.incoming[0]=0x14u;
    s.command[0]=XM87143Queue::QCF_PP;
    s.inputBegin[0]=100u;
    s.kenExpectedAction[0]=0x2E00u;
    s.kenExpectedScript[0]=0xD6722u;
    // Shinryuken is real but the player selected Shoryureppa.
    ww(ram.data(),0x4060u,0x2E02u);
    ww(ram.data(),0x40A0u,0x2A02u);
    wl(ram.data(),0x4034u,0xD6976u);
    const auto before=ram;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),110u,1u);
    assert(s.phase[0]==IDLE && !s.chainUsed[0]);
    assert(ram==before); // never overwrites an actual ROM native super
}
int main(){
    assert(setenv("XM87144_DHC_NATIVE_TEST","1",1)==0);
    assert(setenv("XM87144_DHC_LEGACY_TAG_WRITER","1",1)==0);
    std::vector<std::uint8_t> rom(0xE1000u,0u);
    setKenEntry(rom,0x00u,0xD6722u);
    setKenEntry(rom,0x02u,0xD6976u);
    const auto rep=XM87142::resolve(rom.data(),unsigned(rom.size()),0x14u,0x2Eu,0x00u);
    const auto shin=XM87142::resolve(rom.data(),unsigned(rom.size()),0x14u,0x2Eu,0x02u);
    assert(rep.valid && rep.script==0xD6722u);
    assert(shin.valid && shin.script==0xD6976u);
    trySuper(rom,XM87143Queue::QCF_PP,0x2E00u,0xD6722u);
    trySuper(rom,XM87143Queue::QCF_KK,0x2E02u,0xD6976u);
    wrongSuperOnlyUnlocks();
    return 0;
}
