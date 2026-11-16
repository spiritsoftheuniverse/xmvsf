// TEST 87.397: standalone, no ROM/SDL or GitHub Actions required.
// Build locally with RUN_TEST_HUD_392.bat.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <cstring>
int main(){
    using namespace XM87392;
    // The original runtime report used the stock xmvsfu driver, with
    // version4=1 but CpsXmvSixEnabled=0. The old 3v3-only gate meant
    // every HUD element was always visible.
    assert(eligible("xmvsfu",true));
    assert(eligible("xmvsf",true));
    assert(eligible("xmvsf3v3",true));
    assert(!eligible("xmvsfu",false));
    assert(!eligible("mvsc",true));
    assert(!eligible(nullptr,true));
    // Versus OGG playback (not FF5000 status) is the sole timeline source.
    // The 87.395 replay starts at native frame 290, already in the
    // handoff; the fixture can begin inside or before phase 02.
    Timeline t;
    t.observe(0,98,true);
    assert(t.step(98)==30u);
    t.startWithSong(1u,100u); // first real VS PCM; no phase assumption
    assert(t.step(100)==0u);
    t.observe(0,101u,true);   // may still be in selection transition
    assert(t.step(101)==0u);
    t.observe(2,130u,true);
    t.observe(4,160u,true);
    t.observe(6,240u,true);
    // Hold finishes 30 game frames sooner: real VS setup around native
    // frame 2126; the first observed fight frame was around 2305.
    assert(t.step(279)==0u);
    assert(t.step(280)==1u);
    assert(t.step(308)==29u);
    assert(t.step(309)==30u);
    t.observe(8,410u,true);  // FIGHT does not re-arm
    t.startWithSong(1u,410u); // duplicate mixer read cannot re-arm
    assert(t.step(410)==30u);
    t.observe(10,420u,true); // KO disarms
    assert(t.step(420)==30u);
    t.startWithSong(2u,500u); // next actual VS song starts new hold
    assert(t.step(500)==0u);
    t.observe(2,510u,true);
    assert(t.step(510)==0u);
    assert(!Timeline::mayMaskNativeHud(0));
    assert(!Timeline::mayMaskNativeHud(2));
    assert(!Timeline::mayMaskNativeHud(4)); // never reveal native forest
    assert(Timeline::mayMaskNativeHud(6));
    assert(Timeline::mayMaskNativeHud(8));
    assert(!Timeline::mayMaskNativeHud(10));
    t.observe(6,520u,false); // Version 3 remains unmodified
    assert(t.step(520)==30u);
    t.reset();
    assert(t.step(530)==30u);
    t.startWithSong(3u,550u);
    assert(t.step(550)==0u);
    assert(t.step(729)==0u);
    assert(t.step(730)==1u);

    assert(object(0,1,87808,160,12,2,2));
    assert(!object(0,1,87808,32,100,2,2));
    assert(object(7,2,88463,32,20,2,1));
    assert(object(7,12,90000,352,0,2,4,true));
    assert(!object(6,2,88463,32,20,2,1));
    assert(!object(7,3,50000,20,0,12,10)); // massive intro wall
    assert(object(7,3,50000,20,0,3,2)); // all native HUD decorations
    assert(!object(7,3,50000,20,0,3,2,false,true)); // stage scenery excluded
    assert(!object(7,2,88463,32,90,2,1));  // no fight-field sprites
    const std::uint32_t ink=0x004491ccu;
    assert(shockRGB(ink,30)==ink);
    assert(shockRGB(ink,1)!=ink);
    assert(shockRGB(ink,12)!=ink);
    assert(shineRGB(ink,105,28,15)!=shockRGB(ink,15));
    assert(shineRGB(ink,279,28,15)!=shockRGB(ink,15));
    assert(shineRGB(ink,150,28,15)==shockRGB(ink,15));
    assert(shineRGB(ink,105,28,30)==ink);
    // First materialization frame already contains visible white primer,
    // even when the ordinary first Bayer step is still dark.
    assert(shineCore(23,28,1));
    assert(!visible(23,28,1));
    assert(shineCore(105,28,15));
    assert(!visible(105,28,15)); // white filament separate from Bayer
    assert(shineCore(279,28,15));
    assert(!shineCore(150,28,15));
    assert(!shineCore(105,28,30));
    unsigned last=0;
    for(unsigned s=0;s<=30;++s){
        unsigned seen=0;
        for(int y=0;y<8;++y)for(int x=0;x<8;++x)
            seen+=visible(x,y,s)?1u:0u;
        assert(seen>=last);
        last=seen;
    }
    assert(last==64u);

    const int width=384,height=224,pitch=384*4;
    std::vector<std::uint8_t> fb((std::size_t)pitch*height);
    std::vector<std::uint16_t> z((std::size_t)width*height,14u);
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){
        const std::uint32_t pixel=0x00202020u;
        std::memcpy(fb.data()+(std::size_t)y*pitch+x*4,&pixel,4);
    }
    auto paint=[&](){
        for(int y=20;y<36;++y)for(int x=160;x<176;++x){
            const std::uint32_t pixel=0x00ff3300u;
            std::memcpy(fb.data()+(std::size_t)y*pitch+x*4,&pixel,4);
            z[(std::size_t)y*width+x]=50u;
        }
    };
    Tile hidden;
    hidden.save(fb.data(),pitch,z.data(),width,height,160,20);
    paint();
    hidden.finish(fb.data(),z.data(),width,height,0u);
    for(int y=20;y<36;++y)for(int x=160;x<176;++x){
        std::uint32_t pixel=0u;
        std::memcpy(&pixel,fb.data()+(std::size_t)y*pitch+x*4,4);
        assert(pixel==0x00202020u);
        assert(z[(std::size_t)y*width+x]==14u);
    }
    Tile half;
    half.save(fb.data(),pitch,z.data(),width,height,160,20);
    paint();
    half.finish(fb.data(),z.data(),width,height,15u);
    unsigned shown=0,restored=0;
    for(int y=20;y<36;++y)for(int x=160;x<176;++x){
        std::uint32_t pixel=0u;
        std::memcpy(&pixel,fb.data()+(std::size_t)y*pitch+x*4,4);
        if(visible(x,y,15u)){
            assert(pixel==shineRGB(0x00ff3300u,x,y,15u));++shown;
            assert(z[(std::size_t)y*width+x]==50u);
        }else{
            assert(pixel==0x00202020u);++restored;
            assert(z[(std::size_t)y*width+x]==14u);
        }
    }
    assert(shown>0u && restored>0u);
    // A moving shine streak must be visible even where the ordinary Bayer
    // dissolve would hide that *actual HUD object* source pixel.
    Tile glint;
    glint.save(fb.data(),pitch,z.data(),width,height,104,24);
    for(int y=24;y<40;++y)for(int x=104;x<120;++x){
        const std::uint32_t pixel=0x00ff3300u;
        std::memcpy(fb.data()+(std::size_t)y*pitch+x*4,&pixel,4);
        z[(std::size_t)y*width+x]=50u;
    }
    glint.finish(fb.data(),z.data(),width,height,15u);
    std::uint32_t whiteFilament=0u;
    std::memcpy(&whiteFilament,fb.data()+(std::size_t)28*pitch+105*4,4);
    assert(whiteFilament==shineRGB(0x00ff3300u,105,28,15u));
    assert(z[(std::size_t)28*width+105]==50u);

    std::vector<std::uint8_t> overlay((std::size_t)384*224*4,0u);
    for(int x=60;x<70;++x){
        std::uint8_t* p=overlay.data()+((std::size_t)45*384+x)*4;
        p[0]=255;p[1]=120;p[2]=25;p[3]=255;
    }
    stunOverlay(overlay.data(),384*4,0u);
    for(int x=60;x<70;++x)
        assert(overlay[((std::size_t)45*384+x)*4+3]==0u);
    const int sx=110,sy=45; // core of left shockfront at reveal frame 15
    assert(shineCore(sx,sy,15u));
    assert(!visible(sx,sy,15u));
    std::uint8_t* glowing=overlay.data()+((std::size_t)sy*384+sx)*4;
    glowing[0]=255;glowing[1]=120;glowing[2]=25;glowing[3]=255;
    stunOverlay(overlay.data(),384*4,15u);
    assert(glowing[3]==255u);
    const std::uint32_t shineColor=shineRGB(0x00ff7819u,sx,sy,15u);
    assert(glowing[0]==((shineColor>>16)&255u));
    assert(glowing[1]==((shineColor>>8)&255u));
    assert(glowing[2]==(shineColor&255u));
    std::printf("87.397 HUD shock tests PASS: timeline, object scope, Bayer reveal, Z restore, stun overlay\n");
    return 0;
}
