// 87.193: native outgoing controller and reserve command have DIFFERENT
// input ownership. Verify mirrorable registers/port routing without ROMs.
#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_193_deterministic_exit_protocol.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_192_reserve_super_ownership.h"
using namespace XM87193Exit;
static void bset(std::vector<std::uint8_t>&r,unsigned a,unsigned v){
    r[a^1u]=std::uint8_t(v);
}
static void wset(std::vector<std::uint8_t>&r,unsigned a,unsigned v){
    bset(r,a,v>>8u);bset(r,a+1u,v);
}
static void lset(std::vector<std::uint8_t>&r,unsigned a,unsigned v){
    wset(r,a,v>>16u);wset(r,a+2u,v);
}
int main(){
    assert(actorBase(0u)==0x4000u&&actorBase(1u)==0x4400u);
    assert(reserveBase(0u)==0x4800u&&reserveBase(1u)==0x4C00u);
    assert(actorBase(2u)==0u&&reserveBase(2u)==0u);
    // Existing successful LP+MP/LP+HP/MP+HP remains EXACT.
    for(unsigned raw : {0x03u,0x05u,0x06u}){
        const Chord c=route(raw);
        assert(c.valid && !c.changed);
        assert(c.nativeOutgoing==raw && c.reserveOriginal==raw);
        assert(!beforeCpu(true,true,true,raw).active);
    }
    // 3P -> canonical 2P (existing). Both 2K failure variants from
    // 87.192 become the same NATIVE outgoing 2P edge BEFORE CPU,
    // while the incoming Ken still owns his original KK command.
    for(unsigned raw : {0x07u,0x18u,0x28u,0x30u,0x38u}){
        const Chord c=route(raw);
        assert(c.valid && c.changed);
        assert(c.nativeOutgoing==0x03u);
        assert(c.reserveOriginal==raw);
        const PreCpu p=beforeCpu(true,true,true,raw);
        assert(p.active && p.nativeButtons==0x03u);
        assert(p.originalButtons==raw);
        assert(!beforeCpu(false,true,true,raw).active);
        assert(!beforeCpu(true,false,true,raw).active);
        assert(!beforeCpu(true,true,false,raw).active);
    }
    // Raw fighter attacks and mixed chords cannot invoke a fake DHC.
    for(unsigned raw : {0u,0x01u,0x02u,0x04u,0x08u,0x10u,
                        0x20u,0x09u,0x13u,0x1Bu,0x40u}){
        assert(!route(raw).valid);
        assert(!beforeCpu(true,true,true,raw).active);
    }
    // P1/P2 actor layouts are mirrorable. A controller-8 native exit
    // mutates ONLY the actor supplied by the caller, never its reserve,
    // other side, or the original ROM command bytes. The game itself
    // chooses its real action 3200 after the host arms controller8.
    for(unsigned side=0u;side<2u;++side){
        std::vector<std::uint8_t> ram(65536u,0xA5u);
        const unsigned base=actorBase(side),reserve=reserveBase(side);
        wset(ram,base+0x60u,0x2E02u);
        wset(ram,base+0xA0u,0x2A28u);
        lset(ram,base+0x34u,0xD2DD0u);
        bset(ram,base+0xD6u,0x0Cu);
        bset(ram,base+0x256u,0x00u);
        bset(ram,base+0x259u,0x00u);
        wset(ram,base+0x06u,2u);
        wset(ram,base+0x08u,6u);
        wset(ram,base+0x0Au,0u);
        const auto before=ram;
        const Transaction exit=beginNativeExit(ram.data(),side);
        assert(exit.begun && exit.actor==base && exit.reserve==reserve);
        const Snapshot& snap=exit.before;
        assert(snap.action==0x2E02u&&snap.mode==0x2A28u);
        assert(snap.script==0xD2DD0u&&snap.d6==0x0Cu);
        assert(snap.controller256==0u&&snap.controller259==0u);
        assert(snap.step06==2u&&snap.step08==6u);
        assert(XM87142::ramW(ram.data(),base+0x60u)==0x2E02u);
        assert(XM87142::ramW(ram.data(),base+0xA0u)==0x002Au);
        assert(XM87142::ramL(ram.data(),base+0x34u)==0xD2DD0u);
        assert(XM87142::ramB(ram.data(),base+0xD6u)==0x0Cu);
        assert(XM87142::ramB(ram.data(),base+0x256u)==8u);
        assert(XM87142::ramB(ram.data(),base+0x259u)==8u);
        assert(XM87142::ramW(ram.data(),base+0x06u)==0u);
        assert(XM87142::ramW(ram.data(),base+0x08u)==0u);
        assert(XM87142::ramW(ram.data(),base+0x0Au)==0u);
        for(unsigned i=0;i<65536u;++i){
            const bool touched=
                i==((base+0x256u)^1u) || i==((base+0x259u)^1u) ||
                i==((base+0x06u)^1u) || i==((base+0x07u)^1u) ||
                i==((base+0x08u)^1u) || i==((base+0x09u)^1u) ||
                i==((base+0x0Au)^1u) || i==((base+0x0Bu)^1u) ||
                i==((base+0xA0u)^1u) || i==((base+0xA1u)^1u);
            if(!touched)assert(ram[i]==before[i]);
        }
        assert(ram[reserve+0x100u]==before[reserve+0x100u]);
    }
    assert(!beginNativeExit(nullptr,0u).begun);
    std::vector<std::uint8_t> invalidRam(65536u,0u);
    assert(!beginNativeExit(invalidRam.data(),2u).begun);
    // End-to-end ownership: reserve QCF+2K keeps native KK on Ken
    // even though outgoing Ryu received LP+MP tag edge.
    XM87192Owner::Proposal owned;
    owned.valid=true;owned.reserve=0x14u;
    owned.motion=XM87192Owner::QCF;owned.buttonMask=0x18u;
    const Chord routed=route(owned.buttonMask);
    assert(routed.nativeOutgoing==0x03u && routed.reserveOriginal==0x18u);
    assert(owned.input(5u,true).attacks==0x18u);
    assert(owned.input(6u,true).attacks==0x18u);
    std::puts("PASS 87.193 mirrorable native controller8 snapshot and preCPU 2K->2P outgoing, reserve KK untouched");
}
