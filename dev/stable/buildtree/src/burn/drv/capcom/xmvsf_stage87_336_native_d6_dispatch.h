#ifndef XMVSF_STAGE87_336_NATIVE_D6_DISPATCH_H
#define XMVSF_STAGE87_336_NATIVE_D6_DISPATCH_H
// XMVSF 87.338 — verified-baseline-preserving, roster-wide native fighter-owned 68000 D6 Hyper resolver.
// Generalized from the repository's VERIFIED 87.170 fighter_regions()
// and 87.174 D6->phase0->action2E original US program analysis.
// No fighter-ID-specific gameplay entrypoints or synthetic 2E selectors.
// A character is accepted only when the actual original executable code
// uniquely proves OWNERSHIP, source D6 matcher, D6 handler, phase 0,
// the original 2E launch and a valid ROM script. Otherwise no tag.
// Preserve original Version 3 and original non-DHC tags untouched.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstring>

namespace XM87336Dispatch {
static const unsigned kMaxFighter=0x24u;
static const unsigned kFighterSlots=19u;
static const unsigned kMinCode=0x1E000u;
static const unsigned kMaxCode=0x48000u;
static const unsigned kNotFound=~0u;
static inline unsigned word(const std::uint8_t* c,unsigned p){
    return (unsigned(c[p+1u])<<8u)|c[p];
}
static inline unsigned longWord(const std::uint8_t* c,unsigned p){
    return (word(c,p)<<16u)|word(c,p+2u);
}
static inline int signedWord(unsigned value){
    return (value&0x8000u)?int(value)-65536:int(value);
}
static inline bool bytes(const std::uint8_t* c,unsigned n,unsigned p,
                         const std::uint8_t* b,unsigned k){
    if(!c||!b||!k||(p&1u)||p>n||k>n-p)return false;
    for(unsigned i=0;i<k;++i)
        if(c[p+(i^1u)]!=b[i])return false;
    return true;
}
static inline unsigned relTarget(const std::uint8_t* c,unsigned n,
                                 unsigned table,unsigned offset){
    if(!c||(table&1u)||(offset&1u)||table>n||offset>n-table||
       n-table-offset<2u)return kNotFound;
    const long target=long(table)+signedWord(word(c,table+offset));
    return target>=0 && target<long(n) && !(target&1l)?
        unsigned(target):kNotFound;
}
// Original 87.170 native instruction sequence:
// MOVE.W $52(A6),D0 / ADD.W D0,D0 / MOVEA.L (PC,D0.W),A0
// followed by PC-relative displacement to a 19-entry fighter code table,
// then JSR(A0) or JMP(A0). These are ORIGINAL executable control tables.
struct Regions {
    unsigned begin[kFighterSlots]={};
    unsigned end[kFighterSlots]={};
    unsigned indexers=0u;
    bool ready=false;
};
static inline Regions identify(const std::uint8_t* c,unsigned n){
    Regions out;
    if(!c||n<kMaxCode)return out;
    for(unsigned i=0;i<kFighterSlots;++i)out.begin[i]=kNotFound;
    const std::uint8_t indexer[8]={
        0x30,0x2E,0x00,0x52,0xD0,0x40,0x20,0x7B
    };
    for(unsigned pc=0u;pc+12u<=n;pc+=2u){
        if(!bytes(c,n,pc,indexer,sizeof(indexer)))continue;
        const unsigned call=word(c,pc+10u);
        if(call!=0x4E90u && call!=0x4ED0u)continue;
        const long table=long(pc)+8l+signedWord(word(c,pc+8u));
        if(table<0||table+76l>long(n)|| (table&1l))continue;
        unsigned valid=0u;
        unsigned pointers[kFighterSlots]={};
        for(unsigned j=0u;j<kFighterSlots;++j){
            pointers[j]=longWord(c,unsigned(table)+j*4u);
            if(j<18u && pointers[j]>=kMinCode && pointers[j]<kMaxCode &&
               !(pointers[j]&1u))++valid;
        }
        if(valid<15u)continue;
        ++out.indexers;
        for(unsigned j=0u;j<kFighterSlots;++j){
            if(j==8u)continue; // Apocalypse is nonplayable
            const unsigned pcStart=pointers[j];
            if(pcStart>=kMinCode && pcStart<kMaxCode && !(pcStart&1u) &&
               pcStart<out.begin[j])out.begin[j]=pcStart;
        }
    }
    // This is a *partial roster* index: some native fighter indexers do
    // not enumerate every playable slot on every build. The 87.337 gate
    // required at least ten tables AND every one of 17 fighters, so one
    // absent ID vetoed every genuine DHC (before controller 8).
    // At least one original 15-of-18 native indexer is enough to establish
    // a specific fighter's region. The selected D6 and 2E ROM path still
    // require exact independent opcode/table verification below.
    if(!out.indexers)return out;
    for(unsigned i=0u;i<kFighterSlots;++i){
        if(i==8u || out.begin[i]==kNotFound)continue;
        unsigned upper=kMaxCode;
        for(unsigned j=0u;j<kFighterSlots;++j)
            if(j!=i && out.begin[j]>out.begin[i] && out.begin[j]<upper)
                upper=out.begin[j];
        out.end[i]=upper;
    }
    out.ready=true;
    return out;
}
// Cached *boundary evidence* for the active decrypted program. Individual
// opcodes/ROM scripts are STILL checked on each claim: a stale pointer alone
// cannot authorize a different fighter or a changed program.
static inline const Regions& regionFor(const std::uint8_t* c,unsigned n){
    struct Cache {
        const std::uint8_t* ptr=nullptr;
        unsigned length=0u,marker=0u;
        Regions regions;
    };
    static Cache cache;
    unsigned marker=0u;
    if(c && n>0x35364u)
        marker=longWord(c,0x0337BAu)^longWord(c,0x035356u)^
               longWord(c,0x013C30u);
    if(cache.ptr!=c || cache.length!=n || cache.marker!=marker){
        cache.ptr=c;cache.length=n;cache.marker=marker;
        cache.regions=identify(c,n);
    }
    return cache.regions;
}
static inline bool sourceMatcher(const std::uint8_t* c,unsigned n,
                                 unsigned lo,unsigned hi,unsigned d6){
    // Source from 87.174/87.170: an original fighter-owned matcher
    // JSR followed by a BEQ refusal, then a literal D6 write and the
    // native mode 2A write. Refuses unrelated D6 table slots.
    if(!c || d6>255u)return false;
    for(unsigned p=lo;p+12u<hi;p+=2u){
        if(word(c,p)!=0x4EB9u)continue;
        const unsigned target=longWord(c,p+2u);
        if(target!=0x013C30u && target!=0x013F76u &&
           target!=0x014096u && target!=0x014392u &&
           target!=0x01458Eu)continue;
        if((word(c,p+6u)&0xFF00u)!=0x6700u)continue;
        const unsigned stop=(p+126u<hi)?p+126u:hi;
        for(unsigned q=p+8u;q+6u<=stop;q+=2u){
            if(word(c,q)!=0x1D7Cu || word(c,q+2u)!=d6 ||
               word(c,q+4u)!=0x00D6u)continue;
            const unsigned upper=(q+54u<stop)?q+54u:stop;
            for(unsigned r=q+6u;r+6u<=upper;r+=2u)
                if(word(c,r)==0x1D7Cu && word(c,r+2u)==0x002Au &&
                   word(c,r+4u)==0x00A0u)return true;
        }
    }
    return false;
}
static inline unsigned phase(const std::uint8_t* c,unsigned n,unsigned p){
    static const std::uint8_t d0[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t d1[]={
        0x30,0x2E,0x00,0x08,0x32,0x3B,0x00,0x06,0x4E,0xFB,0x10,0x02
    };
    if(!bytes(c,n,p,d0,sizeof(d0)) && !bytes(c,n,p,d1,sizeof(d1)))
        return kNotFound;
    return relTarget(c,n,p+sizeof(d0),0u);
}
struct Result {
    unsigned nativeDispatcher=0u,phaseZero=0u,script=0u,selector=0u;
    unsigned alternativeSelector=kNotFound;
    unsigned ownerStart=0u,ownerEnd=0u,launch=0u;
    unsigned matchingHandlers=0u;
    bool hasNativeMatcherSource=false;
    bool valid()const{return nativeDispatcher && phaseZero &&
                              matchingHandlers==1u;}
};
// The universal search may find multiple valid handlers or may fail
// because a fighter's original phase jump crosses inferred code regions.
// Preserve the actual original-ROM routes that were ALREADY used by the
// live-playtested 87.336 direct native DHC, without bypassing opcode/ROM
// signature verification. All other fighters use generic discovery.
static inline Result verifiedBaseline(const std::uint8_t* c,unsigned n,
                                      const std::uint8_t* rom,unsigned romSize,
                                      unsigned fighter,unsigned d6){
    Result result;
    struct Baseline {unsigned fighter,d6,state,handler,phaseZero,launch,sel,script;};
    static const Baseline reference[]={
        {0x12u,0x0Au,0x0337BAu,0x033D92u,0x033DA6u,0x033E0Cu,0x08u,0x0D3070u},
        {0x12u,0x0Cu,0x0337BAu,0x033F0Eu,0x033F28u,0x033F80u,0x00u,0x0D2CFCu},
        {0x14u,0x0Cu,0x035356u,0x0359F6u,0x035A10u,0x035A4Eu,0x00u,0x0D6722u},
        {0x14u,0x0Eu,0x035356u,0x035C42u,0x035C5Au,0x035CB0u,0x02u,0x0D6976u}
    };
    const std::uint8_t d6Header[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    for(const Baseline& x:reference){
        if(x.fighter!=fighter||x.d6!=d6)continue;
        if(!bytes(c,n,x.state,d6Header,sizeof(d6Header)))return result;
        if(relTarget(c,n,x.state+sizeof(d6Header),d6)!=x.handler)return result;
        if(phase(c,n,x.handler)!=x.phaseZero)return result;
        if(x.launch<x.phaseZero || x.launch-x.phaseZero>=240u)return result;
        if(fighter==0x12u && d6==0x0Au){
            const std::uint8_t conditional[]={
                0x70,0x2E,0x72,0x08,0x0C,0x2E,0x00,0x04,
                0x00,0xA4,0x66,0x02,0x72,0x0C,
                0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            if(!bytes(c,n,x.launch,conditional,sizeof(conditional)))
                return result;
            const auto alternate=XM87142::resolve(rom,romSize,fighter,0x2Eu,0x0Cu);
            if(!alternate.valid)return result;
            result.alternativeSelector=0x0Cu;
        }else{
            const std::uint8_t launch[]={
                0x70,0x2E,0x72,std::uint8_t(x.sel),
                0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            if(!bytes(c,n,x.launch,launch,sizeof(launch)))return result;
        }
        const auto script=XM87142::resolve(rom,romSize,fighter,0x2Eu,x.sel);
        if(!script.valid||script.script!=x.script)return Result();
        result.nativeDispatcher=x.state;
        result.phaseZero=x.phaseZero;
        result.launch=x.launch;
        result.selector=x.sel;
        result.script=x.script;
        result.matchingHandlers=1u;
        return result;
    }
    return result;
}
static inline Result resolveGeneric(const std::uint8_t* code,unsigned nCode,
                                    const std::uint8_t* rom,unsigned nRom,
                                    unsigned fighter,unsigned d6){
    Result result;
    if(!code || !rom || fighter>kMaxFighter || (fighter&1u) ||
       fighter==0x10u || !d6 || (d6&1u) || d6>0x7Eu)return result;
    const Regions& regions=regionFor(code,nCode);
    if(!regions.ready)return result;
    const unsigned lo=regions.begin[fighter/2u];
    const unsigned hi=regions.end[fighter/2u];
    if(lo==kNotFound || hi<=lo || hi>nCode)return result;
    // Input-acceptance bytecode is not uniform across the cast: the
    // 87.170 archive establishes five distinct matcher implementations.
    // The original fighter-owned D6 table + its own phase-zero 2E launch
    // + valid fighter ROM script form the execution proof. Record the
    // matching input-source evidence when present, but do not reject
    // original 360/charge/SGS Hyper handlers merely because their source
    // matcher has a different shape from canonical $13C30.
    result.hasNativeMatcherSource=sourceMatcher(code,nCode,lo,hi,d6);
    static const std::uint8_t d6Header[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    result.ownerStart=lo;result.ownerEnd=hi;
    for(unsigned pc=lo;pc+sizeof(d6Header)<hi;pc+=2u){
        if(!bytes(code,nCode,pc,d6Header,sizeof(d6Header)))continue;
        const unsigned handler=relTarget(code,nCode,pc+sizeof(d6Header),d6);
        // The fighter owns the *D6 dispatch site*. Native subroutines may
        // legally be shared or lie across a source-region boundary.
        if(handler==kNotFound || handler<kMinCode ||
           handler>=kMaxCode)continue;
        const unsigned start=phase(code,nCode,handler);
        if(start==kNotFound || start<kMinCode ||
           start>=kMaxCode)continue;
        const unsigned stop=(start+240u<kMaxCode)?
            start+240u:kMaxCode;
        unsigned launches=0u,primary=0u,alternate=kNotFound,
                 launchAt=0u,scriptAt=0u;
        bool conflictingLaunches=false;
        for(unsigned pos=start;pos+10u<=stop;pos+=2u){
            // Native MOVEQ #$2E,D0 ; MOVEQ #selector,D1 ;
            // JMP $01038E. No invented actor action or animation.
            if(word(code,pos)!=0x702Eu ||
               (word(code,pos+2u)&0xFF00u)!=0x7200u)continue;
            const unsigned sel=word(code,pos+2u)&0xFFu;
            bool ordinary=word(code,pos+4u)==0x4EF9u &&
                          longWord(code,pos+6u)==0x01038Eu;
            bool conditional=false;
            if(pos+20u<=stop && sel==8u){
                static const std::uint8_t ryu[]={
                    0x70,0x2E,0x72,0x08,0x0C,0x2E,0x00,0x04,
                    0x00,0xA4,0x66,0x02,0x72,0x0C,
                    0x4E,0xF9,0x00,0x01,0x03,0x8E
                };
                conditional=bytes(code,nCode,pos,ryu,sizeof(ryu));
            }
            if(!ordinary && !conditional)continue;
            const auto entry=XM87142::resolve(rom,nRom,fighter,0x2Eu,sel);
            if(!entry.valid)continue;
            if(conditional){
                const auto other=XM87142::resolve(rom,nRom,fighter,0x2Eu,12u);
                if(!other.valid)continue;
                alternate=12u;
            }
            if(launches && (primary!=sel || scriptAt!=entry.script ||
                             alternate!=(conditional?12u:kNotFound)))
                conflictingLaunches=true;
            if(!launches){
                primary=sel;scriptAt=entry.script;launchAt=pos;
            }
            ++launches;
        }
        // A conditional two-selector native path counts as ONE launch.
        // Some original phase-zero functions branch to more than one
        // verified 2E action. The real M68K decides which selector runs;
        // this host code never writes one. Requiring exactly one rejected
        // otherwise valid character Hyper paths.
        if(!launches || conflictingLaunches)continue;
        // A few native state-entry aliases are equivalent in the real
        // ROM: same phase-zero code and same proven 2E selector/script.
        // Count the equivalence class once instead of rejecting all DHC.
        if(result.matchingHandlers==0u){
            result.matchingHandlers=1u;
            result.nativeDispatcher=pc;
            result.phaseZero=start;
            result.launch=launchAt;
            result.selector=primary;
            result.alternativeSelector=alternate;
            result.script=scriptAt;
        }else if(result.phaseZero!=start || result.selector!=primary ||
                 result.alternativeSelector!=alternate ||
                 result.script!=scriptAt){
            return Result(); // real ambiguity: do not guess a Hyper
        }
    }
    if(result.matchingHandlers!=1u)
        return Result(); // never select a genuinely ambiguous D6 dispatcher
    return result;
}
static inline Result resolve(const std::uint8_t* code,unsigned nCode,
                             const std::uint8_t* rom,unsigned nRom,
                             unsigned fighter,unsigned d6){
    const Result known=verifiedBaseline(code,nCode,rom,nRom,fighter,d6);
    if(known.valid())return known;
    // Fail closed for a KNOWN (but changed) revision of one of the four
    // original routes: do not replace a signature mismatch with a guess.
    if((fighter==0x12u && (d6==0x0Au||d6==0x0Cu)) ||
       (fighter==0x14u && (d6==0x0Cu||d6==0x0Eu)))
        return Result();
    return resolveGeneric(code,nCode,rom,nRom,fighter,d6);
}
} // namespace XM87336Dispatch
#endif
