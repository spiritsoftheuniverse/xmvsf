// TEST 87.230: narrow post-FS severe cases; no leaked HP/HK after LP/LK/MP.
// Cases: (1) previous CONNECTED hit HP -> confirmed HK while FS active;
//        (2) physically grounded heavy normal connects during FS.
// ROM-free, real CPS-2 RAM byte/word layout; no synthesized hit counter.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_86_storm_button_guard.h"
using namespace XM8786Flight;
static std::uint8_t mem[65536]={};
static constexpr unsigned V=0x4000u,A=0x4400u;
static void sw(unsigned p,unsigned v){setWord(mem,p,v);}
static unsigned rw(unsigned p){return wordAt(mem,p);}
static void sl(unsigned p,std::uint32_t x){setFixed(mem,p,x);}
static std::uint32_t rl(unsigned p){return rawFixed(mem,p);}
static void setup(unsigned id=0u,unsigned bank=0x21u,
                  unsigned action=0x1a04u,bool ground=false){
    std::memset(mem,0,sizeof(mem));
    setByte(mem,0x5000u,8u);
    sw(A,0x0101u);sw(V,0x0101u);
    sw(A+0x52u,id);sw(A+0x40u,bank);sw(A+0x60u,action);
    sw(V+0x52u,0x0006u);
    sw(A+0x10u,ground?472u:140u);
    sw(A+0x254u,472u);sw(A+0xaau,ground?0u:0x0100u);
    sw(V+0x10u,180u);sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,0x00040000u);sl(V+0x1cu,0x00010000u);
    sw(V+0x06u,0x000cu);sw(V+0x08u,0u);
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
}
static Actor primed(unsigned previousButton=4u,unsigned id=0u){
    Actor a;
    a.observed=true;
    a.stormFlightUsedUntilLanding=true;
    a.postFlightNeedsNativeLanding=true;
    a.nativeLaunched=true;
    a.lastConnectedButton=previousButton;
    a.lastConnectedAttacker=id;a.lastConnectedVictim=6u;
    a.lastAttackerId=id;a.lastVictimId=6u;
    a.prevReaction=0x000cu;a.prevSubstate=2u;
    a.prevAction=SEVERE_ACTION;a.prevProfile=SEVERE_PROFILE;
    a.prevVictimY=170u;
    a.prevAttackNow=true;
    a.prevAttackerAction=0x180au;
    a.attackSerial=1u;a.lastConfirmedSerial=1u;
    a.connectedAirHits=1u;
    return a;
}
static Decision onset(Actor& a,unsigned buttons){
    const Decision d=a.evaluate(mem,V,A,true,true,buttons);
    assert(d.hitConfirmed);
    return d;
}
static Decision finish(Actor& a,unsigned buttons=0u){
    sw(V+0x08u,2u);
    return a.evaluate(mem,V,A,true,true,buttons);
}
static void expectSevere(const Decision& d,const char* label){
    assert(d.velocityWritten);
    assert(d.stateWritten);
    assert(std::strcmp(d.outcome,label)==0);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rw(V+0xe0u)==SEVERE_PROFILE);
    assert(rl(V+0x1cu)==TARGET_DOWN);
}
int main(){
    assert(groundedNormalAttack(0x1000u));
    assert(groundedNormalAttack(0x1002u));
    assert(groundedNormalAttack(0x1208u));
    assert(!groundedNormalAttack(0x1a00u));
    assert(groundedNormalHeavy(0x1004u));
    assert(groundedNormalHeavy(0x100au));
    assert(groundedNormalHeavy(0x1204u));
    assert(groundedNormalHeavy(0x120au));
    assert(!groundedNormalHeavy(0x1002u));
    assert(!groundedNormalHeavy(0x1208u));
    assert(!groundedNormalHeavy(0x1804u));
    assert(!groundedNormalHeavy(0x1a04u)); // super-jump NOT ground
    assert(!groundedNormalHeavy(0x2c1eu)); // Wolverine drill protected

    // In the supplied 87.229 capture: HP 180A connected in flight,
    // HK 1A04 connected next. The old "flight already used" guard
    // rejected a second Storm boost; now convert the actual HK to severe.
    setup();
    Actor hpHk=primed(4u);
    Decision hkHit=onset(hpHk,32u);
    assert(hpHk.fsConditionalPending);
    assert(!hpHk.fsPendingGround);
    assert(std::strcmp(hkHit.outcome,"FS_HP_THEN_HK_CONFIRMED_PENDING")==0);
    expectSevere(finish(hpHk),"FS_HP_THEN_HK_CYCLOPS_SEVERE");
    assert(!hpHk.fsConditionalPending);

    // A preceding CONNECTED LP instead of HP does not qualify.
    setup();
    Actor lpHk=primed(1u);
    Decision ignored=onset(lpHk,32u);
    assert(ignored.hitConfirmed);
    assert(!lpHk.fsConditionalPending);
    Decision lpHkRelease=finish(lpHk);
    assert(!lpHkRelease.velocityWritten);
    assert(rw(V+0x60u)==SOFT_ACTION);

    // HP -> LP -> HK is not HP -> HK. Every connected light/medium hit
    // replaces the stored previous hit rather than retaining an old HP.
    setup(0u,0x21u,0x1800u);
    Actor hpLpHk=primed(4u);
    onset(hpLpHk,1u);
    assert(hpLpHk.lastConnectedButton==1u);
    assert(!hpLpHk.fsConditionalPending);
    finish(hpLpHk,0u);
    sw(A+0x60u,0x1a04u);
    sw(V+0x08u,0u);
    const Decision noSevere=onset(hpLpHk,32u);
    assert(noSevere.hitConfirmed);
    assert(!hpLpHk.fsConditionalPending);

    // A pending HK is cancelled when LK begins before step 2, never
    // delayed 9 frames and applied to the next staggered attack.
    setup();
    Actor switchMove=primed(4u);
    onset(switchMove,32u);
    sw(A+0x60u,0x1a00u);
    const Decision canceled=finish(switchMove,8u);
    assert(!canceled.velocityWritten);
    assert(!switchMove.fsConditionalPending);
    assert(rw(V+0x60u)==SOFT_ACTION);
    assert(rl(V+0x1cu)==0x00010000u);

    // Grounded normal HP connects during ongoing FS, even when the
    // preceding hit is NOT HP. This has its own native hit criterion.
    setup(0u,0x21u,0x1004u,true);
    Actor groundHP=primed(1u);
    const Decision groundHit=onset(groundHP,4u);
    assert(groundHP.fsConditionalPending && groundHP.fsPendingGround);
    assert(std::strcmp(groundHit.outcome,
        "FS_GROUNDED_HEAVY_CONFIRMED_PENDING")==0);
    expectSevere(finish(groundHP),"FS_GROUNDED_HEAVY_CYCLOPS_SEVERE");

    setup(0u,0x21u,0x120au,true);
    Actor groundHK=primed(8u);
    onset(groundHK,32u);
    assert(groundHK.fsPendingGround);
    expectSevere(finish(groundHK),
        "FS_GROUNDED_HEAVY_CYCLOPS_SEVERE");

    // Even if the attacker is at floor height, the SUPER-JUMP family
    // cannot masquerade as a grounded heavy.
    setup(0u,0x21u,0x1a04u,true);
    Actor superJump=primed(1u);
    const Decision rejected=superJump.evaluate(mem,V,A,true,true,32u);
    assert(!rejected.hitConfirmed && !superJump.fsConditionalPending);
    assert(rl(V+0x1cu)==0x00010000u);

    // Grounded heavy without an active FS never becomes artificial severe.
    setup(0u,0x21u,0x1004u,true);
    Actor noFS=primed(1u);
    noFS.stormFlightUsedUntilLanding=false;
    noFS.postFlightNeedsNativeLanding=false;
    const Decision plain=onset(noFS,4u);
    assert(!plain.velocityWritten && !noFS.fsConditionalPending);

    // A grounded MP is a real CONNECTED HIT and replaces the previous
    // HP in the hit ledger. It must never itself induce severe.
    setup(0u,0x21u,0x1002u,true);
    Actor mp=primed(4u);
    const Decision mpHit=onset(mp,2u);
    assert(mpHit.hitConfirmed && !mpHit.velocityWritten);
    assert(mp.lastConnectedButton==2u);
    assert(!mp.fsConditionalPending);

    // Wolverine Drill Claw cannot masquerade as a preceding normal HP.
    setup(0u,0x21u,0x2c1eu);
    Actor drill=primed(4u);
    const Decision special=onset(drill,4u);
    assert(special.excluded);
    assert(!drill.fsConditionalPending);
    assert(drill.lastConnectedButton==0u);

    // Air LK after HK stays native.
    setup(0u,0x21u,0x1a00u);
    Actor lk=primed(4u);
    const Decision lpAfter=onset(lk,8u);
    assert(!lpAfter.velocityWritten && !lk.fsConditionalPending);

    // Native landing is the ONLY ordinary FS latch rearm. Once landed,
    // even another physical HK cannot inherit a former HP.
    sw(V+0x10u,472u);sw(V+0xaau,0u);
    lk.evaluate(mem,V,A,true,true,0u);
    assert(!lk.postFlightNeedsNativeLanding);
    assert(lk.lastConnectedButton==0u);

    // Charlie's already accepted independent native HP / severe HK
    // functions are untouched by this added post-FS rule.
    assert(charlieNativeAirHP(0x20u,0x28u,0x1804u));
    assert(charlieAirHK(0x20u,0x28u,0x1a04u,32u));
    std::puts("87.230 HP->HK in active FS, grounded non-SJ heavy, no LP/LK/MP leak: PASS");
    return 0;
}
