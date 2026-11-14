// 87.341 standalone C++11 test: actual incoming controls select real D6.
// Local only. Never a BUILD_TEST gate or GitHub Action.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_340_morphus_native_2v2.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_335_native_dhc_guest.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_333_morphus_native_dhc_abi.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_189_native_super_family.h"
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
    // 87.346: imported ORIGINAL Cyclops native Super command, not the
    // donor's QCB-assigned DHC gesture. F -> DF -> D + PP or PPP.
    assert(resolveNativeOnly(0x02u,8u,0x03u).hyperD6==0x02u);
    assert(resolveNativeOnly(0x02u,8u,0x07u).hyperD6==0x02u);
    assert(resolveNativeOnly(0x02u,1u,0x07u).hyperD6==0x04u);
    assert(!resolveNativeOnly(0x14u,8u,0x07u).ok());
    assert(!resolveNativeOnly(0x02u,8u,0x38u).ok());
    Slot cyclopsAlternative;
    cyclopsAlternative.sample(70u,0x12u,0x02u,0u,0u,true,0x0Au,
                              true,true,100u,1u);
    cyclopsAlternative.sample(71u,0x12u,0x02u,1u,0u,true,0x0Au,
                              true,true,100u,1u); // F
    cyclopsAlternative.sample(72u,0x12u,0x02u,5u,0u,true,0x0Au,
                              true,true,100u,1u); // DF
    assert(cyclopsAlternative.sample(73u,0x12u,0x02u,4u,0x07u,
              true,0x0Au,true,true,100u,1u)==0x07u); // D+PPP
    assert(cyclopsAlternative.phase==CLAIMED &&
           cyclopsAlternative.selected==7u &&
           cyclopsAlternative.d6==0x02u);

    Slot cyclopsP2;
    cyclopsP2.sample(80u,0x12u,0x02u,0u,0u,false,0x0Au,
                     true,true,100u,1u);
    cyclopsP2.sample(81u,0x12u,0x02u,2u,0u,false,0x0Au,
                     true,true,100u,1u);
    cyclopsP2.sample(82u,0x12u,0x02u,6u,0u,false,0x0Au,
                     true,true,100u,1u);
    assert(cyclopsP2.sample(83u,0x12u,0x02u,4u,0x03u,
            false,0x0Au,true,true,100u,1u)==0x03u);
    assert(cyclopsP2.d6==0x02u && cyclopsP2.selected==7u);

    // Original +$210 is real reserve life. Fighter ID 00 is Wolverine:
    // a living Wolverine may be a valid bench, an empty/dead 00 is not.
    assert(!genuineReserve(0x12u,0x00u,0u));
    assert(genuineReserve(0x12u,0x00u,100u));
    assert(!genuineReserve(0x02u,0x02u,100u));
    assert(!genuineReserve(0x12u,0x03u,100u));
    Slot solo;
    solo.sample(90u,0x12u,0u,0u,0u,true,0x0Au,
                false,false,100u,1u);
    solo.sample(91u,0x12u,0u,1u,0u,true,0x0Au,
                false,false,100u,1u);
    solo.sample(92u,0x12u,0u,5u,0u,true,0x0Au,
                false,false,100u,1u);
    assert(solo.sample(93u,0x12u,0u,4u,0x07u,true,0x0Au,
                       false,false,100u,1u)==0u);
    assert(solo.phase==IDLE && solo.hidden==0u);
    // 87.350: original donor dhc_commands.json native_command entries:
    // Akuma LP,LP,F,LK,HP D6=10; Zangief 360+PP D6=0A.
    assert(resolveNativeOnly(0x1Eu,10u,0x04u).hyperD6==0x10u);
    assert(!resolveNativeOnly(0x1Eu,10u,0x03u).ok());
    assert(!resolveNativeOnly(0x14u,10u,0x04u).ok());
    assert(resolveNativeOnly(0x1Au,9u,0x03u).hyperD6==0x0Au);
    assert(resolveNativeOnly(0x1Au,9u,0x07u).hyperD6==0x0Au);
    assert(!resolveNativeOnly(0x1Au,9u,0x38u).ok());
    assert(!resolveNativeOnly(0x14u,9u,0x07u).ok());
    // A single HP, LP/MP or ordinary QCF must NEVER impersonate
    // Raging Demon. Only both LP rising edges and F, LK, HP do.
    const auto ragingDemon=[](Slot& actor,bool right,
                              unsigned start,bool active){
        const unsigned f=right?1u:2u;
        actor.sample(start,0x12u,0x1Eu,0u,0u,right,0x0Au,
                     active,active,220u,1u);
        actor.sample(start+1u,0x12u,0x1Eu,0u,1u,right,0x0Au,
                     active,active,220u,1u); // LP
        actor.sample(start+2u,0x12u,0x1Eu,0u,0u,right,0x0Au,
                     active,active,220u,1u); // release LP
        actor.sample(start+3u,0x12u,0x1Eu,0u,1u,right,0x0Au,
                     active,active,220u,1u); // LP again
        actor.sample(start+4u,0x12u,0x1Eu,f,0u,right,0x0Au,
                     active,active,220u,1u); // F
        actor.sample(start+5u,0x12u,0x1Eu,f,8u,right,0x0Au,
                     active,active,220u,1u); // LK
        actor.sample(start+6u,0x12u,0x1Eu,f,0u,right,0x0Au,
                     active,active,220u,1u);
        return actor.sample(start+7u,0x12u,0x1Eu,f,4u,right,
                     0x0Au,active,active,220u,1u); // SINGLE HP
    };
    Slot demonP1;
    assert(ragingDemon(demonP1,true,300u,true)==0x04u);
    assert(demonP1.phase==CLAIMED && demonP1.selected==9u &&
           demonP1.d6==0x10u && demonP1.hidden==4u);
    Slot demonP2;
    assert(ragingDemon(demonP2,false,310u,true)==0x04u);
    assert(demonP2.phase==CLAIMED && demonP2.d6==0x10u);
    Slot demonDuringNormal;
    assert(ragingDemon(demonDuringNormal,true,320u,false)==0u);
    assert(demonDuringNormal.phase==IDLE &&
           demonDuringNormal.hidden==0u);
    Slot onlyHp;
    onlyHp.sample(330u,0x12u,0x1Eu,0u,0u,true,0x0Au,
                  true,true,220u,1u);
    assert(onlyHp.sample(331u,0x12u,0x1Eu,1u,4u,true,0x0Au,
                         true,true,220u,1u)==0u);
    Slot noSecondLP;
    noSecondLP.sample(335u,0x12u,0x1Eu,0u,0u,true,0x0Au,
                      true,true,220u,1u);
    noSecondLP.sample(336u,0x12u,0x1Eu,0u,1u,true,0x0Au,
                      true,true,220u,1u);
    noSecondLP.sample(337u,0x12u,0x1Eu,1u,0u,true,0x0Au,
                      true,true,220u,1u);
    noSecondLP.sample(338u,0x12u,0x1Eu,1u,8u,true,0x0Au,
                      true,true,220u,1u);
    assert(noSecondLP.sample(339u,0x12u,0x1Eu,1u,4u,true,
              0x0Au,true,true,220u,1u)==0u);
    // FAB native 360: F,DF,D,DB,B,UB,U,UF (or reverse).
    // P1 right and P2 left both use the same RELATIVE original motion.
    const auto atomicBuster=[](Slot actor,bool right,
                                unsigned start,bool reverse){
        const unsigned clockwiseRight[]={
            1u,5u,4u,6u,2u,10u,8u,9u};
        const unsigned clockwiseLeft[]={
            2u,6u,4u,5u,1u,9u,8u,10u};
        const unsigned reverseRight[]={
            2u,6u,4u,5u,1u,9u,8u,10u};
        const unsigned reverseLeft[]={
            1u,5u,4u,6u,2u,10u,8u,9u};
        // Opposite rotation begins B, then DB,D,DF,F,UF,U,UB.
        const unsigned* sequence=right?
            (reverse?reverseRight:clockwiseRight):
            (reverse?reverseLeft:clockwiseLeft);
        actor.sample(start,0x12u,0x1Au,0u,0u,right,0x0Au,
                     true,true,220u,1u);
        for(unsigned i=0u;i<8u;++i){
            const unsigned mask=i==7u?0x07u:0u;
            actor.sample(start+i+1u,0x12u,0x1Au,
                         sequence[i],mask,right,0x0Au,
                         true,true,220u,1u);
        }
        return actor;
    };
    Slot fabP1=atomicBuster(Slot(),true,350u,false);
    assert(fabP1.phase==CLAIMED && fabP1.selected==8u &&
           fabP1.d6==0x0Au && fabP1.hidden==0x07u);
    Slot fabP2=atomicBuster(Slot(),false,370u,false);
    assert(fabP2.phase==CLAIMED && fabP2.d6==0x0Au);
    Slot fabReverse=atomicBuster(Slot(),true,390u,true);
    assert(fabReverse.phase==CLAIMED && fabReverse.d6==0x0Au);
    Slot fabP2Reverse=atomicBuster(Slot(),false,410u,true);
    assert(fabP2Reverse.phase==CLAIMED && fabP2Reverse.d6==0x0Au);
    Slot notAFAB;
    notAFAB.sample(430u,0x12u,0x1Au,0u,0u,true,0x0Au,
                   true,true,220u,1u);
    // QCF+PPP cannot turn into Zangief's 360 command.
    notAFAB.sample(431u,0x12u,0x1Au,4u,0u,true,0x0Au,
                   true,true,220u,1u);
    notAFAB.sample(432u,0x12u,0x1Au,5u,0u,true,0x0Au,
                   true,true,220u,1u);
    assert(notAFAB.sample(433u,0x12u,0x1Au,1u,7u,true,
           0x0Au,true,true,220u,1u)==0u);
    assert(notAFAB.phase==IDLE);
    // 87.351: original reverse commands, both facings and fast sweeps.
    const auto backward=[](unsigned fighter,bool right,bool half,
                           unsigned skip)->Slot{
        Slot s;
        const unsigned f=right?1u:2u,df=right?5u:6u;
        const unsigned db=right?6u:5u,b=right?2u:1u;
        const unsigned qcb[3]={4u,db,b},hcb[5]={f,df,4u,db,b};
        const unsigned n=half?5u:3u;
        s.sample(500u,0x12u,fighter,0u,0u,right,0x0Au,
                 true,true,220u,1u);
        for(unsigned i=0u;i<n;++i){
            if(i==skip)continue;
            const unsigned attacks=i==n-1u?
                ((fighter==0x04u||fighter==0x1Eu)?0x03u:0x18u):0u;
            s.sample(501u+i,0x12u,fighter,half?hcb[i]:qcb[i],
                     attacks,right,0x0Au,true,true,220u,1u);
        }
        return s;
    };
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned f[]={0x04u,0x12u,0x16u,0x20u,0x22u,0x1Eu};
        const unsigned d6[]={0x0Au,0x0Cu,0x0Au,0x06u,0x16u,0x1Au};
        for(unsigned i=0u;i<6u;++i){
            const Slot exact=backward(f[i],right,false,~0u);
            const Slot skipped=backward(f[i],right,false,1u);
            assert(exact.phase==CLAIMED && exact.selected==1u &&
                   exact.d6==d6[i]);
            assert(skipped.phase==CLAIMED && skipped.d6==d6[i]);
        }
        const Slot hcb=backward(0x0Au,right,true,~0u);
        const Slot skipDF=backward(0x0Au,right,true,1u);
        const Slot skipDB=backward(0x0Au,right,true,3u);
        assert(hcb.phase==CLAIMED && hcb.selected==5u && hcb.d6==0x08u);
        assert(skipDF.phase==CLAIMED && skipDF.d6==0x08u);
        assert(skipDB.phase==CLAIMED && skipDB.d6==0x08u);
        Slot notHcb;
        const unsigned forward=right?1u:2u,back=right?2u:1u;
        notHcb.sample(600u,0x12u,0x0Au,0u,0u,right,0x0Au,
                      true,true,220u,1u);
        notHcb.sample(601u,0x12u,0x0Au,forward,0u,right,0x0Au,
                      true,true,220u,1u);
        notHcb.sample(615u,0x12u,0x0Au,4u,0u,right,0x0Au,
                      true,true,220u,1u);
        notHcb.sample(616u,0x12u,0x0Au,back,0x18u,right,0x0Au,
                      true,true,220u,1u);
        assert(notHcb.phase==IDLE); // Slow F,D,B cannot fake an HCB.
    }
    // 87.352: genuine short sampled reverse inputs, not motion aliases.
    // The identical native D6 is selected on both physical facings.
    for(unsigned side=0u;side<2u;++side){
        const bool right=side==0u;
        const unsigned forward=right?1u:2u,back=right?2u:1u;
        const unsigned start=2200u+side*100u;
        Slot hcbFast;
        hcbFast.sample(start,0x12u,0x0Au,0u,0u,right,
                       0x0Au,true,true,220u,1u);
        hcbFast.sample(start+1u,0x12u,0x0Au,forward,0u,right,
                       0x0Au,true,true,220u,1u);
        hcbFast.sample(start+2u,0x12u,0x0Au,4u,0u,right,
                       0x0Au,true,true,220u,1u);
        assert(hcbFast.sample(start+3u,0x12u,0x0Au,back,0x18u,
                       right,0x0Au,true,true,220u,1u)==0x18u);
        assert(hcbFast.phase==CLAIMED && hcbFast.selected==5u &&
               hcbFast.d6==0x08u);
        Slot hcbLate;
        hcbLate.sample(start,0x12u,0x0Au,0u,0u,right,
                       0x0Au,true,true,220u,1u);
        hcbLate.sample(start+1u,0x12u,0x0Au,forward,0u,right,
                       0x0Au,true,true,220u,1u);
        hcbLate.sample(start+12u,0x12u,0x0Au,4u,0u,right,
                       0x0Au,true,true,220u,1u);
        assert(hcbLate.sample(start+15u,0x12u,0x0Au,back,0x18u,
                       right,0x0Au,true,true,220u,1u)==0u);
        assert(hcbLate.phase==IDLE);
        Slot opticFast;
        opticFast.sample(start,0x12u,0x02u,0u,0u,right,
                         0x0Au,true,true,220u,1u);
        opticFast.sample(start+1u,0x12u,0x02u,forward,0u,right,
                         0x0Au,true,true,220u,1u);
        assert(opticFast.sample(start+2u,0x12u,0x02u,4u,0x03u,
                         right,0x0Au,true,true,220u,1u)==0x03u);
        assert(opticFast.phase==CLAIMED && opticFast.selected==7u &&
               opticFast.d6==0x02u);
    }
    // 87.353: KKK may be pressed just BEFORE a final sampled direction.
    // The true QCF/QCB/HCB must still complete; D6 never changes.
    assert(resolveNativeOnly(0x0Au,6u,0x38u).hyperD6==0x08u); // Sabretooth
    assert(resolveNativeOnly(0x16u,1u,0x38u).hyperD6==0x08u); // Chun-Li
    assert(resolveNativeOnly(0x20u,2u,0x38u).hyperD6==0x06u); // Charlie
    const auto earlyKicks=[](unsigned reserve,bool right,unsigned start,
                            unsigned motion,bool early)->Slot{
        Slot s;
        const unsigned f=right?1u:2u,df=right?5u:6u;
        const unsigned b=right?2u:1u,db=right?6u:5u;
        const unsigned qcf[]={4u,df,f};
        const unsigned qcb[]={4u,db,b};
        const unsigned hcb[]={f,df,4u,db,b};
        const unsigned* d=motion==1u?qcf:(motion==2u?qcb:hcb);
        const unsigned n=motion==6u?5u:3u;
        s.sample(start,0x12u,reserve,0u,0u,right,
                 0x0Au,true,true,220u,1u);
        for(unsigned i=0u;i<n;++i){
            const unsigned btn=i==n-1u|| (early && i==n-2u)?
                               0x38u:0u;
            s.sample(start+i+1u,0x12u,reserve,d[i],btn,right,
                     0x0Au,true,true,220u,1u);
        }
        return s;
    };
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned targets[]={0x0Au,0x16u,0x20u};
        const unsigned motions[]={6u,1u,2u};
        const unsigned nativeD6[]={0x08u,0x08u,0x06u};
        for(unsigned j=0u;j<3u;++j){
            const unsigned at=2600u+facing*120u+j*25u;
            const Slot exact=earlyKicks(targets[j],right,at,
                                        motions[j],false);
            const Slot buffered=earlyKicks(targets[j],right,at+400u,
                                           motions[j],true);
            assert(exact.phase==CLAIMED && exact.d6==nativeD6[j] &&
                   !exact.claimedBufferedKKK);
            assert(buffered.phase==CLAIMED &&
                   buffered.d6==nativeD6[j] &&
                   buffered.claimedBufferedKKK);
        }
        // A held KKK without a completed direction cannot pick D6.
        Slot noMotion;
        noMotion.sample(3500u,0x12u,0x20u,0u,0u,right,
                        0x0Au,true,true,220u,1u);
        noMotion.sample(3501u,0x12u,0x20u,0u,0x38u,right,
                        0x0Au,true,true,220u,1u);
        noMotion.sample(3502u,0x12u,0x20u,0u,0x38u,right,
                        0x0Au,true,true,220u,1u);
        assert(noMotion.phase==IDLE);
        // Expired KKK may NOT create a DHC after a delayed QCF.
        Slot expired;
        const unsigned df=right?5u:6u,forward=right?1u:2u;
        expired.sample(3600u,0x12u,0x16u,0u,0u,right,
                       0x0Au,true,true,220u,1u);
        expired.sample(3601u,0x12u,0x16u,0u,0x38u,right,
                       0x0Au,true,true,220u,1u);
        expired.sample(3612u,0x12u,0x16u,4u,0x38u,right,
                       0x0Au,true,true,220u,1u);
        expired.sample(3613u,0x12u,0x16u,df,0x38u,right,
                       0x0Au,true,true,220u,1u);
        expired.sample(3614u,0x12u,0x16u,forward,0x38u,right,
                       0x0Au,true,true,220u,1u);
        assert(expired.phase==IDLE);
        // A newly released KKK must also lose the buffered edge.
        Slot released;
        released.sample(3700u,0x12u,0x20u,0u,0u,right,
                        0x0Au,true,true,220u,1u);
        released.sample(3701u,0x12u,0x20u,4u,0x38u,right,
                        0x0Au,true,true,220u,1u);
        released.sample(3702u,0x12u,0x20u,right?6u:5u,0u,right,
                        0x0Au,true,true,220u,1u);
        released.sample(3703u,0x12u,0x20u,right?2u:1u,0u,right,
                        0x0Au,true,true,220u,1u);
        assert(released.phase==IDLE);
    }
    // 87.354: Charlie's normal Flash Kick must not seed a DHC.
    Slot normal;
    normal.quiet(100u,0x20u,0x38u);
    normal.quiet(101u,0x20u,0x38u);
    normal.quiet(102u,0x20u,0u);
    assert(normal.phase==IDLE && !normal.windowReady &&
           normal.hidden==0u && normal.kkkEdgeAt==~0u);
    for(unsigned i=0u;i<8u;++i)
        assert(normal.step[i]==0u && normal.timer[i]==0u);
    normal.openWindow(103u,0x20u,0x38u);
    assert(normal.windowReady && normal.prev==0x38u);
    assert(normal.sample(104u,0x20u,0x16u,0u,0x38u,
           true,0x06u,true,true,220u,1u)==0u);
    assert(normal.phase==IDLE && normal.kkkEdgeAt==~0u);
    normal.sample(105u,0x20u,0x16u,0u,0u,
                  true,0x06u,true,true,220u,1u);
    normal.sample(106u,0x20u,0x16u,4u,0u,
                  true,0x06u,true,true,220u,1u);
    normal.sample(107u,0x20u,0x16u,5u,0u,
                  true,0x06u,true,true,220u,1u);
    assert(normal.sample(108u,0x20u,0x16u,1u,0x38u,
           true,0x06u,true,true,220u,1u)==0x38u);
    assert(normal.phase==CLAIMED && normal.d6==0x08u);
    normal.pendingInput(109u,0u);
    assert(normal.phase==CLAIMED && normal.hidden==0u);
    normal.phase=IDLE;
    normal.quiet(110u,0x20u,0x38u);
    assert(!normal.windowReady && normal.kkkEdgeAt==~0u);
    for(unsigned i=0u;i<8u;++i)
        assert(normal.step[i]==0u && normal.timer[i]==0u);
    // 87.355: open Charlie QCB+KKK and Sabretooth HCB+KKK
    // from the very FIRST native Hyper cancel frame, both facings.
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned f=right?1u:2u;
        const unsigned df=right?5u:6u;
        const unsigned db=right?6u:5u;
        const unsigned back=right?2u:1u;
        // Normal play never samples the QCB motion before arming.
        Slot charlie;
        charlie.quiet(5000u,0x16u,0u);
        charlie.openWindow(5001u,0x16u,0u,0x20u,4u,right);
        assert(charlie.windowReady && charlie.step[1]==1u);
        charlie.sample(5002u,0x16u,0x20u,db,0u,right,
                       0x0Au,true,true,220u,1u);
        assert(charlie.sample(5003u,0x16u,0x20u,back,0x38u,right,
                       0x0Au,true,true,220u,1u)==0x38u);
        assert(charlie.phase==CLAIMED && charlie.selected==1u &&
               charlie.d6==0x06u);
        // Sabretooth original full HCB uses all five native sectors.
        Slot sabre;
        sabre.quiet(5100u,0x16u,0u);
        sabre.openWindow(5101u,0x16u,0u,0x0Au,f,right);
        assert(sabre.windowReady && sabre.step[5]==1u);
        sabre.sample(5102u,0x16u,0x0Au,df,0u,right,
                     0x0Au,true,true,220u,1u);
        sabre.sample(5103u,0x16u,0x0Au,4u,0u,right,
                     0x0Au,true,true,220u,1u);
        sabre.sample(5104u,0x16u,0x0Au,db,0u,right,
                     0x0Au,true,true,220u,1u);
        assert(sabre.sample(5105u,0x16u,0x0Au,back,0x38u,right,
                     0x0Au,true,true,220u,1u)==0x38u);
        assert(sabre.phase==CLAIMED && sabre.selected==5u &&
               sabre.d6==0x08u);
        // KKK held before this Hyper cannot silently start a tag.
        Slot heldBefore;
        heldBefore.quiet(5200u,0x16u,0x38u);
        heldBefore.openWindow(5201u,0x16u,0x38u,0x20u,4u,right);
        heldBefore.sample(5202u,0x16u,0x20u,db,0x38u,right,
                          0x0Au,true,true,220u,1u);
        heldBefore.sample(5203u,0x16u,0x20u,back,0x38u,right,
                          0x0Au,true,true,220u,1u);
        assert(heldBefore.phase==IDLE && heldBefore.hidden==0u);
        heldBefore.quiet(5204u,0x16u,0x38u);
        assert(!heldBefore.windowReady && heldBefore.step[1]==0u);
        // No normal-play sniff: a supposed HCB without a verified
        // Hyper window must never enter the native command parser.
        Slot dormant;
        dormant.quiet(5300u,0x16u,0u);
        dormant.quiet(5301u,0x16u,0u);
        dormant.quiet(5302u,0x16u,0x38u);
        assert(!dormant.windowReady && dormant.phase==IDLE &&
               dormant.step[5]==0u && dormant.kkkEdgeAt==~0u);
    }
    // 87.356: Charlie's genuine QCB+KKK may start while the actor is
    // already in a verified native Hyper but the final native cancel
    // controller is not yet eligible. It must not claim until eligible.
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned db=right?6u:5u,back=right?2u:1u;
        Slot delayedCharlie;
        delayedCharlie.quiet(6000u,0x16u,0u);
        delayedCharlie.openWindow(6001u,0x16u,0u,0x20u,4u,right);
        assert(delayedCharlie.windowReady &&
               delayedCharlie.step[1]==1u);
        assert(delayedCharlie.sample(6002u,0x16u,0x20u,db,0u,right,
                 0x0Au,true,false,220u,1u)==0u);
        assert(delayedCharlie.step[1]==2u &&
               delayedCharlie.phase==IDLE);
        // The final KKK is physical but still cannot swap while the
        // original M68K Hyper is not DHC-cancellable.
        assert(delayedCharlie.sample(6003u,0x16u,0x20u,back,
                 0x38u,right,0x0Au,true,false,220u,1u)==0u);
        assert(delayedCharlie.step[1]==3u &&
               delayedCharlie.phase==IDLE &&
               delayedCharlie.hidden==0u);
        // Same held KKK edge is still fresh when the original cancel
        // gate opens one frame later: select only Charlie's D6=06.
        assert(delayedCharlie.sample(6004u,0x16u,0x20u,back,
                 0x38u,right,0x0Au,true,true,220u,1u)==0x38u);
        assert(delayedCharlie.phase==CLAIMED &&
               delayedCharlie.selected==1u &&
               delayedCharlie.d6==0x06u &&
               delayedCharlie.claimedBufferedKKK);
        // Physical KKK held before any native Super cannot produce a
        // fresh claim when the native window first becomes available.
        Slot heldCharlie;
        heldCharlie.quiet(6100u,0x16u,0x38u);
        heldCharlie.openWindow(6101u,0x16u,0x38u,
                               0x20u,4u,right);
        heldCharlie.sample(6102u,0x16u,0x20u,db,
                           0x38u,right,0x0Au,true,false,220u,1u);
        heldCharlie.sample(6103u,0x16u,0x20u,back,
                           0x38u,right,0x0Au,true,true,220u,1u);
        assert(heldCharlie.phase==IDLE && heldCharlie.hidden==0u &&
               heldCharlie.kkkEdgeAt==~0u);
        // A real exit to normal combat clears the entire cue.
        heldCharlie.quiet(6104u,0x16u,0u);
        assert(!heldCharlie.windowReady && heldCharlie.step[1]==0u);
    }
    // 87.357: original Charlie QCB+KKK is recognized during Hyper,
    // not delayed until the final native DHC window. Both facings;
    // no input held or DHC written during normal play.
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned db=right?6u:5u,back=right?2u:1u;
        Slot q;
        q.quiet(7000u,0x0Eu,0u);
        q.openWindow(7001u,0x0Eu,0u,0x20u,4u,right);
        q.sample(7002u,0x0Eu,0x20u,db,0u,right,
                 0x06u,true,false,220u,0u);
        assert(q.sample(7003u,0x0Eu,0x20u,back,0x38u,
                 right,0x06u,true,false,220u,0u)==0u);
        assert(q.charlieQueued && q.phase==IDLE &&
               q.charlieQueuedD6==0x06u && !q.hidden);
        // Let KKK go; original cancel gate has not opened yet.
        for(unsigned n=7004u;n<7059u;++n){
            assert(q.sample(n,0x0Eu,0x20u,back,0u,
                   right,0x06u,true,false,220u,0u)==0u);
            assert(q.phase==IDLE && q.charlieQueued);
        }
        // Only actual stock+native ROM cancellation authorizes tag.
        assert(q.sample(7059u,0x0Eu,0x20u,back,0u,
                right,0x06u,true,true,220u,1u)==0u);
        assert(q.phase==CLAIMED && q.d6==0x06u &&
               q.selected==1u && q.claimedQueuedCharlie &&
               q.hidden==0u && !q.charlieQueued);
        Slot wrongHyper;
        wrongHyper.quiet(7100u,0x0Eu,0u);
        wrongHyper.openWindow(7101u,0x0Eu,0u,0x20u,4u,right);
        wrongHyper.sample(7102u,0x0Eu,0x20u,db,0u,right,
                          0x06u,true,false,220u,1u);
        wrongHyper.sample(7103u,0x0Eu,0x20u,back,0x38u,right,
                          0x06u,true,false,220u,1u);
        assert(wrongHyper.charlieQueued);
        wrongHyper.sample(7104u,0x0Eu,0x20u,back,0u,right,
                          0x04u,true,true,220u,1u);
        assert(!wrongHyper.charlieQueued &&
               wrongHyper.phase==IDLE);
        Slot normalPlay;
        normalPlay.quiet(7200u,0x0Eu,0u);
        normalPlay.openWindow(7201u,0x0Eu,0u,0x20u,4u,right);
        normalPlay.sample(7202u,0x0Eu,0x20u,db,0u,right,
                          0x06u,true,false,220u,1u);
        normalPlay.sample(7203u,0x0Eu,0x20u,back,0x38u,right,
                          0x06u,true,false,220u,1u);
        assert(normalPlay.charlieQueued);
        normalPlay.quiet(7204u,0x0Eu,0u);
        assert(!normalPlay.charlieQueued && !normalPlay.windowReady);
    }
    // 87.359: exact original Charlie QCB, even if the 68000 facing
    // flag flips between the initial Down and the terminal Back.
    // A mirrored QCF must NOT falsely turn into Charlie QCB.
    for(unsigned facing=0u;facing<2u;++facing){
        const bool right=facing==0u;
        const unsigned db=right?6u:5u,back=right?2u:1u;
        const unsigned df=right?5u:6u,forward=right?1u:2u;
        CharlieQcbSweep exact;
        exact.observe(8100u,4u,right);
        exact.observe(8101u,db,!right);
        exact.observe(8102u,back,!right);
        assert(exact.complete() && exact.facingAtDown==right);
        CharlieQcbSweep fast;
        fast.observe(8200u,4u,right);
        fast.observe(8201u,back,!right);
        assert(fast.complete());
        CharlieQcbSweep wrong;
        wrong.observe(8300u,4u,right);
        wrong.observe(8301u,df,!right);
        wrong.observe(8302u,forward,!right);
        assert(!wrong.complete());
        Slot charlieFlip;
        charlieFlip.quiet(8400u,0x02u,0u);
        charlieFlip.openWindow(8401u,0x02u,0u,0x20u,4u,right);
        charlieFlip.sample(8402u,0x02u,0x20u,db,0u,!right,
                           0x04u,true,false,220u,1u);
        assert(charlieFlip.sample(8403u,0x02u,0x20u,back,
                           0x38u,!right,0x04u,true,false,
                           220u,1u)==0u);
        assert(charlieFlip.charlieQueued && charlieFlip.phase==IDLE);
        charlieFlip.sample(8404u,0x02u,0x20u,0u,0u,
                           !right,0x04u,true,true,220u,1u);
        assert(charlieFlip.phase==CLAIMED &&
               charlieFlip.d6==0x06u &&
               charlieFlip.claimedQueuedCharlie);
        Slot noReverse;
        noReverse.quiet(8500u,0x02u,0u);
        noReverse.openWindow(8501u,0x02u,0u,0x20u,4u,right);
        noReverse.sample(8502u,0x02u,0x20u,df,0u,!right,
                         0x04u,true,true,220u,1u);
        noReverse.sample(8503u,0x02u,0x20u,forward,0x38u,
                         !right,0x04u,true,true,220u,1u);
        assert(noReverse.phase==IDLE && !noReverse.charlieQueued);
        noReverse.quiet(8504u,0x02u,0u);
        assert(!noReverse.charlieSweep.complete());
    }
    // 87.360: original 2A + native 2C/2E + real D6 alone define
    // the continuous ROM-owned Hyper lifespan, not 2E script windows.
    // Old 87.359 ROM tracker could lose long Senretsukyaku while the
    // original 2E0A / 2E0E / 2E10 and D6=08 still owned the actor.
    const unsigned chunStages[]={0x2E0Au,0x2E0Eu,0x2E10u};
    for(unsigned frame=1u;frame<=536u;++frame){
        const unsigned stage=chunStages[(frame-1u)/179u];
        assert(nativeHyperActive(0x2A02u,stage,0x08u));
        assert(nativeHyperActive(0x2A28u,stage,0x08u));
    }
    const unsigned charlieStages[]={0x2C24u,0x2C26u,0x2C28u};
    for(unsigned i=0u;i<3u;++i)
        assert(nativeHyperActive(0x2A00u,charlieStages[i],0x06u));
    assert(!nativeHyperActive(0x0000u,0x2E0Au,0x08u));
    assert(!nativeHyperActive(0x0002u,0x2E0Au,0x08u));
    assert(!nativeHyperActive(0x2A02u,0x0200u,0x08u));
    assert(!nativeHyperActive(0x2A02u,0x2A00u,0x08u));
    assert(!nativeHyperActive(0x2A02u,0x2E0Au,0x00u));
    // Actual native QCF+KKK for incoming Chun-Li, begun hundreds
    // of frames into an outgoing Charlie super, still chooses D6=08.
    Slot lateChun;
    lateChun.quiet(0u,0x20u,0u);
    lateChun.openWindow(1u,0x20u,0u,0x16u,0u,true);
    for(unsigned n=2u;n<536u;++n){
        assert(lateChun.sample(n,0x20u,0x16u,0u,0u,
                true,0x06u,true,true,220u,3u)==0u);
        assert(lateChun.phase==IDLE);
    }
    lateChun.sample(536u,0x20u,0x16u,4u,0u,
                    true,0x06u,true,true,220u,3u);
    lateChun.sample(537u,0x20u,0x16u,5u,0u,
                    true,0x06u,true,true,220u,3u);
    assert(lateChun.sample(538u,0x20u,0x16u,1u,0x38u,
                true,0x06u,true,true,220u,3u)==0x38u);
    assert(lateChun.phase==CLAIMED && lateChun.d6==0x08u);
    // Actual native QCB+KKK for incoming Charlie, likewise entered
    // late in a continuing Chun-Li native Hyper, chooses D6=06.
    Slot lateCharlie;
    lateCharlie.quiet(0u,0x16u,0u);
    lateCharlie.openWindow(1u,0x16u,0u,0x20u,0u,true);
    for(unsigned n=2u;n<536u;++n)
        lateCharlie.sample(n,0x16u,0x20u,0u,0u,
                           true,0x08u,true,true,220u,3u);
    lateCharlie.sample(536u,0x16u,0x20u,4u,0u,
                       true,0x08u,true,true,220u,3u);
    lateCharlie.sample(537u,0x16u,0x20u,6u,0u,
                       true,0x08u,true,true,220u,3u);
    assert(lateCharlie.sample(538u,0x16u,0x20u,2u,0x38u,
                true,0x08u,true,true,220u,3u)==0x38u);
    assert(lateCharlie.phase==CLAIMED && lateCharlie.d6==0x06u);
    // 87.343: position against ACTUAL opponent X, for P1 or P2.
    assert(nearOpponentX(80u,220u)==204u);  // P1 approaches from left
    assert(nearOpponentX(360u,220u)==236u); // P2 approaches from right
    assert(nearOpponentX(210u,220u)==210u); // already close: do not move
    assert(nearOpponentX(228u,220u)==228u);
    assert(nearOpponentX(0u,8u)==0u);       // safe stage-left clamp
    assert(nearOpponentX(65530u,65520u)==65530u);
    assert(nearOpponentX(65535u,65500u)==65516u);
    // 87.344 regression: original US $011DBE is a FIGHTER TAG-IN
    // INDEXER, not a D6 Hyper dispatcher. Its documented opcodes begin
    // MOVE.W $0052(A6),D0 / ADD.W D0,D0; it must NEVER be
    // substituted for a character-specific D6-to-phase-zero routine.
    const auto* usSites=XM87333NativeDHC::sites();
    assert(usSites[0].address==XM87333NativeDHC::kUSIncoming);
    assert(usSites[0].bytesBE[0]==0x30u &&
           usSites[0].bytesBE[1]==0x2Eu &&
           usSites[0].bytesBE[2]==0x00u &&
           usSites[0].bytesBE[3]==0x52u);
    // Only an original opcode-verified per-fighter Hyper route is valid.
    std::uint8_t ryuGuest[4096]={};
    assert(XM87335Guest::emit(ryuGuest,sizeof(ryuGuest),0u,
         0x0Au,nearOpponentX(80u,220u),0x0337BAu));
    assert(XM87335Guest::isImage(ryuGuest,sizeof(ryuGuest),0u,
         0x0Au,204u,0x0337BAu));
    assert(word(ryuGuest,0x20u)==0x00CCu);
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
    // 87.349: real original actor action 2C is a native Hyper too,
    // even if its M68K phase remains 0/2 (Storm, Sabretooth, Bison,
    // Charlie). Create genuine ROM action2C script tables, not 2E data.
    std::vector<std::uint8_t> rom2C(0x160000u,0u);
    const auto put16=[&](unsigned at,unsigned v){
        rom2C[at]=std::uint8_t(v);
        rom2C[at+1u]=std::uint8_t(v>>8u);
    };
    const auto put32=[&](unsigned at,unsigned v){
        put16(at,v);put16(at+2u,v>>16u);
    };
    const unsigned stormTable=0x104000u;
    const unsigned stormSec=stormTable+0x80u;
    const unsigned stormRoot=stormSec+0x100u;
    const unsigned stormNext=stormSec+0x200u;
    put32(0x8A946u+0x04u*2u,stormTable);
    put16(stormTable+0x2Cu,0x80u);
    put16(stormSec+0x08u,0x100u);
    put16(stormSec+0x0Au,0x200u);
    XM87189Family::Tracker flight;
    XM87184Life::Sample s2c(0x04u,0x2C08u,0x2A00u,2u,
                             stormRoot,0x02u);
    assert(XM87189Family::Tracker::originalSuperController(s2c));
    XM87189Family::Tracker unchanged;
    assert(unchanged.update(100u,s2c,rom2C.data(),
                            unsigned(rom2C.size()))==
           XM87189Family::NOTHING); // legacy 2E-only semantics intact
    assert(!unchanged.live);
    assert(flight.update(100u,s2c,rom2C.data(),
                         unsigned(rom2C.size()),true)==XM87189Family::ENTER);
    assert(flight.native2CWindowAt(100u,s2c));
    s2c.script+=16u;
    assert(flight.update(101u,s2c,rom2C.data(),
                         unsigned(rom2C.size()),true)==
           XM87189Family::ATTACK_BEGIN);
    assert(flight.unifiedDhcWindowAt(101u,s2c));
    s2c.action=0x2C0Au;s2c.script=stormNext;
    assert(flight.update(102u,s2c,rom2C.data(),
                         unsigned(rom2C.size()),true)==
           XM87189Family::CONTINUATION_BEGIN);
    assert(flight.native2CWindowAt(102u,s2c));
    s2c.d6=0x0Au;
    assert(!flight.native2CWindowAt(102u,s2c)); // wrong real Super
    s2c.mode=0u;
    assert(flight.update(103u,s2c,rom2C.data(),
                         unsigned(rom2C.size()),true)==
           XM87189Family::ACTION_EXIT);
    // 2C startup with original step 0 is accepted; only exact ROM
    // action/script may arm this route; fake D6/mode never qualifies.
    XM87189Family::Tracker bison;
    put32(0x8A946u+0x1Cu*2u,0x11C000u);
    put16(0x11C000u+0x2Cu,0x80u);
    put16(0x11C080u+0x2Au,0x100u);
    XM87184Life::Sample s0(0x1Cu,0x2C2Au,0x2A00u,0u,
                            0x11C180u,0x0Cu);
    assert(bison.update(200u,s0,rom2C.data(),
                        unsigned(rom2C.size()),true)==XM87189Family::ENTER);
    assert(bison.unifiedDhcWindowAt(200u,s0));
    assert(!XM87189Family::Tracker::originalSuperController(
         XM87184Life::Sample(0x1Cu,0x2C2Au,0u,0u,
                              0x11C180u,0x0Cu)));
    assert(XM87189Family::Tracker::originalSuperController(
         XM87184Life::Sample(0x12u,0x2E08u,0x2A00u,2u,
                              0x0D3070u,0x0Au))); // 2E unmodified
    // Default enablement must follow the in-game Version 4 selection.
    assert(XM87340Donor::enabled()==XM87262Game::version4());
    assert(XM87334Native::enabled()==XM87262Game::version4());
    std::puts("XMVSF 87.361 original M68K DHC built into Version 4 PASS");
}
