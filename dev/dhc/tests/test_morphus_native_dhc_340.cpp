// 87.341 standalone C++11 test: actual incoming controls select real D6.
// Local only. Never a BUILD_TEST gate or GitHub Action.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_340_morphus_native_2v2.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_335_native_dhc_guest.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_333_morphus_native_dhc_abi.h"
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
    // Real native controls, not Morphus-assigned DHC gestures.
    // One selected incoming command -> one exact +D6 byte.
    using XM87334Native::resolveNativeOnly;
    assert(resolveNativeOnly(0x14u,1u,0x03u).hyperD6==0x0Cu); // Ken QCF+PP
    assert(resolveNativeOnly(0x14u,1u,0x18u).hyperD6==0x0Eu); // Ken QCF+KK
    assert(!resolveNativeOnly(0x14u,2u,0x18u).ok());  // no invented QCB Shinryuken
    assert(!resolveNativeOnly(0x14u,1u,0x12u).ok());  // MP+MK is not a Hyper chord
    assert(resolveNativeOnly(0x02u,1u,0x03u).hyperD6==0x04u); // Cyclops
    assert(resolveNativeOnly(0x04u,1u,0x03u).hyperD6==0x02u); // Storm
    assert(resolveNativeOnly(0x0Eu,1u,0x18u).hyperD6==0x06u); // Magneto
    assert(resolveNativeOnly(0x22u,1u,0x18u).hyperD6==0x0Eu); // Cammy
    assert(!resolveNativeOnly(0x1Au,1u,0x03u).ok()); // no fake QCF for Zangief 360
    assert(!resolveNativeOnly(0x1Eu,3u,0x03u).ok()); // no fake DP for Akuma SGS
    assert(facingDir(1u,true)==2u && facingDir(2u,true)==1u);
    assert(facingDir(5u,true)==6u && facingDir(6u,true)==5u);

    const auto runMotion=[](Slot& slot,unsigned target,
        unsigned start,unsigned b,unsigned fD6){
        // Initial neutral sample owns the tracked side. Real QCF:
        // D, DF, F, then the native PP/KK chord at the terminal F.
        slot.sample(start,0x12u,target,0u,0u,true,fD6,true,true,0x1234u,1u);
        slot.sample(start+1u,0x12u,target,4u,0u,true,fD6,true,true,0x1234u,1u);
        slot.sample(start+2u,0x12u,target,5u,0u,true,fD6,true,true,0x1234u,1u);
        return slot.sample(start+3u,0x12u,target,1u,b,true,
                           fD6,true,true,0x1234u,1u);
    };
    Slot shoryu;
    assert(runMotion(shoryu,0x14u,1u,0x03u,0x0Au)==0x03u);
    assert(shoryu.phase==CLAIMED && shoryu.d6==0x0Cu &&
           shoryu.selected==0u && shoryu.outgoingD6==0x0Au &&
           shoryu.x==0x1234u && shoryu.stock==1u);
    assert(shoryu.sample(5u,0x12u,0x14u,1u,0x03u,true,
                         0x0Au,true,true,0x1234u,1u)==0x03u);
    assert(shoryu.sample(6u,0x12u,0x14u,0u,0u,true,
                         0x0Au,true,true,0x1234u,1u)==0u);
    Slot shinryu;
    assert(runMotion(shinryu,0x14u,10u,0x18u,0x0Au)==0x18u);
    assert(shinryu.phase==CLAIMED && shinryu.d6==0x0Eu &&
           shinryu.selected==0u && shinryu.claimedAt==13u);
    // A DHC-style MP+MK chord is never a trigger, even with QCF complete.
    Slot assist;
    assert(runMotion(assist,0x14u,20u,0x12u,0x0Au)==0u);
    assert(assist.phase==IDLE && assist.d6==0u && assist.hidden==0u);
    // The old donor-assigned QCB+MP+MK was not Ken's actual Shinryuken.
    Slot falseQcb;
    falseQcb.sample(30u,0x12u,0x14u,0u,0u,true,0x0Au,true,true,0u,1u);
    falseQcb.sample(31u,0x12u,0x14u,4u,0u,true,0x0Au,true,true,0u,1u);
    falseQcb.sample(32u,0x12u,0x14u,6u,0u,true,0x0Au,true,true,0u,1u);
    assert(falseQcb.sample(33u,0x12u,0x14u,2u,0x12u,true,
                           0x0Au,true,true,0u,1u)==0u);
    assert(falseQcb.phase==IDLE);
    Slot storm;
    assert(runMotion(storm,0x04u,40u,0x03u,0x0Au)==0x03u);
    assert(storm.d6==0x02u);
    Slot magneto;
    assert(runMotion(magneto,0x0Eu,50u,0x18u,0x0Au)==0x18u);
    assert(magneto.d6==0x06u);
    Slot cyclops;
    assert(runMotion(cyclops,0x02u,60u,0x03u,0x0Au)==0x03u);
    assert(cyclops.d6==0x04u);
    // 87.343: position against ACTUAL opponent X, for P1 or P2.
    assert(nearOpponentX(80u,220u)==204u);  // P1 approaches from left
    assert(nearOpponentX(360u,220u)==236u); // P2 approaches from right
    assert(nearOpponentX(210u,220u)==210u); // already close: do not move
    assert(nearOpponentX(228u,220u)==228u);
    assert(nearOpponentX(0u,8u)==0u);       // safe stage-left clamp
    assert(nearOpponentX(65530u,65520u)==65530u);
    assert(nearOpponentX(65535u,65500u)==65516u);
    // All fighter IDs use the same stock original US actor dispatcher.
    // A genuine native 2E ROM table and an actual incoming PP/KK
    // selection are required by the bridge before selecting this route.
    std::uint8_t stockGuest[4096]={};
    assert(XM87335Guest::emit(stockGuest,sizeof(stockGuest),0u,
        0x28u,nearOpponentX(80u,220u),
        XM87333NativeDHC::kUSIncoming)); // P2 Rogue D6=28
    assert(XM87335Guest::isImage(stockGuest,sizeof(stockGuest),0u,
        0x28u,204u,XM87333NativeDHC::kUSIncoming));
    assert(!XM87335Guest::isImage(stockGuest,sizeof(stockGuest),0u,
        0x28u,204u,0x035356u)); // not Ken's per-fighter handler
    // Host-swapped immediate X is now 204, not the outgoing X=80.
    assert(word(stockGuest,0x20u)==0x00CCu);
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
    std::puts("XMVSF 87.343 both-side opponent X and original US fighter dispatcher PASS");
}
