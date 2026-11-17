// 87.409: narrow the historical Storm FS baseline to confirmed 1Axx
// superjump HP/HK only. Ordinary 18xx air and grounded 10xx/12xx/14xx
// normals must preserve native reaction even with an active-FS latch.
// The pending conversion still belongs to its originating native hit.
// ROM-free CPS-2 actor RAM fixture; source compilation remains Windows-local.
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
static void sl(unsigned p,std::uint32_t v){setFixed(mem,p,v);}
static std::uint32_t rl(unsigned p){return rawFixed(mem,p);}
static void setup(unsigned action=0x1a04u,unsigned fighter=0u,
                  unsigned bank=0x21u,bool ground=false){
    std::memset(mem,0,sizeof(mem));
    setByte(mem,0x5000u,8u);
    sw(A,0x0101u);sw(V,0x0101u);
    sw(A+0x52u,fighter);sw(A+0x40u,bank);sw(A+0x60u,action);
    sw(V+0x52u,0x0006u);
    sw(A+0x10u,ground?472u:140u);
    sw(A+0x254u,472u);sw(A+0xaau,ground?0u:0x0100u);
    sw(V+0x10u,180u);sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,0x00040000u);sl(V+0x1cu,0x00010000u);
    sw(V+0x06u,0x000cu);sw(V+0x08u,0u);
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
}
static Actor activeFS(unsigned previous=0u,unsigned fighter=0u){
    Actor a;
    a.observed=true;a.nativeLaunched=true;
    a.stormFlightUsedUntilLanding=true;
    a.postFlightNeedsNativeLanding=true;
    a.lastConnectedButton=previous;
    a.lastConnectedAttacker=fighter;a.lastConnectedVictim=6u;
    a.lastAttackerId=fighter;a.lastVictimId=6u;
    a.prevReaction=0x000cu;a.prevSubstate=2u;
    a.prevAction=SEVERE_ACTION;a.prevProfile=SEVERE_PROFILE;
    a.prevVictimY=170u;
    a.prevAttackNow=true;a.prevAttackerAction=0x180au;
    a.attackSerial=1u;a.lastConfirmedSerial=1u;
    a.connectedAirHits=1u;
    return a;
}
static Decision onset(Actor& a,unsigned button){
    const Decision d=a.evaluate(mem,V,A,true,true,button);
    assert(d.hitConfirmed);
    return d;
}
static Decision finish(Actor& a){
    sw(V+0x08u,2u);
    return a.evaluate(mem,V,A,true,true,0u);
}
static void storm(const Decision& d){
    assert(d.velocityWritten && d.launch);
    assert(std::strcmp(d.outcome,"FS_STORM_KNOCKDOWN")==0);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rw(V+0xe0u)==SEVERE_PROFILE);
    assert(rl(V+0x14u)==XM87223Flight::flightVX());
    // Preserve a stronger but non-severe native Y exactly as 87.223 did.
    assert(rl(V+0x1cu)==0x00010000u);
}
static void severe(const Decision& d,const char* label){
    assert(d.velocityWritten && d.launch);
    assert(std::strcmp(d.outcome,label)==0);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rw(V+0xe0u)==SEVERE_PROFILE);
    assert(rl(V+0x14u)==TARGET_RIGHT);
    assert(rl(V+0x1cu)==TARGET_DOWN);
}
int main(){
    // Earlier 87.231 blanket floor leaked into ordinary jumping normals
    // and grounded light/medium attacks. Only SJ HK still gets Storm.
    const unsigned airActions[]={0x1800u,0x1808u,0x180au,
                                 0x1a00u,0x1a08u,0x1a04u};
    const unsigned airButtons[]={1u,2u,4u,8u,16u,32u};
    for(unsigned i=0;i<6u;++i){
        setup(airActions[i]);
        Actor a=activeFS(1u);
        Decision hit=onset(a,airButtons[i]);
        if(airActions[i]==0x1a04u){
            assert(a.fsConditionalPending && !a.fsPendingSevere);
            assert(std::strcmp(hit.outcome,"FS_STORM_HIT_CONFIRMED_PENDING")==0);
            storm(finish(a));
        }else{
            assert(!a.fsConditionalPending);
            assert(!finish(a).velocityWritten);
            assert(rl(V+0x1cu)==0x00010000u);
        }
    }
    // Grounded normals still register as native hits but never write FS.
    const unsigned groundActions[]={0x1000u,0x1002u,0x1008u,0x1208u};
    const unsigned groundButtons[]={1u,2u,8u,16u};
    for(unsigned i=0;i<4u;++i){
        setup(groundActions[i],0u,0x21u,true);
        Actor a=activeFS(4u);
        onset(a,groundButtons[i]);
        assert(!a.fsConditionalPending && !a.fsPendingSevere);
        assert(!finish(a).velocityWritten);
        assert(rl(V+0x1cu)==0x00010000u);
    }
    // Two exact severe upgrades on real native impact.
    setup(0x1a04u);
    Actor hpHk=activeFS(4u);
    assert(std::strcmp(onset(hpHk,32u).outcome,
        "FS_HP_THEN_HK_CONFIRMED_PENDING")==0);
    assert(hpHk.fsPendingSevere);
    const Decision hpHkRelease=finish(hpHk);
    severe(hpHkRelease,"FS_HP_THEN_HK_CYCLOPS_SEVERE");
    assert(hpHkRelease.previousHitButton==4u); // originating HP, not HK
    setup(0x1004u,0u,0x21u,true);
    Actor groundHp=activeFS(1u);
    onset(groundHp,4u);
    assert(!groundHp.fsConditionalPending);
    assert(!finish(groundHp).velocityWritten);
    setup(0x120au,0u,0x21u,true);
    Actor groundHk=activeFS(8u);
    onset(groundHk,32u);
    assert(!groundHk.fsConditionalPending);
    assert(!finish(groundHk).velocityWritten);

    // HP->LP->HK: the ordinary-jump LP is native; subsequent SJ HK may
    // get Storm but NEVER inherits a previous HP for severe.
    setup(0x1800u);
    Actor gap=activeFS(4u);
    onset(gap,1u);
    assert(gap.lastConnectedButton==1u);
    assert(!finish(gap).velocityWritten);
    sw(A+0x60u,0x1a04u);sw(V+0x08u,0u);
    Decision nowHk=onset(gap,32u);
    assert(nowHk.previousHitButton==1u);
    assert(!gap.fsPendingSevere);
    storm(finish(gap));
    // A connected grounded medium also breaks previous HP without FS.
    setup(0x1002u,0u,0x21u,true);
    Actor groundMp=activeFS(4u);
    onset(groundMp,2u);
    assert(groundMp.lastConnectedButton==2u);
    assert(!finish(groundMp).velocityWritten);

    // Native hit step 0 is only a promise. No conversion until step 2.
    setup(0x1800u);
    Actor noImmediate=activeFS(4u);
    Decision early=onset(noImmediate,1u);
    assert(!early.velocityWritten && rl(V+0x14u)==0x00040000u);

    // Changing move before the original native hit settles cancels ALL
    // pending knockback, whether the proposed variant was Storm or severe.
    setup(0x1a04u);
    Actor severeCancelled=activeFS(4u);
    onset(severeCancelled,32u);
    sw(A+0x60u,0x1a00u);sw(V+0x08u,2u);
    Decision cancelSevere=severeCancelled.evaluate(mem,V,A,true,true,8u);
    assert(!cancelSevere.velocityWritten && !severeCancelled.fsConditionalPending);
    assert(rl(V+0x14u)==0x00040000u);
    setup(0x1800u);
    Actor stormCancelled=activeFS(1u);
    onset(stormCancelled,1u);
    sw(A+0x60u,0x1a02u);sw(V+0x08u,2u);
    Decision cancelStorm=stormCancelled.evaluate(mem,V,A,true,true,16u);
    assert(!cancelStorm.velocityWritten && !stormCancelled.fsConditionalPending);
    assert(rl(V+0x14u)==0x00040000u);

    // An old FS latch cannot cause ordinary-jump LP to overwrite RAM.
    setup(0x1800u);
    Actor hostOnly=activeFS(1u);
    hostOnly.postFlightNeedsNativeLanding=false;
    onset(hostOnly,1u);
    assert(!finish(hostOnly).velocityWritten);

    // A grounded hit never gains Tournament FS retroactively because
    // the attacker jumps before the native reaction settles.
    setup(0x1004u,0u,0x21u,true);
    Actor groundMotion=activeFS(1u);
    onset(groundMotion,4u);
    sw(A+0x10u,140u);sw(A+0xaau,0x0100u);
    const Decision groundMotionRelease=finish(groundMotion);
    assert(!groundMotionRelease.velocityWritten);
    assert(rl(V+0x1cu)==0x00010000u);

    setup(0x1a04u);
    Actor airMotion=activeFS(4u);
    onset(airMotion,32u);
    sw(A+0x10u,472u);sw(A+0xaau,0u); // lands during native hit settle
    severe(finish(airMotion),"FS_HP_THEN_HK_CYCLOPS_SEVERE");

    // Without FS, no new baseline or grounded severe is manufactured.
    setup(0x1004u,0u,0x21u,true);
    Actor without=activeFS(1u);
    without.stormFlightUsedUntilLanding=false;
    without.postFlightNeedsNativeLanding=false;
    onset(without,4u);
    assert(!without.fsConditionalPending);
    assert(!finish(without).velocityWritten);

    // Never overwrite an authentically steep native knockdown.
    setup(0x1a04u);
    Actor native=activeFS(1u);
    onset(native,32u);
    sw(V+0x60u,SEVERE_ACTION);sw(V+0xe0u,SEVERE_PROFILE);
    sl(V+0x14u,0x00060000u);sl(V+0x1cu,0x00120000u);
    const Decision kept=finish(native);
    assert(!kept.velocityWritten && kept.excluded);
    assert(std::strcmp(kept.outcome,"FS_ALREADY_NATIVE_SEVERE_PRESERVED")==0);
    assert(rl(V+0x1cu)==0x00120000u);

    // Preserve the approved Charlie original HP and special HK split.
    setup(0x1804u,0x20u,0x28u);
    Actor charlie=activeFS(1u,0x20u);
    const Decision ch=onset(charlie,4u);
    assert(std::strcmp(ch.outcome,"CHARLIE_AIR_HP_NATIVE_UNCHANGED")==0);
    assert(!charlie.fsConditionalPending && !finish(charlie).velocityWritten);
    assert(charlieAirHK(0x20u,0x28u,0x1a04u,32u));
    setup(0x1a04u,0x20u,0x28u);
    Actor charlieHK=activeFS(1u,0x20u); // first HK, NOT an HP->HK chain
    assert(std::strcmp(onset(charlieHK,32u).outcome,
        "FS_CHARLIE_HK_CONFIRMED_PENDING")==0);
    severe(finish(charlieHK),"CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN");
    // Sabretooth/Cammy HP and Wolverine Drill Claw remain exceptions.
    const unsigned protectedFighters[]={0x000au,0x0022u};
    for(unsigned fighter: protectedFighters){
        setup(0x180au,fighter);
        Actor exception=activeFS(1u,fighter);
        onset(exception,4u);
        assert(!exception.fsConditionalPending);
        assert(!finish(exception).velocityWritten);
    }
    setup(0x2c1eu);
    Actor drill=activeFS(4u);
    onset(drill,4u);
    assert(!drill.fsConditionalPending && drill.lastConnectedButton==0u);

    // A super-jump action at floor height is not a grounded heavy.
    setup(0x1a04u,0u,0x21u,true);
    Actor sj=activeFS(1u);
    assert(!sj.evaluate(mem,V,A,true,true,32u).hitConfirmed);
    assert(!sj.fsConditionalPending);

    // Real native landing resets the FS priority and previous-hit record.
    setup(0x1800u);
    Actor landed=activeFS(4u);
    sw(V+0x10u,472u);sw(V+0xaau,0u);
    landed.evaluate(mem,V,A,true,true,0u);
    assert(!landed.postFlightNeedsNativeLanding &&
           !landed.stormFlightUsedUntilLanding &&
           landed.lastConnectedButton==0u);
    std::puts("87.409 87.231 revised: native grounded/18xx, superjump heavy-only FS: PASS");
    return 0;
}
