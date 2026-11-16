// TEST 87.398: standalone, no ROM/SDL or GitHub Actions required.
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
    // The Versus soundtrack is the only clock source. This now begins
    // 120 frames earlier than the 87.397 hold, but the fight-screen HUD
    // stays invisible for six native frames when its scene appears late.
    Timeline t;
    t.observe(0,98,true);
    assert(t.step(98)==30u);
    t.startWithSong(1u,100u);
    assert(t.step(100)==0u);
    t.observe(0,101u,true);
    t.observe(2,130u,true);
    t.observe(4,160u,true);
    assert(t.step(219u)==0u); // song clock elapsed but no fight HUD yet
    assert(t.step(220u)==0u); // must never complete offscreen
    assert(!Timeline::mayMaskNativeHud(2));
    assert(!Timeline::mayMaskNativeHud(4)); // forest transition untouched
    assert(Timeline::mayMaskNativeHud(6));
    assert(Timeline::mayMaskNativeHud(8));
    assert(!Timeline::mayMaskNativeHud(10));
    t.observe(6,240u,true); // first fight HUD, after song timer expired
    assert(t.fightHudSeen && t.firstHudFrame==240u);
    assert(t.step(240u)==0u);
    assert(t.step(245u)==0u); // invisible on first six fight HUD frames
    assert(t.step(246u)==1u); // start primer immediately afterward
    assert(t.step(274u)==29u);
    assert(t.step(275u)==30u);
    t.observe(8,300u,true);
    assert(t.step(300u)==30u); // no repeat on FIGHT
    t.startWithSong(1u,305u); // same audio event cannot re-arm
    assert(t.step(305u)==30u);
    t.observe(10,320u,true);
    assert(t.step(320u)==30u); // KO/results fully visible

    // Scenario 2: fight HUD appears BEFORE the song hold finishes. The
    // source audio timing remains authoritative; no extra startup delay.
    t.startWithSong(2u,500u);
    t.observe(2,510u,true);
    t.observe(6,550u,true);
    assert(t.step(550u)==0u);
    assert(t.step(619u)==0u);
    assert(t.step(620u)==1u); // 500+120
    assert(t.step(649u)==30u);

    // Version 3 disables the effect without changing the song generation.
    t.observe(6,650u,false);
    assert(t.step(650u)==30u);
    t.reset();
    assert(t.step(660u)==30u);
    t.startWithSong(3u,700u);
    t.observe(6,705u,true);
    assert(t.step(819u)==0u);
    assert(t.step(820u)==1u);

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
    std::printf("87.398 HUD shock tests PASS: timeline, object scope, Bayer reveal, Z restore, stun overlay\n");
    return 0;
}
