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
    byte(r,0x5000u,8u);word(r,0x4052u,0x12u);word(r,0x4852u,0x14u);
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
    assert(has(s,"RESERVE 14 act=180C anim=1234 script=12345678"));
    assert(has(s,"KEN SHORYUREPPA = QCF+PP"));
    assert(has(s,"KEN SHINRYUKEN = QCF+KK"));
    // CPS2 direction bits: 4=down,5=down+right,1=right.
    s.frame(r.data(),12u,4u,0,0,0);
    s.frame(r.data(),13u,5u,0,0,0);
    s.frame(r.data(),14u,1u,0,3u,0); // LP+MP pressed
    assert(has(s,"MATCH KEN SHORYUREPPA (QCF+PP)"));
    assert(has(s,"P1 INPUT LP+MP"));
    assert(has(s,"[INPUT ONLY]"));
    assert(has(s,"P1 DIR DF"));
    assert(s.visibleLines(14u)>0u);
    // Repeated held buttons must not create further input-press events.
    const unsigned before=s.chatMessages;
    s.frame(r.data(),15u,1u,0,3u,0);
    assert(s.chatMessages==before);
    // Parser remains observational: it has not altered actor or meter RAM.
    assert(b(r.data(),0x4214u)==2u);
    assert(w(r.data(),0x4052u)==0x12u);
    assert(w(r.data(),0x4852u)==0x14u);
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
    // User-confirmed reserve super commands; native action IDs still unknown.
    const ListedSuper* ryuKick=known(0x12u,1u);
    assert(ryuKick && ryuKick->kicks && ryuKick->back);
    assert(!known(0x12u,2u));
    const ListedSuper* kenKick=known(0x14u,1u);
    assert(kenKick && kenKick->kicks && !kenKick->back);
    auto makeWindow=[](unsigned reserveId){
        std::vector<std::uint8_t> ram(65536,0);
        byte(ram,0x5000u,8u);
        word(ram,0x4052u,0x20u);word(ram,0x4852u,reserveId);
        byte(ram,0x4214u,3u);byte(ram,0x404Bu,0u);
        return ram;
    };
    auto testMove=[&](unsigned reserveId,unsigned d1,unsigned d2,
                      unsigned d3,unsigned buttons,const char* label,
                      bool shouldMatch,bool leftFacing=false){
        std::vector<std::uint8_t> ram=makeWindow(reserveId);
        State t;
        if(leftFacing)byte(ram,0x404Bu,1u);
        t.frame(ram.data(),100u,0u,0u,0u,0u);
        byte(ram,0x40A0u,0x2Au);
        t.frame(ram.data(),101u,0u,0u,0u,0u);
        t.frame(ram.data(),102u,d1,0u,0u,0u);
        t.frame(ram.data(),103u,d2,0u,0u,0u);
        t.frame(ram.data(),104u,d3,0u,buttons,0u);
        assert(has(t,label)==shouldMatch);
        assert(w(ram.data(),0x4852u)==reserveId);
    };
    // Facing right: QCB is absolute D, D+LEFT, LEFT, not D+RIGHT.
    testMove(0x12u,4u,6u,2u,24u,"MATCH RYU SHINKUU TATSUMAKI (QCB+KK)",true);
    testMove(0x12u,4u,5u,1u,24u,"MATCH RYU SHINKUU TATSUMAKI (QCB+KK)",false);
    testMove(0x12u,4u,5u,1u,3u,"MATCH RYU SHINKUU HADOKEN (QCF+PP)",true);
    testMove(0x14u,4u,5u,1u,3u,"MATCH KEN SHORYUREPPA (QCF+PP)",true);
    testMove(0x14u,4u,5u,1u,24u,"MATCH KEN SHINRYUKEN (QCF+KK)",true);
    // Facing left: QCB becomes absolute D, D+RIGHT, RIGHT.
    testMove(0x12u,4u,5u,1u,24u,"MATCH RYU SHINKUU TATSUMAKI (QCB+KK)",true,true);
    return 0;
}
