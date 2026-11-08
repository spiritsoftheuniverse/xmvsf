// TEST 87.224: Charlie-only 1.5x Storm-like flying-screen vector.
// Native first-hit gating and the one-airborne-boost latch are not replaced.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_86_storm_button_guard.h"
using namespace XM8786Flight;
static std::uint8_t ram[65536]={};
static const unsigned V=0x4000u,A=0x4400u;
static void sw(unsigned a,unsigned x){setWord(ram,a,x);}
static unsigned rw(unsigned a){return wordAt(ram,a);}
static void sl(unsigned a,std::uint32_t x){setFixed(ram,a,x);}
static std::uint32_t rl(unsigned a){return rawFixed(ram,a);}
static Decision run(unsigned id,unsigned bank,unsigned move,
                    std::uint32_t vx=0u,std::uint32_t vy=0u,
                    unsigned action=0x1c16u,unsigned index=0x0220u){
    std::memset(ram,0,sizeof(ram));
    ram[(0x5000u)^1u]=8u;
    sw(A+0x52u,id);sw(A+0x40u,bank);sw(A+0x60u,move);
    sw(V+0x52u,0x0008u);
    sw(A+0x10u,160u);sw(V+0x10u,180u);
    sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,vx);sl(V+0x1cu,vy);
    sw(V+0x06u,0x000cu);sw(V+0x08u,2u);
    sw(V+0x60u,action);sw(V+0xe0u,index);
    Actor actor;
    // A prior genuine native aerial hit has armed the existing heavy
    // follow-up gate; this test isolates only the release-time vector.
    actor.nativeLaunched=true;
    actor.pendingSevere=true;
    actor.pendingAge=0u;
    actor.pendingAction=move;
    actor.pendingBank=bank;
    actor.pendingAttackerId=id;
    const Decision d=actor.evaluate(ram,V,A,true,true,0u);
    return d;
}
int main(){
    assert(charlieAirHeavyFS(0x20u,0x28u,0x1804u));
    assert(charlieAirHeavyFS(0x20u,0x28u,0x1a04u));
    assert(!charlieAirHeavyFS(0x20u,0x28u,0x1a0au));
    assert(!charlieAirHeavyFS(0x02u,0x28u,0x1804u));
    assert(!charlieAirHeavyFS(0x20u,0x21u,0x1804u));
    assert(charlieFS150(0x0010aa00u,0x00300000u)==0x0018ff00u);
    assert(charlieFS150(0x0000b000u,0x000c0000u)==0x00010800u);
    assert(charlieFS150(0x00300000u,0x00300000u)==0x00300000u);

    Decision hp=run(0x20u,0x28u,0x1804u);
    assert(hp.launch && hp.velocityWritten && hp.stateWritten);
    assert(rw(V+0x60u)==0x1c18u && rw(V+0xe0u)==0x0230u);
    assert(rl(V+0x14u)==0x0018ff00u);
    assert(rl(V+0x1cu)==0x00010800u);
    assert(std::strcmp(hp.outcome,"CHARLIE_AIR_HEAVY_FS_150PCT_NATIVE_INDEX")==0);

    Decision hk=run(0x20u,0x28u,0x1a04u,0xfffc0000u);
    assert(hk.launch && hk.velocityWritten);
    assert(rl(V+0x14u)==0xffe70100u); // negative -0x0018FF00
    assert(rl(V+0x1cu)==0x00010800u);

    // Non-Charlie fighters keep precisely the Storm reference vector.
    Decision cyclops=run(0x02u,0x21u,0x1804u);
    assert(cyclops.launch);
    assert(rl(V+0x14u)==0x0010aa00u);
    assert(rl(V+0x1cu)==0x0000b000u);

    // Other Charlie actions are not accidentally scaled.
    Decision other=run(0x20u,0x28u,0x1a0au);
    assert(other.launch);
    assert(rl(V+0x14u)==0x0010aa00u);

    // A native steep-severe hit remains native; no guessed 68000
    // special-knockdown flag is erased by the Charlie multiplier.
    Decision special=run(0x20u,0x28u,0x1804u,
                         0x00060000u,0x00120000u,0x1c18u,0x0230u);
    assert(!special.launch && special.excluded);
    assert(rl(V+0x14u)==0x00060000u);
    assert(rl(V+0x1cu)==0x00120000u);
    std::puts("87.224 Charlie HP/HK-only 1.5x FS, native severe protected: PASS");
    return 0;
}
