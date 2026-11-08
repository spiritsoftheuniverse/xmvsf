// TEST 87.222: switch Tournament -> Classic -> Tournament without
// crossing dash-neutral snapshots, and retain 87.220 instant startup.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
static std::uint8_t ram[65536]={};
static const unsigned P=0x4000u;
static void make(unsigned mode,unsigned timer,unsigned vx){
    setWord(ram,P,0x0101u);
    setWord(ram,P+0x52u,0x0004u);
    setWord(ram,P+0x06u,0u);
    setWord(ram,P+0xa0u,mode);
    setWord(ram,P+0xaau,0u);
    setWord(ram,P+0x3eu,timer);
    setWord(ram,P+0x60u,mode?0x1400u:0u);
    setLong(ram,P+0x14u,vx);
    setLong(ram,P+0x34u,mode?0x000ac268u:0x000ab4d0u);
    setLong(ram,P+0x38u,mode?0x000ac278u:0x000ab4e0u);
}
int main(){
    ram[0x5001u]=8u;
    Controller c;
    make(0u,5u,0u);
    c.frame(100u,ram,true,true);
    assert(c.actor[0].neutral.valid);
    make(0x1402u,5u,0u);
    c.frame(101u,ram,true,true);
    assert(word(ram,P+0x3eu)==1u);  // tournament immediate dash
    // Selecting Classic while a dash observer is armed must wipe
    // its saved native ROM controller, even with a duplicate frame.
    c.frame(101u,ram,true,false);
    assert(!c.actor[0].neutral.valid);
    assert(c.actor[0].dashAge==1u);
    // Native Classic neutral is also valid for future observation.
    make(0u,5u,0u);
    c.frame(102u,ram,true,false);
    assert(c.actor[0].neutral.valid);
    make(0x1402u,5u,0u);
    c.frame(103u,ram,true,false);
    assert(word(ram,P+0x3eu)==1u); // Classic instant dash preserved
    make(0x1402u,4u,0x00090000u);
    for(unsigned f=104u;f<139u;++f){
        c.frame(f,ram,true,false);
        assert(word(ram,P+0xa0u)==0x1402u);
    }
    c.frame(139u,ram,true,false);
    assert(word(ram,P+0xa0u)==0u); // Classic also exits a genuine runaway dash
    assert(longword(ram,P+0x34u)==0x000ab4d0u);
    // 87.226+ regression: a dash one frame after a confirmed 36f
    // watchdog exit is NOT an independent voluntary dash. The 68000
    // repeatedly regenerated that state in the captured FS loop.
    // Preserve the original 5f timer rather than forcing it to 1.
    make(0x1402u,5u,0u);
    c.frame(140u,ram,true,false);
    assert(word(ram,P+0x3eu)==5u);
    assert(word(ram,P+0xa0u)==0u); // native repeat was suppressed
    assert(c.actor[0].recoveryActive);
    // Three distinct committed neutral frames rearm voluntary dash.
    // A fresh player dash then retains the 87.220 zero-delay behavior.
    for(unsigned f=141u;f<=143u;++f){
        make(0u,5u,0u);
        c.frame(f,ram,true,false);
    }
    assert(!c.actor[0].recoveryActive);
    make(0x1402u,5u,0u);
    c.frame(144u,ram,true,false);
    assert(word(ram,P+0x3eu)==1u);
    // Reenter Tournament on the SAME native frame. Classic's captured
    // neutral cannot be reused; Tournament must observe its own.
    c.frame(144u,ram,true,true);
    assert(!c.actor[0].neutral.valid);
    for(unsigned f=145u;f<183u;++f)c.frame(f,ram,true,true);
    // Switching rules deliberately invalidated the prior Classic neutral.
    // Without a NEW Tournament neutral, the watchdog must fail closed:
    // it cannot restore the old Classic script just to end this dash.
    assert(!c.actor[0].neutral.valid);
    assert(word(ram,P+0xa0u)==0x1402u);
    // After a native neutral in Tournament, the original 87.221 watchdog
    // still terminates a runaway ground dash without touching world X.
    make(0u,4u,0u);
    c.frame(200u,ram,true,true);
    assert(c.actor[0].neutral.valid);
    make(0x1402u,4u,0x00090000u);
    for(unsigned f=201u;f<=238u;++f)c.frame(f,ram,true,true);
    assert(word(ram,P+0xa0u)==0u);
    assert(longword(ram,P+0x34u)==0x000ab4d0u);
    // Native game reset/rollback also invalidates saved controller state.
    c.frame(20u,ram,true,true);
    assert(c.actor[0].neutral.valid); // new actual neutral snapshot
    make(0x1402u,4u,0x00090000u);
    c.frame(21u,ram,true,true);
    assert(c.actor[0].dashAge==1u);
    std::puts("XMVSF 87.222 Classic reentry and dash safety: PASS");
    return 0;
}
