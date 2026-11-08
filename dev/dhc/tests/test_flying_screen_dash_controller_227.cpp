// TEST 87.227: original 87.226 single-field neutral restore was overridden
// by 497 consecutive 68000 dash reentries, with X stuck at 0x704E7800.
// After a bounded number of repeats, restore the previously observed SAME
// fighter native idle/controller slice, but never the health/meter region.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
static std::uint8_t ram[65536]={};
static constexpr unsigned P=0x4000u;
static void sw(unsigned p,unsigned v){setWord(ram,p,v);}
static void sl(unsigned p,std::uint32_t v){setLong(ram,p,v);}
static unsigned rw(unsigned p){return word(ram,p);}
static std::uint32_t rl(unsigned p){return longword(ram,p);}
static void neutral(unsigned fighter){
    sw(P,0x0101u);sw(P+0x52u,fighter);
    sw(P+0x06u,0u);sw(P+0x08u,0u);
    sw(P+0xa0u,0u);sw(P+0xaau,0u);
    sw(P+0x60u,0u);sw(P+0x3eu,6u);
    sw(P+0x42u,0x0032u);
    sl(P+0x0cu,0x02b90000u);
    sl(P+0x14u,0u);sl(P+0x18u,0u);
    sl(P+0x34u,0x000ac000u);sl(P+0x38u,0x000ac010u);
    sw(P+0x82u,0u);sw(P+0x90u,0u);
}
static void corrupt(){
    sw(P+0xa0u,0x1400u);
    sl(P+0x0cu,0x704e7800u);
    sl(P+0x14u,0u);
    sl(P+0x34u,0x000ec944u);
    sl(P+0x38u,0x000ec94eu);
    sw(P+0x3eu,6u);
    sw(P+0x82u,0x8080u);
    sw(P+0x90u,0x9badu);
}
int main(){
    ram[0x5001u]=8u;
    Controller ctrl;
    neutral(0x0020u);
    sw(P+0x210u,0x038fu);
    ctrl.frame(1u,ram,true);
    assert(ctrl.actor[0].neutralSnapshotValid);
    assert(ctrl.actor[0].safeValid);
    assert(ctrl.actor[0].snapshotFrame==1u);
    assert(rw(P+0x210u)==0x038fu);

    corrupt();
    ctrl.frame(2u,ram,true);
    assert(ctrl.actor[0].recoveryActive);
    assert(rl(P+0x0cu)==0x02b90000u);
    assert(rw(P+0x90u)==0x9badu); // selective 87.226 restore only

    for(unsigned frame=3u;frame<=6u;++frame){
        corrupt(); // ROM reinstates its invalid X and dash every frame
        ctrl.frame(frame,ram,true);
    }
    assert(ctrl.actor[0].snapshotRestores==1u);
    assert(rl(P+0x0cu)==0x02b90000u);
    assert(rw(P+0xa0u)==0u);
    assert(rw(P+0x90u)==0u); // CPU-owned idle state really restored
    assert(rw(P+0x82u)==0u);
    assert(rl(P+0x34u)==0x000ac000u);
    assert(rw(P+0x210u)==0x038fu); // HP outside controller copy

    // Non-dash native frames rearm legitimate dash startup.
    for(unsigned frame=7u;frame<=9u;++frame){
        neutral(0x0020u);ctrl.frame(frame,ram,true);
    }
    assert(!ctrl.actor[0].recoveryActive);
    sw(P+0xa0u,0x1402u);
    sw(P+0x3eu,12u);
    ctrl.frame(10u,ram,true);
    assert(rw(P+0xa0u)==0x1402u);
    assert(rw(P+0x3eu)==1u);

    // Never replay a different fighter's neutral memory into a tagged-in
    // actor. The existing identity gate resets the controller history.
    neutral(0x0004u);ctrl.frame(11u,ram,true);
    assert(ctrl.actor[0].identity==~0u ||
           ctrl.actor[0].neutral.identity==0x0004u);
    assert(!ctrl.actor[0].recoveryActive);
    std::puts("XMVSF 87.227 dash native controller restore / HP safe / rearm: PASS");
    return 0;
}
