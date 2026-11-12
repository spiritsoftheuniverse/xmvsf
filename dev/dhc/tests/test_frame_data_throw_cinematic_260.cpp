// XMVSF 87.260: standalone/manual ROM-model coverage.
// NOT a mandatory BUILD_TEST step. No character-name or ROM-address whitelist.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"
static XM41ActorSample actor(unsigned mode,unsigned action,unsigned step,
                            unsigned phase,unsigned anim,unsigned timer=1u){
    XM41ActorSample a{};
    a.valid=true;a.sourceBase=0xFF4000u;a.mode=mode;
    a.nativeAction=action;a.nativeStep=step;a.attackPhase=phase;
    a.animPtr=anim;a.animTimer=timer;a.health=144u;
    return a;
}
static XM41ActorSample dummy(){
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD1000u);
    b.sourceBase=0xFF4400u;return b;
}
static void portraitPhase81AcrossNativeSubstages(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    auto p=actor(0x2A28u,0x2E06u,4u,0x81u,0xAA000u);
    auto b=dummy();XM30Sample none{};
    f.onFrame(none,none,p,b); // super accepted before global hold
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.history[0].attackFamily==0x2Eu && f.history[0].noBox);
    assert(f.wallTick(0,1).stopped && f.wallTick(1,1).stopped);
    for(int i=0;i<12;++i)f.onFrame(none,none,p,b);
    assert(f.count==1 && f.playerFrame[0]==0u);
    assert(f.history[0].lastWallFrame==13u);
    p.attackPhase=0x80u;p.animTimer=2u;
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.playerFrame[0]==1u);
    assert(!f.wallTick(0,14).stopped);
}
static void stageTwoPhase80StasisMakesRealGrayTime(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    auto p=actor(0x2A28u,0x2E00u,2u,0x80u,0xAA100u);
    auto b=dummy();XM30Sample none{};
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.wallTick(0,1).stopped);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.wallTick(0,2).stopped && f.wallTick(1,2).stopped);
}
static void grabSuperKeepsOneCardAcrossThrowController(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    auto p=actor(0x2A28u,0x2E00u,2u,0x81u,0xAA200u);
    auto b=dummy();XM30Sample none{};
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==0x2Eu);
    p=actor(0x3200u,0x0200u,4u,0u,0xAA210u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.history[0].attackEnded);
    for(int i=0;i<8;++i){p.nativeAction=0x0202u;p.animTimer+=1u;
        f.onFrame(none,none,p,b);}
    assert(f.count==1 && f.eventsCreated==1u);
    p=actor(0x2A28u,0x2E08u,4u,0x80u,0xAA220u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.history[0].attackEnded);
    p=actor(0x3600u,0x1C00u,0u,0u,0xAA300u);
    f.onFrame(none,none,p,b);
    assert(f.history[0].attackEnded);
    p=actor(0x2A28u,0x2E00u,2u,0x81u,0xAA200u);
    f.onFrame(none,none,p,b);
    assert(f.count==2 && f.eventsCreated==2u);
}
static void lateSuperOpcodePromotesTheExistingGrabBar(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x3200u,0x0200u,2u,0x81u,0xAA330u);
    f.press(0,0x80);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==XM30_THROW_FAMILY);
    const auto originalWall=f.history[0].inputWallFrame;
    for(int i=0;i<6;++i)f.onFrame(none,none,p,b);
    p=actor(0x2A28u,0x2E00u,4u,0x80u,0xAA340u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.history[0].attackFamily==0x2Eu);
    assert(f.history[0].inputWallFrame==originalWall);
    p=actor(0x3200u,0x0202u,4u,0u,0xAA341u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.history[0].attackEnded);
}
static void passiveGrabbedFighterNeverGetsAnAttackCard(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};
    auto p=actor(0x3200u,0x0200u,2u,0u,0xAA350u);
    auto b=dummy(); // P1 was just grabbed by P2, no P1 input
    f.onFrame(none,none,p,b);
    assert(f.count==0 && f.eventsCreated==0u);
    p.nativeAction=0x0202u;
    f.onFrame(none,none,p,b);
    assert(f.count==0 && f.eventsCreated==0u);
}
static void standaloneThrowAndRealChainedThrow(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    auto p=actor(0x3200u,0x0200u,2u,0u,0xAA400u);
    auto b=dummy();XM30Sample none{};
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==XM30_THROW_FAMILY);
    assert(f.history[0].noBox && f.history[0].active==0);
    p.nativeAction=0x0202u;p.animTimer+=1u;
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.eventsCreated==1u);
    f.press(0,0x81);p.nativeAction=0x020Cu;p.animTimer+=1u;
    f.onFrame(none,none,p,b);
    assert(f.count==2 && f.history[1].attackFamily==XM30_THROW_FAMILY);
}
static void counterFlipAndFollowupWithoutBoxes(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    auto p=actor(0x2A28u,0x2C12u,2u,0u,0xAA500u);
    auto b=dummy();XM30Sample none{};
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==0x2Cu);
    assert(f.history[0].noBox && f.history[0].active==0);
    p=actor(0x3200u,0x0200u,4u,0u,0xAA510u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.history[0].attackEnded);
    f.press(0,0x81);
    p=actor(0x2A28u,0x2C36u,2u,0u,0xAA520u);
    f.onFrame(none,none,p,b);
    assert(f.count==2 && f.eventsCreated==2u);
    assert(f.history[0].attackEnded);
    assert(f.history[1].attackFamily==0x2Cu);
}

