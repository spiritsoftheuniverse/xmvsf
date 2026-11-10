// TEST 87.246: supplied 87.245 native trace, one 2E00 super remains
// ONE frame-data record while new input edges and projectile boxes recur.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample makeActor(unsigned mode,unsigned action,
                                 unsigned stage,unsigned phase,unsigned hp){
    XM41ActorSample a{};
    a.valid=true;
    a.mode=mode;a.nativeAction=action;a.nativeStep=stage;
    a.attackPhase=phase;a.health=hp;
    return a;
}
static XM30Sample projectile(unsigned base){
    XM30Sample s{};
    if(base){
        s.projectileCount=1;
        s.projectileBases[0]=base;
        s.projectileHits[0].active=true;
        s.projectileHits[0].sourceBase=base;
        s.projectile.active=true;
        s.projectile.sourceBase=base;
    }
    return s;
}
static void repeatedProjectileInsideUnchangedSuperMustNotCreateCard(){
    XM30FrameData f{};
    f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=makeActor(0x2A28u,0x2E00u,4u,0x80u,144u);
    XM41ActorSample b=makeActor(0x3636u,0x1C04u,2u,0u,140u);
    XM30Sample blank{};
    f.press(0,0x80);
    f.onFrame(projectile(0xFF6000u),blank,a,b);
    assert(f.count==1 && f.eventsCreated==1);
    assert(f.history[0].attackFamily==0x2Eu);
    assert(f.history[0].source==1);
    assert(f.history[0].attackStartFrame==1);
    // In the supplied native trace, a new button is pressed while
    // 2E00 is still the active controller. No native 2E restart occurs.
    f.press(0,0x81);
    for(int i=0;i<70;++i){
        // Renewed projectile base and intermittent collision: each is
        // a new HIT from the same super, not a new attack.
        const unsigned base=(i%5==2)?0u:
            ((i/6)%2==0?0xFF6000u:0xFF6200u);
        if(i%5==0)b.health-=1;
        f.onFrame(projectile(base),blank,a,b);
        if(i==16 || i==39) f.press(0,0x82);
        assert(f.count==1 && f.eventsCreated==1);
        assert(f.history[0].attackStartFrame==1);
    }
    assert(f.history[0].contactCount==14);
    assert(f.history[0].attackLastFrame==71);
    // Long continuing super passes the MAX_WAIT limit of stray input.
    // It must not produce a bogus "NO BOX" row on timeout.
    for(int i=0;i<150;++i){
        f.onFrame(projectile(i%2?0xFF6200u:0xFF6000u),
                  blank,a,b);
        assert(f.count==1 && f.eventsCreated==1);
    }
    assert(f.history[0].attackLastFrame==221);
    assert(f.history[0].contactCount==14);
}
static void realNewNativeAttackAfterPreviousExits(){
    XM30FrameData f{};
    f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=makeActor(0x2A28u,0x2E00u,4u,0x80u,144u);
    XM41ActorSample b=makeActor(0,0,0,0,144);
    XM30Sample blank{};
    f.press(0,0x80);
    f.onFrame(projectile(0xFF6000u),blank,a,b);
    assert(f.count==1 && f.eventsCreated==1);
    f.press(0,0x81);
    a.mode=0;a.nativeAction=0;a.attackPhase=0;
    for(int i=0;i<4;++i)f.onFrame(blank,blank,a,b);
    assert(f.history[0].attackEnded);
    // Same 2E controller value is valid again because the native
    // owner first exited (pendingOriginLeft), not merely a new box edge.
    a=makeActor(0x2A28u,0x2E00u,4u,0x80u,144u);
    f.onFrame(projectile(0xFF6200u),blank,a,b);
    assert(f.count==2 && f.eventsCreated==2);
    assert(f.history[1].attackFamily==0x2Eu);
    assert(f.history[1].firstFrame>f.history[0].firstFrame);
}
int main(){
    repeatedProjectileInsideUnchangedSuperMustNotCreateCard();
    realNewNativeAttackAfterPreviousExits();
    std::puts("XMVSF 87.246 continuing-super identity PASS");
}
