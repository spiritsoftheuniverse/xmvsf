// XMVSF 87.239: deterministic, ROM-free frame-data regressions.
// Compiles as a standalone host C++ unit; no SDL, ROM, or gameplay writes.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample superActor(unsigned phase=0) {
    XM41ActorSample a{};
    a.valid=true;a.mode=0x2A00u;a.nativeAction=0x2E00u;
    a.nativeStep=4u; // verified 2E attack/continuation stage
    a.attackPhase=phase;
    return a;
}
static XM41ActorSample normalActor(unsigned phase=0) {
    XM41ActorSample a{};
    a.valid=true;a.mode=0x2800u;a.nativeAction=0x2800u;
    a.attackPhase=phase;
    return a;
}
static XM30FrameData start() {
    XM30FrameData f;
    f.setEnabled(true);
    f.beginCombat();
    f.press(0,0x80);
    return f;
}
static void superPortraitAndShinryuuken() {
    XM30FrameData f=start();
    XM30Sample none{};
    XM41ActorSample a=superActor();
    f.onFrame(none,none,a,{},30);
    f.onFrame(none,none,a,{},29); // clock is calibrated
    assert(f.frame==2 && f.count==0);
    f.onFrame(none,none,a,{},29); // super portrait: no logical update
    assert(f.frame==2 && f.superFreezeFrames==1);
    a.hitStop=2;
    f.onFrame(none,none,a,{},28); // explicit native freeze despite clock changing
    assert(f.frame==2 && f.superFreezeFrames==2);
    a.hitStop=0;a.attackPhase=0x80;
    XM30Sample hit{};hit.melee.active=true;hit.melee.x=88;hit.melee.y=45;
    f.onFrame(hit,none,a,{},27); // first hitbox AFTER portraits
    assert(f.count==1 && f.history[0].startup==3 && f.history[0].active==1);
    a.attackPhase=0; // phase byte may go blank between 2E super substeps
    for(unsigned c=26;c>=23;--c){
        // Shinryuuken is still in native 2E attack step 04 while box70/72
        // is temporarily absent; no false recovery or first-box-only chart.
        f.onFrame(none,none,a,{},c);
    }
    assert(f.history[0].active==5 && f.history[0].nativeSuperPhase);
    assert(!f.history[0].closed);
    a.attackPhase=0x40;
    f.onFrame(none,none,a,{},22);
    assert(f.history[0].closed && f.history[0].active==5);
    assert(f.history[0].endFrame==7);
    // ROM attack tenure includes actual recovery but never the two frozen ticks.
    assert(f.history[0].attackObserved);
}
static void normalHitboxGapStillCloses() {
    XM30FrameData f=start();
    XM30Sample hit{};hit.melee.active=true;
    XM30Sample none{};
    XM41ActorSample a=normalActor(0x80);
    f.onFrame(hit,none,a,{},30);
    assert(f.count==1 && f.history[0].active==1);
    f.onFrame(none,none,a,{},29);
    assert(f.history[0].closed && f.history[0].active==1);
    // Non-super normal attack never treats the stopped clock as a cinematic.
    const auto tick=f.frame;
    f.onFrame(none,none,a,{},29);
    assert(f.frame==tick+1);
}
static void projectileSuperTracksNativeAttackPhase() {
    XM30FrameData f=start();
    XM30Sample none{};
    XM41ActorSample a=superActor(0x80);
    f.onFrame(none,none,a,{},30);
    XM30Sample shot{};
    shot.projectileCount=1;
    shot.projectileBases[0]=0xFF6000;
    shot.projectileHits[0].active=true;
    shot.projectileHits[0].sourceBase=0xFF6000;
    shot.projectile.active=true;
    shot.projectile.sourceBase=0xFF6000;
    f.onFrame(shot,none,a,{},29);
    assert(f.count==1 && f.history[0].source==1 && f.history[0].active==1);
    shot.projectileHits[0].active=false;
    shot.projectile.active=false;
    f.onFrame(shot,none,a,{},28); // shot object still exists; ROM attack persists
    f.onFrame(none,none,a,{},27); // shot disappears; ROM attack persists
    assert(f.history[0].active==3 && !f.history[0].closed);
    assert(f.history[0].despawnFrame==4);
    a.attackPhase=0x40;
    f.onFrame(none,none,a,{},26);
    assert(f.history[0].closed && f.history[0].active==3);
}
int main() {
    superPortraitAndShinryuuken();
    normalHitboxGapStillCloses();
    projectileSuperTracksNativeAttackPhase();
    std::puts("XMVSF super-freeze and native-super active frame data PASS");
}
