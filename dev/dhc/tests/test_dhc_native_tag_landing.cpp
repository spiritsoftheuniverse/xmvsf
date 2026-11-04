// Portable opt-in Stage 05 tag-landing prototype tests.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_138_tag_land_test.h"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace XM87138Tag;
static void actorId(std::uint8_t* r,unsigned base,unsigned id,unsigned x,unsigned y) {
    sw(r,base,0x0101u);sw(r,base+0x52u,id);
    sw(r,base+0x0Cu,x);sw(r,base+0x10u,y);
}
static void setup(std::vector<std::uint8_t>& ram){
    std::uint8_t* r=ram.data();
    sb(r,0x5000u,8u);sb(r,0x5113u,5u);
    sw(r,0xA014u,0x00FFu);sw(r,0xA016u,0x4000u);
    actorId(r,0x4000u,0x20u,350u,472u);
    actorId(r,0x4400u,0x24u,500u,472u);
    actorId(r,0x4800u,0x22u,320u,360u);
    actorId(r,0x4C00u,0x26u,700u,360u);
}
static void swapP1(std::uint8_t* r,unsigned incomingY) {
    actorId(r,0x4000u,0x22u,330u,incomingY);
    actorId(r,0x4800u,0x20u,350u,472u);
}
int main(){
    std::vector<std::uint8_t> ram(65536,0);
    setup(ram);
    State test;test.initialized=true;test.enabled=true;
    test.frame(ram.data(),100u,"xmvsfu",false);
    assert(test.swaps==0 && test.positioned==0);
    swapP1(ram.data(),410u);
    sw(ram.data(),0x400Eu,0xBEEFu); // retain fractional X after relocation
    test.frame(ram.data(),101u,"xmvsfu",false);
    assert(test.swaps==1 && test.positioned==1);
    assert(w(ram.data(),0x400Cu)==452u);
    assert(w(ram.data(),0x400Eu)==0xBEEFu);
    assert((b(ram.data(),0x404Bu)&1u)==0u); // faces opponent to the right
    assert(w(ram.data(),0x440Cu)==500u); // opponent unmoved
    assert(w(ram.data(),0x4000u+0x52u)==0x22u); // identity intact
    test.frame(ram.data(),101u,"xmvsfu",false);
    assert(test.swaps==1); // no duplicate frame
    test.frame(ram.data(),102u,"xmvsfu",true);
    assert(test.swaps==1); // speculative pass inert

    // Swap occurring on or after native ground height must never teleport.
    setup(ram);State grounded;grounded.initialized=true;grounded.enabled=true;
    grounded.frame(ram.data(),200u,"xmvsfu",false);
    swapP1(ram.data(),472u);
    grounded.frame(ram.data(),201u,"xmvsfu",false);
    assert(grounded.swaps==1 && grounded.positioned==0);
    assert(w(ram.data(),0x400Cu)==330u);

    // Disabled experiment leaves normal native tags untouched.
    setup(ram);State disabled;disabled.initialized=true;disabled.enabled=false;
    disabled.frame(ram.data(),300u,"xmvsfu",false);
    swapP1(ram.data(),410u);
    disabled.frame(ram.data(),301u,"xmvsfu",false);
    assert(w(ram.data(),0x400Cu)==330u);

    // P2 incoming native tag lands on the right side of its opponent.
    setup(ram);actorId(ram.data(),0x4000u,0x20u,500u,472u);
    actorId(ram.data(),0x4400u,0x24u,600u,472u);
    State other;other.initialized=true;other.enabled=true;
    other.frame(ram.data(),400u,"xmvsfu",false);
    actorId(ram.data(),0x4400u,0x26u,610u,410u);
    actorId(ram.data(),0x4C00u,0x24u,600u,472u);
    other.frame(ram.data(),401u,"xmvsfu",false);
    assert(other.swaps==1 && other.positioned==1);
    assert(w(ram.data(),0x440Cu)==548u);
    assert((b(ram.data(),0x444Bu)&1u)==1u); // faces opponent to left
    assert(w(ram.data(),0x400Cu)==500u);

    // This placement is Stage05-only until other stage bounds are verified.
    setup(ram);State stage;stage.initialized=true;stage.enabled=true;
    stage.frame(ram.data(),500u,"xmvsfu",false);
    swapP1(ram.data(),410u);sb(ram.data(),0x5113u,4u);
    stage.frame(ram.data(),501u,"xmvsfu",false);
    assert(stage.swaps==0 && stage.positioned==0);
    assert(w(ram.data(),0x400Cu)==330u);
    return 0;
}
