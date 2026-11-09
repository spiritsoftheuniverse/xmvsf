// XMVSF TEST 87.245: real 87.243 ROM trace patterns, no guessed global pause.
// A native 2E portrait freeze holds BOTH fighters while +85 remains zero;
// after contact, 2E stage 04 progresses although the VICTIM's +85 reloads.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample fighter(unsigned mode,unsigned action,unsigned stage,
                               unsigned phase,unsigned health){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;a.nativeStep=stage;
    a.attackPhase=phase;a.health=health;
    return a;
}
static XM30Sample projectile(){
    XM30Sample s{};
    s.projectileCount=1;s.projectileBases[0]=0xFF6000u;
    s.projectileHits[0].active=true;
    s.projectileHits[0].sourceBase=0xFF6000u;
    s.projectile.active=true;s.projectile.sourceBase=0xFF6000u;
    return s;
}
static void traceBasedSuperCinematicAndDefenderStop(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    f.press(0,0x80);
    XM41ActorSample a=fighter(0x2A28,0x2E00,2,0x80,144);
    XM41ActorSample b=fighter(0x3636,0x1C04,0,0,130);
    XM30Sample none{},shot=projectile();
    f.onFrame(none,none,a,b); // host 87, step 02 native startup
    assert(f.frame==1 && f.playerFrame[0]==1 && f.playerFrame[1]==1);
    a.attackPhase=0x81;
    for(int i=0;i<45;++i)f.onFrame(none,none,a,b);
    // Host 88..130 in capture: phase 81, +85=0 on BOTH players,
    // unchanged native animation timers/pointers (portrait/streak pause).
    assert(f.frame==1 && f.count==0);
    assert(f.playerFrame[0]==1 && f.playerFrame[1]==1);
    assert(f.superFreezeFrames==45);
    a.attackPhase=0x80;f.onFrame(none,none,a,b);
    assert(f.frame==2 && f.playerFrame[0]==2 && f.playerFrame[1]==2);

    a.nativeStep=4;
    f.onFrame(shot,none,a,b); // verified offensive 2E stage 04
    assert(f.count==1 && f.history[0].source==1);
    assert(f.history[0].startup==3); // never includes 45 portrait samples
    assert(f.history[0].active==1);
    const unsigned long long startP1=f.playerFrame[0];
    const unsigned long long startP2=f.playerFrame[1];
    for(int i=0;i<52;++i){
        b.hitStop=(unsigned)(10-i%8);
        // Defender takes ten confirmed strikes during stage 04.
        if(i%5==0)b.health-=1;
        f.onFrame(shot,none,a,b);
        assert(f.playerFrame[0]==startP1+(unsigned)(i+1));
        assert(f.playerFrame[1]==startP2);
        assert(f.history[0].attackLastFrame==f.playerFrame[0]);
    }
    assert(f.frame==startP1+52);
    assert(f.history[0].active==53);
    assert(f.history[0].contactCount==11); // 11 actual health drops
    assert(f.superFreezeFrames==45);
    const unsigned long long attackLast=f.history[0].attackLastFrame;
    a.nativeStep=6;a.nativeAction=0x2E02;a.attackPhase=0x40;
    f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==attackLast+1);
    assert(f.history[0].closed && f.history[0].active==53);
    assert(f.history[0].attackLastFrame==attackLast+1);
    assert(f.playerFrame[1]==startP2);
    b.hitStop=0;
    f.onFrame(none,none,a,b);
    assert(f.playerFrame[1]==startP2+1);
}
static void bothFightersNativeHitstop(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    f.press(0,0x80);
    XM41ActorSample a=fighter(0x2828,0x1000,0,0x80,144);
    XM41ActorSample b=fighter(0x3636,0x1C04,0,0,144);
    XM30Sample hit{},none{};
    hit.melee.active=true;hit.melee.sourceBase=0xFF4000u;
    f.onFrame(hit,none,a,b);
    assert(f.frame==1 && f.count==1);
    a.hitStop=10;b.hitStop=10;b.health=140;
    for(unsigned k=10;k>0;--k){
        a.hitStop=k;b.hitStop=k;
        f.onFrame(hit,none,a,b);
        assert(f.frame==1 && f.playerFrame[0]==1 && f.playerFrame[1]==1);
    }
    assert(f.superFreezeFrames==10);
    assert(f.history[0].contactCount==1);
    a.hitStop=0;b.hitStop=0;
    f.onFrame(hit,none,a,b);
    assert(f.frame==2 && f.history[0].active==2);
}
int main(){
    traceBasedSuperCinematicAndDefenderStop();
    bothFightersNativeHitstop();
    std::puts("XMVSF 87.245 independent native-clock and super cinematic PASS");
}
