// TEST 87.413: whole upper HUD shock-entry regression, no ROM/SDL or GitHub Actions.
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
    // 87.399: concealment is PRIMED at the native VS music request;
    // the 120-frame countdown begins at the first ACTUAL audio PCM packet.
    // No first-fight-frame max(): that was why 87.397 and 87.398
    // appeared to have exactly the same timing.
    Timeline t;
    t.observe(0,98,true);
    assert(t.step(98)==30u);
    t.prepareForSong(100u);
    assert(t.step(100)==0u); // hide before the music decoder starts
    t.observe(2,101u,true);
    assert(t.step(101)==0u);
    t.observe(4,103u,true);
    t.startWithSong(1u,105u); // first actual VS PCM
    t.noteHudSource(110u); // actual bar drawn before the audio deadline
    assert(t.sourceSeen && t.firstSourceFrame==110u);
    assert(t.step(105)==0u);
    assert(t.step(224)==0u);
    assert(t.step(225)==1u);
    assert(t.step(253)==29u);
    assert(t.step(254)==30u);
    t.observe(6,260u,true);
    assert(t.step(260)==30u); // do NOT re-delay until combat
    t.startWithSong(1u,261u);
    assert(t.step(261)==30u); // no duplicate start
    t.observe(8,300u,true);
    assert(t.step(300)==30u); // FIGHT must not re-arm
    t.observe(10,310u,true);
    assert(t.step(310)==30u); // results visible

    t.prepareForSong(400u); // new match, immediately hidden again
    assert(t.step(400)==0u);
    t.observe(2,401u,true);
    t.startWithSong(2u,410u);
    t.noteHudSource(420u);
    assert(t.step(529)==0u);
    assert(t.step(530)==1u);
    assert(t.step(559)==30u);
    t.observe(4,560u,false);
    assert(t.step(560)==30u); // Version 3 clean
    // Within one persistent SDL process, a fresh VS song replaces the old
    // source frame while retaining legitimate pre-PCM evidence.
    t.prepareForSong(600u);
    t.noteHudSource(602u);
    t.startWithSong(4u,610u);
    assert(t.firstSourceFrame==602u);
    t.prepareForSong(700u);
    assert(!t.armed && !t.sourceSeen);
    t.startWithSong(5u,705u);
    t.noteHudSource(840u);
    assert(t.step(840u)==0u);
    assert(t.step(841u)==1u);
    t.reset();
    assert(t.step(600)==30u);
    t.startWithSong(3u,700u); // fallback if song starts before priming
    assert(t.step(700)==0u);
    assert(t.step(819)==0u);
    assert(t.step(820)==1u); // internal clock: source not yet available
    t.noteHudSource(840u);   // first drawable HUD arrives very late
    assert(t.step(840u)==0u); // MUST be invisible on the first source frame
    assert(t.step(841u)==1u);
    assert(t.step(869u)==29u);
    assert(t.step(870u)==30u);

    // 87.416 shock-only comprehensive top HUD from native priority 7.
    // Source-level key 9 remains separate and is tested below.
    // 87.414 transition shock: strict source/tile + documented name regions.
    // Wolverine's long name may span >9 tiles, while 02/04 forest art
    // must never be masked by the former generic priority-7 fallback.
    assert(Timeline::mayMaskNativeHud(2));
    assert(Timeline::mayMaskNativeHud(4));
    assert(Timeline::mayMaskNativeHud(6));
    assert(Timeline::mayMaskNativeHud(8));
    assert(!Timeline::mayMaskNativeHud(10));
    assert(objectForPhase(2,7,2,88463,32,20,2,1));
    assert(objectForPhase(4,7,4,88547,300,32,2,1));
    assert(objectForPhase(2,0,1,87808,160,12,2,2));
    assert(objectForPhase(6,0,1,91999,160,12,2,2));
    assert(objectForPhase(4,0,1,91999,160,12,2,2)); // timer atlas changes
    assert(!objectForPhase(6,0,1,91999,20,12,2,2));
    assert(!objectForPhase(6,0,1,91999,160,80,2,2));
    assert(!objectForPhase(2,7,3,50000,20,0,3,2)); // unverified 02/04 object
    assert(!objectForPhase(2,7,3,89500,20,16,11,2)); // intro tile is protected; name begins in 06/08
    assert(objectForPhase(8,7,11,89540,271,18,11,2)); // long P2 name in combat
    assert(objectForPhase(6,7,3,89500,20,16,11,2)); // combat as well
    assert(!objectForPhase(2,7,3,89500,168,16,11,2)); // outside names
    assert(!objectForPhase(4,7,3,109500,20,16,3,2)); // tree source
    assert(!objectForPhase(4,7,2,109100,32,20,2,1)); // stage sprite
    assert(objectForPhase(6,7,12,90200,12,0,6,4)); // left portrait during combat
    assert(objectForPhase(2,7,2,89113,16,-2,2,1)); // verified portrait box
    assert(objectForPhase(4,7,2,89115,352,8,2,1)); // right portrait box
    assert(objectForPhase(8,7,2,89250,20,8,3,2)); // top HUD point panel in combat
    assert(objectForPhase(8,7,1,87820,174,12,2,2)); // clock panel in combat
    assert(!objectForPhase(2,7,2,89250,180,8,6,3)); // center intro
    assert(!objectForPhase(4,7,2,109100,20,8,2,2)); // forest
    assert(objectForPhase(8,7,15,90200,322,0,6,4)); // right portrait in combat
    assert(!objectForPhase(2,7,3,50000,20,0,12,10)); // intro wall
    assert(!objectForPhase(2,7,3,50000,20,0,5,3)); // wall fragment
    assert(!objectForPhase(4,7,3,89500,165,16,5,2)); // top mid forest
    // TEST 87.417: reproduce the uploaded 384x224 screenshot's
    // disappearing top-row logo boxes. Same tile source below y=56 must
    // remain visible above y=56 during phases 02/04: no intro cutouts.
    assert(!objectForPhase(2,7,3,89500,32,0,3,3));
    assert(!objectForPhase(4,7,3,89500,32,32,3,3));
    assert(!objectForPhase(2,7,12,90200,12,0,6,4));
    assert(!objectForPhase(4,7,15,90200,322,0,6,4));
    assert(!objectForPhase(2,7,3,104000,16,8,2,2));
    assert(!objectForPhase(4,7,8,106100,338,8,2,2));
    // Verified health strip, portrait border and center numeral still
    // respect the pre-combat shock. Full remaining HUD hides in 06/08.
    assert(objectForPhase(2,7,2,88463,32,20,2,1));
    assert(objectForPhase(4,7,2,89113,16,-2,2,1));
    assert(objectForPhase(2,0,1,87808,160,12,2,2));
    assert(objectForPhase(8,7,3,89500,32,0,3,3));
    // Fonts/portrait/name/point sub-sprites may use alternate palettes
    // and ROM tiles. Source priority 7 (not a speculative atlas) binds
    // them to the same fade step on both sides and ALL stages.
    assert(objectForPhase(6,7,3,104000,16,8,2,2)); // alternate left HUD source during combat
    assert(objectForPhase(8,7,8,106100,338,8,2,2)); // alternate right HUD source during combat
    assert(objectForPhase(6,7,10,50000,18,8,3,2)); // combat HUD source
    assert(objectForPhase(8,7,24,87000,315,8,2,2)); // alternate palette
    assert(objectForPhase(6,0,1,95000,160,8,4,2)); // clock face during combat
    assert(objectForPhase(8,0,1,87832,176,8,2,2)); // varying clock atlas during combat
    assert(!objectForPhase(2,0,1,95000,24,8,4,2)); // not side stage
    assert(!objectForPhase(2,7,3,50000,20,0,3,2)); // intro logo
    assert(!objectForPhase(2,7,25,104000,20,0,3,2)); // scenery palette
    assert(!objectForPhase(2,7,3,109100,20,0,3,2)); // forest family
    assert(!objectForPhase(2,7,3,104000,20,0,3,2,false,true)); // caller scenery
    // Key 9 uses topHudObject with transition=true. Shock's broader
    // phase-aware coverage does NOT alter that source capture predicate.
    assert(!topHudObject(7,3,104000,16,8,2,2,false,false,true));
    assert(!topHudObject(0,1,95000,160,8,4,2,false,false,true));
    assert(!objectForPhase(2,7,3,98000,20,16,12,5)); // tall scenery
    assert(!objectForPhase(2,7,25,50000,20,0,3,2)); // scenery palette
    assert(!objectForPhase(2,7,2,88463,32,90,2,1));
    assert(!objectForPhase(2,7,2,88463,32,20,2,1,false,true));
    assert(objectForPhase(6,7,3,50000,20,0,3,2));
    assert(!objectForPhase(10,7,2,88463,32,20,2,1));

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
    std::printf("87.417 shock tests PASS: VS intro tile rows untouched; combat HUD unified reveal; key9 unchanged\n");
    return 0;
}
