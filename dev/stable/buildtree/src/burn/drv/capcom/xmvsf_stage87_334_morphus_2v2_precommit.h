#ifndef XMVSF_STAGE87_334_MORPHUS_2V2_PRECOMMIT_H
#define XMVSF_STAGE87_334_MORPHUS_2V2_PRECOMMIT_H
// Native +D6 Hyper selection for a *two-member* team, adapted from
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
// The owner parser supplies its own Motion enum (1=QCF, 2=QCB, ...);
// this is not a new/guessed ROM 2E selector. Two-quarter-circles and the
// double-down pattern in Morphus' source are *not* interchangeable.
static inline Decision resolve(unsigned incoming,unsigned ownerMotion,
                               unsigned attacks){
    Decision d;
    if(incoming>0x24u || (incoming&1u))return d;
    d.character=std::uint8_t(incoming);
    const int column=XM87329Morphus::columnFromOwnerMotion(ownerMotion);
    if(column<0){d.reason=MOTION_NOT_MAPPED;return d;}
    d.motionColumn=std::uint8_t(column);
    const XM87329Morphus::Row& row=XM87329Morphus::rows()[incoming>>1u];
    const unsigned hyper=row.hyper[column];
    if(!hyper){d.reason=NO_HYPER;return d;}
    if(row.air&(1u<<unsigned(column))){d.reason=AIR_ONLY;return d;}
    if(row.conditional&(1u<<unsigned(column))){
        d.reason=CONDITIONAL;return d;
    }
    if(!qualifiedChord(attacks)){d.reason=BAD_BUTTONS;return d;}
    d.hyperD6=std::uint8_t(hyper);
    d.terminalThrow=(row.throwing&(1u<<unsigned(column)))!=0u;
    d.reason=ACCEPTED;
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
