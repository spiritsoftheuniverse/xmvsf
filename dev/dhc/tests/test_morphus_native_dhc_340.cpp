// 87.340 standalone C++11 test: Morphus56k/dhc.s TWO-MEMBER port.
// Local only. Never a BUILD_TEST gate or GitHub Action.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_340_morphus_native_2v2.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using namespace XM87340Donor;
static unsigned word(const std::uint8_t* p,unsigned n){
    return (unsigned(p[n+1u])<<8u)|p[n];
}
static void be(std::vector<std::uint8_t>& p,unsigned at,
               const std::uint8_t* src,unsigned n){
    for(unsigned i=0;i<n;++i)p[at+(i^1u)]=src[i];
}
int main(){
    // Source/engine/dhc_table.inc; not a per-character guessed 2E selector.
    assert(sourceHyper(0x12u,0x0Au,true));
    assert(sourceHyper(0x14u,0x0Eu,true));
    assert(!sourceHyper(0x1Au,0x0Au,true)); // Zangief throw ends chain
    assert(XM87329Morphus::rows()[0x14u/2u].hyper[0]==0x0Cu);
    assert(XM87329Morphus::rows()[0x14u/2u].hyper[1]==0x0Eu);
    assert(XM87329Morphus::rows()[0x02u/2u].hyper[0]==0x04u);
    assert(XM87329Morphus::rows()[0x02u/2u].hyper[1]==0x02u);
    assert(facingDir(1u,true)==2u && facingDir(2u,true)==1u);
    assert(facingDir(5u,true)==6u && facingDir(6u,true)==5u);
    Slot claim;
    claim.sample(1u,0x12u,0x14u,0u,0u,true,0x0Au,true,
                 true,0x1234u,1u);
    claim.sample(2u,0x12u,0x14u,4u,0u,true,0x0Au,true,
                 true,0x1234u,1u);
    claim.sample(3u,0x12u,0x14u,5u,0u,true,0x0Au,true,
                 true,0x1234u,1u);
    assert(claim.sample(4u,0x12u,0x14u,1u,0x12u,true,0x0Au,
                        true,true,0x1234u,1u)==0x32u);
    assert(claim.phase==CLAIMED && claim.d6==0x0Cu &&
           claim.selected==0u && claim.x==0x1234u);
    assert(claim.claimedAt==4u);
    // Native MP+MK is reserve A. Native MP+HK is *absent reserve B*,
    // not a made-up alternate kick super selector.
    Slot unavailable;
    unavailable.sample(1u,0x12u,0x14u,0u,0u,true,0x0Au,
                       true,true,0u,1u);
    unavailable.sample(2u,0x12u,0x14u,4u,0u,true,0x0Au,
                       true,true,0u,1u);
    unavailable.sample(3u,0x12u,0x14u,5u,0u,true,0x0Au,
                       true,true,0u,1u);
    unavailable.sample(4u,0x12u,0x14u,1u,0x22u,true,0x0Au,
                       true,true,0u,1u);
    assert(unavailable.phase==IDLE && unavailable.refusals==1u);
    // Ken's donor-assigned QCB DHC names Shinryuken (D6 0E).
    Slot shinryu;
    shinryu.sample(1u,0x12u,0x14u,0u,0u,true,0x0Au,
                   true,true,0u,1u);
    shinryu.sample(2u,0x12u,0x14u,4u,0u,true,0x0Au,
                   true,true,0u,1u);
    shinryu.sample(3u,0x12u,0x14u,6u,0u,true,0x0Au,
                   true,true,0u,1u);
    shinryu.sample(4u,0x12u,0x14u,2u,0x12u,true,0x0Au,
                   true,true,0u,1u);
    assert(shinryu.phase==CLAIMED && shinryu.d6==0x0Eu &&
           shinryu.selected==1u);
    std::uint8_t guest[4096]={};
    assert(!emitExit(nullptr,sizeof(guest),0x200u));
    assert(emitExit(guest,sizeof(guest),kExitOffsetP1));
    assert(emitExit(guest,sizeof(guest),kExitOffsetP2));
    assert(word(guest,0x200u)==0x48E7u);
    assert(word(guest,0x202u)==0xFFFEu);  // A5 preserved by MOVEM
    assert(word(guest,0x204u)==0x7004u);
    assert(word(guest,0x206u)==0x4EB9u);
    assert(word(guest,0x208u)==0x0004u &&
           word(guest,0x20Au)==0xBF66u); // verified original US helper
    assert(word(guest,0x20Cu)==0x4CDFu &&
           word(guest,0x20Eu)==0x7FFFu);
    assert(word(guest,0x210u)==0x1D7Cu &&
           word(guest,0x212u)==0x0002u &&
           word(guest,0x214u)==0x0256u);
    assert(word(guest,0x216u)==0x1D7Cu &&
           word(guest,0x218u)==0x0002u &&
           word(guest,0x21Au)==0x0259u);
    assert(word(guest,0x21Cu)==0x70FFu &&
           word(guest,0x21Eu)==0x4E75u);
    std::vector<std::uint8_t> original(0x50000u);
    const std::uint8_t site[]={
        0x30,0x3C,0x00,0x02,0x4E,0xB9,0x00,0x04,0xBF,0x66
    };
    be(original,0xD590u,site,sizeof(site));
    assert(validateExit(original.data(),unsigned(original.size())));
    original[0xD590u]^=1u;
    assert(!validateExit(original.data(),unsigned(original.size())));
    std::puts("XMVSF 87.340 Morphus source motion/native 2v2 exit PASS");
}
