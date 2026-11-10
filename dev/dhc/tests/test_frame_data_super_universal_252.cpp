// XMVSF 87.252: generic native 2E super / linked projectile ownership.
// ROM-free source regressions. Windows BUILD_TEST.bat executes these locally.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample actor(unsigned mode,unsigned act,unsigned step,
                             unsigned phase,unsigned hp=144u){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=act;a.nativeStep=step;
    a.attackPhase=phase;a.health=hp;return a;
}
static XM30Sample shot(unsigned base,bool box=false){
    XM30Sample s{};s.projectileCount=1;s.projectileBases[0]=base;
    s.projectileX[0]=80;s.projectileY[0]=60;
    if(box){
        s.projectileHits[0].active=true;s.projectileHits[0].sourceBase=base;
        s.projectile.active=true;s.projectile.sourceBase=base;
    }
    return s;
}
static void aimedBeamObjectThenBox(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,2,0x80),b=actor(0x3600,0x1C00,0,0);
    XM30Sample none{};
    d.press(0,0x80);d.onFrame(none,none,a,b);
    assert(d.count==0);
    a.nativeStep=4;d.onFrame(shot(0xFF6000),none,a,b);
    assert(d.count==1 && d.eventsCreated==1);
    assert(d.history[0].noBox && !d.history[0].contactOnly);
    assert(d.history[0].active==0 && d.history[0].hitboxLastFrame==0);
    b.health=139;d.onFrame(shot(0xFF6000),none,a,b);
    assert(d.history[0].contactOnly && d.history[0].contactCount==1);
    d.onFrame(shot(0xFF6000,true),none,a,b);
    assert(!d.history[0].noBox && !d.history[0].contactOnly);
    assert(d.history[0].active==1 && d.history[0].contactCount==1);
    assert(d.history[0].firstFrame==d.playerFrame[0]);
    d.press(0,0x81);
    for(int i=0;i<8;++i){
        if(i==2)b.health-=2;
        d.onFrame(shot(0xFF6000,true),none,a,b);
    }
    assert(d.count==1 && d.history[0].contactCount==2);
}
static void ryuStyleProjectileHitsAfterFighterExit(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,2,0x80),b=actor(0x3600,0x1C00,0,0);
    XM30Sample none{};
    d.press(0,0x80);d.onFrame(none,none,a,b);
    a.nativeStep=4;d.onFrame(shot(0xFF6000,true),none,a,b);
    assert(d.count==1 && d.history[0].source==1);
    d.onFrame(shot(0xFF6400),none,a,b);
    assert(d.history[0].ownedProjectileCount>=1);
    a=actor(0x3600,0x1C00,0,0);
    d.onFrame(shot(0xFF6400),none,a,b);
    assert(d.history[0].attackEnded && !d.history[0].closed);
    b.health=137;d.onFrame(shot(0xFF6400),none,a,b);
    assert(d.history[0].contactCount==1);
    assert(d.history[0].contactFrames[0]>d.history[0].attackEndFrame);
    for(int i=0;i<4;++i)d.onFrame(none,none,a,b);
    assert(d.history[0].despawnFrame>0 && d.history[0].closed);
    assert(d.count==1);
}
static void kenStyleMeleeSuperRedContactTicks(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,2,0x80),b=actor(0x3600,0x1C00,0,0);
    XM30Sample none{},hit{};hit.melee.active=true;
    d.press(0,0x80);d.onFrame(none,none,a,b);
    a.nativeStep=4;d.onFrame(hit,none,a,b);
    assert(d.count==1 && d.history[0].source==0);
    b.health=140;d.onFrame(none,none,a,b);
    assert(d.history[0].contactCount==1);
    d.press(0,0x81);
    a.hitStop=3;b.hitStop=3;b.health=137;
    d.onFrame(none,none,a,b);
    a.hitStop=0;b.hitStop=0;b.health=133;
    d.onFrame(none,none,a,b);
    assert(d.count==1 && d.history[0].contactCount>=2);
}
static void oldProjectileNotAttributedToNewSuper(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0,0,0,0),b=actor(0x3600,0x1C00,0,0);
    XM30Sample none{},old=shot(0xFF6200);
    d.onFrame(old,none,a,b);
    d.press(0,0x80);a=actor(0x2A28,0x2E00,2,0x80);
    d.onFrame(old,none,a,b);
    b.health=132;d.onFrame(old,none,a,b);
    assert(d.count==0 && d.deferredSuperContactCount[0]==0);
    a.nativeStep=4;d.onFrame(shot(0xFF6600),none,a,b);
    assert(d.count==1);
    assert(d.history[0].projectileBase==0xFF6600);
    assert(d.history[0].contactCount==0);
}
static void preboxFrozenHitDeferredToSuper(){
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=actor(0x2A28,0x2E00,2,0x80),b=actor(0x3600,0x1C00,0,0);
    XM30Sample none{};
    d.press(0,0x80);d.onFrame(none,none,a,b);
    a.hitStop=4;b.hitStop=4;d.onFrame(none,none,a,b);
    a.hitStop=3;b.hitStop=3;b.health=140;
    d.onFrame(none,none,a,b);
    assert(d.count==0 && d.deferredSuperContactCount[0]==1);
    a.hitStop=0;b.hitStop=0;a.nativeStep=4;
    d.onFrame(shot(0xFF6800),none,a,b);
    assert(d.count==1 && d.history[0].contactCount==1);
    assert(d.history[0].contactOnly && d.history[0].active==0);
}
int main(){
    aimedBeamObjectThenBox();
    ryuStyleProjectileHitsAfterFighterExit();
    kenStyleMeleeSuperRedContactTicks();
    oldProjectileNotAttributedToNewSuper();
    preboxFrozenHitDeferredToSuper();
    std::puts("XMVSF 87.252 universal super ownership and red contact ticks PASS");
}
