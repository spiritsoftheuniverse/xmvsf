// XMVSF TEST 87.254 — ROM-owned super entries begin at input/native
// acceptance, regardless of first box, portrait freeze or QCD heuristics.
// Renderer uses the identical fixed-size wrap geometry for both slim lanes.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"
static XM41ActorSample fighter(unsigned mode,unsigned action,
                             unsigned stage,unsigned phase,
                             unsigned ptr,unsigned timer=3){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;
    a.nativeStep=stage;a.attackPhase=phase;a.health=144;
    a.animPtr=ptr;a.animTimer=timer;return a;
}
static XM30Sample beam(unsigned base){
    XM30Sample s{};s.projectileCount=1;s.projectileBases[0]=base;
    s.projectileX[0]=120;s.projectileY[0]=73;
    s.projectileHits[0].active=true;
    s.projectileHits[0].sourceBase=base;
    s.projectile.active=true;s.projectile.sourceBase=base;
    return s;
}
static void acceptedQcd08WithPhaseZeroHasAnImmediateCard(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto p=fighter(0x2A28u,0x2E08u,2u,0u,0xA9000u);
    auto b=fighter(0x3600u,0x1C00u,0u,0u,0xD9000u);
    XM30Sample none{};
    // Button sequence and static-motion labeling are not prerequisites:
    // the native action family and entry transition prove acceptance.
    d.onFrame(none,none,p,b);
    assert(d.count==1 && d.eventsCreated==1);
    assert(d.history[0].attackFamily==0x2Eu);
    assert(d.history[0].attackAction==0x2E08u);
    assert(d.history[0].noBox && d.history[0].active==0);
    assert(!d.history[0].inputObserved);
    assert(d.history[0].inputWallFrame==1);
    assert(d.history[0].lastWallFrame==1);
    const auto startNative=d.history[0].attackStartFrame;
    for(int i=0;i<460;++i){
        d.onFrame(none,none,p,b); // unchanged portraits/animation pointers
        assert(d.count==1 && d.eventsCreated==1);
    }
    assert(d.wallFrame[0]==461 && d.history[0].lastWallFrame==461);
    assert(d.playerFrame[0]==1); // visual bar moved, native time did not
    assert(d.history[0].attackStartFrame==startNative);
    assert(d.history[0].active==0 && d.history[0].noBox);
    p.nativeStep=4;p.animTimer=2;
    d.onFrame(beam(0xFF6000u),none,p,b); // first observed collision AFTER portrait
    assert(d.count==1 && d.eventsCreated==1);
    assert(!d.history[0].noBox && d.history[0].source==1);
    assert(d.history[0].firstFrame==d.playerFrame[0]);
    assert(d.history[0].active==1);
    // Native ID and wall start never moved when the hitbox arrived.
    assert(d.history[0].inputWallFrame==0);
}
static void registeredInputPreservesBeginningThroughRelease(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    XM30Sample none{};
    auto p=fighter(0x2800u,0x2800u,0u,0u,0xA9000u);
    auto b=fighter(0x3600u,0x1C00u,0u,0u,0xD9000u);
    d.onFrame(none,none,p,b); // clock=1
    d.direction(0,false,false,false,true);
    d.direction(0,false,true,false,true);
    d.direction(0,false,true,false,false);
    d.press(0,0x80);
    assert(d.track[0].motion==XM32_QCD);
    const auto inputWall=d.track[0].pendingWallFrame;
    const auto inputNative=d.track[0].pending;
    p=fighter(0x2A28u,0x2E08u,2u,0u,0xA9000u);
    d.onFrame(none,none,p,b);
    assert(d.count==1 && d.eventsCreated==1);
    assert(d.history[0].inputObserved && d.history[0].code==0x80);
    assert(d.history[0].motion==XM32_QCD);
    assert(d.history[0].inputWallFrame==inputWall);
    assert(d.history[0].inputFrame==(unsigned long long)inputNative);
    assert(d.history[0].noBox && !d.history[0].contactOnly);
    d.press(0,0x81); // another PP edge must not restart accepted 2E
    for(int i=0;i<20;++i)d.onFrame(none,none,p,b);
    assert(d.count==1 && d.eventsCreated==1);
    assert(d.history[0].inputWallFrame==inputWall);
}
static void fixedWidthOverflowDoesNotRescaleEarlierRows(){
    XM30TimelineWrap layout{};
    layout.start=18;layout.ticksPerRow=200;layout.finish=117;
    assert(layout.rows()==1 && layout.row(18)==0u);
    assert(layout.column(18)==0u && layout.column(117)==99u);
    layout.finish=218; // first wrap begins at tick 218
    assert(layout.rows()==2 && layout.row(217)==0u);
    assert(layout.column(217)==199u);
    assert(layout.row(218)==1u && layout.column(218)==0u);
    layout.finish=680;
    assert(layout.rows()==4);
    assert(layout.row(117)==0u && layout.column(117)==99u);
    assert(layout.row(218)==1u && layout.column(218)==0u);
    assert(layout.row(618)==3u && layout.column(618)==0u);
    assert(layout.row(680)==3u && layout.column(680)==62u);
}
static void aNewRealSuperAfterNativeExitGetsANewCard(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    XM30Sample none{};
    auto p=fighter(0x2A28u,0x2E00u,2u,0x80u,0xA9000u);
    auto b=fighter(0x3600u,0x1C00u,0u,0u,0xD9000u);
    d.onFrame(none,none,p,b);
    assert(d.count==1);
    p=fighter(0,0,0,0,0xA9000u);
    d.onFrame(none,none,p,b);
    assert(d.history[0].attackEnded);
    p=fighter(0x2A28u,0x2E08u,2u,0x00u,0xA9000u);
    d.onFrame(none,none,p,b);
    assert(d.count==2 && d.eventsCreated==2);
    assert(d.history[0].attackAction==0x2E00u);
    assert(d.history[1].attackAction==0x2E08u);
    assert(d.history[1].inputWallFrame>=d.history[0].lastWallFrame);
}
static void nativeSuperCancelClosesOldMeleeButNotProjectiles(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto p=fighter(0x2800u,0x2800u,0u,0x80u,0xA9000u);
    auto b=fighter(0x3600u,0x1C00u,0u,0u,0xD9000u);
    XM30Sample none{},hit{};hit.melee.active=true;
    d.press(0,0x80);d.onFrame(hit,none,p,b);
    assert(d.count==1 && d.history[0].attackFamily==0x28u);
    assert(d.history[0].active==1);
    d.press(0,0x81);
    p=fighter(0x2A28u,0x2E08u,2u,0u,0xA9000u);
    d.onFrame(none,none,p,b); // no super hitbox yet
    assert(d.count==2 && d.eventsCreated==2);
    assert(d.history[0].attackEnded && d.history[0].closed);
    assert(d.history[0].active==1);
    assert(d.history[1].attackFamily==0x2Eu);
    assert(d.history[1].noBox && d.history[1].active==0);
    assert(d.history[1].inputObserved);
}
int main(){
    nativeSuperCancelClosesOldMeleeButNotProjectiles();
    acceptedQcd08WithPhaseZeroHasAnImmediateCard();
    registeredInputPreservesBeginningThroughRelease();
    fixedWidthOverflowDoesNotRescaleEarlierRows();
    aNewRealSuperAfterNativeExitGetsANewCard();
    std::puts("XMVSF 87.254 native super entry and wrapped timeline PASS");
}
