#ifndef XMVSF_STAGE87_334_MORPHUS_2V2_PRECOMMIT_H
#define XMVSF_STAGE87_334_MORPHUS_2V2_PRECOMMIT_H
// Native-input-backed +D6 Hyper preselection for a *two-member* team, adapted from
// Morphus56k / Daniel Plas Rivera, XMvsSF-3v3-DHC-v1.1.1.
// This module is a PRECOMMIT VALIDATOR. It does not fabricate a 2E action,
// deduct meter, start an animation, or replace the original 68000 handoff.
// Use only with the experimental Version-4 unified DHC; revision equivalence
// of the European per-character +D6 table still requires US gameplay proof.
#include "xmvsf_stage87_329_morphus_dhc_catalog.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace XM87334Native {
enum Reason {
    ACCEPTED=0, BAD_FIGHTER=1, MOTION_NOT_MAPPED=2, NO_HYPER=3,
    AIR_ONLY=4, CONDITIONAL=5, BAD_BUTTONS=6
};
struct Decision {
    Reason reason=BAD_FIGHTER;
    std::uint8_t character=0, hyperD6=0, motionColumn=0;
    bool terminalThrow=false;
    bool ok()const{return reason==ACCEPTED && hyperD6!=0u;}
};
static inline unsigned bits(unsigned n) {
    unsigned sum=0u;for(;n;n>>=1u)sum+=n&1u;return sum;
}
static inline bool qualifiedChord(unsigned attacks){
    const unsigned p=attacks&0x07u,k=attacks&0x38u;
    return (p && !k && bits(p)>=2u) || (k && !p && bits(k)>=2u);
}
// Unlike the *assigned* DHC input directions in dhc_table.inc, the
// native inputs below come from Morphus' dhc_commands.json native_command
// field. They are the ORIGINAL fighter commands with PP / KK button
// families. This prevents QCF+KK for Ken from being mistaken for the
// artificial DHC QCF assignment (Shoryureppa instead of Shinryuken).
// Only commands expressible by the current owner motion parser are listed.
// F,DF,D / 360 / SGS / air-only / custom sequences require a separate
// original-ROM input decoder and are intentionally NOT guessed.
struct Command {
    std::uint8_t fighter, motion, family, hyperD6;
    bool terminalThrow;
};
static inline const Command* commands(unsigned* count=nullptr) {
    static const Command a[]={
        {0x00u,1u,1u,0x08u,false}, // Wolverine QCF+PP
        {0x00u,3u,1u,0x06u,false}, // Wolverine DP+PP
        {0x02u,1u,1u,0x04u,false}, // Cyclops QCF+PP
        {0x04u,1u,1u,0x02u,false}, // Storm QCF+PP
        {0x04u,2u,1u,0x0Au,false}, // Storm QCB+PP
        {0x06u,1u,1u,0x28u,false}, // Rogue QCF+PP
        {0x08u,1u,1u,0x0Au,false}, // Gambit QCF+PP
        {0x0Au,1u,1u,0x16u,false}, // Sabretooth QCF+PP
        {0x0Au,3u,1u,0x0Eu,false}, // Sabretooth DP+PP
        {0x0Au,6u,2u,0x08u,false}, // Sabretooth HCB+KK
        {0x0Cu,1u,1u,0x02u,false}, // Juggernaut QCF+PP
        {0x0Eu,1u,1u,0x04u,false}, // Magneto QCF+PP
        {0x0Eu,1u,2u,0x06u,false}, // Magneto QCF+KK
        {0x12u,1u,1u,0x0Au,false}, // Ryu QCF+PP
        {0x12u,2u,2u,0x0Cu,false}, // Ryu QCB+KK
        {0x14u,1u,1u,0x0Cu,false}, // Ken QCF+PP
        {0x14u,1u,2u,0x0Eu,false}, // Ken QCF+KK
        {0x16u,1u,1u,0x06u,false}, // Chun-Li QCF+PP
        {0x16u,2u,2u,0x0Au,false}, // Chun-Li QCB+KK
        {0x16u,1u,2u,0x08u,false}, // Chun-Li QCF+KK
        {0x18u,1u,1u,0x0Au,false}, // Dhalsim QCF+PP
        {0x18u,1u,2u,0x0Cu,false}, // Dhalsim QCF+KK
        {0x1Cu,1u,1u,0x0Cu,false}, // M. Bison QCF+PP
        {0x1Cu,1u,2u,0x0Au,false}, // M. Bison QCF+KK
        {0x1Eu,2u,1u,0x1Au,false}, // Akuma QCB+PP
        {0x1Eu,1u,1u,0x16u,false}, // Akuma ground QCF+PP
        {0x20u,1u,1u,0x04u,false}, // Charlie QCF+PP
        {0x20u,2u,2u,0x06u,false}, // Charlie QCB+KK
        {0x20u,1u,2u,0x08u,false}, // Charlie QCF+KK
        {0x22u,1u,2u,0x0Eu,false}, // Cammy QCF+KK
        {0x22u,2u,2u,0x16u,false}, // Cammy QCB+KK
        {0x24u,1u,1u,0x06u,false}, // Alt Chun-Li QCF+PP
        {0x24u,2u,2u,0x0Au,false}, // Alt Chun-Li QCB+KK
        {0x24u,1u,2u,0x08u,false}  // Alt Chun-Li QCF+KK
    };
    if(count)*count=unsigned(sizeof(a)/sizeof(a[0]));
    return a;
}
static inline Decision resolve(unsigned incoming,unsigned ownerMotion,
                               unsigned attacks){
    Decision d;
    if(incoming>0x24u || (incoming&1u))return d;
    d.character=std::uint8_t(incoming);
    if(ownerMotion<1u || ownerMotion>8u){
        d.reason=MOTION_NOT_MAPPED;return d;
    }
    if(ownerMotion==7u || ownerMotion==8u){
        d.reason=MOTION_NOT_MAPPED;return d; // 2QCF/2QCB not mapped
    }
    if(!qualifiedChord(attacks)){d.reason=BAD_BUTTONS;return d;}
    const unsigned family=(attacks&0x07u)?1u:2u;
    unsigned n=0u;
    const Command* table=commands(&n);
    for(unsigned i=0u;i<n;++i) {
        const Command& cmd=table[i];
        if(cmd.fighter!=incoming || cmd.motion!=ownerMotion ||
           cmd.family!=family)continue;
        d.motionColumn=std::uint8_t(ownerMotion-1u);
        d.hyperD6=cmd.hyperD6;
        d.terminalThrow=cmd.terminalThrow;
        d.reason=ACCEPTED;
        return d;
    }
    // A real command unknown to this restricted table is NOT interpreted
    // as a different Hyper. In particular the 3v3 artificial motion
    // assignment must never cause a 2v2 misfire.
    d.reason=NO_HYPER;
    return d;
}
static inline const char* reasonName(Reason reason){
    switch(reason){
    case ACCEPTED:return "MORPHUS_D6_PREQUALIFIED";
    case BAD_FIGHTER:return "MORPHUS_D6_INVALID_RESERVE";
    case MOTION_NOT_MAPPED:return "MORPHUS_D6_UNSUPPORTED_MOTION";
    case NO_HYPER:return "MORPHUS_D6_NO_HYPER_FOR_MOTION";
    case AIR_ONLY:return "MORPHUS_D6_AIR_ONLY_REJECTED";
    case CONDITIONAL:return "MORPHUS_D6_CONDITIONAL_REJECTED";
    case BAD_BUTTONS:return "MORPHUS_D6_INCOMPATIBLE_BUTTONS";
    }
    return "MORPHUS_D6_UNKNOWN";
}
static inline bool enabled(){
    const char* value=std::getenv("XM87334_MORPHUS_PRECOMMIT");
    return value && std::strcmp(value,"1")==0;
}
struct Pending {
    std::uint8_t point=0,reserve=0,d6=0;
    unsigned side=0,epoch=0;
    bool throwHyper=false, armed=false, consumed=false;
    void clear(){*this=Pending();}
    bool stage(unsigned which,unsigned source,unsigned target,
               unsigned now,const Decision& selected){
        if(which>1u||source>0x24u||(source&1u)||source==target||
           !selected.ok()||target!=selected.character||armed)return false;
        point=std::uint8_t(source);
        reserve=std::uint8_t(target);
        d6=selected.hyperD6;
        side=which;epoch=now;throwHyper=selected.terminalThrow;
        armed=true;consumed=false;
        return true;
    }
    // This is ONLY a side/identity guard; it does not mutate the actor,
    // claim a native hook was executed or consume any level of super meter.
    bool matchesAfterOriginalSwap(unsigned which,unsigned actualPoint,
                                  unsigned actualReserve)const{
        return armed&&!consumed && which==side && actualPoint==reserve &&
               actualReserve==point;
    }
    void markConsumed(){consumed=true;armed=false;}
};
} // namespace XM87334Native
#endif
