// TEST 87.226: Charlie native HP vs confirmed HK Cyclops severe flight.
// ROM-free; uses the real post-CPU victim-state write functions.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_86_storm_button_guard.h"
using namespace XM8786Flight;
static std::uint8_t mem[65536]={};
static const unsigned V=0x4000u,A=0x4400u;
static void sw(unsigned p,unsigned v){setWord(mem,p,v);}
static unsigned rw(unsigned p){return wordAt(mem,p);}
static void sl(unsigned p,std::uint32_t x){setFixed(mem,p,x);}
static std::uint32_t rl(unsigned p){return rawFixed(mem,p);}
static void setup(unsigned fighter,unsigned bank,unsigned action,
                  unsigned substate=2u){
    std::memset(mem,0,sizeof(mem));
    mem[0x5001u]=8u; // byte-swapped RAM 0x5000=8: match active
    sw(A,0x0101u);sw(V,0x0101u);
    sw(A+0x52u,fighter);sw(A+0x40u,bank);sw(A+0x60u,action);
    sw(V+0x52u,0x0012u);
    sw(A+0x10u,140u);sw(V+0x10u,180u);
    sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);
    sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,0x00030000u);
    sl(V+0x1cu,0x00010000u);
    sw(V+0x06u,0x000cu);
    sw(V+0x08u,substate);
    sw(V+0x60u,0x1c16u);
    sw(V+0xe0u,0x0220u);
}
static Decision release(unsigned fighter,unsigned bank,unsigned action,
                        unsigned taggedButton){
    setup(fighter,bank,action);
    Actor actor;
    actor.nativeLaunched=true; // a real previous 1C14/0210 up-launch
    actor.pendingSevere=true;
    actor.pendingAge=0u;actor.pendingAction=action;
    actor.pendingButton=taggedButton;
    actor.pendingBank=bank;actor.pendingAttackerId=fighter;
    return actor.evaluate(mem,V,A,true,true,0u);
}
int main(){
    assert(charlieNativeAirHP(0x20u,0x28u,0x1804u));
    assert(!charlieNativeAirHP(0x04u,0x28u,0x1804u));
    assert(charlieAirHK(0x20u,0x28u,0x1a04u,32u));
    assert(!charlieAirHK(0x20u,0x28u,0x1a04u,4u));
    assert(!charlieAirHK(0x02u,0x28u,0x1a04u,32u));
    assert(!charlieAirHK(0x20u,0x21u,0x1a04u,32u));

    // Charlie HP: verify native action/index/motion survive the original
    // confirmed aerial hit, rather than merely a direct release bypass.
    setup(0x20u,0x28u,0x1804u,0u);
    Actor hp;
    hp.observed=true;hp.nativeLaunched=true;
    hp.prevReaction=0x000cu;hp.prevSubstate=2u;hp.prevVictimY=160u;
    hp.prevAction=0x1c14u;hp.prevProfile=0x0210u;
    hp.connectedAirHits=1u;hp.attackSerial=1u;
    hp.prevAttackNow=true;hp.prevAttackerAction=0x1800u;
    hp.lastAttackerId=0x20u;hp.lastVictimId=0x12u;
    Decision hpHit=hp.evaluate(mem,V,A,true,true,4u);
    assert(hpHit.hitConfirmed && hpHit.excluded);
    assert(std::strcmp(hpHit.outcome,"CHARLIE_AIR_HP_NATIVE_UNCHANGED")==0);
    assert(!hpHit.velocityWritten && !hp.pendingSevere);
    assert(rw(V+0x60u)==0x1c16u && rw(V+0xe0u)==0x0220u);
    assert(rl(V+0x14u)==0x00030000u && rl(V+0x1cu)==0x00010000u);
    assert(hp.connectedAirHits==2u);

    // Charlie HK uses the explicit Cyclops severe variant, NOT Storm x1.5.
    Decision hk=release(0x20u,0x28u,0x1a04u,32u);
    assert(hk.launch && hk.velocityWritten && hk.stateWritten);
    assert(std::strcmp(hk.outcome,
        "CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN")==0);
    assert(rw(V+0x60u)==0x1c18u && rw(V+0xe0u)==0x0230u);
    assert(rl(V+0x14u)==TARGET_RIGHT);
    assert(rl(V+0x1cu)==TARGET_DOWN);

    // Same move without verified HK physical button evidence is not
    // allowed to receive Charlie's special severe slam.
    Decision ambiguous=release(0x20u,0x28u,0x1a04u,0u);
    assert(ambiguous.launch && ambiguous.velocityWritten);
    assert(std::strcmp(ambiguous.outcome,
        "CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN")!=0);
    assert(rl(V+0x14u)==XM87223Flight::NativeObservedVX);
    // Storm keeps an already stronger *native* downward Y in 87.223.
    // setup() gives the victim 0x00010000, above Storm's 0x0000B000.
    assert(rl(V+0x1cu)==0x00010000u);

    // The rest of the roster retains the 87.223 Storm X/Y variant.
    Decision storm=release(0x04u,0x22u,0x1a04u,4u);
    assert(storm.launch);
    assert(rl(V+0x14u)==XM87223Flight::NativeObservedVX);
    // Storm keeps an already stronger *native* downward Y in 87.223.
    // setup() gives the victim 0x00010000, above Storm's 0x0000B000.
    assert(rl(V+0x1cu)==0x00010000u);

    // Conversely, weaker initial native Y is raised to Storm's
    // reference. Test both branches of the 87.223 Y-selection rule.
    setup(0x04u,0x22u,0x1a04u);
    sl(V+0x1cu,0x00002000u);
    Actor weak;
    weak.nativeLaunched=true;
    weak.pendingSevere=true;
    weak.pendingAction=0x1a04u;
    weak.pendingBank=0x22u;
    weak.pendingAttackerId=0x04u;
    Decision baseline=weak.evaluate(mem,V,A,true,true,0u);
    assert(baseline.launch);
    assert(rl(V+0x1cu)==XM87223Flight::NativeObservedVY);

    // Existing severe native slam is authoritative; do not override
    // already steep knockdown trajectories.
    setup(0x20u,0x28u,0x1a04u);
    sw(V+0x60u,0x1c18u);sw(V+0xe0u,0x0230u);
    sl(V+0x14u,0x00060000u);sl(V+0x1cu,0x00120000u);
    Actor native;
    native.nativeLaunched=true;native.pendingSevere=true;
    native.pendingAction=0x1a04u;native.pendingButton=32u;
    native.pendingBank=0x28u;native.pendingAttackerId=0x20u;
    Decision existing=native.evaluate(mem,V,A,true,true,0u);
    assert(!existing.launch && existing.excluded);
    assert(rl(V+0x1cu)==0x00120000u);

    std::puts("87.226 Charlie HP ROM-default, HK Cyclops severe, Storm separate: PASS");
    return 0;
}
