// Stage 87.274 standalone C++11 tests for the opt-in universal DHC core.
// Run locally: g++ -std=c++11 -O2 -Wall -Wextra -Idev/stable/buildtree/src/burn/drv/capcom
//   dev/dhc/tests/test_dhc_unified_274.cpp -o test_dhc_unified_274.exe
// No GitHub Actions or additional native BUILD_TEST gates.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_274_unified_dhc.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void envSet(const char*name,const char*value){
#if defined(_WIN32) && !defined(__MSYS__)
    _putenv_s(name,value);
#else
    setenv(name,value,1);
#endif
}
static void player(std::uint8_t*ram,unsigned side,unsigned point,unsigned reserve){
    const unsigned p=XM87193Exit::actorBase(side);
    const unsigned r=XM87193Exit::reserveBase(side);
    XM87193Exit::wput(ram,p+0x52u,point);
    XM87193Exit::wput(ram,r+0x52u,reserve);
}
int main(){
    envSet("XM8755_GAME_VERSION","4");
    envSet("XM87274_UNIFIED_DHC","1");
    auto &s=XM87274Unified::state();
    assert(s.enabled());
    assert(XM87274Unified::State::validPair(0x12u,0x14u));
    assert(XM87274Unified::State::validPair(0x20u,0x22u));
    assert(!XM87274Unified::State::validPair(0x12u,0x12u));
    assert(!XM87274Unified::State::validPair(0x13u,0x14u));
    std::uint8_t ram[65536]={};
    std::uint8_t rom[32]={};
    XM87193Exit::bput(ram,0x5000u,8u);
    player(ram,0,0x12u,0x14u);
    player(ram,1,0x22u,0x20u);
    const unsigned p2=XM87193Exit::actorBase(1);
    XM87193Exit::wput(ram,p2+0x60u,0u);
    XM87193Exit::wput(ram,p2+0xA0u,0u);
    assert(XM87274Unified::State::neutral(ram,p2));
    s.reset();
    XM87274Unified::Slot &p=s.slot[1];
    p.phase=XM87274Unified::AWAIT_NEUTRAL;
    p.point=0x20u;p.reserve=0x22u;p.started=10u;
    const std::uint8_t before=XM87142::ramB(ram,p2+0x256u);
    s.frame(ram,rom,sizeof(rom),10u);
    s.frame(ram,rom,sizeof(rom),11u);
    s.frame(ram,rom,sizeof(rom),12u);
    assert(p.phase==XM87274Unified::REPLAY_RESERVE_INPUT);
    assert(p.inputStart==13u);
    p.request.valid=true;p.request.reserve=0x22u;
    p.request.motion=XM87192Owner::QCF;
    p.request.buttonMask=0x07u;
    XM87193Exit::bput(ram,p2+0x4Bu,1u); // face right
    const auto down=s.inputForFrame(ram,1u,13u,0u,0u,true);
    assert(down.active && down.directions==4u && down.attacks==0u);
    const auto forward=s.inputForFrame(ram,1u,18u,0u,0u,true);
    assert(forward.active && forward.directions==1u &&
           forward.attacks==0x03u); // 3P -> original native 2P
    const auto other=s.inputForFrame(ram,0u,18u,0u,0u,true);
    assert(!other.active); // no cross-controller port contamination
    assert(XM87142::ramB(ram,p2+0x256u)==before);
    // Wrong action after original-ROM input replay is rejected and does
    // not invent a 2E super or manipulate the point/reserve actor.
    s.frame(ram,rom,sizeof(rom),50u);
    assert(p.phase==XM87274Unified::IDLE);
    assert(XM87142::ramW(ram,p2+0x52u)==0x22u);
    assert(XM87142::ramW(ram,0x4C52u)==0x20u);
    // Once an original-ROM super has been accepted, it retains the
    // one-chain lock until the incoming actor actually returns to neutral.
    p.phase=XM87274Unified::INCOMING_SUPER;
    p.point=0x20u;p.reserve=0x22u;
    XM87193Exit::wput(ram,p2+0x60u,0x2E04u);
    XM87193Exit::wput(ram,p2+0xA0u,0x2A28u);
    s.frame(ram,rom,sizeof(rom),51u);
    assert(p.phase==XM87274Unified::INCOMING_SUPER);
    XM87193Exit::wput(ram,p2+0x60u,0u);
    XM87193Exit::wput(ram,p2+0xA0u,0u);
    s.frame(ram,rom,sizeof(rom),52u);
    s.frame(ram,rom,sizeof(rom),53u);
    s.frame(ram,rom,sizeof(rom),54u);
    assert(p.phase==XM87274Unified::IDLE);
    // 87.310: reproduce the captured continuation-only DHC window.
    // A ROM-bounded 2E22 stage is eligible only for the EXPERIMENTAL
    // unified path; a terminal tail or mismatched script is not.
    XM87189Family::Tracker tracker;
    tracker.live=true;tracker.lastFrame=2579u;
    tracker.phase=XM87189Family::CONTINUATION;
    tracker.fighter=0x0Eu;tracker.rootAction=0x2E20u;
    tracker.stageAction=0x2E22u;
    tracker.stageScript=0x0CDA3Eu;tracker.stageEnd=0x0CDB00u;
    tracker.lastObservedScript=0x0CDA3Eu;
    tracker.originalD6=0x0Cu;
    const XM87184Life::Sample continuation={
        0x0Eu,0x2E22u,0x2A28u,4u,0x0CDA3Eu,0x0Cu};
    assert(!tracker.activeAt(2579u,continuation)); // legacy root only
    assert(tracker.boundedContinuationAt(2579u,continuation));
    assert(tracker.unifiedDhcWindowAt(2579u,continuation));
    tracker.stageEnd=0u; // terminal 2E table tail is NOT writable
    assert(!tracker.unifiedDhcWindowAt(2579u,continuation));
    tracker.stageEnd=0x0CDB00u;
    tracker.lastObservedScript=0x0CDA4Eu;
    assert(!tracker.unifiedDhcWindowAt(2579u,continuation));
    tracker.lastObservedScript=0x0CDA3Eu;
    assert(!tracker.unifiedDhcWindowAt(2580u,continuation)); // stale frame
    // 87.339: the uploaded original-ROM session showed controller 8
    // armed but NEVER accepted: outgoing punches were being forced to
    // zero, not to a fresh native tag edge. Verify the ACTUAL outgoing
    // port while preserving the incoming fighter's stored Hyper chord.
    s.reset();
    player(ram,0,0x12u,0x14u);
    player(ram,1,0x22u,0x20u);
    XM87193Exit::wput(ram,0x4060u,0x2E08u);
    XM87193Exit::wput(ram,0x40A0u,0x2A00u);
    XM87193Exit::wput(ram,0x4460u,0x2E00u);
    XM87193Exit::wput(ram,0x44A0u,0x2A00u);
    XM87274Unified::Slot& p1=s.slot[0];
    p1.phase=XM87274Unified::TAG_REQUESTED;
    p1.point=0x12u;p1.reserve=0x14u;p1.started=100u;
    p1.request.valid=true;p1.request.reserve=0x14u;
    p1.request.motion=XM87192Owner::QCF;
    p1.request.buttonMask=0x18u; // incoming Ken Shinryuken QCF+KK
    auto release=s.inputForFrame(ram,0u,100u,0u,0x18u,true);
    assert(release.active && release.attacks==0u);
    const auto p1edge=s.inputForFrame(ram,0u,101u,0u,0x18u,true);
    assert(p1edge.active && p1edge.attacks==0x03u);
    assert(s.inputForFrame(ram,0u,102u,0u,0x18u,true).attacks==0x03u);
    assert(s.inputForFrame(ram,0u,103u,0u,0x18u,true).attacks==0u);
    assert(p1.request.buttonMask==0x18u); // reserve KK NOT replaced by PP
    XM87274Unified::Slot& p2slot=s.slot[1];
    p2slot.phase=XM87274Unified::TAG_REQUESTED;
    p2slot.point=0x22u;p2slot.reserve=0x20u;p2slot.started=200u;
    p2slot.request.valid=true;p2slot.request.reserve=0x20u;
    p2slot.request.motion=XM87192Owner::QCF;
    p2slot.request.buttonMask=0x07u; // full 3P, must release then edge
    assert(s.inputForFrame(ram,1u,201u,0u,0x07u,true).attacks==0u);
    const auto p2edge=s.inputForFrame(ram,1u,202u,0u,0x07u,true);
    assert(p2edge.active && p2edge.attacks==0x03u);
    assert(s.inputForFrame(ram,1u,203u,0u,0x07u,true).attacks==0x03u);
    assert(s.inputForFrame(ram,1u,204u,0u,0x07u,true).attacks==0u);
    p2slot.phase=XM87274Unified::AWAIT_SWAP;
    assert(s.inputForFrame(ram,1u,205u,0u,0x07u,true).attacks==0u);
    // No synthetic edge on the opposite actor in the same input frame.
    assert(s.inputForFrame(ram,0u,104u,0u,0u,true).attacks==0u);
    envSet("XM8755_GAME_VERSION","3");
    assert(!s.enabled()); // Version 3 never uses this path
    std::puts("XMVSF 87.274: two-sided native DHC policy PASS");
    return 0;
}
