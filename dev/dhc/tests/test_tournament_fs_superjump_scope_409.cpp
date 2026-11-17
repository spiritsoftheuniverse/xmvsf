// TEST 87.409: Tournament's host FS writer is SUPERJUMP-NORMAL only.
// Ground, ordinary-jump 18xx, unknown physical button, and stale pending
// hits must not alter victim native action, profile, VX or VY.
// A valid airborne 1Axx HP->HK follow-up can still become severe.
// Pure CPS2 RAM fixture, no ROM or SDL access. Local fast preflight only.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_86_storm_button_guard.h"
using namespace XM8786Flight;
static std::uint8_t ram[65536]={};
static constexpr unsigned V=0x4000u,A=0x4400u;
static inline void sw(unsigned a,unsigned v){setWord(ram,a,v);}
static inline void sl(unsigned a,std::uint32_t v){setFixed(ram,a,v);}
static inline unsigned rw(unsigned a){return wordAt(ram,a);}
static inline std::uint32_t rl(unsigned a){return rawFixed(ram,a);}
static void setup(unsigned attack,bool grounded=false,bool missingAirFlag=false){
    std::memset(ram,0,sizeof(ram));
    setByte(ram,0x5000u,8u);
    sw(A,0x0101u);sw(V,0x0101u);
    sw(A+0x52u,0x0006u);sw(V+0x52u,0x0008u);
    sw(A+0x40u,0x0021u);sw(A+0x60u,attack);
    sw(A+0x10u,grounded?472u:140u);
    sw(A+0x254u,472u);
    sw(A+0xaau,grounded||missingAirFlag?0u:0x0100u);
    sw(V+0x10u,180u);sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,0x00040000u);sl(V+0x1cu,0x00010000u);
    sw(V+0x06u,0x000cu);sw(V+0x08u,0u);
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
}
static Actor primed(unsigned lastButton=4u){
    Actor a;
    a.observed=true;
    a.nativeLaunched=true;
    a.stormFlightUsedUntilLanding=true;
    a.postFlightNeedsNativeLanding=true;
    a.lastConnectedButton=lastButton;
    a.lastConnectedAttacker=6u;a.lastConnectedVictim=8u;
    a.lastAttackerId=6u;a.lastVictimId=8u;
    a.prevReaction=0x000cu;a.prevSubstate=2u;
    a.prevAction=SEVERE_ACTION;a.prevProfile=SEVERE_PROFILE;
    a.prevVictimY=170u;
    a.prevAttackNow=true;a.prevAttackerAction=0x180au;
    a.attackSerial=1u;a.lastConfirmedSerial=1u;
    a.connectedAirHits=1u;
    return a;
}
static void verifyNative(unsigned attack,unsigned button,bool ground=false,
                         bool missingAirFlag=false){
    setup(attack,ground,missingAirFlag);
    Actor a=primed();
    const Decision first=a.evaluate(ram,V,A,true,true,button);
    const bool eligibleHit=ground?groundedNormalAttack(attack):
        (!missingAirFlag && normalAirAttack(attack));
    assert(first.hitConfirmed==eligibleHit);
    assert(!a.fsConditionalPending);
    assert(!a.pendingSevere);
    assert(!first.velocityWritten);
    assert(rw(V+0x60u)==SOFT_ACTION);
    assert(rw(V+0xe0u)==SOFT_PROFILE);
    assert(rl(V+0x14u)==0x00040000u);
    assert(rl(V+0x1cu)==0x00010000u);
    sw(V+0x08u,2u);
    const Decision settled=a.evaluate(ram,V,A,true,true,0u);
    assert(!settled.velocityWritten && !settled.stateWritten);
    assert(rw(V+0x60u)==SOFT_ACTION);
    assert(rw(V+0xe0u)==SOFT_PROFILE);
    assert(rl(V+0x14u)==0x00040000u);
    assert(rl(V+0x1cu)==0x00010000u);
}
int main(){
    // Classification is 1Axx only; 18xx is the ordinary jump family,
    // and 10xx/12xx/14xx are the grounded family.
    assert(superjumpNormalAttack(0x1a00u));
    assert(superjumpNormalAttack(0x1a04u));
    assert(superjumpNormalAttack(0x1a0au));
    assert(!superjumpNormalAttack(0x1804u));
    assert(!superjumpNormalAttack(0x1004u));
    assert(!superjumpNormalAttack(0x120au));
    assert(!superjumpHeavyAttack(0x1a02u));
    assert(!superjumpHeavyAttack(0x1804u));
    assert(superjumpHeavyAttack(0x1a04u));
    assert(superjumpHeavyAttack(0x1a0au));
    assert(confirmedSuperjumpHeavy(0x1a04u,4u,true,true));
    assert(!confirmedSuperjumpHeavy(0x1a04u,4u,true,false));
    assert(!confirmedSuperjumpHeavy(0x1804u,4u,true,true));
    assert(!confirmedHeavyNormal(0x1004u,4u,true,false,true));
    assert(!confirmedHeavyNormal(0x1804u,4u,true,true,false));

    // Both ground heavy strengths and ordinary jumping HP/HK are native,
    // EVEN if an earlier valid flying-screen is still latched.
    verifyNative(0x1004u,4u,true);
    verifyNative(0x120au,32u,true);
    verifyNative(0x1404u,4u,true);
    verifyNative(0x1000u,1u,true);
    verifyNative(0x1002u,2u,true);
    verifyNative(0x1800u,1u);
    verifyNative(0x1802u,2u);
    verifyNative(0x1804u,4u);
    verifyNative(0x180au,32u);
    // Even a superjump action with an above-floor Y must not qualify
    // after the real native +AA airborne flag has dropped.
    verifyNative(0x1a04u,4u,false,true);
    verifyNative(0x1a04u,32u,true);

    // A stale delayed FS proposal cannot release through a 10xx or
    // 18xx normal when the original input/move did NOT originate in SJ.
    for(unsigned action : {0x1004u,0x1804u}){
        setup(action,action==0x1004u);
        Actor stale=primed();
        stale.fsConditionalPending=true;
        stale.fsPendingAction=action;
        stale.fsPendingButton=4u;
        stale.fsPendingOriginGrounded=action==0x1004u;
        stale.fsPendingAttacker=6u;stale.fsPendingVictim=8u;
        stale.fsPendingSerial=1u;stale.fsPendingAge=0u;
        sw(V+0x08u,2u);
        const Decision noLeak=stale.evaluate(ram,V,A,true,true,0u);
        assert(!noLeak.velocityWritten && !stale.fsConditionalPending);
        assert(rw(V+0x60u)==SOFT_ACTION);
        assert(rl(V+0x1cu)==0x00010000u);
    }
    // The older fallback latch must also reject ordinary 18xx HP.
    setup(0x1804u);
    sw(V+0x08u,2u);
    Actor fallback=primed();
    fallback.pendingSevere=true;
    fallback.pendingAge=0u;fallback.pendingAction=0x1804u;
    fallback.pendingButton=4u;fallback.pendingBank=0x0021u;
    fallback.pendingAttackerId=6u;
    const Decision oldLatch=fallback.evaluate(ram,V,A,true,true,0u);
    assert(!oldLatch.velocityWritten && !fallback.pendingSevere);
    assert(rw(V+0x60u)==SOFT_ACTION);
    // Legal superjump HP->HK after a connected HP still becomes severe.
    setup(0x1a04u);
    Actor sj=primed(4u);
    const Decision onset=sj.evaluate(ram,V,A,true,true,32u);
    assert(onset.hitConfirmed && sj.fsConditionalPending);
    assert(sj.fsPendingSevere && !sj.fsPendingGround);
    sw(V+0x08u,2u);
    const Decision release=sj.evaluate(ram,V,A,true,true,0u);
    assert(release.launch && release.velocityWritten);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rw(V+0xe0u)==SEVERE_PROFILE);
    assert(rl(V+0x1cu)==TARGET_DOWN);

    // A new ordinary-jump move cancels an earlier SJ pending conversion.
    setup(0x1a04u);
    Actor cancelled=primed(4u);
    cancelled.evaluate(ram,V,A,true,true,32u);
    assert(cancelled.fsConditionalPending);
    sw(A+0x60u,0x1804u);sw(V+0x08u,2u);
    const Decision noCarry=cancelled.evaluate(ram,V,A,true,true,4u);
    assert(!noCarry.velocityWritten && !cancelled.fsConditionalPending);
    assert(rw(V+0x60u)==SOFT_ACTION);
    std::puts("TEST 87.409: ground and ordinary jump FS leaks blocked; SJ HK preserved PASS");
    return 0;
}
