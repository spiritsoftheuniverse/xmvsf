// TEST 87.209, standalone C++11 regression of the original PNG-HUD
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
    int first30[6],first42[6],last;
    for(int row=0;row<6;row++){
        assert(XM8733::FillBounds(base,row,30,60,first30[row],last));
        assert(XM8733::FillBounds(base,row,42,60,first42[row],last));
        assert(first42[row]<first30[row]); // dynamic moving tip, not static
        assert(XM8733::HasBasePixel(base,first42[row],row<2?0:row-2));
        int fullFirst=0,fullLast=0,sourceLeft=0,sourceRight=0;
        XM8733::SourceSpan(base,row,sourceLeft,sourceRight);
        assert(XM8733::FillBounds(base,row,60,60,fullFirst,fullLast));
        assert(fullFirst==sourceLeft && fullLast==sourceRight);
    }
    assert(first30[5]>first30[0]); // diagonal leading edge

    std::vector<unsigned char> right(384u*224u*4u,0);
    std::vector<unsigned char> left(right.size(),0);
    XM8733::Reading partial;partial.total=60;partial.used=30;
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
    assert(XM8733::FillBounds(base,0,30,60,first,stop));
    assert(pixel(right,262+first+1,43)==XM8733::WarmStun(0));
    assert(pixel(right,262+first-1,43)==0u);

    // Bottom shadow is clipped to the SAME currently growing fill; it
    // neither paints two extra pixels nor the unmoving original 76px meter.
    assert(XM8733::FillBounds(base,5,30,60,first,stop));
    for(int x=0;x<61;x++){
        if(x<first||x>stop)
            assert(pixel(right,262+x,49)!=XM8733::PackRGBA(67,68,69));
    }

    XM8733::Reading full=partial;full.used=60;
    std::fill(right.begin(),right.end(),0);
    XM8733::DrawOne(right.data(),384*4,1,full,false,201,&base);
    assert(XM8733::FillBounds(base,0,60,60,first,stop));
    assert(first==0);
    assert(pixel(right,262+first+5,43)==XM8733::RainbowPan(201,first+5,0));
    assert(pixel(right,262+first+5,43)!=XM8733::WarmStun(0));
    assert(XM8733::RainbowPan(201,12,0)!=
           XM8733::RainbowPan(203,12,0)); // rapid diagonal pan

    // A missing PNG must NOT resurrect the old 76-pixel custom meter.
    std::fill(right.begin(),right.end(),0);
    XM8733::DrawOne(right.data(),384*4,1,full,false,202,nullptr);
    for(unsigned char byte:right)assert(byte==0);
    std::puts("87.209 STUN_UI_PASS: mirrored source, full coverage, live tip, 1px shadow, threshold");
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
    setW(ram,p+0x00u,0x0101u); // actual Cyclops slot identity
    setW(ram,p+0x40u,0x0021u); // Cyclops action bank
    setW(ram,p+0x10u,400u);    // airborne
    setW(ram,p+0x60u,0x0200u);
    setW(ram,p+0xA0u,0x1C00u);
    setW(ram,p+0xAAu,0x0100u);
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
    tracker.frame(100,ram.data()); // exact preceding airborne snapshot
    setW(ram,p+0x60u,0x200Cu);
    tracker.frame(101,ram.data()); // arm 87.121 release
    setW(ram,p+0x60u,0x200Eu);
    setW(ram,p+0x06u,4u);
    setW(ram,p+0x08u,6u);
    setW(ram,p+0xA0u,0x3200u);
    setW(ram,p+0xAAu,0x0200u);
    if(victimLanded){
        setW(ram,o+0x10u,472u);
        setW(ram,o+0x60u,0x1C34u);
    }
    std::vector<std::uint8_t> opponent(ram.begin()+o,ram.begin()+o+0x400u);
    tracker.frame(102,ram.data()); // 200C -> 200E after impact
    const bool shouldRecover=permit && victimLanded;
    assert(XM87121::w(ram.data(),p+0x60u)==
           (shouldRecover?0x0200u:0x200Eu));
    if(shouldRecover){
        assert(XM87121::d(ram.data(),p+0x14u)==0x00028000u);
        assert(XM87121::d(ram.data(),p+0x1Cu)==0xFFFA0000u);
        assert(XM87121::d(ram.data(),p+0x34u)==0x000A5F4Au);
        assert(XM87121::w(ram.data(),p+0xA0u)==0x1C00u);
    }
    assert(XM87121::w(ram.data(),p+0x0Cu)==233u); // no teleport
    assert(XM87121::w(ram.data(),p+0x10u)==400u); // no forced jump
    assert(std::memcmp(opponent.data(),ram.data()+o,0x400u)==0); // victim untouched
}
static void checkCyclops(){
    runRecovery(true,true);
    runRecovery(false,true);
    runRecovery(true,false);
    std::puts("87.209 CYCLOPS_121_PASS: exact airborne resume, opt-out, victim gate");
}
int main(){
    checkStun();
    checkCyclops();
    return 0;
}
