// TEST 87.240: stun bar uses capacity/10 horizontal bands, never Y-row tint.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_33_dizzy_hud.h"

static void bins(int total){
    using namespace XM8733;
    const int origin=FaceWidth-1;
    assert(total==50 || total==60);
    const int perStep=total/10;
    assert(perStep==5 || perStep==6);
    assert(StunColorStep(0,origin,total)==0u);
    for(int d=0;d<FaceWidth;++d){
        const int stun=(d*total)/FaceWidth;
        const unsigned expected=(unsigned)std::min(9,stun/perStep);
        assert(StunColorStep(0,origin-d,total)==expected);
    }
    for(int row=0;row<6;++row){
        // Identical stun depth, identical color: not a vertical gradient.
        assert(StunColorStep(row,row+FaceWidth-1,total)==0u);
        assert(StunColorStep(row,row+FaceWidth-20,total)==
               StunColorStep(0,FaceWidth-1-20,total));
    }
    assert(WarmStun(0)!=WarmStun(9));
    assert(WarmStun(StunColorStep(0,origin,total))==
           WarmStun(StunColorStep(5,origin+5,total)));
}
static void mirroredNativeBar(){
    using namespace XM8733;
    std::vector<unsigned char> sprite(61u*6u*4u,255u);
    std::vector<unsigned char> canvas(384u*224u*4u,0u);
    const BarArt base={sprite.data(),61,6,61*4};
    Reading r; r.total=60;r.used=54;
    DrawOne(canvas.data(),384*4,1,r,false,0u,&base);
    const unsigned c0=StunColorStep(0,53,60);
    const unsigned c8=StunColorStep(0,10,60);
    assert(c0==0 && c8>=7);
    // In P2 the full face begins at x=262 and fills from source right.
    const auto pixel=[&](int x,int y)->std::uint32_t{
        const auto* p=&canvas[(unsigned)(y*384+x)*4u];
        return (std::uint32_t)p[0] | ((std::uint32_t)p[1]<<8u) |
               ((std::uint32_t)p[2]<<16u) |
               ((std::uint32_t)p[3]<<24u);
    };
    assert(pixel(262+53,43)==WarmStun(c0));
    assert(pixel(262+10,43)==WarmStun(c8));
    DrawOne(canvas.data(),384*4,0,r,false,0u,&base);
    // P1 mirrors positions without reversing its native ten-step order.
    assert(pixel(61+(60-53),43)==WarmStun(c0));
    assert(pixel(61+(60-10),43)==WarmStun(c8));
}
int main(){
    bins(50);
    bins(60);
    mirroredNativeBar();
    std::puts("XMVSF 87.240 stun horizontal 1/10-capacity gradient PASS");
}
