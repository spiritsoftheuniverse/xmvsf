#ifndef XMVSF_STAGE87_142_ROM_DISPATCH_H
#define XMVSF_STAGE87_142_ROM_DISPATCH_H
// XM87.142: direct, READ-ONLY CPS2 native-script table resolver.
// Ground truth: user-supplied xmvsfu program ROM and 87.141 native TSV.
// The CPS2 native 68000 code at PC 0x01038E stores action/selector at
// FFxxxx60/61, reads the fighter's ROM table at 0x8A946+id*2,
// indexes its action table and adds the selector-relative entry to locate
// the exact 16-byte sprite-script record. NO writes, patches or injections.
// CpsRom is the host word-swapped data ROM; NOT decrypted opcode CpsCode.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87142 {
static inline unsigned ramB(const std::uint8_t* r,unsigned p){
    return r[(p&65535u)^1u];
}
static inline unsigned ramW(const std::uint8_t* r,unsigned p){
    return (ramB(r,p)<<8u)|ramB(r,p+1u);
}
static inline unsigned ramL(const std::uint8_t* r,unsigned p){
    return (ramW(r,p)<<16u)|ramW(r,p+2u);
}
static inline unsigned romW(const std::uint8_t* r,unsigned n,unsigned p){
    if(!r || p>n || n-p<2u)return 0u;
    return unsigned(r[p])|(unsigned(r[p+1u])<<8u);
}
static inline unsigned romL(const std::uint8_t* r,unsigned n,unsigned p){
    if(!r || p>n || n-p<4u)return 0u;
    return (romW(r,n,p)<<16u)|romW(r,n,p+2u);
}
struct Lookup {
    bool valid=false;
    unsigned fighter=0,action=0,selector=0;
    unsigned fighterTable=0,actionTable=0,script=0,animation=0;
};
static inline Lookup resolve(const std::uint8_t* rom,unsigned n,
                             unsigned fighter,unsigned action,unsigned selector){
    Lookup out;out.fighter=fighter;out.action=action;out.selector=selector;
    // Native id values are even, stored as *even byte offset* to 32-bit
    // character table. Action and subaction selectors are also even offsets.
    if(!rom || n<0x8AA08u || fighter>0x40u || (fighter&1u) ||
       action>0x34u || (action&1u) || selector>0x4Eu || (selector&1u))
        return out;
    const unsigned at=0x8A946u+fighter*2u;
    const unsigned table=romL(rom,n,at);
    if(table>=n || n-table<=0x38u)return out;
    const unsigned actionDelta=romW(rom,n,table+action);
    const unsigned section=table+actionDelta;
    if(!actionDelta || section>=n || n-section<selector+2u)return out;
    const unsigned scriptDelta=romW(rom,n,section+selector);
    const unsigned script=section+scriptDelta;
    // The actual native entry must be a bounded 16-byte record. Reject
    // action holes/zero entries instead of accidentally following table data.
    if(!scriptDelta || script>=n || n-script<16u)return out;
    out.valid=true;out.fighterTable=table;out.actionTable=section;
    out.script=script;out.animation=romW(rom,n,script+6u);
    return out;
}
enum Kind { NO_EVENT=0, NATIVE_SUPER_ENTRY=1, NATIVE_TAG_OUT=2,
            NATIVE_TAG_IN=3, POINT_SWAP=4 };
