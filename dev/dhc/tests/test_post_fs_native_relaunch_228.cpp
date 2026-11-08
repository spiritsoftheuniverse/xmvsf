// TEST 87.229: prevent late post-FS severe conversions on LP/LK/MP.
// The 87.228 runtime records four bad writes on an airborne light attack,
// 9 frames after an LP native up-launch. Only a new, verified HP/HK
// may be converted and only while its EXACT action remains active.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage87_86_storm_button_guard.h"
using namespace XM8786Flight;
static std::uint8_t ram[65536]={};
static constexpr unsigned V=0x4000u,A=0x4400u;
static void sw(unsigned p,unsigned x){setWord(ram,p,x);}
static unsigned rw(unsigned p){return wordAt(ram,p);}
static void sl(unsigned p,std::uint32_t x){setFixed(ram,p,x);}
static std::uint32_t rl(unsigned p){return rawFixed(ram,p);}
static void setup(unsigned id=0u,unsigned bank=0x21u){
    std::memset(ram,0,sizeof(ram));
    setByte(ram,0x5000u,8u);
    sw(A,0x0101u);sw(V,0x0101u);
    sw(A+0x52u,id);sw(A+0x40u,bank);
    sw(V+0x52u,0x0006u);
    sw(A+0x10u,140u);sw(V+0x10u,275u);
    sw(V+0x254u,472u);sw(V+0xaau,0x0100u);
    sl(A+0x0cu,0x01000000u);sl(V+0x0cu,0x02000000u);
    sl(V+0x14u,0x00070000u);sl(V+0x1cu,0x00002000u);
    sw(V+0x06u,0x000cu);sw(V+0x08u,2u);
    sw(V+0x60u,0x1c14u);sw(V+0xe0u,0x0210u);
}
static void originalStormFS(Actor& actor){
    // Matched to the existing 87.223 ROM-free positive integration test.
    sw(A+0x60u,0u);
    actor.evaluate(ram,V,A,true,true,0u); // native 1C14/0210 evidence
    assert(actor.nativeLaunched);
    sw(A+0x60u,0x1a00u);
    sw(V+0x08u,0u);sw(V+0x60u,0x1c16u);sw(V+0xe0u,0x0220u);
    const Decision lp=actor.evaluate(ram,V,A,true,true,1u);
    assert(lp.hitConfirmed && !lp.velocityWritten);
    sw(V+0x08u,2u);
    actor.evaluate(ram,V,A,true,true,0u);
    sw(A+0x60u,0x1a04u);sw(V+0x08u,0u);
    const Decision heavy=actor.evaluate(ram,V,A,true,true,32u);
    assert(heavy.hitConfirmed && actor.pendingSevere);
    sw(V+0x08u,2u);
    const Decision fs=actor.evaluate(ram,V,A,true,true,0u);
    assert(fs.launch && fs.velocityWritten);
    assert(actor.stormFlightUsedUntilLanding);
    assert(actor.postFlightNeedsNativeLanding);
    assert(!actor.postFlightRelaunchPending);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rl(V+0x1cu)==XM87223Flight::NativeObservedVY);
}
static void nativeRelaunchOnset(Actor& actor,unsigned action,unsigned button,
                                bool expectHeavy){
    sw(A+0x60u,action);
    sw(V+0x60u,0x1c14u);sw(V+0xe0u,0x0210u);
    sw(V+0x08u,0u);
    sl(V+0x1cu,0xfffc6000u); // original ROM launches upward again
    const Decision onset=actor.evaluate(ram,V,A,true,true,button);
    assert(onset.hitConfirmed);
    assert(!onset.velocityWritten);
    assert(actor.postFlightRelaunchPending==expectHeavy);
}
static void finishRelaunch(Actor& actor,bool expectHeavy){
    sw(V+0x08u,2u);
    const Decision resolved=actor.evaluate(ram,V,A,true,true,0u);
    assert(!resolved.launch); // no synthetic second Storm FS
    if(expectHeavy){
        assert(resolved.velocityWritten && resolved.stateWritten);
        assert(std::strcmp(resolved.outcome,
            "POST_FS_SAME_HEAVY_NATIVE_RELAUNCH_SEVERE")==0);
        assert(rw(V+0x60u)==SEVERE_ACTION);
        assert(rw(V+0xe0u)==SEVERE_PROFILE);
        assert(rl(V+0x1cu)==TARGET_DOWN);
    }else{
        assert(!resolved.velocityWritten && !resolved.stateWritten);
        assert(rw(V+0x60u)==0x1c14u);
        assert(rw(V+0xe0u)==0x0210u);
        assert(rl(V+0x1cu)==0xfffc6000u);
    }
    assert(!actor.postFlightRelaunchPending);
    assert(actor.postFlightNeedsNativeLanding); // still airborne
}
static void tryLightNative(Actor& actor,unsigned action,unsigned button){
    nativeRelaunchOnset(actor,action,button,false);
    finishRelaunch(actor,false);
}
static void tryConfirmedHeavy(Actor& actor,unsigned action,unsigned button){
    nativeRelaunchOnset(actor,action,button,true);
    finishRelaunch(actor,true);
}
int main(){
    setup();
    Actor actor;
    originalStormFS(actor);

    // A nonlaunching soft-followup is allowed, with no invented
    // additional severe knockdown. The real anti-infinite is on the
    // subsequent original 1C14/0210 relaunch only.
    sw(A+0x60u,0x1808u);sw(V+0x08u,0u);
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
    sl(V+0x1cu,0x00020000u);
    const Decision medium=actor.evaluate(ram,V,A,true,true,2u);
    assert(medium.hitConfirmed);
    assert(!actor.postFlightRelaunchPending);
    assert(rl(V+0x1cu)==0x00020000u);
    sw(V+0x08u,2u);actor.evaluate(ram,V,A,true,true,0u);

    // 87.228.zip reproducer: all four unwanted severe writes were
    // armed by LP 1800 and applied as LK 1A00 started 9f later.
    // Even ten successive LP/LK native launches must NOT acquire severe
    // from an HK forty or more frames earlier. All these individual
    // connected hits and their real 68000 velocity stay intact.
    for(unsigned i=0;i<10u;++i)
        tryLightNative(actor,(i&1u)?0x1a00u:0x1800u,
                       (i&1u)?8u:1u);
    // Medium attacks must also remain native.
    tryLightNative(actor,0x1808u,2u);

    // Exact 87.228 leak reproduction: LP native hit onset, eight more
    // native 68000 frames with substate 0, then the player switches to
    // LK just as the victim reaches substate 2. No delayed severe write.
    nativeRelaunchOnset(actor,0x1800u,1u,false);
    for(unsigned i=0u;i<8u;++i){
        const Decision held=actor.evaluate(ram,V,A,true,true,0u);
        assert(!held.velocityWritten);
        assert(!actor.postFlightRelaunchPending);
    }
    sw(A+0x60u,0x1a00u);sw(V+0x08u,2u);
    const Decision lpToLk=actor.evaluate(ram,V,A,true,true,8u);
    assert(!lpToLk.velocityWritten && !lpToLk.stateWritten);
    assert(!actor.postFlightRelaunchPending);
    assert(rw(V+0x60u)==0x1c14u);
    assert(rl(V+0x1cu)==0xfffc6000u);

    // If a fresh verified HP/HK itself really starts the native launcher,
    // that SAME action can still receive Cyclops severe at substate 2.
    tryConfirmedHeavy(actor,0x1a04u,32u);

    // Critical anti-leak case: an HP arms a pending conversion, but
    // LK begins before the 68000 reaches native hit step 2. The pending
    // decision MUST be cancelled instead of transferring to the LK.
    nativeRelaunchOnset(actor,0x180au,4u,true);
    sw(A+0x60u,0x1a00u);sw(V+0x08u,2u);
    const Decision interrupted=actor.evaluate(ram,V,A,true,true,8u);
    assert(!interrupted.velocityWritten && !interrupted.stateWritten);
    assert(!actor.postFlightRelaunchPending);
    assert(rw(V+0x60u)==0x1c14u);
    assert(rl(V+0x1cu)==0xfffc6000u);

    // Only authentic native floor/air state re-arms a new aerial sequence.
    sw(V+0x10u,472u);sw(V+0xaau,0u);
    actor.evaluate(ram,V,A,true,true,0u);
    assert(!actor.postFlightNeedsNativeLanding);
    assert(!actor.stormFlightUsedUntilLanding);
    assert(!actor.postFlightRelaunchPending);

    // Charlie's already accepted native HP / Cyclops HK remains separate:
    // his INITIAL severe is untouched. A new *native* re-launch before
    // actual landing still receives the universal anti-infinite recovery.
    setup(0x20u,0x28u);sw(A+0x60u,0x1a04u);
    Actor charlie;
    charlie.nativeLaunched=true;charlie.pendingSevere=true;
    charlie.pendingAge=0u;
    charlie.pendingAction=0x1a04u;charlie.pendingButton=32u;
    charlie.pendingBank=0x28u;charlie.pendingAttackerId=0x20u;
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
    const Decision hk=charlie.evaluate(ram,V,A,true,true,0u);
    assert(hk.launch && hk.velocityWritten);
    assert(rl(V+0x1cu)==TARGET_DOWN);
    assert(charlie.postFlightNeedsNativeLanding);
    assert(!charlie.postFlightRelaunchPending);
    tryLightNative(charlie,0x1800u,1u); // light input remains native
    tryConfirmedHeavy(charlie,0x1a04u,32u); // only actual new HK goes severe

    // Original Charlie airborne HP is still never replaced by an
    // initial synthetic FS, even in the universal anti-infinite test.
    setup(0x20u,0x28u);
    sw(A+0x60u,0x1804u);sw(V+0x08u,0u);
    sw(V+0x60u,SOFT_ACTION);sw(V+0xe0u,SOFT_PROFILE);
    Actor nativeHP;
    nativeHP.observed=true;nativeHP.nativeLaunched=true;
    nativeHP.prevReaction=0x000cu;nativeHP.prevSubstate=2u;
    nativeHP.prevVictimY=180u;nativeHP.prevAction=0x1c14u;
    nativeHP.prevProfile=0x0210u;
    nativeHP.connectedAirHits=1u;nativeHP.attackSerial=1u;
    nativeHP.prevAttackNow=true;nativeHP.prevAttackerAction=0x1800u;
    nativeHP.lastAttackerId=0x20u;nativeHP.lastVictimId=0x06u;
    const Decision nativeHit=nativeHP.evaluate(ram,V,A,true,true,4u);
    assert(nativeHit.hitConfirmed && nativeHit.excluded);
    assert(!nativeHit.velocityWritten);
    assert(!nativeHP.postFlightNeedsNativeLanding);

    std::puts("87.229 LP/LK/MP native after FS; same-heavy-only conversion; action switch cancel; Charlie preserved: PASS");
    return 0;
}
