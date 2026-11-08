// TEST 87.210, standalone C++11 regression of the original PNG-HUD
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
    std::puts("87.210 STUN_UI_PASS: mirrored source, full coverage, live tip, 1px shadow, threshold");
}
static void setW(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    XM87121::sw(r.data(),p,v);
}
static void setD(std::vector<std::uint8_t>& r,unsigned p,std::uint32_t v){
    XM87121::sd(r.data(),p,v);
}
static void runRecovery(bool permit,bool victimLanded){
    std::vector<std::uint8_t> ram(65536u,0u);
    const unsigned p=0x4000u,o=0x4400u;
    XM87121::sb(ram.data(),0x5000u,8u);
    setW(ram,p+0x00u,0x0101u); // native Cyclops identity
    setW(ram,p+0x40u,0x0021u);
    setW(ram,p+0x10u,400u); // airborne world Y
    setW(ram,p+0x60u,0x0200u);
    setW(ram,p+0xA0u,0x1C00u);
    setW(ram,p+0xAAu,0x0100u);
    setW(ram,p+0x42u,0x8826u);
    setD(ram,p+0x14u,0x00028000u);
    setD(ram,p+0x18u,0x00010000u);
    setD(ram,p+0x1Cu,0xFFFA0000u);
    setD(ram,p+0x20u,0x00006400u);
    setD(ram,p+0x34u,0x000A5F4Au);
    setD(ram,p+0x38u,0x000A5F5Au);
    setW(ram,p+0x0Cu,233u);
    setW(ram,o+0x10u,460u);
    setW(ram,o+0x60u,0x1C30u);
    XM87121::Tracker tracker;
    tracker.initialized=true;tracker.enabled=permit;
    tracker.frame(100,ram.data()); // pre-throw airborne snapshot
    setW(ram,p+0x60u,0x200Cu);
    tracker.frame(101,ram.data()); // capture
    setW(ram,p+0x60u,0x200Eu);
    setW(ram,p+0x06u,4u);
    setW(ram,p+0x08u,6u);
    setW(ram,p+0xA0u,0x3200u);
    setW(ram,p+0xAAu,0x0200u);
    setW(ram,p+0x42u,0x7000u);
    if(victimLanded){
        setW(ram,o+0x10u,472u);
        setW(ram,o+0x60u,0x1C34u);
    }
    tracker.frame(102,ram.data()); // grab ends; WAIT, do not recover
    assert(XM87121::w(ram.data(),p+0x60u)==0x200Eu);
    assert(tracker.animationAdvances[0]==0u);

    // No sprite frame change despite 20 elapsed game frames. Must NOT
    // recover; the user explicitly requested ANIMATION frames.
    tracker.frame(122,ram.data());
    assert(tracker.animationAdvances[0]==0u);
    assert(XM87121::w(ram.data(),p+0x60u)==0x200Eu);
    std::vector<std::uint8_t> originalOpponent(ram.begin()+o,ram.begin()+o+0x400u);

    // Five unique sprite frame advances at nonuniform emulated times.
    const unsigned frames[5]={125,127,134,151,170};
    for(unsigned i=0;i<5u;i++){
        setW(ram,p+0x42u,0x7001u+i);
        tracker.frame(frames[i],ram.data());
        if(permit){
            assert(tracker.animationAdvances[0]==i+1u);
        }
        const bool released=permit && victimLanded && i==4u;
        assert(XM87121::w(ram.data(),p+0x60u)==
               (released?0x0200u:0x200Eu));
    }
    const bool shouldRecover=permit && victimLanded;
    if(shouldRecover){
        assert(XM87121::d(ram.data(),p+0x14u)==0x00028000u);
        assert(XM87121::d(ram.data(),p+0x1Cu)==0xFFFA0000u);
        assert(XM87121::d(ram.data(),p+0x34u)==0x000A5F4Au);
        assert(XM87121::w(ram.data(),p+0xA0u)==0x1C00u);
    }else if(permit && !victimLanded){
        // Still at 200E after five animation frames until impact.
        setW(ram,o+0x10u,472u);
        setW(ram,o+0x60u,0x1C34u);
        tracker.frame(171,ram.data()); // NO new animation frame
        assert(XM87121::w(ram.data(),p+0x60u)==0x0200u);
    }
    assert(XM87121::w(ram.data(),p+0x0Cu)==233u);
    assert(XM87121::w(ram.data(),p+0x10u)==400u);
    if(victimLanded || !permit)
        assert(std::memcmp(originalOpponent.data(),ram.data()+o,0x400u)==0);
}
static void checkCyclops(){
    runRecovery(true,true);
    runRecovery(false,true);
    runRecovery(true,false);
    std::puts("87.210 CYCLOPS_ANIMATION_DELAY_PASS: 5 sprite advances, not 5 elapsed frames, impact gate");
}
int main(){
    checkStun();
    checkCyclops();
    return 0;
}
