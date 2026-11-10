// XMVSF 87.255 regression: native +85 reload is not a real contact
// when HUD health is still able to decrease. Preserve HP=1 fallback.
// Standalone/ROM-free C++ test, invoked by local BUILD_TEST.sh gate.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample fighter(unsigned action,unsigned hp) {
    XM41ActorSample a{};
    a.valid=true;a.health=hp;
    a.mode=action==0x2E00u?0x2A28u:0x3600u;
    a.nativeAction=action;
    a.nativeStep=action==0x2E00u?4u:0u;
    a.attackPhase=action==0x2E00u?0x80u:0u;
    return a;
}
static XM30Sample hitbox() {
    XM30Sample s{};
    s.melee.active=true;s.melee.sourceBase=0xFF4000u;
    return s;
}
// Exactly 11 health drops over 52 frames, alongside seven independent
// defender hitstop reloads. This originally failed at 87.254 because
// reload edges were being counted as additional red hits.
static void replayAboveFloorDoesNotInventExtraHits() {
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=fighter(0x2E00u,144u);
    auto b=fighter(0x1C00u,130u);
    const XM30Sample hit=hitbox(),none{};
    d.onFrame(hit,none,a,b);
    assert(d.count==1);
    for(int i=0;i<52;++i){
        b.hitStop=(unsigned)(10-i%8);
        if(i%5==0)--b.health;
        d.onFrame(hit,none,a,b);
    }
    assert(d.count==1 && d.history[0].contactCount==11);
    assert(d.hpContactProofs[0]==11);
    assert(d.comboContactProofs[0]==0);
    assert(d.reloadContactProofs[0]==0);
}
// The fallback signal remains intentionally available when Training
// protects HP=1 and the HP-loss detector can no longer fire.
static void reloadAtHealthFloorStillRecordsHits() {
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=fighter(0x2E00u,144u);
    auto b=fighter(0x1C00u,1u);
    const XM30Sample hit=hitbox(),none{};
    d.onFrame(hit,none,a,b);
    assert(d.count==1 && d.history[0].contactCount==0);
    for(int i=0;i<24;++i){
        b.hitStop=(unsigned)(10-i%8);
        d.onFrame(hit,none,a,b);
    }
    assert(d.history[0].contactCount==3);
    assert(d.reloadContactProofs[0]==3);
}
// A delayed combo-counter update while HP is normal cannot silently
// create another hit marker. At pinned HP, it may prove a new impact.
static void comboUpdatesNeedTheHealthFloor() {
    XM30FrameData d{};d.setEnabled(true);d.beginCombat();
    auto a=fighter(0x2E00u,144u);
    auto b=fighter(0x1C00u,120u);
    a.comboKnown=true;b.comboKnown=true;
    const XM30Sample hit=hitbox(),none{};
    d.onFrame(hit,none,a,b);
    b.combo=1;d.onFrame(hit,none,a,b);
    assert(d.history[0].contactCount==0);
    b.health=1;d.onFrame(hit,none,a,b); // real HP decrease
    assert(d.history[0].contactCount==1);
    b.combo=2;d.onFrame(hit,none,a,b); // HP remains pinned
    assert(d.history[0].contactCount==2);
    assert(d.comboContactProofs[0]==1);
}
int main(){
    replayAboveFloorDoesNotInventExtraHits();
    reloadAtHealthFloorStillRecordsHits();
    comboUpdatesNeedTheHealthFloor();
    std::puts("XMVSF 87.255 HP-floor-only hit-proof fallback PASS");
}
