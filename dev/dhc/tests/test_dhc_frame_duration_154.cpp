// XMVSF 87.154: measured attack action durations are independent of held
// directional input, airborne/landing states, and projectile lifetimes.
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"
#include <cassert>
#include <cstdio>

static XM41ActorSample actor(unsigned action,unsigned mode) {
    XM41ActorSample a;
    a.valid=true;
    a.nativeAction=action;
    a.mode=mode;
    a.action=(mode>>8u)&255u;
    return a;
}
static XM30Sample melee(bool active){
    XM30Sample s;
    s.melee.active=active;
    s.melee.x=100;s.melee.y=100;
    return s;
}
static void testWalkJumpCannotExtendAttack(){
    XM30FrameData fd;fd.setEnabled(true);fd.beginCombat();
    fd.press(0,0x80);
    fd.onFrame(melee(true),melee(false),actor(0x1000u,0x2814u));
    assert(fd.count==1);
    assert(fd.history[0].attackObserved);
    assert(fd.history[0].attackStartFrame==1u);
    fd.onFrame(melee(true),melee(false),actor(0x1000u,0x2828u));
    // The next ROM action is directional walking, NOT the idle pose.
    fd.direction(0,false,false,false,true);
    fd.onFrame(melee(false),melee(false),actor(0x0006u,0x0002u));
    assert(fd.history[0].attackEnded);
    assert(fd.history[0].attackEndFrame==2u);
    const unsigned long long locked=fd.history[0].attackEndFrame;
    // Even an indefinitely held direction or jumping for 200f cannot extend it.
    for(int i=0;i<200;++i){
        fd.direction(0,i%3==0,false,false,true);
        fd.onFrame(melee(false),melee(false),actor(0x0600u,0x0A02u));
    }
    assert(fd.history[0].attackEndFrame==locked);
    assert(fd.history[0].attackEndFrame-fd.history[0].attackStartFrame+1u==2u);
}
static void testNormalFollowupIsDistinctAction(){
    XM30FrameData fd;fd.setEnabled(true);fd.beginCombat();
    fd.press(0,0x80);
    fd.onFrame(melee(true),melee(false),actor(0x1000u,0x2814u));
    fd.onFrame(melee(true),melee(false),actor(0x1000u,0x2828u));
    // New attack input does not end the preceding attack on its own.
    fd.press(0,0x81);
    fd.onFrame(melee(true),melee(false),actor(0x1000u,0x2828u));
    assert(!fd.history[0].attackEnded);
    // ROM switches 1000->1002 within 28xx, proving a new normal attack.
    fd.onFrame(melee(false),melee(false),actor(0x1002u,0x2828u));
    assert(fd.history[0].attackEnded && fd.history[0].attackEndFrame==3u);
}
static void testMultistageSuperStaysOneAttack(){
    XM30FrameData fd;fd.setEnabled(true);fd.beginCombat();
    fd.press(0,0x80);
    fd.onFrame(melee(true),melee(false),actor(0x2E08u,0x2A02u));
    assert(fd.count==1);
    for(int i=0;i<14;++i){
        // Native Shinkuu Hadoken 2E08->2E0A, still the same super.
        fd.onFrame(melee(false),melee(false),
                   actor(i<5?0x2E08u:0x2E0Au,0x2A02u));
        assert(!fd.history[0].attackEnded);
    }
    fd.onFrame(melee(false),melee(false),actor(0x0000u,0x002Au));
    assert(fd.history[0].attackEnded);
    assert(fd.history[0].attackEndFrame==15u);
    for(int i=0;i<20;++i)
        fd.onFrame(melee(false),melee(false),actor(0x0600u,0x0A02u));
    assert(fd.history[0].attackEndFrame==15u);
}
static void testNoAttackRemainsUnknown(){
    XM30FrameData fd;fd.setEnabled(true);fd.beginCombat();
    fd.press(0,0x80);
    for(int i=0;i<XM30FrameData::MAX_WAIT+2;++i)
        fd.onFrame(melee(false),melee(false),actor(0x0600u,0x0A02u));
    assert(fd.count==1 && fd.history[0].noBox);
    assert(!fd.history[0].attackObserved);
}
int main(){
    testWalkJumpCannotExtendAttack();
    testNormalFollowupIsDistinctAction();
    testMultistageSuperStaysOneAttack();
    testNoAttackRemainsUnknown();
    std::puts("XMVSF 87.154 native attack-duration tests passed");
    return 0;
}