static inline Kind classify(const Lookup& x,unsigned nativeScript){
    if(!x.valid||nativeScript!=x.script)return NO_EVENT;
    // Observed AND resolved through original ROM data, NOT user-provided
    // move names or synthetic proxies. New fighters require more captures.
    if(x.fighter==0x12u && x.action==0x2Eu && x.selector==0x08u &&
       x.script==0xD3070u)return NATIVE_SUPER_ENTRY;
    if(x.fighter==0x14u &&
       ((x.action==0x2Cu && x.selector==0x04u && x.script==0xD5E2Eu) ||
        (x.action==0x2Cu && x.selector==0x00u && x.script==0xD5C46u) ||
        (x.action==0x2Eu && x.selector==0x00u && x.script==0xD6722u)))
       return NATIVE_SUPER_ENTRY;
    if(x.fighter==0x12u && x.action==0x32u && x.selector==0x00u &&
       x.script==0xD1DACu)return NATIVE_TAG_OUT;
    if(x.fighter==0x14u && x.action==0x2Au && x.selector==0x0Cu &&
       x.script==0xD5702u)return NATIVE_TAG_IN;
    return NO_EVENT;
}
static inline const char* name(Kind k){
    switch(k){
    case NATIVE_SUPER_ENTRY:return "ROM_SUPER_SCRIPT";
    case NATIVE_TAG_OUT:return "ROM_OUTGOING_TAG_SCRIPT";
    case NATIVE_TAG_IN:return "ROM_INCOMING_TAG_SCRIPT";
    case POINT_SWAP:return "ROM_POINT_RESERVE_SWAP";
    default:return "OTHER";
    }
}
struct Event {
    Kind kind=NO_EVENT;
    unsigned side=0,pointId=0,reserveId=0,action=0,selector=0;
    unsigned nativeScript=0,tableScript=0,anim=0,partnerPointer=0;
};
struct Report {
    unsigned verifiedSuperMask=0,tagMask=0;
    Event event[4];
    unsigned count=0;
    void push(Event e){if(count<4u)event[count++]=e;}
};
struct State {
    bool seeded[2]={false,false};
    unsigned lastPoint[2]={0u,0u},lastReserve[2]={0u,0u};
    unsigned lastAction[2]={0u,0u},lastScript[2]={0u,0u};
    unsigned lastFrame=~0u;
    bool checkedLog=false;
    std::FILE* log=nullptr;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    Report frame(const std::uint8_t* ram,const std::uint8_t* rom,unsigned romSize,
                 unsigned frameNumber,bool runahead){
        Report report;
        if(!ram||!rom||runahead||frameNumber==lastFrame)return report;
        if(lastFrame!=~0u && frameNumber<lastFrame){
            seeded[0]=seeded[1]=false;
        }
        lastFrame=frameNumber;
        if(ramB(ram,0x5000u)!=8u){
            seeded[0]=seeded[1]=false;return report;
        }
        if(!checkedLog){
            checkedLog=true;
            const char* path=std::getenv("XM87142_ROM_TRACE");
            if(path && *path){
                log=std::fopen(path,"wb");
                if(log)std::fputs("frame\tside\tevent\tpointId\treserveId\taction60"
                    "\tsubaction61\tnativeScript\ttableScript\tanim\tpartnerPtr\n",log);
            }
        }
        unsigned point[2]={ramW(ram,0x4052u),ramW(ram,0x4452u)};
        unsigned reserve[2]={ramW(ram,0x4852u),ramW(ram,0x4C52u)};
        for(unsigned s=0;s<2u;++s){
            const unsigned base=0x4000u+s*0x400u;
            const unsigned action=ramB(ram,base+0x60u);
            const unsigned selector=ramB(ram,base+0x61u);
            const unsigned nativeScript=ramL(ram,base+0x34u);
            const unsigned partnerPtr=ramL(ram,base+0x224u);
            Lookup lookup=resolve(rom,romSize,point[s],action,selector);
            const Kind known=classify(lookup,nativeScript);
            const bool changed=!seeded[s]||lastAction[s]!=(action<<8u|selector)||
                               lastScript[s]!=nativeScript||lastPoint[s]!=point[s];
            if(known==NATIVE_SUPER_ENTRY && changed)
                report.verifiedSuperMask|=1u<<s;
            const bool swapped=seeded[s] && lastPoint[s]==reserve[s] &&
                lastReserve[s]==point[s] && lastPoint[s]!=point[s];
            if(swapped)report.tagMask|=1u<<s;
            if(swapped){
                Event e;e.kind=POINT_SWAP;e.side=s;e.pointId=point[s];
                e.reserveId=reserve[s];e.partnerPointer=partnerPtr;
                e.action=action;e.selector=selector;e.nativeScript=nativeScript;
                e.tableScript=lookup.script;e.anim=lookup.animation;
                report.push(e);
            }
            if(known!=NO_EVENT && changed){
                Event e;e.kind=known;e.side=s;e.pointId=point[s];
                e.reserveId=reserve[s];e.partnerPointer=partnerPtr;
                e.action=action;e.selector=selector;e.nativeScript=nativeScript;
                e.tableScript=lookup.script;e.anim=lookup.animation;
                report.push(e);
            }
            lastPoint[s]=point[s];lastReserve[s]=reserve[s];
            lastAction[s]=(action<<8u)|selector;lastScript[s]=nativeScript;
            seeded[s]=true;
        }
        if(log && report.count){
            for(unsigned i=0;i<report.count;++i){
                const Event& e=report.event[i];
                std::fprintf(log,"%u\t%u\t%s\t%04X\t%04X\t%02X\t%02X"
                    "\t%06X\t%06X\t%04X\t%06X\n",
                    frameNumber,e.side+1u,name(e.kind),e.pointId,e.reserveId,
                    e.action,e.selector,e.nativeScript,e.tableScript,e.anim,
                    e.partnerPointer);
            }
            std::fflush(log);
        }
        return report;
    }
};
static inline State& state(){static State s;return s;}
} // XM87142
#endif
