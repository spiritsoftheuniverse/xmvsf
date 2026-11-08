// TEST 87.219, standalone C++11 regression of the original PNG-HUD
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
#include "xmvsf_stage30_frame_data.h"
#include "xmvsf_stage87_217_native_float.h"

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
    std::puts("87.219 STUN_UI_PASS: mirrored source, full coverage, live tip, 1px shadow, threshold");
}
static void setW(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    XM87121::sw(r.data(),p,v);
}
static void setD(std::vector<std::uint8_t>& r,unsigned p,std::uint32_t v){
    XM87121::sd(r.data(),p,v);
}
static void setFixture(std::vector<std::uint8_t>& r){
    XM87121::sb(r.data(),0x5000u,8u);
    setW(r,0x4000u,0x0101u);setW(r,0x4040u,0x0021u);
    setW(r,0x4010u,400u);setW(r,0x4060u,0x0200u);
    setW(r,0x40A0u,0x1C00u);setW(r,0x40AAu,0x0100u);
    // Pre-grab forward X velocity, separate +18 horizontal motion state,
    // and rising Y velocity must be independently preserved/tested.
    setD(r,0x4014u,0x00028000u);setD(r,0x4018u,0x0000A800u);
    setD(r,0x401Cu,0xFFFA0000u);setD(r,0x4020u,0x00006400u);
    setD(r,0x4034u,0x000A5F4Au);
    setW(r,0x400Cu,233u);
    setW(r,0x4410u,472u);setW(r,0x4460u,0x1C34u);
}
static void enter(std::vector<std::uint8_t>& r,XM87121::Tracker& t,
                  unsigned base){
    setW(r,0x4060u,0x0200u);
    setW(r,0x40A0u,0x1C00u);
    t.frame(base,r.data());
    setW(r,0x4060u,0x200Cu);
    t.frame(base+1u,r.data());
    setW(r,0x4060u,0x200Eu);
    setW(r,0x40A0u,0x3200u);
    t.frame(base+2u,r.data());
    assert(t.recoveryTicks[0]==0u);
}
static void testRelease(bool enabled){
    std::vector<std::uint8_t> r(65536u,0u);setFixture(r);
    XM87121::Tracker t;t.initialized=true;t.enabled=enabled;
    enter(r,t,100u);
    assert(t.waiting[0]==enabled);
    for(unsigned tick=1u;tick<=XM87121::ReleaseDelayFrames;tick++){
        const unsigned f=102u+tick;
        // Hold animation fixed: release is the 24th committed native update.
        setW(r,0x4042u,0x7000u);
        t.frame(f,r.data());
        assert(XM87121::w(r.data(),0x4060u)==
            (enabled&&tick==XM87121::ReleaseDelayFrames?0x0200u:0x200Eu));
        // Before the exact release tick the throw owns velocity.
        // At the deadline, only +14 X velocity is neutralized.
        assert(XM87121::d(r.data(),0x4014u)==
            (enabled&&tick==XM87121::ReleaseDelayFrames?
             0u:0x00028000u));
        t.frame(f,r.data()); // duplicate callback must be inert
        if(enabled)assert(t.recoveryTicks[0]==tick);
    }
    // The ROM motion input and vertical physics are not the requested
    // velocity-X field. Do not conflate +18 with +14 or teleport either.
    assert(XM87121::d(r.data(),0x4018u)==0x0000A800u);
    assert(XM87121::d(r.data(),0x401Cu)==0xFFFA0000u);
    assert(XM87121::d(r.data(),0x4020u)==0x00006400u);
    assert(XM87121::w(r.data(),0x400Cu)==233u);
    assert(XM87121::w(r.data(),0x4010u)==400u);
}
static void testRepeatedThrow(){
    std::vector<std::uint8_t> r(65536u,0u);setFixture(r);
    XM87121::Tracker t;t.initialized=true;t.enabled=true;
    enter(r,t,100u);
    for(unsigned f=103u;f<=126u;f++)t.frame(f,r.data());
    assert(XM87121::w(r.data(),0x4060u)==0x0200u);
    assert(t.attempt[0]==1u);
    assert(XM87121::d(r.data(),0x4014u)==0u);
    // New airborne travel before the second grab: the second release
    // must independently remove that new momentum, not reuse the first.
    setD(r,0x4014u,0x00034000u);
    enter(r,t,127u);
    assert(t.attempt[0]==2u);
    for(unsigned f=130u;f<=153u;f++)t.frame(f,r.data());
    assert(XM87121::w(r.data(),0x4060u)==0x0200u);
    assert(XM87121::d(r.data(),0x4014u)==0u);
    assert(XM87121::d(r.data(),0x4018u)==0x0000A800u);
}
static void testNaturalExit(){
    std::vector<std::uint8_t> r(65536u,0u);setFixture(r);
    XM87121::Tracker t;t.initialized=true;t.enabled=true;
    enter(r,t,100u);
    for(unsigned f=103u;f<=115u;f++)t.frame(f,r.data());
    setW(r,0x4060u,0x3400u);setW(r,0x40A0u,0x0800u);
    t.frame(116u,r.data());
    assert(!t.waiting[0]);
    for(unsigned f=117u;f<=150u;f++)t.frame(f,r.data());
    assert(XM87121::w(r.data(),0x4060u)==0x3400u);
    // A native action that already exited throw recovery does not get
    // a forced release, and MUST retain its legitimate X momentum.
    assert(XM87121::d(r.data(),0x4014u)==0x00028000u);
}
static void testFloatPose(){
    assert(XM30FrameData::FLOAT_APEX==120);
    assert(XM30FrameData::FLOAT_EXIT==6);
    assert(XM30FrameData::FLOAT_LIFE==126);
    XM30FloatPose p=xm30FloatPose(119,8);
    assert(p.horizontal==1.0 && p.rise==39 && p.alpha==255);
    p=xm30FloatPose(120,8);
    assert(p.horizontal>0.0 && p.horizontal<1.0 && p.rise==41);
    p=xm30FloatPose(122,8);
    assert(p.horizontal==0.5 && p.rise==44);
    p=xm30FloatPose(125,8);
    assert(p.horizontal==0.0 && p.rise==48 && p.alpha==0);
}
struct FloatBounds{
    int left=384,top=224,right=-1,bottom=-1,active=0;
    int width() const{return right>=left?right-left+1:0;}
    int height() const{return bottom>=top?bottom-top+1:0;}
};
static FloatBounds bounds(const std::vector<std::uint8_t>& pixels){
    FloatBounds b;
    for(int y=0;y<224;++y)for(int x=0;x<384;++x){
        if(pixels[((std::size_t)y*384u+(std::size_t)x)*4u+3u]==0)continue;
        b.left=std::min(b.left,x);b.top=std::min(b.top,y);
        b.right=std::max(b.right,x);b.bottom=std::max(b.bottom,y);
        ++b.active;
    }
    return b;
}
static void testNativePixels(){
    // Synthetic 128x48 RGBA SF2 atlas with five alpha-defined glyphs.
    // The real game obtains its pixels from the actual small PNG.
    std::vector<std::uint8_t> bitmap(128u*48u*4u,0u);
    const char* chars="S:12F";
    for(const char* p=chars;*p;++p){
        const int cellX=((*p-32)%16)*8;
        const int cellY=((*p-32)/16)*8;
        for(int y=0;y<8;++y)for(int x=1;x<=4;++x){
            const unsigned i=(unsigned)(((cellY+y)*128+cellX+x)*4);
            bitmap[i]=200;bitmap[i+1]=80;bitmap[i+2]=30;
            bitmap[i+3]=255;
        }
    }
    XM87217::Font font;
    assert(font.prepare(bitmap.data(),128,48,128*4));
    assert(font.glyph['S'].w==4);
    assert(font.glyph['S'].advance==5);
    XM30FrameData frame;
    frame.enabled=true;frame.inCombat=true;
    frame.floats[0].visible=true;
    frame.floats[0].startup=12;
    frame.floats[0].x=192;
    frame.floats[0].y=160;
    std::vector<std::uint8_t> native(384u*224u*4u,0u);
    XM87217::Image image;
    image.rgba=native.data();image.w=384;image.h=224;image.pitch=384*4;
    auto renderAt=[&](int age){
        std::fill(native.begin(),native.end(),0u); // one fresh frame
        frame.floats[0].age=age;
        XM87217::draw(image,font,frame);
        return bounds(native);
    };
    FloatBounds first=renderAt(0);
    assert(first.active>0 && first.top==152 && first.height()==8);
    assert(native[((std::size_t)first.top*384u+first.left)*4u]==200u);
    const FloatBounds apex=renderAt(119);
    assert(apex.active>0 && apex.top==113 && apex.height()==8);
    const FloatBounds half=renderAt(122);
    assert(half.active>0 && half.top==108 && half.height()==8);
    assert(half.width()<apex.width());
    assert(half.left>apex.left); // collapse toward the same center
    const FloatBounds last=renderAt(124);
    assert(last.active>0 && last.top==106 && last.width()<half.width());
    const FloatBounds ended=renderAt(125);
    assert(ended.active==0); // no residual one-pixel column or trail
    frame.inCombat=false;
    const FloatBounds inactive=renderAt(0);
    assert(inactive.active==0); // invisible outside actual combat

    // A second held native paint cannot accumulate old alpha samples:
    // the caller gives each paint a freshly cleared 384x224 image.
    frame.inCombat=true;
    FloatBounds once=renderAt(30);
    FloatBounds twice=renderAt(30);
    assert(once.active==twice.active && once.top==twice.top);
    std::puts("87.219 NATIVE_FLOAT_PASS: native raster bounds, 6f squeeze, 8px rise, no SDL coordinates");
}
// TEST 87.219 observed failure: native combo throws at Y=681..865
// were incorrectly rejected by the hardcoded Y<472 \"floor\" gate.
// Regression also covers action 020C / 0214 (mode 1C, AA=1/2).
static void checkHighWorldYNeutral(){
    std::vector<std::uint8_t> ram(65536u,0u);setFixture(ram);
    setW(ram,0x4010u,810u);
    setW(ram,0x4410u,801u); // opponent is also above old Y cutoff
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    enter(ram,tracker,300u);
    assert(tracker.attempt[0]==1u && tracker.waiting[0]);
    assert(tracker.selected[0].action==0x0200u);
    for(unsigned f=303u;f<=326u;++f)tracker.frame(f,ram.data());
    assert(tracker.recoveryTicks[0]==24u);
    assert(XM87121::w(ram.data(),0x4060u)==0x0200u);
    assert(XM87121::d(ram.data(),0x4014u)==0u);
    assert(XM87121::w(ram.data(),0x4010u)==810u);
    assert(XM87121::w(ram.data(),0x4410u)==801u);
}
static void checkComboRetainsProvenNeutral(){
    std::vector<std::uint8_t> ram(65536u,0u);setFixture(ram);
    setW(ram,0x4010u,865u);
    setW(ram,0x4060u,0x0202u);
    setW(ram,0x40AAu,0x0200u);
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    tracker.frame(400u,ram.data()); // genuine neutral-air controller
    setW(ram,0x4060u,0x1A02u);
    setW(ram,0x40A0u,0x2800u); // ongoing aerial attack
    for(unsigned f=401u;f<=444u;++f)tracker.frame(f,ram.data());
    setW(ram,0x4060u,0x0214u);
    setW(ram,0x40A0u,0x1C28u); // native air-combo movement
    tracker.frame(445u,ram.data());
    assert(tracker.lastComboAir[0].action==0x0214u);
    setW(ram,0x4060u,0x200Cu);
    setW(ram,0x40A0u,0x3200u);
    tracker.frame(446u,ram.data());
    assert(tracker.armed[0]);
    assert(tracker.selected[0].action==0x0202u);
    assert(tracker.attempt[0]==1u);
    setW(ram,0x4060u,0x200Eu);
    tracker.frame(447u,ram.data());
    assert(tracker.waiting[0]);
    for(unsigned f=448u;f<=471u;++f)tracker.frame(f,ram.data());
    assert(XM87121::w(ram.data(),0x4060u)==0x0202u);
    assert(XM87121::d(ram.data(),0x4014u)==0u);
    assert(XM87121::w(ram.data(),0x4010u)==865u);
}
static void checkNativeComboSubstateFallback(){
    std::vector<std::uint8_t> ram(65536u,0u);setFixture(ram);
    setW(ram,0x4010u,681u);
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    setW(ram,0x4060u,0x020Cu);
    setW(ram,0x40A0u,0x1C1Au);
    tracker.frame(500u,ram.data());
    assert(!tracker.lastAir[0].valid);
    assert(tracker.lastComboAir[0].valid);
    setW(ram,0x4060u,0x200Cu);
    setW(ram,0x40A0u,0x3200u);
    tracker.frame(501u,ram.data());
    assert(tracker.armed[0] && tracker.selected[0].action==0x020Cu);
    setW(ram,0x4060u,0x200Eu);
    tracker.frame(502u,ram.data());
    for(unsigned f=503u;f<=526u;++f)tracker.frame(f,ram.data());
    assert(XM87121::w(ram.data(),0x4060u)==0x020Cu);
    assert(XM87121::d(ram.data(),0x4014u)==0u);
    assert(XM87121::w(ram.data(),0x4010u)==681u);
}
static void checkNoFakeComboState(){
    std::vector<std::uint8_t> ram(65536u,0u);setFixture(ram);
    setW(ram,0x4010u,805u);
    setW(ram,0x4060u,0x020Cu);
    setW(ram,0x40A0u,0x1C1Au);
    setW(ram,0x40AAu,0x0000u); // not native rising/falling
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    tracker.frame(600u,ram.data());
    assert(!tracker.lastComboAir[0].valid);
    setW(ram,0x4060u,0x200Cu);
    setW(ram,0x40A0u,0x3200u);
    tracker.frame(601u,ram.data());
    assert(!tracker.armed[0]);
    setW(ram,0x4060u,0x200Eu);
    tracker.frame(602u,ram.data());
    assert(!tracker.waiting[0]); // do not manufacture a jump
}
static void checkUserXCancelUnaffected(){
    std::vector<std::uint8_t> ram(65536u,0u);setFixture(ram);
    XM87121::Tracker tracker;tracker.initialized=true;tracker.enabled=true;
    enter(ram,tracker,700u);
    for(unsigned f=703u;f<=726u;++f)tracker.frame(f,ram.data());
    assert(XM87121::w(ram.data(),0x4060u)==0x0200u);
    setW(ram,0x4060u,0x1A02u);
    setW(ram,0x40A0u,0x2800u);
    setD(ram,0x4014u,0x00050000u); // subsequent player movement
    tracker.frame(727u,ram.data());
    assert(XM87121::w(ram.data(),0x4060u)==0x1A02u);
    assert(XM87121::d(ram.data(),0x4014u)==0x00050000u);
}
int main(){
    checkStun();
    assert(XM87121::ReleaseDelayFrames==24u);
    testRelease(true);testRelease(false);
    testRepeatedThrow();testNaturalExit();testFloatPose();testNativePixels();
    checkHighWorldYNeutral();checkComboRetainsProvenNeutral();
    checkNativeComboSubstateFallback();checkNoFakeComboState();
    checkUserXCancelUnaffected();
    std::puts("87.219 PASS: native combo Y681..865 + air 020C/0214, 24f release, user cancel intact");
    return 0;
}
