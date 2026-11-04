// 87.160 catalog is READ-ONLY: native 2E script is data evidence, not a
// proof that the recorded directional input is its actual ROM command.
// Do not enable discovered new supers for DHC by inference.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_160_super_discovery.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
using namespace XM87160;
static void putW(std::vector<std::uint8_t>& a,unsigned p,unsigned x){
    a[p]=std::uint8_t(x);a[p+1u]=std::uint8_t(x>>8);
}
static void putL(std::vector<std::uint8_t>& a,unsigned p,unsigned x){
    putW(a,p,x>>16);putW(a,p+2u,x);
}
static void script(std::vector<std::uint8_t>& rom,unsigned id,
                   unsigned selector,unsigned addr,unsigned root,unsigned section){
    assert(addr>section);
    putL(rom,0x8A946u+id*2u,root);
    putW(rom,root+0x2Eu,section-root);
    putW(rom,section+selector,addr-section);
    putW(rom,addr+0u,0u);
    putW(rom,addr+2u,3u);
    putL(rom,addr+4u,0x0026A5C2u);
    putL(rom,addr+8u,0x30008000u);
    putW(rom,addr+12u,0u);
    putW(rom,addr+14u,0u);
}
int main(){
    assert(setenv("XM87160_DISCOVER","1",1)==0);
    std::vector<std::uint8_t> rom(0xE1000u,0u),ram(65536u,0u);
    script(rom,0x12u,0x08u,0xD3070u,0xCFAD4u,0xCFCA2u);
    script(rom,0x14u,0x00u,0xD6722u,0xD378Eu,0xD3966u);
    script(rom,0x14u,0x02u,0xD6976u,0xD378Eu,0xD3966u);
    assert(XM87160::State::selectorLimit(rom.data(),
        unsigned(rom.size()),0x14u)>2u);
    assert(XM87142::resolve(rom.data(),unsigned(rom.size()),
        0x14u,0x2Eu,0u).script==0xD6722u);
    State s;s.discover(rom.data(),unsigned(rom.size()));
    assert(s.enabled && s.catalogDone && s.candidateCount>=3u);
    assert(XM87160::State::label(0x14u,0x2Eu,0u,0xD6722u));
    assert(!XM87160::State::label(0x14u,0x2Eu,0u,0xD6976u));
    // Input co-occurrence is not automatically a callable super.
    // Synthetic first Ryu Shinkuu action from actual ROM pointer pair:
    XM87142::ramB(ram.data(),0x5000u); // untouched
    ram[(0x5000u)^1u]=8u;
    ram[(0x4052u)^1u]=0u;ram[(0x4053u)^1u]=0x12u;
    ram[(0x4852u)^1u]=0u;ram[(0x4853u)^1u]=0x14u;
    ram[(0x4060u)^1u]=0x2Eu;ram[(0x4061u)^1u]=0x08u;
    ram[(0x4034u)^1u]=0x00u;ram[(0x4035u)^1u]=0x0Du;
    ram[(0x4036u)^1u]=0x30u;ram[(0x4037u)^1u]=0x70u;
    ram[(0x404Bu)^1u]=1u;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),
            100u,1u,0u,3u,0u,false);
    assert(s.observedCount==1u);
    assert(s.lastObservedFighter[0]==0x12u);
    assert(s.lastObservedAction[0]==0x2E08u);
    assert(s.lastObservedScript[0]==0xD3070u);
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),
            101u,1u,0u,3u,0u,false);
    assert(s.observedCount==1u); // no repeated event for same action/script
    // Actual next fighter, Ken, with a separate true native 2E entry:
    ram[(0x4052u)^1u]=0u;ram[(0x4053u)^1u]=0x14u;
    ram[(0x4852u)^1u]=0u;ram[(0x4853u)^1u]=0x12u;
    ram[(0x4060u)^1u]=0x2Eu;ram[(0x4061u)^1u]=0u;
    ram[(0x4034u)^1u]=0x00u;ram[(0x4035u)^1u]=0x0Du;
    ram[(0x4036u)^1u]=0x67u;ram[(0x4037u)^1u]=0x22u;
    s.frame(ram.data(),rom.data(),unsigned(rom.size()),
            102u,1u,0u,0u,0u,true);
    assert(s.observedCount==2u);
    assert(s.lastObservedFighter[0]==0x14u);
    assert(s.lastObservedAction[0]==0x2E00u);
    assert(s.lastObservedScript[0]==0xD6722u);
    return 0;
}
