// TEST 87.212, standalone C++11 regression of the original PNG-HUD
// geometry and the previously successful 87.121 Cyclops air-throw release.
// Does NOT need CPS2 ROMs, SDL, F9 sessions, Windows GPU or Stable.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include "xmvsf_stage87_33_dizzy_hud.h"
#include "xmvsf_stage87_121_cyclops_release.h"

static std::uint32_t pixel(const std::vector<unsigned char>& image,int x,int y){
    std::uint32_t p=0;
    std::memcpy(&p,&image[((std::size_t)y*384u+(std::size_t)x)*4u],4);
    return p;
}
static void checkStun(){
    // Synthetic 61x6 source, with a slanted border and distinct pixels.
    // The actual production PNG is loaded in native video (not this test).
    std::vector<unsigned char> art(61u*6u*4u,0);
    for(int row=0;row<6;row++)
        for(int x=row;x<=60-(5-row);x++){
            const std::size_t p=((std::size_t)row*61u+(std::size_t)x)*4u;
            art[p]=(unsigned char)(80+x);
            art[p+1]=(unsigned char)(100+row);
            art[p+2]=90;
            art[p+3]=255;
        }
    XM8733::BarArt base;
    base.rgba=art.data();base.width=61;base.height=6;base.pitch=244;
    assert(base.valid());
    // Recover the exact original example geometry, not PNG alpha width:
    // 44-pixel partial active fill at 44/54; 2px gray tip stays visible.
    for(int row=0;row<6;row++){
        int first=0,last=-1,faceLeft=0,faceRight=0;
        XM8733::SourceSpan(base,row,faceLeft,faceRight);
        assert(faceLeft==row && faceRight==row+53);
        assert(XM8733::FillBounds(base,row,44,54,first,last));
        assert(first==row+10 && last==row+53);
        assert(XM8733::FillBounds(base,row,54,54,first,last));
        assert(first==row && last==row+53);
        assert(XM8733::FillBounds(base,row,30,54,first,last));
        assert(first==row+24); // LIVE tip follows accumulated stun
        assert(!XM8733::FillBounds(base,row,0,54,first,last));
    }

    std::vector<unsigned char> right(384u*224u*4u,0);
    std::vector<unsigned char> left(right.size(),0);
    XM8733::Reading partial;partial.total=54;partial.used=44;
    XM8733::DrawOne(right.data(),384*4,1,partial,false,200,&base);
    XM8733::DrawOne(left.data(),384*4,0,partial,false,200,&base);
    // Both P1 and P2 have EXACT mirrored fill, including moving tip.
    for(int row=0;row<8;row++)
        for(int x=0;x<61;x++){
            const int y=43+row;
            assert(pixel(right,262+x,y)==pixel(left,61+(60-x),y));
        }
    // Warm partial fill does not use rainbow, and remains one-pixel sharp.
    int first=0,stop=0;
    assert(XM8733::FillBounds(base,0,44,54,first,stop));
    assert(pixel(right,262+first+1,43)==XM8733::WarmStun(0));
    assert(pixel(right,262+first-1,43)==0u);

    // Bottom shadow is clipped to the SAME currently growing fill; it
    // neither paints two extra pixels nor the unmoving original 76px meter.
    assert(XM8733::FillBounds(base,5,44,54,first,stop));
    for(int x=0;x<61;x++){
        if(x<first||x>stop+1)
            assert(pixel(right,262+x,49)!=XM8733::PackRGBA(67,68,69));
    }
    // The artist's sample has the single extra lower shadow pixel at
    // x59, not x60; do not recreate the former 2-pixel overrun.
    assert(stop==58);
    assert(pixel(right,262+59,49)==XM8733::PackRGBA(67,68,69));
    assert(pixel(right,262+60,49)!=XM8733::PackRGBA(67,68,69));

    XM8733::Reading full=partial;full.used=54;
    std::fill(right.begin(),right.end(),0);
    XM8733::DrawOne(right.data(),384*4,1,full,false,201,&base);
    assert(XM8733::FillBounds(base,0,54,54,first,stop));
    assert(first==0 && stop==53);
    assert(pixel(right,262+53,43)==XM8733::RainbowPan(201,53,0));
    assert(pixel(right,262+54,43)==0u); // the two-pixel bevel remains clear
    assert(pixel(right,262+first+5,43)==XM8733::RainbowPan(201,first+5,0));
    assert(pixel(right,262+first+5,43)!=XM8733::WarmStun(0));
    assert(XM8733::RainbowPan(201,12,0)!=
           XM8733::RainbowPan(203,12,0)); // rapid diagonal pan

    // Stage 06 draws ONLY the empty base immediately as combat screen
    // begins (before 08 FIGHT), never at 04 Versus/02 character select.
    std::fill(right.begin(),right.end(),0);
    std::vector<unsigned char> ram(65536u,0);
    XM8733::Draw(ram.data(),right.data(),384*4,4,202,6,false,&base);
    assert(pixel(right,262+1,45)!=0u);
    assert(pixel(right,262+10,43)==0u); // no false rainbow during intro
    std::fill(right.begin(),right.end(),0);
    XM8733::Draw(ram.data(),right.data(),384*4,4,203,4,false,&base);
    assert(pixel(right,262+1,45)==0u); // Versus has no stun meter

    // A missing PNG must NOT resurrect the old 76-pixel custom meter.
    std::fill(right.begin(),right.end(),0);
    XM8733::DrawOne(right.data(),384*4,1,full,false,202,nullptr);
    for(unsigned char byte:right)assert(byte==0);
    std::puts("87.212 STUN_UI_PASS: mirrored source, full coverage, live tip, 1px shadow, threshold");
}
static void setW(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    XM87121::sw(r.data(),p,v);
}
static void setD(std::vector<std::uint8_t>& r,unsigned p,std::uint32_t v){
    XM87121::sd(r.data(),p,v);
}
// Fixed 100 distinct native updates from 200C->200E: no animation-word
// gate; the +42 field deliberately holds its value for the full delay.
static void setFixture(std::vector<std::uint8_t>& ram){
    const unsigned p=0x4000u,o=0x4400u;
    XM87121::sb(ram.data(),0x5000u,8u);
    setW(ram,p+0x00u,0x0101u);setW(ram,p+0x40u,0x0021u);
    setW(ram,p+0x10u,400u);setW(ram,p+0x60u,0x0200u);
    setW(ram,p+0xA0u,0x1C00u);setW(ram,p+0xAAu,0x0100u);
    setW(ram,p+0x42u,0x8826u);
    setD(ram,p+0x14u,0x00028000u);setD(ram,p+0x18u,0x00010000u);
    setD(ram,p+0x1Cu,0xFFFA0000u);setD(ram,p+0x20u,0x00006400u);
    setD(ram,p+0x34u,0x000A5F4Au);setD(ram,p+0x38u,0x000A5F5Au);
    setW(ram,p+0x0Cu,233u);
    setW(ram,o+0x10u,460u);setW(ram,o+0x60u,0x1C30u);
}
static void enterThrow(std::vector<std::uint8_t>& ram,XM87121::Tracker& tracker){
    const unsigned p=0x4000u;
    tracker.frame(100u,ram.data()); // saved real preceding airborne motion
    setW(ram,p+0x60u,0x200Cu);
    tracker.frame(101u,ram.data()); // capture saved airborne controller
    setW(ram,p+0x60u,0x200Eu);
    // The original stage 87.121 exact substate requirement was
    // unnecessarily strict. Vary these while keeping the real action pair.
    setW(ram,p+0x06u,3u);setW(ram,p+0x08u,4u);
    setW(ram,p+0xA0u,0x3100u);setW(ram,p+0xAAu,0u);
    setW(ram,p+0x42u,0x7000u);
    tracker.frame(102u,ram.data()); // grab ended; countdown = 0
}
static void checkHundredFrameRelease(bool enabled,bool groundedEarly){
    std::vector<std::uint8_t> ram(65536u,0u);
    setFixture(ram);
    const unsigned p=0x4000u,o=0x4400u;
    XM87121::Tracker tracker;
    tracker.initialized=true;tracker.enabled=enabled;
    enterThrow(ram,tracker);
    assert(tracker.recoveryTicks[0]==0u);
    assert(tracker.waiting[0]==enabled);
    assert(XM87121::w(ram.data(),p+0x60u)==0x200Eu);
    for(unsigned tick=1u;tick<=XM87121::ReleaseDelayFrames;tick++){
        // The victim lands for only two frames, well BEFORE release:
        // 1C34 need not persist exactly to the frame-100 boundary.
        if(groundedEarly && tick==50u){
            setW(ram,o+0x10u,472u);
            setW(ram,o+0x60u,0x1C34u);
        }else if(groundedEarly && tick==52u){
            setW(ram,o+0x60u,0x1C00u); // already left landing pose
        }
        // The test itself may change the opponent's landing animation,
        // but the release controller must not change even a single
        // byte of the victim's RAM record.
        const std::vector<std::uint8_t> opponentBefore(
            ram.begin()+o,ram.begin()+o+0x400u);
        tracker.frame(102u+tick,ram.data());
        assert(std::memcmp(opponentBefore.data(),ram.data()+o,0x400u)==0);
        const bool release=enabled && groundedEarly &&
                           tick==XM87121::ReleaseDelayFrames;
        assert(XM87121::w(ram.data(),p+0x60u)==
               (release?0x0200u:0x200Eu));
        if(enabled){
            assert(tracker.recoveryTicks[0]==tick);
            assert(tracker.animationAdvances[0]==0u); // static sprite
        }else assert(tracker.recoveryTicks[0]==0u);
        // One game frame cannot count twice from a video/runahead callback.
        tracker.frame(102u+tick,ram.data());
        if(enabled)assert(tracker.recoveryTicks[0]==tick);
    }
    if(enabled && groundedEarly){
        assert(XM87121::d(ram.data(),p+0x14u)==0x00028000u);
        assert(XM87121::d(ram.data(),p+0x1Cu)==0xFFFA0000u);
        assert(XM87121::d(ram.data(),p+0x34u)==0x000A5F4Au);
        assert(XM87121::w(ram.data(),p+0xA0u)==0x1C00u);
        // The controller never writes into the victim record.
        // Opponent's landing state changed in the TEST, not the controller.
        assert(XM87121::w(ram.data(),o+0x60u)==0x1C00u);
    }else if(enabled && !groundedEarly){
        assert(tracker.recoveryTicks[0]==XM87121::ReleaseDelayFrames);
        assert(tracker.waiting[0]);
        setW(ram,o+0x10u,472u);
        setW(ram,o+0x60u,0x1C34u);
        tracker.frame(203u,ram.data());
        assert(XM87121::w(ram.data(),p+0x60u)==0x0200u);
        assert(!tracker.waiting[0]);
    }else{
        assert(XM87121::w(ram.data(),p+0x60u)==0x200Eu);
    }
    assert(XM87121::w(ram.data(),p+0x0Cu)==233u); // no teleport
    assert(XM87121::w(ram.data(),p+0x10u)==400u); // no forced new jump
}
static void checkNaturalActionExit(){
    std::vector<std::uint8_t> ram(65536u,0u);
    setFixture(ram);
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    enterThrow(ram,tracker);
    for(unsigned frame=103u;frame<150u;frame++)tracker.frame(frame,ram.data());
    assert(tracker.recoveryTicks[0]==47u);
    setW(ram,0x4060u,0x3400u); // ROM naturally advanced past throw
    tracker.frame(150u,ram.data());
    assert(!tracker.waiting[0]);
    for(unsigned frame=151u;frame<225u;frame++)tracker.frame(frame,ram.data());
    assert(XM87121::w(ram.data(),0x4060u)==0x3400u); // never rewound
}
static void checkCyclops(){
    checkHundredFrameRelease(true,true);
    checkHundredFrameRelease(true,false);
    checkHundredFrameRelease(false,true);
    checkNaturalActionExit();
    std::puts("87.212 CYCLOPS_100_FRAMES_PASS: no early restore, latched victim landing, static pose, natural exit, opt-out");
}
int main(){
    checkStun();
    checkCyclops();
    return 0;
}
