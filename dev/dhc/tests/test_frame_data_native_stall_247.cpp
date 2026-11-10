// TEST 87.247: reconstructed from user-provided 87.245 ROM clock TSV.
// +85 can be zero during 43-44 idle animation samples of BOTH 2C36
// (phase81) and 2C12 (phase80), not only during 2E super portraits.
// Native move acceptance can also be observed without a mapped input edge.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h"

static XM41ActorSample actor(unsigned mode,unsigned action,unsigned stage,
                            unsigned phase,unsigned animPtr,unsigned timer=3u){
    XM41ActorSample a{};
    a.valid=true;a.mode=mode;a.nativeAction=action;a.nativeStep=stage;
    a.attackPhase=phase;a.health=144;a.animPtr=animPtr;a.animTimer=timer;
    return a;
}
static XM30Sample box(){
    XM30Sample s{};
    s.melee.active=true;s.melee.sourceBase=0xFF4000u;
    return s;
}
static void specialCinematicPhase81DoesNotAccumulate44Frames(){
    XM30FrameData f{};
    f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample a=actor(0x2A28u,0x2C36u,2u,0x80u,0xA953Eu);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0B0Au);
    XM30Sample none{};
    f.onFrame(none,none,a,b);
    a.animPtr=0xA954Eu;
    f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==2 && f.playerFrame[1]==2);
    a.attackPhase=0x81u;
    for(int i=0;i<44;++i) {
        f.onFrame(none,none,a,b);
        assert(f.playerFrame[0]==2 && f.playerFrame[1]==2);
    }
    assert(f.superFreezeFrames==44);
    a.nativeStep=4u;a.attackPhase=0x80u;a.animPtr=0xA9742u;
    f.onFrame(box(),none,a,b);
    assert(f.playerFrame[0]==3);
    assert(f.count==1 && f.history[0].attackFamily==0x2Cu);
    assert(f.history[0].startup==3); // no 44F fake startup
    assert(f.history[0].attackStartFrame==1);
    assert(f.history[0].attackLastFrame==3);
}
static void phase80SpecialStillRecognizedAs44FrameHold(){
    XM30FrameData f{};
    f.setEnabled(true);f.beginCombat();f.press(0,0x80);
    XM41ActorSample a=actor(0x2A28u,0x2C12u,2u,0x80u,0xA9236u,7u);
    XM41ActorSample b=actor(0x3636u,0x1C02u,0u,0u,0xD0FC4u,3u);
    XM30Sample none{};
    f.onFrame(none,none,a,b);
    for(int i=0;i<44;++i)f.onFrame(none,none,a,b);
    assert(f.playerFrame[0]==1 && f.playerFrame[1]==1);
    assert(f.superFreezeFrames==44);
    a.nativeStep=4u;a.animPtr=0xA9442u;a.animTimer=1u;
    f.onFrame(box(),none,a,b);
    assert(f.count==1 && f.history[0].startup==2);
    assert(f.history[0].attackLastFrame==2);
}
static void oneFighterAnimatingDoesNotPauseBoth(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2A28u,0x2C12u,2u,0x80u,0xA9236u,7u);
    XM41ActorSample b=actor(0x3636u,0x1C02u,0u,0u,0xD0FC4u,3u);
    XM30Sample none{};
    f.onFrame(none,none,a,b);
    for(int i=0;i<8;++i){
        ++b.animTimer; // actual native movement during 2C setup
        f.onFrame(none,none,a,b);
    }
    assert(f.playerFrame[0]==9 && f.playerFrame[1]==9);
    assert(f.superFreezeFrames==0);
}
static void captureSecondNormalWithoutNewButtonEdge(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2814u,0x1000u,0u,0x80u,0xA677Au);
    XM41ActorSample b=actor(0x3636u,0x1C00u,0u,0u,0xD0B0Au);
    XM30Sample hit=box(),none{};
    // A native attack is valid evidence even if the SDL edge was missed.
    f.onFrame(hit,none,a,b);
    assert(f.count==1 && f.eventsCreated==1 && !f.history[0].inputObserved);
    assert(f.history[0].attackFamily==0x28u);
    // Same +60 action word, but different +A0 native controller:
    // the second normal IS a distinct combo attack.
    a.mode=0x2828u;
    f.onFrame(hit,none,a,b);
    assert(f.count==2 && f.eventsCreated==2);
    assert(f.history[0].closed && f.history[0].attackEnded);
    assert(f.history[0].attackLastFrame==1);
    assert(f.history[1].attackMode==0x2828u);
    assert(!f.history[1].inputObserved);
    f.onFrame(hit,none,a,b);
    assert(f.count==2 && f.history[1].active==2);
}
static void captureDirectSpecialCancelWithoutButtonEdge(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2A28u,0x2C36u,4u,0x80u,0xA9742u);
    XM41ActorSample b=actor(0x3600u,0x1C00u,0u,0u,0xD0B0Au);
    XM30Sample hit=box(),none{};
    f.onFrame(hit,none,a,b);
    assert(f.count==1 && f.history[0].attackFamily==0x2Cu);
    // A distinct special begins at 2C stage02 after the previous
    // special reached stage04, without returning to neutral.
    a.nativeAction=0x2C12u;a.nativeStep=2u;a.animPtr=0xA9206u;
    f.onFrame(hit,none,a,b);
    assert(f.count==2 && f.eventsCreated==2);
    assert(f.history[0].attackEnded && f.history[0].attackEndFrame==1);
    assert(f.history[1].attackStartFrame==2);
    assert(f.history[1].attackAction==0x2C12u);
    // Internal special stages do NOT represent separate new moves.
    a.nativeAction=0x2C2Cu;a.nativeStep=4u;
    f.onFrame(hit,none,a,b);
    assert(f.count==2 && f.eventsCreated==2);
}
static void firstAcceptedNativeImpactDuringHitstopWithoutButtonEdge(){
    XM30FrameData f{};f.setEnabled(true);f.beginCombat();
    XM41ActorSample a=actor(0x2814u,0x1000u,0u,0x80u,0xA677Au);
    XM41ActorSample b=actor(0x3636u,0x1C00u,0u,0u,0xD0B0Au);
    XM30Sample hit=box(),none{};
    // ROM has already assigned +85 on the first sample. A lost mapped
    // edge must not erase this genuinely accepted normal.
    a.hitStop=10;b.hitStop=10;
    f.onFrame(hit,none,a,b);
    assert(f.playerFrame[0]==1 && f.playerFrame[1]==0);
    assert(f.count==1 && f.eventsCreated==1);
    assert(!f.history[0].inputObserved && f.history[0].active==1);
    for(unsigned k=9;k>0;--k){
        a.hitStop=k;b.hitStop=k;
        f.onFrame(hit,none,a,b);
        assert(f.playerFrame[0]==1 && f.eventsCreated==1);
    }
    a.hitStop=0;b.hitStop=0;f.onFrame(hit,none,a,b);
    assert(f.playerFrame[0]==2 && f.history[0].active==2);
}
int main(){
    firstAcceptedNativeImpactDuringHitstopWithoutButtonEdge();
    specialCinematicPhase81DoesNotAccumulate44Frames();
    phase80SpecialStillRecognizedAs44FrameHold();
    oneFighterAnimatingDoesNotPauseBoth();
    captureSecondNormalWithoutNewButtonEdge();
    captureDirectSpecialCancelWithoutButtonEdge();
    std::puts("XMVSF 87.247 native animation stalls and combo acquisition PASS");
}
