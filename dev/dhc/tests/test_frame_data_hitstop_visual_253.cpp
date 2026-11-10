// XMVSF 87.253 — independent visual hitstop ticks and HP=1 contacts.
// No ROM writes. Source-only standalone host C++ regression for local builds.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample actor(unsigned mode,unsigned act,unsigned step,
                             unsigned phase,unsigned hp=144u,
                             unsigned combo=0u){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=act;a.nativeStep=step;
    a.attackPhase=phase;a.health=hp;a.comboKnown=true;a.combo=combo;
    return a;
}
static XM30Sample melee(){
    XM30Sample s{};s.melee.active=true;
    s.melee.sourceBase=0xFF4000u;return s;
}
static void qcdIsGenericAndDistinctFromQcf(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    d.direction(0,false,false,false,true); // 6 forward
    d.direction(0,false,true,false,true);  // 3 down-forward
    d.direction(0,false,true,false,false); // 2 down
    d.press(0,0x80);
    assert(d.track[0].motion==XM32_QCD);
    assert(xm32MotionTag(d.track[0].motion)[0]=='Q');
    assert(xm32MotionTag(d.track[0].motion)[1]=='D');
    d.directions[0].count=0;
    d.direction(1,false,true,false,false); // 2 down
    d.direction(1,false,true,false,true);  // 3 down-forward
    d.direction(1,false,false,false,true); // 6 forward
    d.press(1,0x81);
    assert(d.track[1].motion==XM32_QCF);
}
static void redContactsContinueAtMinimumHpAndDuringHitstop(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,4,0x80);
    auto b=actor(0x3600,0x1C00,0,0,1u,0u);
    XM30Sample hit=melee(),none{};
    d.press(0,0x80);d.onFrame(hit,none,a,b);
    assert(d.count==1 && d.playerFrame[0]==1);
    b.combo=2;d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==1 && d.comboContactProofs[0]==1);
    const auto firstWall=d.history[0].contactWallFrames[0];
    a.hitStop=4;b.hitStop=4;b.combo=3;
    d.onFrame(hit,none,a,b);
    assert(d.playerFrame[0]==2 && d.wallFrame[0]==3);
    assert(d.wallTick(0,3).stopped && d.wallTick(1,3).stopped);
    assert(d.history[0].contactCount==2);
    assert(d.history[0].contactFrames[1]==2);
    assert(d.history[0].contactWallFrames[1]>firstWall);
    a.hitStop=3;b.hitStop=3;b.combo=4;
    d.onFrame(hit,none,a,b);
    assert(d.playerFrame[0]==2 && d.wallFrame[0]==4);
    assert(d.history[0].contactCount==3);
    assert(d.history[0].contactFrames[2]==2);
    assert(d.history[0].contactWallFrames[2]==4);
    assert(d.comboContactProofs[0]==3);
    // The chart's two lanes use the SAME held wall samples, yet startup,
    // active and native attack duration are not incremented here.
    const int active=d.history[0].active;
    assert(active==2);
    a.hitStop=2;b.hitStop=2;
    d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==3); // no new combo/stop edge
    assert(d.wallTick(0,5).stopped);
    assert(d.history[0].active==active);
}
static void defenderStopReloadIsIndependentBackup(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,4,0x80);
    auto b=actor(0x3600,0x1C00,0,0,1u,0u);
    XM30Sample hit=melee(),none{};
    d.press(0,0x80);d.onFrame(hit,none,a,b);
    b.hitStop=4;d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==1);
    assert(d.reloadContactProofs[0]==1);
    b.hitStop=3;d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==1);
    b.hitStop=0;d.onFrame(hit,none,a,b);
    b.hitStop=4;d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==2);
    assert(d.reloadContactProofs[0]==2);
}
static void cinematicHitstopWithoutAttackDoesNotProduceRedTicks(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,2,0x80);
    auto b=actor(0x3600,0x1C00,0,0,1u,0u);
    XM30Sample none{};
    d.press(0,0x80);d.onFrame(none,none,a,b);
    a.hitStop=6;b.hitStop=6;
    d.onFrame(none,none,a,b);
    assert(d.count==0 && d.reloadContactProofs[0]==0);
    assert(d.wallTick(0,2).stopped);
    assert(d.playerFrame[0]==1 && d.wallFrame[0]==2);
}
int main(){
    qcdIsGenericAndDistinctFromQcf();
    redContactsContinueAtMinimumHpAndDuringHitstop();
    defenderStopReloadIsIndependentBackup();
    cinematicHitstopWithoutAttackDoesNotProduceRedTicks();
    std::puts("XMVSF 87.253 QCD, minimum HP contacts, moving gray hitstop PASS");
}