static void whiffedGrabControllerWithoutCollisionStillHasARealCard(){
    // A ROM-accepted command grab whiff may animate in controller 30,
    // never reaching successful catch controller 32 or action 2E.
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x3000u,0x0200u,2u,0u,0xAA610u);
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.history[0].attackFamily==XM30_THROW_FAMILY);
    assert(f.history[0].noBox && f.history[0].active==0);
    assert(f.history[0].inputObserved && f.history[0].contactCount==0);
    const auto wall=f.history[0].inputWallFrame;
    p.nativeAction=0x0202u;p.animTimer+=1u;
    f.onFrame(none,none,p,b);
    assert(f.count==1 && !f.history[0].attackEnded);
    p=actor(0x3200u,0x020Cu,4u,0u,0xAA620u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].inputWallFrame==wall);
    p=actor(0x3600u,0x1C00u,0u,0u,0xAA630u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackEnded);
}
static void whiffedSuperGrabDoesNotDuplicateWhen2EAppears(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x2000u,0x0200u,2u,0u,0xAA640u);
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].noBox);
    p=actor(0x2A28u,0x2E00u,2u,0x81u,0xAA641u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.history[0].attackFamily==0x2Eu);
}
static void passiveWhiffControllersNeverInventCards(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x3000u,0x0200u,2u,0u,0xAA650u);
    f.onFrame(none,none,p,b);
    assert(f.count==0 && f.eventsCreated==0u);
    p=actor(0x2000u,0x0202u,4u,0u,0xAA651u);
    f.onFrame(none,none,p,b);
    assert(f.count==0);
}
static void superVictimHitstopIsVisualNotANativeClockFreeze(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x2A28u,0x2E00u,4u,0x80u,0xAA660u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==0x2Eu);
    const auto earlier=f.playerFrame[0];
    b.hitStop=5u;
    p.animTimer+=1u;
    f.onFrame(none,none,p,b);
    const auto tick=f.wallTick(0,2);
    assert(tick.wall==2u && !tick.stopped && tick.opponentHitStop);
    assert(f.playerFrame[0]==earlier+1u);
    assert(xm30VisualHeld(tick,f.history[0],2u,tick.native));
    const XM30Event& e=f.history[0];
    XM30Event normal=e;normal.attackFamily=0x28u;
    assert(!xm30VisualHeld(tick,normal,2u,tick.native));
    XM30WallTick idle=tick;idle.opponentHitStop=false;
    assert(!xm30VisualHeld(idle,e,2u,tick.native));
    assert(!xm30VisualHeld(tick,e,2u,e.attackStartFrame-1u));
}
// 87.285: an ordinary 20/30 controller after a whiff is not a throw.
// The old generic 20/30->32 family mapping left this card running until
// the 360-wall-tick rescue timeout, hiding its actual ROM end.
static void grabWhiffReturningToNeutralControllerEndsImmediately(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x3000u,0x0200u,2u,0u,0xAA700u);
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==XM30_THROW_FAMILY);
    for(int i=0;i<8;++i){
        p.nativeAction=0x0202u;++p.animTimer;
        f.onFrame(none,none,p,b);
    }
    assert(!f.history[0].attackEnded);
    const unsigned long long lastAttack=f.history[0].attackLastFrame;
    // The controller stays in the 20 family while the ROM leaves 02xx.
    p=actor(0x2000u,0x1C00u,0u,0u,0xAA710u);
    f.onFrame(none,none,p,b);
    assert(f.history[0].attackEnded);
    assert(f.history[0].attackEndFrame==lastAttack);
    const unsigned long long wallAtEnd=f.history[0].lastWallFrame;
    for(int i=0;i<400;++i){
        ++p.animTimer;f.onFrame(none,none,p,b);
    }
    assert(f.count==1 && f.eventsCreated==1u);
    assert(f.history[0].lastWallFrame==wallAtEnd);
    assert(f.history[0].attackEndFrame==lastAttack);
}
static void grabSuperAlsoEndsWhenThrowReleasesToNeutralTwenty(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x2A28u,0x2E00u,4u,0x80u,0xAA720u);
    f.onFrame(none,none,p,b);
    assert(f.count==1 && f.history[0].attackFamily==0x2Eu);
    p=actor(0x3200u,0x0200u,4u,0u,0xAA721u);
    f.onFrame(none,none,p,b);
    assert(!f.history[0].attackEnded);
    p=actor(0x2000u,0x1C00u,0u,0u,0xAA722u);
    f.onFrame(none,none,p,b);
    assert(f.history[0].attackEnded);
    assert(f.eventsCreated==1u);
}
static void neutralTwentyThirtyNeverPretendToBeAttacks(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample none{};auto b=dummy();
    auto p=actor(0x2000u,0x1C00u,0u,0u,0xAA730u);
    f.press(0,0x80);f.onFrame(none,none,p,b);
    assert(xm30NativeAttackFamily(p)==0u && f.count==0);
    p.mode=0x3000u;p.nativeAction=0x1000u;
    f.onFrame(none,none,p,b);
    assert(xm30NativeAttackFamily(p)==0u && f.count==0);
    p.nativeAction=0x0200u;p.nativeStep=2u;
    assert(xm30NativeAttackFamily(p)==XM30_THROW_FAMILY);
}

int main(){
    portraitPhase81AcrossNativeSubstages();
    stageTwoPhase80StasisMakesRealGrayTime();
    grabSuperKeepsOneCardAcrossThrowController();
    lateSuperOpcodePromotesTheExistingGrabBar();
    passiveGrabbedFighterNeverGetsAnAttackCard();
    standaloneThrowAndRealChainedThrow();
    counterFlipAndFollowupWithoutBoxes();
    whiffedGrabControllerWithoutCollisionStillHasARealCard();
    whiffedSuperGrabDoesNotDuplicateWhen2EAppears();
    passiveWhiffControllersNeverInventCards();
    superVictimHitstopIsVisualNotANativeClockFreeze();
    grabWhiffReturningToNeutralControllerEndsImmediately();
    grabSuperAlsoEndsWhenThrowReleasesToNeutralTwenty();
    neutralTwentyThirtyNeverPretendToBeAttacks();
    std::puts("XMVSF 87.260 grab whiff and defender hitstop display coverage PASS");
    return 0;
}
