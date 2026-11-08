// TEST 87.228: real native 1C14/0210 relaunches cannot produce an
// indefinite airborne loop after a Storm-profile FS. This is NOT a
// second Storm boost; Cyclops' verified severe downward knockdown ends it.
// The user's 87.227.tsv shows repeated ~4-hit air strings while the old
// synthetic-flight latch correctly blocked repeat artificial launches.
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
    assert(actor.stormFlightNeedsNativeLanding);
    assert(!actor.postFlightRelaunchPending);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rl(V+0x1cu)==XM87223Flight::NativeObservedVY);
}
static void tryNativeRelaunch(Actor& actor,unsigned action,unsigned button){
    // Original 68000 starts another air string without reaching the
    // true native floor. This is the real 87.227 infinite mechanism.
    sw(A+0x60u,action);
    sw(V+0x60u,0x1c14u);sw(V+0xe0u,0x0210u);
    sw(V+0x08u,0u);
    sl(V+0x1cu,0xfffc6000u); // up again, even though already in FS
    const Decision onset=actor.evaluate(ram,V,A,true,true,button);
    assert(onset.hitConfirmed);
    assert(onset.nativeLaunched);
    assert(!onset.velocityWritten);
    assert(actor.postFlightRelaunchPending);
    sw(V+0x08u,2u);
    const Decision resolved=actor.evaluate(ram,V,A,true,true,0u);
    assert(resolved.velocityWritten && resolved.stateWritten);
    assert(!resolved.launch); // no fabricated second Storm FS
    assert(std::strcmp(resolved.outcome,
        "POST_STORM_FS_NATIVE_RELAUNCH_SEVERE_LANDING")==0);
    assert(rw(V+0x60u)==SEVERE_ACTION);
    assert(rw(V+0xe0u)==SEVERE_PROFILE);
    assert(rl(V+0x1cu)==TARGET_DOWN);
    assert(!actor.postFlightRelaunchPending);
    assert(actor.stormFlightNeedsNativeLanding); // until actual landing
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

    // The same native relaunch repeated enough times for a 40-hit
    // infinite must always be sent DOWN; no repeat soft launch or
    // additional Storm-boost allowed.
    for(unsigned i=0;i<10u;++i)
        tryNativeRelaunch(actor,(i&1u)?0x1806u:0x1a06u,
                          (i&1u)?1u:8u);

    // Only authentic native floor/air state re-arms a new aerial sequence.
    sw(V+0x10u,472u);sw(V+0xaau,0u);
    actor.evaluate(ram,V,A,true,true,0u);
    assert(!actor.stormFlightNeedsNativeLanding);
    assert(!actor.stormFlightUsedUntilLanding);
    assert(!actor.postFlightRelaunchPending);

    // Charlie's already accepted native HP / Cyclops HK remains separate:
    // his original severe is applied directly, so no Storm-specific
    // follow-up relaunch intervention is ever armed.
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
    assert(!charlie.stormFlightNeedsNativeLanding);
    assert(!charlie.postFlightRelaunchPending);

    std::puts("87.228 native re-launch anti-infinite + ordinary followup + Charlie preserved: PASS");
    return 0;
}
