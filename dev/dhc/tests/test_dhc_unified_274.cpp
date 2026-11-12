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
    envSet("XM8755_GAME_VERSION","3");
    assert(!s.enabled()); // Version 3 never uses this path
    std::puts("XMVSF 87.274: two-sided native DHC policy PASS");
    return 0;
}
