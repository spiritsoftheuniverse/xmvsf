// TEST 87.395: standalone, no ROM/SDL or GitHub Actions required.
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
    // Actual 87.394 Sigma capture: scene handoff in phase 02 near frame 1,
    // introduction phase 06 at ~182, first combat phase 08 at ~362.
    // The 210-frame delay must NOT restart at either later transition.
    Timeline t;
    t.observe(1,99,true);
    assert(t.step(99)==30u);
    t.observe(2,100,true); // first handoff frame
    assert(t.step(100)==0u);
    t.observe(3,170,true); // transitional/odd phase still same scene
    assert(t.step(170)==0u);
    t.observe(6,260,true);
    assert(t.step(260)==0u);
    assert(t.step(309)==0u);
    assert(t.step(310)==1u);
    assert(t.step(338)==29u);
    assert(t.step(339)==30u);
    t.observe(8,400,true); // FIGHT must NOT restart the completed reveal
    assert(t.step(400)==30u);
    t.observe(8,400,true); // repeated paused paint is not another frame
    assert(t.step(400)==30u);
    t.observe(10,410,true); // KO/results stay visible
    assert(t.step(410)==30u);
    t.observe(2,500,true); // new handoff re-arms
    assert(t.step(500)==0u);
    t.observe(4,520,true);
    assert(t.step(520)==0u);
    t.observe(8,530,false); // Version 3 stays unmodified
    assert(t.step(530)==30u);
    t.observe(8,660,true); // snapshot/direct-combat fallback
    assert(t.step(660)==0u);
    assert(t.step(870)==1u);

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
    assert(shineRGB(ink,90,28,15)!=shockRGB(ink,15));
    assert(shineRGB(ink,294,28,15)!=shockRGB(ink,15));
    assert(shineRGB(ink,150,28,15)==shockRGB(ink,15));
    assert(shineRGB(ink,90,28,30)==ink);
    assert(shineCore(91,28,15));
    assert(!visible(91,28,15)); // white filament is separate from Bayer
    assert(!shineCore(150,28,15));
    assert(!shineCore(91,28,30));
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
    glint.save(fb.data(),pitch,z.data(),width,height,88,24);
    for(int y=24;y<40;++y)for(int x=88;x<104;++x){
        const std::uint32_t pixel=0x00ff3300u;
        std::memcpy(fb.data()+(std::size_t)y*pitch+x*4,&pixel,4);
        z[(std::size_t)y*width+x]=50u;
    }
    glint.finish(fb.data(),z.data(),width,height,15u);
    std::uint32_t whiteFilament=0u;
    std::memcpy(&whiteFilament,fb.data()+(std::size_t)28*pitch+91*4,4);
    assert(whiteFilament==shineRGB(0x00ff3300u,91,28,15u));
    assert(z[(std::size_t)28*width+91]==50u);

    std::vector<std::uint8_t> overlay((std::size_t)384*224*4,0u);
    for(int x=60;x<70;++x){
        std::uint8_t* p=overlay.data()+((std::size_t)45*384+x)*4;
        p[0]=255;p[1]=120;p[2]=25;p[3]=255;
    }
    stunOverlay(overlay.data(),384*4,0u);
    for(int x=60;x<70;++x)
        assert(overlay[((std::size_t)45*384+x)*4+3]==0u);
    const int sx=94,sy=45; // core of left shockfront at reveal frame 15
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
    std::printf("87.395 HUD shock tests PASS: timeline, object scope, Bayer reveal, Z restore, stun overlay\n");
    return 0;
}
