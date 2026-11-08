// TEST 87.226: 87.225.zip recurring FS dash and corrupt world-X recovery.
// Restores a genuine same-fighter neutral ROM script; no invented pointers.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_220_zero_dash_startup.h"
using namespace XM87220Dash;
static std::uint8_t mem[65536]={};
static const unsigned P=0x4000u;
static void sw(unsigned a,unsigned x){setWord(mem,a,x);}
static void sl(unsigned a,std::uint32_t x){setLong(mem,a,x);}
static unsigned rw(unsigned a){return word(mem,a);}
static std::uint32_t rl(unsigned a){return longword(mem,a);}
static void setupNeutral(){
    sw(P,0x0101u);sw(P+0x52u,0x0020u); // native Charlie
    sw(P+0x06u,0u);sw(P+0x08u,0u);
    sw(P+0xa0u,0u);sw(P+0xaau,0u);
    sw(P+0x3eu,4u);sw(P+0x42u,0x4321u);sw(P+0x60u,0u);
    sl(P+0x0cu,0x02ba0000u);sl(P+0x14u,0u);
    sl(P+0x34u,0x000ac000u);sl(P+0x38u,0x000ac010u);
}
static void setupDash(unsigned mode,std::uint32_t x){
    sw(P+0xa0u,mode);sw(P+0x3eu,6u);
    sl(P+0x0cu,x);sl(P+0x14u,0xfff70000u);
    sl(P+0x34u,0x000ecf0cu);sl(P+0x38u,0x000ecf1cu);
}
int main(){
    mem[0x5001u]=8u;
    Controller c;
    setupNeutral();c.frame(1u,mem,true);
    assert(c.actor[0].safeValid);
    assert(c.actor[0].neutral.valid);

    // 87.225 exact symptom: from real 0x02BA X to 0x7041D600
    // (28,737 world pixels), then 37-frame recurring dash 1400.
    setupDash(0x1400u,0x7041d600u);
    c.frame(2u,mem,true);
    assert(rw(P+0xa0u)==0u);
    assert(rl(P+0x0cu)==0x02ba0000u);
    assert(rl(P+0x14u)==0u);
    assert(rl(P+0x34u)==0x000ac000u);
    assert(c.actor[0].recoveryActive);

    // Original CPU keeps returning to dash despite the last correction.
    // Never allow the 12 successive 37f loops in the uploaded log.
    for(unsigned f=3u;f<=50u;++f){
        setupDash(0x1400u,0x70415600u);
        c.frame(f,mem,true);
        assert(rw(P+0xa0u)==0u);
        assert(rl(P+0x0cu)==0x02ba0000u);
        assert(!c.actor[0].ready);
    }
    // Actual consecutive ROM non-dash frames re-enable normal movement.
    for(unsigned f=51u;f<=53u;++f){
        setupNeutral();
        c.frame(f,mem,true);
    }
    assert(!c.actor[0].recoveryActive);
    setupDash(0x1402u,0x02ba0000u);sl(P+0x14u,0u);
    c.frame(54u,mem,true);
    assert(rw(P+0xa0u)==0x1402u);
    assert(rw(P+0x3eu)==1u); // native instantaneous dash retained

    // Separate case: position looks plausible but original 36f watchdog
    // is immediately undone by the next native frame. Quarantine on
    // repeated reentry; not on a new dash dozens of frames later.
    Controller repeat;
    setupNeutral();repeat.frame(100u,mem,true);
    setupDash(0x1400u,0x02b00000u);
    for(unsigned f=101u;f<=137u;++f){
        sw(P+0x3eu,5u);
        repeat.frame(f,mem,true);
        if(f<137u)assert(rw(P+0xa0u)==0x1400u);
    }
    assert(rw(P+0xa0u)==0u); // first watchdog still fires at 36f
    setupDash(0x1400u,0x02b00000u);
    repeat.frame(138u,mem,true);
    assert(repeat.actor[0].recoveryActive);
    assert(rw(P+0xa0u)==0u); // immediate recurrence suppressed

    std::puts("87.226 FS corrupted world-X and repeat dash re-entry: PASS");
    return 0;
}
