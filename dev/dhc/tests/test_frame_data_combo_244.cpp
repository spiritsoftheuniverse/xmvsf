// XMVSF TEST 87.244: native action handoffs, not only new box edges.
// Self-contained ROM-free regression; does not modify native gameplay.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample fighter(unsigned mode,unsigned action,unsigned phase=0x80){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;a.attackPhase=phase;
    a.health=144;
    return a;
}
static XM30Sample oneContinuousBox(){
    XM30Sample s{};
    s.melee.active=true;s.melee.sourceBase=0xFF4000u;
    return s;
}
static void comboNormalsWithoutBoxGap(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample box=oneContinuousBox(),none{};
    XM41ActorSample a=fighter(0x2800,0x2800);
    XM41ActorSample b=fighter(0,0,0);
    f.press(0,0x80);
    f.onFrame(box,none,a,b); // LP frame 1
    f.onFrame(box,none,a,b); // LP frame 2; collision remains high
    assert(f.count==1 && f.history[0].active==2);
    f.press(0,0x81);         // MP combo input during ongoing LP
    f.onFrame(box,none,a,b); // LP still native owner
    assert(f.count==1);
    assert(f.track[0].pending>=0);
    a.nativeAction=0x2810;   // ROM accepts new MP action
    f.onFrame(box,none,a,b);
    assert(f.count==2);     // old code ignored permanently-high box
    const XM30Event& lp=f.history[0];
    const XM30Event& mp=f.history[1];
    assert(lp.closed && lp.attackEnded);
    assert(lp.endFrame==3 && lp.attackEndFrame==3);
    assert(lp.active==3);
    assert(mp.firstFrame==4 && mp.active==1);
    assert(mp.attackStartFrame==4 && mp.attackLastFrame==4);
    assert(mp.code==0x81 && mp.attackFamily==0x28u);
    f.onFrame(box,none,a,b);
    assert(f.count==2 && mp.active==2 && mp.attackLastFrame==5);
}
static void comboCancelNormalIntoSuper(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample box=oneContinuousBox(),none{};
    XM41ActorSample a=fighter(0x2800,0x2800),b=fighter(0,0,0);
    f.press(0,0x80);
    f.onFrame(box,none,a,b);
    f.press(0,0x81); // second attack command before collision drops
    a.mode=0x2A00;a.nativeAction=0x2E00;a.nativeStep=4;
    f.onFrame(box,none,a,b);
    assert(f.count==2);
    assert(f.history[0].attackEnded && f.history[0].closed);
    assert(f.history[1].attackFamily==0x2Eu);
    assert(f.history[1].attackStartFrame==2);
    assert(f.history[1].active==1);
}
static void noPhantomComboWithoutNativeAcceptance(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample box=oneContinuousBox(),none{};
    XM41ActorSample a=fighter(0x2800,0x2800),b=fighter(0,0,0);
    f.press(0,0x80);
    f.onFrame(box,none,a,b);
    f.press(0,0x81);
    for(int i=0;i<12;++i)f.onFrame(box,none,a,b);
    // An input edge alone is not proof the game accepted a second move.
    assert(f.count==1 && f.history[0].active==13);
    assert(f.history[0].attackStartFrame==1);
}
static void hitstopThenComboHandoff(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM30Sample box=oneContinuousBox(),none{};
    XM41ActorSample a=fighter(0x2800,0x2800),b=fighter(0,0,0);
    f.press(0,0x80);f.onFrame(box,none,a,b);
    a.hitStop=3;
    f.onFrame(box,none,a,b);
    assert(f.frame==1);
    // Player has queued next action while ROM hitstop holds its old box.
    f.press(0,0x81);
    a.hitStop=2;f.onFrame(box,none,a,b);
    a.hitStop=1;f.onFrame(box,none,a,b);
    assert(f.count==1 && f.frame==1);
    a.hitStop=0;a.nativeAction=0x2810;
    f.onFrame(box,none,a,b);
    assert(f.frame==2 && f.count==2);
    assert(f.history[0].active==1);
    assert(f.history[0].endFrame==1);
    assert(f.history[1].firstFrame==2);
}
int main(){
    comboNormalsWithoutBoxGap();
    comboCancelNormalIntoSuper();
    noPhantomComboWithoutNativeAcceptance();
    hitstopThenComboHandoff();
    std::puts("XMVSF 87.244 native combo-frame handoff PASS");
}
