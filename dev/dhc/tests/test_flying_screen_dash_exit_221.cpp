// TEST 87.221: Storm flying-screen dash runaway; no change to normal dash.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
static std::uint8_t ram[65536]={};
static void actor(unsigned p,unsigned id,unsigned mode,unsigned timer){
    setWord(ram,p,0x0101u);
    setWord(ram,p+0x52u,id);
    setWord(ram,p+0x06u,0u);
    setWord(ram,p+0xa0u,mode);
    setWord(ram,p+0x3eu,timer);
}
int main(){
    Controller c;
    ram[0x5001u]=8u;
    const unsigned p=0x4000u;
    actor(p,4u,0x0000u,5u); // Storm, real neutral controller
    setWord(ram,p+0x60u,0x0000u);
    setWord(ram,p+0x42u,0x1234u);
    setLong(ram,p+0x34u,0x000ac000u);
    setLong(ram,p+0x38u,0x000ac010u);
    c.frame(1u,ram,true);
    actor(p,4u,0x1400u,4u);
    setLong(ram,p+0x34u,0x000ac268u);
    setLong(ram,p+0x38u,0x000ac278u);
    setLong(ram,p+0x14u,0xfff62000u);
    for(unsigned f=2u;f<=38u;++f){
        // Native animation cycles through the same dash mode; at f=22
        // the recorded 87.220 ROM frame pointer wraps to its entry.
        setLong(ram,p+0x34u,f%20u==2u?0x000ac268u:0x000ac2dcu);
        setWord(ram,p+0x3eu,4u);
        c.frame(f,ram,true);
        if(f<38u)assert(word(ram,p+0xa0u)==0x1400u);
    }
    assert(word(ram,p+0xa0u)==0u);
    assert(word(ram,p+0x60u)==0u);
    assert(word(ram,p+0x42u)==0x1234u);
    assert(longword(ram,p+0x34u)==0x000ac000u);
    assert(longword(ram,p+0x38u)==0x000ac010u);
    assert(longword(ram,p+0x14u)==0u);
    // No neutral snapshot => do not fabricate a fighter-specific ROM script.
    Controller noSnapshot;
    actor(p,0x22u,0x1400u,4u);
    for(unsigned f=50u;f<110u;++f)noSnapshot.frame(f,ram,true);
    assert(word(ram,p+0xa0u)==0x1400u);
    // Grounded native dash can still start immediately after exit.
    actor(p,4u,0x0000u,5u);setLong(ram,p+0x14u,0u);
    c.frame(120u,ram,true);
    actor(p,4u,0x1402u,5u);
    c.frame(121u,ram,true);
    assert(word(ram,p+0x3eu)==1u);
    std::puts("XMVSF 87.221 flying-screen dash loop exit: PASS");
}
