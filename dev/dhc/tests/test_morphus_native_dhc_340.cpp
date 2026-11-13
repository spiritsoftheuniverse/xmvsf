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
        notHcb.sample(602u,0x12u,0x0Au,4u,0u,right,0x0Au,
                      true,true,220u,1u);
        notHcb.sample(603u,0x12u,0x0Au,back,0x18u,right,0x0Au,
                      true,true,220u,1u);
        assert(notHcb.phase==IDLE); // F,D,B cannot fake an HCB.
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
    std::puts("XMVSF 87.351 backward QCB/HCB original inputs PASS");
}
