// XMVSF TEST 87.240: damage never terminates native action duration.
// This standalone test needs no SDL renderer, ROM, or platform compiler.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample actor(unsigned mode,unsigned action,unsigned hp){
    XM41ActorSample a{};
    a.valid=true;
    a.mode=mode;
    a.nativeAction=action;
    a.health=hp;
    return a;
}
static XM30Sample melee(){
    XM30Sample m{};
    m.melee.active=true;
    m.melee.sourceBase=0xFF4000;
    return m;
}
static void contactPreservesDuration(){
    XM30FrameData f{};
    f.setEnabled(true);
    f.beginCombat();
    f.press(0,0x80);
    XM41ActorSample attacker=actor(0x2800,0x2800,144);
    XM41ActorSample victim=actor(0,0,144);
    XM30Sample hit=melee(),none{};
    f.onFrame(hit,none,attacker,victim,50);
    assert(f.count==1 && f.history[0].active==1);
    f.onFrame(hit,none,attacker,victim,49);
    assert(f.history[0].active==2 && f.history[0].contactCount==0);
    // A real hit reduces defender HP and the collision box disappears.
    // On that precise frame the attack controller may transiently change.
    victim.health=136;
    attacker.mode=0;
    f.onFrame(none,none,attacker,victim,49);
    assert(f.history[0].closed); // hitbox ended, not the attack
    assert(!f.history[0].attackEnded);
    assert(f.history[0].attackLastFrame==2);
    assert(f.history[0].attackExitPending==1); // no invented ROM-owned frame
    assert(f.history[0].contactCount==1);
    assert(f.history[0].contactFrames[0]==3);
    // The native move resumes for recovery: orange duration keeps growing.
    attacker.mode=0x2800;
    f.onFrame(none,none,attacker,victim,49);
    assert(!f.history[0].attackEnded);
    assert(f.history[0].attackLastFrame==4);
    assert(f.history[0].contactCount==1);
    // True return to neutral ends the original ROM action exactly.
    attacker.mode=0;
    f.onFrame(none,none,attacker,victim,49);
    assert(f.history[0].attackEnded);
    assert(f.history[0].attackEndFrame==4);
    // Multi-hits attach to the same ongoing super and get separate ticks.
}
static void bothMetersResumeAfterStopAndStalledClock(){
    XM30FrameData f{};
    f.setEnabled(true);
    f.beginCombat();
    f.press(0,0x80);
    f.press(1,0x80);
    XM41ActorSample a=actor(0x2A00,0x2E00,144);
    a.nativeStep=4;
    XM41ActorSample b=actor(0x2800,0x2800,144);
    XM30Sample hit=melee();
    f.onFrame(hit,hit,a,b,39);
    assert(f.count==2 && f.frame==1);
    // FF5009 stops changing, but the native ROM keeps processing.
    for(int i=0;i<6;++i)f.onFrame(hit,hit,a,b,39);
    assert(f.frame==7 && f.superFreezeFrames==0);
    assert(f.history[0].attackLastFrame==7);
    assert(f.history[1].attackLastFrame==7);
    // Hitstop starts and counts down. Only those actual stop steps freeze.
    a.hitStop=3;
    f.onFrame(hit,hit,a,b,39);
    a.hitStop=2;
    f.onFrame(hit,hit,a,b,39);
    assert(f.frame==7 && f.superFreezeFrames==2);
    // Repeated stale nonzero values cannot deadlock both meters.
    f.onFrame(hit,hit,a,b,39);
    assert(f.frame==8);
    a.hitStop=0;
    f.onFrame(hit,hit,a,b,39);
    assert(f.frame==9 && f.history[0].attackLastFrame==9 &&
           f.history[1].attackLastFrame==9);
}
static void multipleSuperHitsKeepOneAction(){
    XM30FrameData f{};
    f.setEnabled(true);
    f.beginCombat();f.press(0,0x80);
    XM41ActorSample a=actor(0x2A00,0x2E00,144);
    a.nativeStep=4;a.attackPhase=0x80;
    XM41ActorSample b=actor(0,0,144);
    XM30Sample hit=melee(), none{};
    f.onFrame(hit,none,a,b,40);
    b.health=140;
    f.onFrame(none,none,a,b,39);
    b.health=136;
    f.onFrame(none,none,a,b,38);
    assert(f.count==1 && f.history[0].contactCount==2);
    assert(f.history[0].contactFrames[0]==2);
    assert(f.history[0].contactFrames[1]==3);
    assert(!f.history[0].closed && !f.history[0].attackEnded);
    a.attackPhase=0x40; a.mode=0;
    f.onFrame(none,none,a,b,37);
    assert(!f.history[0].closed && !f.history[0].attackEnded);
    f.onFrame(none,none,a,b,36);
    assert(!f.history[0].closed && !f.history[0].attackEnded);
    f.onFrame(none,none,a,b,35);
    assert(f.history[0].closed && f.history[0].attackEnded);
    assert(f.history[0].attackEndFrame==3);
}
static void frozenContactRetainsOneSuperTimeline(){
    XM30FrameData f{};
    f.setEnabled(true);
    f.beginCombat();
    f.press(0,0x80);
    XM41ActorSample a=actor(0x2A00,0x2E00,144);
    a.nativeStep=4u;a.attackPhase=0x80u;
    XM41ActorSample b=actor(0,0,144);
    XM30Sample hit=melee(),none{};
    f.onFrame(hit,none,a,b,40);
    assert(f.count==1 && f.frame==1);
    // Contact arrives on a ROM-stopped step: the frame counter is frozen,
    // but the hit belongs to the already-running attack's existing tick.
    a.hitStop=3;b.health=140;
    f.onFrame(none,none,a,b,40);
    assert(f.frame==1 && f.history[0].contactCount==1);
    a.hitStop=2;
    f.onFrame(none,none,a,b,40);
    assert(f.frame==1 && f.count==1);
    // The first post-stop sample briefly carries a non-attack controller.
    a.hitStop=0;a.mode=0;a.attackPhase=0;
    f.onFrame(none,none,a,b,40);
    assert(f.frame==2 && f.count==1);
    assert(!f.history[0].closed && !f.history[0].attackEnded);
    // A repeated input must not create a new card when 2E/2E00 resumes.
    f.press(0,0x81);
    a.mode=0x2A00;a.attackPhase=0x80;
    f.onFrame(hit,none,a,b,40);
    assert(f.count==1);
    assert(f.history[0].attackStartFrame==1);
    assert(f.history[0].contactCount==1);
    assert(!f.history[0].attackEnded);
    b.health=136;
    f.onFrame(none,none,a,b,40);
    assert(f.count==1 && f.history[0].contactCount==2);
    assert(f.history[0].attackLastFrame==4);
}
int main(){
    contactPreservesDuration();
    bothMetersResumeAfterStopAndStalledClock();
    multipleSuperHitsKeepOneAction();
    frozenContactRetainsOneSuperTimeline();
    std::puts("XMVSF 87.240 attack contact and two-meter resume PASS");
}
