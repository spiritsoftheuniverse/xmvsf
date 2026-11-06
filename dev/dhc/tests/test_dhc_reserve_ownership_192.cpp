// 87.192: no hardcoded named super list; the incoming native 68000
// actor's ROM action owns super classification and catalog insertion.
#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_192_reserve_super_ownership.h"
using namespace XM87192Owner;
using XM87184Life::Sample;
static void w16(std::vector<std::uint8_t>&r,unsigned a,unsigned n){
    r[a]=std::uint8_t(n);r[a+1u]=std::uint8_t(n>>8u);
}
static void w32(std::vector<std::uint8_t>&r,unsigned a,unsigned n){
    w16(r,a,n>>16u);w16(r,a+2u,n);
}
static void add(std::vector<std::uint8_t>&r,unsigned actor,
                unsigned section,unsigned selector,unsigned script){
    const unsigned table=actor==0x14u?0xD378Eu:0xCFAD4u;
    w32(r,0x8A946u+actor*2u,table);
    w16(r,table+0x2Eu,section-table);
    w16(r,section+selector,script-section);
}
int main(){
    // The proposal must only encode a HUMAN gesture. It does NOT
    // supply a fighter-specific ROM action, special name or selector.
    State owner;
    assert(!owner.propose(0u,0x14u,100u,NONE,0x03u));
    assert(!owner.propose(0u,0x14u,100u,QCF,0u));
    assert(!owner.propose(0u,0x14u,100u,QCF,0x01u)); // ordinary special, no swap
    assert(owner.propose(0u,0x14u,100u,QCF,0x07u));
    assert(owner.fresh(0u,0x14u,100u));
    assert(!owner.fresh(0u,0x12u,100u));
    assert(!owner.fresh(0u,0x14u,101u));
    const Proposal p=owner.pending[0];
    assert(p.valid && p.reserve==0x14u && p.duration()==9u);
    assert(p.input(0u,true).active && p.input(0u,true).directions==4u);
    assert(p.input(2u,true).directions==5u);
    assert(p.input(4u,true).directions==1u);
    assert(p.input(5u,true).attacks==0x03u); // 3P -> 2P
    assert(p.input(6u,true).attacks==0x03u);
    assert(!p.input(9u,true).active);
    assert(p.input(2u,false).directions==6u);
    // Reserve owns QCB, DPF, HCF and double QCF too; no Ken list.
    assert(owner.propose(0u,0x14u,101u,QCB,0x18u));
    assert(owner.pending[0].input(0u,true).directions==4u);
    assert(owner.pending[0].input(2u,true).directions==6u);
    assert(owner.pending[0].input(4u,true).directions==2u);
    assert(owner.pending[0].input(5u,true).attacks==0x18u);
    assert(owner.propose(0u,0x14u,102u,DPF,0x03u));
    assert(owner.pending[0].input(0u,true).directions==1u);
    assert(owner.pending[0].input(2u,true).directions==4u);
    assert(owner.pending[0].input(4u,true).directions==5u);
    assert(owner.propose(0u,0x14u,103u,HCF,0x03u));
    assert(owner.pending[0].duration()==13u);
    assert(owner.propose(0u,0x14u,104u,DOUBLE_QCF,0x03u));
    assert(owner.pending[0].duration()==15u);
    std::vector<std::uint8_t> rom(0xE1000u,0u);
    add(rom,0x14u,0xD3966u,0x00u,0xD6722u);
    add(rom,0x14u,0xD3966u,0x02u,0xD6976u);
    add(rom,0x14u,0xD3966u,0x04u,0xD6A00u);
    // Native special has valid script and input but is NOT super.
    Sample observed(0x14u,0x2C02u,0x2A02u,2u,0xD6722u,0x0Cu);
    assert(!State::nativeSuper(rom.data(),unsigned(rom.size()),observed));
    assert(!owner.confirm(0u,110u,p,observed,rom.data(),unsigned(rom.size())));
    assert(owner.learnedCount==0u);
    observed=Sample(0x14u,0x2E00u,0x2A02u,2u,0xD6722u,0x0Cu);
    unsigned resolved=0u;
    assert(State::nativeSuper(rom.data(),unsigned(rom.size()),
                              observed,&resolved));
    assert(resolved==0xD6722u);
    // A wrong owner, fake ROM pointer, no D6, or odd native step rejects.
    assert(!owner.confirm(0u,110u,p,
        Sample(0x12u,0x2E00u,0x2A02u,2u,0xD6722u,0x0Cu),
        rom.data(),unsigned(rom.size())));
    assert(!State::nativeSuper(rom.data(),unsigned(rom.size()),
        Sample(0x14u,0x2E00u,0x2A02u,2u,0xD6732u,0u)));
    assert(!State::nativeSuper(rom.data(),unsigned(rom.size()),
        Sample(0x14u,0x2E00u,0x2A02u,3u,0xD6722u,0x0Cu)));
    assert(!State::nativeSuper(rom.data(),unsigned(rom.size()),
        Sample(0x14u,0x2E00u,0x2A02u,4u,0xD6A00u,0x0Cu)));
    assert(owner.confirm(0u,110u,p,observed,
        rom.data(),unsigned(rom.size())));
    assert(owner.learnedCount==1u);
    assert(owner.learned[0].fighter==0x14u);
    assert(owner.learned[0].action==0x2E00u);
    assert(owner.learned[0].script==0xD6722u);
    assert(owner.confirm(0u,111u,p,observed,
        rom.data(),unsigned(rom.size())));
    assert(owner.learnedCount==1u); // no duplicate
    observed=Sample(0x14u,0x2E02u,0x2A28u,4u,0xD69A6u,0x0Cu);
    assert(State::nativeSuper(rom.data(),unsigned(rom.size()),observed));
    assert(owner.confirm(0u,112u,p,observed,
        rom.data(),unsigned(rom.size())));
    assert(owner.learnedCount==2u);
    assert(owner.learned[1].action==0x2E02u);
    // Any ROM 2E action, not only known Ken moves, can be discovered.
    observed=Sample(0x14u,0x2E04u,0x2A28u,2u,0xD6A00u,0x0Cu);
    assert(owner.confirm(0u,113u,p,observed,
        rom.data(),unsigned(rom.size())));
    assert(owner.learnedCount==3u);
    // Native super list is discovered from original ROM *point*
    // actions, regardless of character name, even before any DHC.
    // It remains an INPUT-UNMAPPED candidate until reserve ownership
    // confirms a real outgoing command after the native swap.
    State natural;
    const Sample root(0x14u,0x2E00u,0x2A28u,2u,0xD6722u,0x0Cu);
    assert(natural.observeNatural(0u,120u,root,
                                 rom.data(),unsigned(rom.size())));
    assert(natural.learnedCount==1u &&
           natural.learned[0].motion==NONE);
    assert(!natural.observeNatural(0u,121u,root,
                                  rom.data(),unsigned(rom.size())));
    assert(!natural.observeNatural(0u,122u,
        Sample(0x14u,0x2C00u,0x2A28u,2u,0xD6722u,0x0Cu),
        rom.data(),unsigned(rom.size())));
    assert(natural.confirm(0u,123u,p,root,
                           rom.data(),unsigned(rom.size())));
    assert(natural.learnedCount==1u);
    assert(natural.learned[0].motion==QCF &&
           natural.learned[0].buttons==0x07u);
    owner.clear(0u);
    assert(!owner.fresh(0u,0x14u,104u));
    std::puts("PASS 87.192 reserve-owned motion, 68000 2E super discovery, non-super refused");
}
