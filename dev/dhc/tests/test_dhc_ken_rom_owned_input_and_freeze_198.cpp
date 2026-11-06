// 87.198: replay evidence for five native Ken supers vs nine
// failed HCF proposals, and original super hitbox freeze cue.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_192_reserve_super_ownership.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_197_cinematic_opponent_hold.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_198_native_super_freeze_cue.h"
static void w(std::uint8_t* r,unsigned a,unsigned value){
    r[a^1u]=std::uint8_t(value>>8u);
    r[(a+1u)^1u]=std::uint8_t(value);
}
int main(){
    using namespace XM87192Owner;
    using XM87198Cue::realKenSuperOffense;
    assert(setenv("XM87198_TEST_KEN_HCF_PRETAG_REJECTION","1",1)==0);
    State catalog;
    // The five native 2E accepted inputs in 87.197 were ONLY QCF.
    assert(catalog.propose(0u,0x14u,100u,QCF,0x03u));
    assert(catalog.pending[0].valid);
    assert(catalog.propose(0u,0x14u,101u,QCF,0x18u));
    assert(catalog.propose(0u,0x14u,102u,QCF,0x30u));
    // Nine of nine half-circle forward Ken handoffs did NOT start 2E:
    // 0006/D3A4A. Do not swap Ryu out for the wrong super, and do
    // not forge a QCF out of the player's actual HCF gesture.
    assert(!catalog.propose(0u,0x14u,103u,HCF,0x03u));
    assert(!catalog.pending[0].valid);
    assert(!catalog.fresh(0u,0x14u,103u));
    assert(!catalog.propose(0u,0x14u,104u,HCF,0x18u));
    assert(!catalog.propose(0u,0x14u,105u,HCF,0x30u));
    assert(!catalog.propose(1u,0x14u,106u,HCF,0x30u));
    // Don't presume Ken HCB, QCB or other characters have the same
    // ROM command table as this isolated Ken HCF failure.
    assert(catalog.propose(0u,0x12u,107u,HCF,0x30u));
    assert(catalog.propose(0u,0x14u,108u,HCB,0x30u));
    assert(catalog.propose(0u,0x14u,109u,QCB,0x18u));
    assert(!State::provenKenNonSuper(0x14u,HCF,0x01u));
    assert(!State::provenKenNonSuper(0x14u,HCF,0x09u));
    // A/B compatibility switch: old unrestricted 87.197 gesture capture.
    assert(setenv("XM87198_TEST_KEN_HCF_PRETAG_REJECTION","0",1)==0);
    assert(catalog.propose(0u,0x14u,110u,HCF,0x30u));

    // Ken 2E00 has no offensive box for 47 native frames after
    // action selection (10553), and 2E02 for 52-53 native frames
    // (2326, 11049, 11891, 17490). Release on the real box.
    assert(!realKenSuperOffense(0x14u,0x2E02u,0x2A02u,2u,0u));
    assert(!realKenSuperOffense(0x14u,0x2E00u,0x2A02u,4u,0u));
    assert(realKenSuperOffense(0x14u,0x2E02u,0x2A02u,4u,0x50u));
    assert(realKenSuperOffense(0x14u,0x2E00u,0x2A02u,4u,0x64u));
    assert(!realKenSuperOffense(0x14u,0x0006u,0x0002u,4u,0x50u));
    assert(!realKenSuperOffense(0x14u,0x2E02u,0x2A02u,2u,0x50u));
    assert(!realKenSuperOffense(0x12u,0x2E02u,0x2A02u,4u,0x50u));
    assert(!realKenSuperOffense(0x14u,0x2E02u,0x020Eu,4u,0x50u));
    assert(!realKenSuperOffense(0x14u,0x2E02u,0x2A02u,4u,0x8080u));
    assert(XM87197Hold::Freeze::phaseHolds(6u));

    // Final-frame release must NOT roll back native first-hit changes.
    // The 197 branch restored a full opponent snapshot; repeating it
    // on the first attacking frame could erase real ROM damage/stun.
    std::vector<std::uint8_t> ram(65536u,0u);
    XM87197Hold::wb(ram.data(),0x5000u,8u);
    w(ram.data(),0x4452u,0x20u);
    w(ram.data(),0x4460u,0x1C18u);
    w(ram.data(),0x440Cu,650u);
    w(ram.data(),0x4470u,0x0050u);
    XM87197Hold::Freeze hold;
    assert(hold.capture(ram.data(),0u,200u,3u,0x12u,0x14u));
    assert(hold.hold(ram.data(),201u,12u));
    assert(hold.hold(ram.data(),202u,8u));
    assert(hold.hold(ram.data(),203u,6u));
    assert(hold.active && hold.held==3u);
    // First offensive-frame CPU impact happened, simulate its new
    // opponent location / health marker. The host releases without
    // replacing those native writes with the pre-DHC snapshot.
    w(ram.data(),0x440Cu,651u);
    w(ram.data(),0x4460u,0x0600u);
    w(ram.data(),0x4430u,0x1234u);
    assert(hold.releaseAtNativeOffense(ram.data()));
    assert(!hold.active);
    assert(XM87197Hold::rw(ram.data(),0x440Cu)==651u);
    assert(XM87197Hold::rw(ram.data(),0x4460u)==0x0600u);
    assert(XM87197Hold::rw(ram.data(),0x4430u)==0x1234u);
    std::puts("PASS 87.198: Ken HCF non-super rejected before 3200; QCF unaffected; original 2E live-offense freeze release preserves first hit");
}
