// C++11: validate first-super candidate, reserve data and real-native input
// log. No ROM, game windows, sprites or mutable gameplay state required.
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_140_super_chat.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdint>
using namespace XM87140Chat;
static void byte(std::vector<std::uint8_t>& r,unsigned a,unsigned val){r[a^1u]=std::uint8_t(val);}
static void word(std::vector<std::uint8_t>& r,unsigned a,unsigned val){byte(r,a,val>>8);byte(r,a+1u,val);}
static bool has(const State& s,const char* fragment){
    for(unsigned k=0;k<s.count;++k)if(std::strstr(s.lines[k].text,fragment))return true;
    return false;
}
int main(){
    std::vector<std::uint8_t> r(65536,0);
    byte(r,0x5000u,8u);word(r,0x4052u,0x20u);word(r,0x4852u,0x22u);
    word(r,0x4860u,0x180Cu);word(r,0x4842u,0x1234u);
    word(r,0x4834u,0x1234u);word(r,0x4836u,0x5678u);
    byte(r,0x4214u,3u);byte(r,0x404Bu,0u); // point facing right
    State s;
    s.frame(r.data(),10u,0,0,0,0);
    assert(!s.candidate[0] && s.count==0u);
    // Native action transition and stock debit open a *candidate*, not a
    // fictional ROM-verified super ID.
    byte(r,0x40A0u,0x2Au);byte(r,0x4214u,2u);
    s.frame(r.data(),11u,0,0,0,0);
    assert(s.candidate[0]&&has(s,"FIRST SUPER?"));
    assert(has(s,"RESERVE 22 act=180C anim=1234 script=12345678"));
    assert(has(s,"SUPER LIST: EMPTY"));
    // CPS2 direction bits: 4=down,5=down+right,1=right.
    s.frame(r.data(),12u,4u,0,0,0);
    s.frame(r.data(),13u,5u,0,0,0);
    s.frame(r.data(),14u,1u,0,3u,0); // LP+MP pressed
    assert(has(s,"MATCH QCF+PP"));
    assert(has(s,"P1 INPUT LP+MP"));
    assert(has(s,"NOT VERIFIED"));
    assert(has(s,"P1 DIR DF"));
    assert(s.visibleLines(14u)>0u);
    // Repeated held buttons must not create further input-press events.
    const unsigned before=s.chatMessages;
    s.frame(r.data(),15u,1u,0,3u,0);
    assert(s.chatMessages==before);
    // Parser remains observational: it has not altered actor or meter RAM.
    assert(b(r.data(),0x4214u)==2u);
    assert(w(r.data(),0x4052u)==0x20u);
    assert(w(r.data(),0x4852u)==0x22u);
    s.toggle();
    assert(s.open && s.visibleLines(14u)>0u);
    s.toggle();assert(!s.open);
    // Delayed expiration: after 180 frames all earlier lines are gone.
    // Keep window from producing new messages by leaving its action idle.
    byte(r,0x40A0u,0u);
    s.frame(r.data(),50u,0u,0u,0u,0u);
    s.frame(r.data(),260u,0u,0u,0u,0u);
    assert(!s.candidate[0]);
    assert(s.visibleLines(260u)==0u);
    s.toggle();assert(s.visibleLines(260u)>0u);
    // P2 is independent and must not inherit P1's super window.
    State other;
    byte(r,0x40A0u,0u);byte(r,0x44A0u,0u);
    byte(r,0x4614u,3u);word(r,0x4452u,0x24u);word(r,0x4C52u,0x26u);
    other.frame(r.data(),300u,0u,0u,0u,0u);
    byte(r,0x44A0u,0x2Au);byte(r,0x4614u,2u);
    other.frame(r.data(),301u,0u,0u,0u,0u);
    assert(!other.candidate[0] && other.candidate[1]);
    return 0;
}
