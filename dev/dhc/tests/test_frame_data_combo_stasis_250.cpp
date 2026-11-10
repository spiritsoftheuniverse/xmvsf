// TEST 87.250: isolated read-only ROM observer regression.
// No new gameplay capture was available. Exercise identified source-level
// omissions without assuming new CPS2 timing facts.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample actor(unsigned mode,unsigned action,
                            unsigned stage,unsigned phase,unsigned ptr) {
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;
    a.nativeStep=stage;a.attackPhase=phase;
    a.animPtr=ptr;a.animTimer=3u;a.health=144u;
    return a;
}
static XM30Sample melee() {
    XM30Sample s{};s.melee.active=true;s.melee.sourceBase=0xFF4000u;
    return s;
}
static XM30Sample projectileObject() {
    XM30Sample s{};s.projectileCount=1;
    s.projectileBases[0]=0xFF6000u;
    s.projectileX[0]=123;s.projectileY[0]=78;
    return s;
}
// The old 87.247 gate counted these otherwise identical hold samples
// simply because the 2C action had already passed stage02.
static void laterSpecialHoldDoesNotAdd44Frames() {
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2A28u,0x2C36u,4u,0x80u,0xA9700u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0000u);
    XM30Sample none{};
    f.press(0,0x80);
    f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==1 && f.playerFrame[1]==1);
    for(int i=0;i<44;++i) {
        f.onFrame(none,none,a,b);
        assert(f.playerFrame[0]==1 && f.playerFrame[1]==1);
    }
    assert(f.superFreezeFrames==44);
    a.animTimer=2u;f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==2 && f.playerFrame[1]==2);
}
static void liveProjectileDoesNotStopClockOnStaticFighterPose() {
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2A28u,0x2C36u,4u,0x80u,0xA9700u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0000u);
    XM30Sample shot=projectileObject(),none{};
    f.onFrame(shot,none,a,b);
    for(int i=0;i<14;++i)f.onFrame(shot,none,a,b);
    assert(f.playerFrame[0]==15 && f.playerFrame[1]==15);
    assert(f.superFreezeFrames==0);
}
static void confirmedComboImpactWithMissingBoxIsStillCaptured() {
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2800u,0x2800u,0u,0x80u,0xA6000u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0000u);
    XM30Sample box=melee(),none{};
    f.press(0,0x80);f.onFrame(box,none,a,b);
    assert(f.count==1 && f.history[0].active==1);
    f.press(0,0x81);
    a.mode=0x2828u;a.nativeAction=0x2810u;
    a.hitStop=8;b.hitStop=8;b.health=140;
    // The new ROM-owned attack landed, but its one-frame collision box
    // vanished before the read-only hitbox observer sampled RAM.
    f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==2);
    assert(f.count==2 && f.eventsCreated==2);
    assert(f.history[1].attackFamily==0x28u);
    assert(f.history[1].contactOnly && f.history[1].noBox);
    assert(f.history[1].active==0); // no invented active hitbox frame
    assert(f.history[1].contactCount==1);
    assert(f.history[1].contactFrames[0]==2);
    assert(f.history[1].attackStartFrame==2);
    assert(!f.floats[1].visible); // no fabricated startup popup
    for(unsigned k=7;k>0;--k) {
        a.hitStop=k;b.hitStop=k;f.onFrame(none,none,a,b);
        assert(f.count==2 && f.history[1].contactCount==1);
        assert(f.playerFrame[0]==2);
    }
}
static void unchangedSuperStillCannotStealRepeatedHpContacts() {
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2A28u,0x2E00u,4u,0x80u,0xA9000u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0000u);
    XM30Sample box=melee(),none{};
    f.press(0,0x80);f.onFrame(box,none,a,b);
    f.press(0,0x81);
    b.health=140;f.onFrame(none,none,a,b);
    assert(f.count==1 && f.eventsCreated==1);
}
static void staleMeleeBoxStopsOnNativeActionExit() {
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2800u,0x2800u,0u,0x80u,0xA6000u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0000u);
    XM30Sample box=melee(),none{};
    f.press(0,0x80);f.onFrame(box,none,a,b);
    assert(f.count==1 && f.history[0].active==1);
    a.mode=0x3600u;a.nativeAction=0x1C00u;a.attackPhase=0;
    for(int i=0;i<40;++i)f.onFrame(box,none,a,b);
    assert(f.count==1 && f.eventsCreated==1);
    assert(f.history[0].attackEnded);
    assert(f.history[0].attackLastFrame==1);
    assert(f.history[0].endFrame==1);
    assert(f.history[0].closed);
    assert(f.history[0].active==1); // do not tack on 40F of stale RAM box
    assert(f.history[0].hitboxLastFrame==1);
}
int main() {
    laterSpecialHoldDoesNotAdd44Frames();
    liveProjectileDoesNotStopClockOnStaticFighterPose();
    confirmedComboImpactWithMissingBoxIsStillCaptured();
    unchangedSuperStillCannotStealRepeatedHpContacts();
    staleMeleeBoxStopsOnNativeActionExit();
    std::puts("XMVSF 87.250 combo contact fallback / native stasis PASS");
}
