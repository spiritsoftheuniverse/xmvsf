// XMVSF TEST 87.251: Cyclops 2E beam can appear long after ROM accepted
// its startup. Read-only tracker regressions based on 87.250 HP/contact trace.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample fighter(unsigned mode,unsigned action,
                              unsigned stage,unsigned phase,unsigned hp=144u){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;
    a.nativeStep=stage;a.attackPhase=phase;a.health=hp;
    return a;
}
static XM30Sample beam(unsigned base,bool sampledAttackBox){
    XM30Sample s{};
    s.projectileCount=1;s.projectileBases[0]=base;
    s.projectileX[0]=153;s.projectileY[0]=92;
    if(sampledAttackBox){
        s.projectile.active=true;s.projectile.sourceBase=base;
        s.projectileHits[0].active=true;
        s.projectileHits[0].sourceBase=base;
    }
    return s;
}
static void acceptedSuperSurvivesRepeatedButtonsUntilLateBeam(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=fighter(0x2A28u,0x2E00u,2u,0x80u);
    XM41ActorSample b=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{};
    f.press(0,0x80);
    f.onFrame(none,none,a,b); // ROM accepts 2E00; beam not yet spawned
    assert(f.count==0 && f.track[0].pendingAttackFamily==0x2Eu);
    const long long originalPending=f.track[0].pending;
    for(int i=0;i<55;++i){
        if(i==6 || i==22 || i==38)
            f.press(0,0x81); // spurious buttons must not erase 2E00
        f.onFrame(none,none,a,b);
        assert(f.track[0].pending==originalPending);
        assert(f.track[0].pendingAttackFamily==0x2Eu);
        assert(f.count==0);
    }
    a.nativeStep=4u;b.health=136u;
    f.onFrame(beam(0xFF6000u,true),none,a,b);
    assert(f.count==1 && f.eventsCreated==1);
    const XM30Event& e=f.history[0];
    assert(e.attackFamily==0x2Eu && e.attackAction==0x2E00u);
    assert(e.source==1 && e.projectileBase==0xFF6000u);
    assert(e.contactCount==1 && e.contactFrames[0]==e.firstFrame);
    assert(e.inputObserved && !e.noBox);
    for(int i=0;i<40;++i){
        if(i==8 || i==19)f.press(0,0x82);
        if(i%5==0)--b.health;
        f.onFrame(beam(0xFF6000u,true),none,a,b);
    }
    assert(f.count==1 && f.eventsCreated==1);
    assert(f.history[0].contactCount==9);
}
static void linkedBeamWithNoSampledAttackBoxStillMakesContactCard(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=fighter(0x2A28u,0x2E00u,2u,0x80u);
    XM41ActorSample b=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{};
    f.press(0,0x80);f.onFrame(none,none,a,b);
    f.press(0,0x81); // must not replace accepted 2E
    a.nativeStep=4u;
    f.onFrame(beam(0xFF6400u,false),none,a,b); // linked spawn
    assert(f.count==0 && f.track[0].pendingSpawnBase==0xFF6400u);
    b.health=135u;
    f.onFrame(beam(0xFF6400u,false),none,a,b); // real HP loss, no kind-5 box
    assert(f.count==1 && f.eventsCreated==1);
    const XM30Event& e=f.history[0];
    assert(e.attackFamily==0x2Eu && e.contactOnly && e.noBox);
    assert(e.source==1 && e.projectileBase==0xFF6400u);
    assert(e.spawnFrame>0 && e.lastProjectileFrame>0);
    assert(e.active==0 && e.hitboxLastFrame==0);
    assert(e.contactCount==1);
    assert(!f.floats[0].visible); // no made-up measured startup
    for(int i=0;i<7;++i){
        --b.health;f.onFrame(beam(0xFF6400u,false),none,a,b);
    }
    assert(f.count==1 && f.history[0].contactCount==8);
}
static void oldProjectileCannotClaimNewNativeSuper(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=fighter(0x0000u,0x0000u,0u,0u);
    XM41ActorSample b=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{},old=beam(0xFF6200u,false);
    f.onFrame(old,none,a,b); // a projectile was already on screen
    f.press(0,0x80);
    a=fighter(0x2A28u,0x2E00u,2u,0x80u);
    f.onFrame(old,none,a,b);
    b.health=138u;
    f.onFrame(old,none,a,b);
    assert(f.count==0); // do not assign old beam damage to new 2E
    a.nativeStep=4u;b.health=130u;
    f.onFrame(beam(0xFF6600u,false),none,a,b); // new linked beam + damage
    assert(f.count==1 && f.history[0].contactOnly);
    assert(f.history[0].projectileBase==0xFF6600u);
}
static void nativeOnlySuperAlsoRetainsIdentity(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=fighter(0x2A28u,0x2E00u,2u,0x80u);
    XM41ActorSample b=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{};
    f.onFrame(none,none,a,b); // SDL input edge was not sampled
    assert(f.track[0].pendingNativeOnly);
    f.press(0,0x81);
    a.nativeStep=4u;b.health=139u;
    f.onFrame(beam(0xFF6800u,false),none,a,b);
    assert(f.count==1 && f.history[0].contactOnly);
    assert(!f.history[0].inputObserved);
    assert(!f.floats[0].visible);
}
int main(){
    acceptedSuperSurvivesRepeatedButtonsUntilLateBeam();
    linkedBeamWithNoSampledAttackBoxStillMakesContactCard();
    oldProjectileCannotClaimNewNativeSuper();
    nativeOnlySuperAlsoRetainsIdentity();
    std::puts("XMVSF 87.251 projectile-super candidate and ownership PASS");
}
