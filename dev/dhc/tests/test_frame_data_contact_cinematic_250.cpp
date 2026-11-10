// TEST 87.250: super dark-screen time and independent hit confirmation.
// Read-only ROM sampling; no SDL, Windows APIs, RAM writes or ROM required.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample fighter(unsigned mode,unsigned action,
                             unsigned phase,unsigned step,unsigned combo=0u){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;
    a.attackPhase=phase;a.nativeStep=step;
    a.health=144u;a.combo=combo;
    a.animPtr=0xAA000u;a.animTimer=3u;
    return a;
}
static XM30Sample melee(){
    XM30Sample s{};
    s.melee.active=true;s.melee.sourceBase=0xFF4000u;
    s.melee.x=96;s.melee.y=64;
    return s;
}
static void entireAnimated2ePortraitIsNotAttackTime(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample ken=fighter(0x2A00u,0x2E00u,0x81u,2u);
    XM41ActorSample opponent=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{},lingering{};
    // A previously linked opponent projectile is NOT live gameplay time.
    lingering.projectile.active=true;
    lingering.projectile.sourceBase=0xFF6000u;
    for(int k=0;k<45;++k){
        // Native animation pointers/countdowns can change on the first
        // dark-screen callbacks. They are STILL part of the portrait freeze.
        ken.animPtr=0xAA000u+(unsigned)(k%3)*2u;
        opponent.animTimer=(unsigned)(k%6+1);
        f.onFrame(none,lingering,ken,opponent);
        assert(f.frame==0);
        assert(f.playerFrame[0]==0 && f.playerFrame[1]==0);
    }
    assert(f.superFreezeFrames==45);
    ken.attackPhase=0x80u;ken.nativeStep=4u;
    f.onFrame(melee(),none,ken,opponent);
    assert(f.playerFrame[0]==1u && f.playerFrame[1]==1u);
    assert(f.count==1 && f.history[0].attackFamily==0x2Eu);
    assert(f.history[0].startup==1);
    assert(f.history[0].attackStartFrame==1);
}
static void confirmedComboKeepsBothRedTicksWithoutNetHpLoss(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample ken=fighter(0x2800u,0x2800u,0x80u,0u);
    XM41ActorSample target=fighter(0x3600u,0x1C00u,0u,0u);
    f.onFrame(melee(),{},ken,target);
    assert(f.count==1 && f.history[0].contactCount==0);
    // Training may replenish the visible HP before the observer reads it.
    // Rising ROM combo counts still confirm each hit.
    ken.combo=1;
    f.onFrame(melee(),{},ken,target);
    assert(f.history[0].contactCount==1);
    assert(f.history[0].contactFrames[0]==2u);
    ken.combo=2;
    f.onFrame(melee(),{},ken,target);
    assert(f.count==1 && f.history[0].contactCount==2);
    assert(f.history[0].contactFrames[1]==3u);
    assert(f.history[0].active==3);
    assert(!f.history[0].attackEnded);
}
static void kenAcceptedRomSuperWithMissedBoxHasContactCard(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample ken=fighter(0x2A00u,0x2E00u,0x80u,4u);
    XM41ActorSample target=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{};
    f.onFrame(none,none,ken,target); // native acceptance, no SDL/box
    assert(f.count==0);
    ken.combo=1;
    f.onFrame(none,none,ken,target); // confirmed hit without HP delta
    assert(f.count==1 && f.eventsCreated==1u);
    const XM30Event& e=f.history[0];
    assert(e.noBox && e.attackObserved && !e.inputObserved);
    assert(e.attackFamily==0x2Eu && e.active==0);
    assert(e.contactCount==1 && e.contactFrames[0]==2u);
    ken.combo=2;
    f.onFrame(none,none,ken,target);
    assert(f.count==1 && f.eventsCreated==1u);
    assert(e.contactCount==2 && e.contactFrames[1]==3u);
    assert(e.attackLastFrame==3u && !e.attackEnded);
}
static void firstComboOnlyImpactWithHitstopCountsOneLiveFrame(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample ken=fighter(0x2800u,0x2800u,0x80u,0u);
    XM41ActorSample target=fighter(0x3600u,0x1C00u,0u,0u);
    XM30Sample none{};
    f.onFrame(none,none,ken,target); // native action accepted before offense
    assert(f.count==0 && f.playerFrame[0]==1u);
    ken.hitStop=8u;target.hitStop=8u;ken.combo=1u;
    f.onFrame(melee(),none,ken,target); // HP already restored
    assert(f.playerFrame[0]==2u && f.playerFrame[1]==1u);
    assert(f.count==1 && f.history[0].active==1);
    assert(f.history[0].contactCount==1);
    assert(f.history[0].contactFrames[0]==2u);
    ken.hitStop=7u;target.hitStop=7u;
    f.onFrame(melee(),none,ken,target);
    assert(f.playerFrame[0]==2u && f.history[0].active==1);
}
static void frozenHpAndComboConfirmationMakesOnlyOneRedTick(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample ken=fighter(0x2A00u,0x2E00u,0x80u,4u);
    XM41ActorSample target=fighter(0x3600u,0x1C00u,0u,0u);
    f.onFrame(melee(),{},ken,target); // 1 counted attack frame
    ken.hitStop=6u;target.hitStop=6u;
    ken.combo=1u;target.health=136u;
    f.onFrame(melee(),{},ken,target); // both counters indicate same hit
    assert(f.playerFrame[0]==1u && f.playerFrame[1]==1u);
    assert(f.count==1 && f.history[0].contactCount==1);
    ken.hitStop=5u;target.hitStop=5u;
    f.onFrame(melee(),{},ken,target);
    assert(f.history[0].contactCount==1);
    ken.hitStop=0;target.hitStop=0;
    f.onFrame(melee(),{},ken,target);
    assert(f.playerFrame[0]==2u);
    assert(f.history[0].contactFrames[0]==1u);
    assert(f.history[0].attackLastFrame==2u);
}
int main(){
    entireAnimated2ePortraitIsNotAttackTime();
    confirmedComboKeepsBothRedTicksWithoutNetHpLoss();
    kenAcceptedRomSuperWithMissedBoxHasContactCard();
    frozenHpAndComboConfirmationMakesOnlyOneRedTick();
    firstComboOnlyImpactWithHitstopCountsOneLiveFrame();
    std::puts("XMVSF 87.250 hit confirmation / super cinematic PASS");
}
