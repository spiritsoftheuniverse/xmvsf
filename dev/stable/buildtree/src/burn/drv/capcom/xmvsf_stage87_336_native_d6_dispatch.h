#ifndef XMVSF_STAGE87_336_NATIVE_D6_DISPATCH_H
#define XMVSF_STAGE87_336_NATIVE_D6_DISPATCH_H
// Stage 87.336: ORIGINAL US XMVSFU D6->phase-zero->action 2E dispatch.
// Unlike 87.335's incorrect JSR $1038E with D0=D1=0 (which selects
// idle/ordinary action), this invokes the stock CHARACTER STATE dispatcher.
// Source: in-repo 87.174 ROM-proven tables and original M68K signatures.
// No guessed US address translation. Refuse any fighter/command without
// a fully ROM-confirmed native command handler, phase and script.
//
// This starts with the four US-verified Ryu/Ken D6 states. Expansion must
// be by verifying ORIGINAL dispatch signatures, not inventing animations.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstring>

namespace XM87336Dispatch {
struct Case {
    unsigned fighter,d6,state,handler,phaseZero,launch,selector,script;
};
static inline const Case* cases(unsigned* count=nullptr) {
    static const Case a[]={
        {0x12u,0x0Au,0x0337BAu,0x033D92u,0x033DA6u,0x033E0Cu,0x08u,0x0D3070u},
        {0x12u,0x0Cu,0x0337BAu,0x033F0Eu,0x033F28u,0x033F80u,0x00u,0x0D2CFCu},
        {0x14u,0x0Cu,0x035356u,0x0359F6u,0x035A10u,0x035A4Eu,0x00u,0x0D6722u},
        {0x14u,0x0Eu,0x035356u,0x035C42u,0x035C5Au,0x035CB0u,0x02u,0x0D6976u}
    };
    if(count)*count=unsigned(sizeof(a)/sizeof(a[0]));
    return a;
}
static inline bool at(const std::uint8_t* code,unsigned len,
                      unsigned where,const std::uint8_t* expected,unsigned n){
    if(!code||!expected||!n||(n&1u)||(where&1u)||
       where>len||len-where<n)return false;
    for(unsigned i=0u;i<n;++i)
        if(code[where+(i^1u)]!=expected[i])return false;
    return true;
}
static inline unsigned w16(const std::uint8_t* code,unsigned at) {
    return (unsigned(code[at+1u])<<8u)|unsigned(code[at]);
}
static inline unsigned relTarget(const std::uint8_t* code,unsigned len,
                                 unsigned table,unsigned index) {
    if(!code || (table&1u) || (index&1u) || table>len ||
       len-table<index+2u)return ~0u;
    const unsigned word=w16(code,table+index);
    const int displacement=word&0x8000u?int(word)-0x10000:int(word);
    const long result=long(table)+long(displacement);
    if(result<0 || result>=long(len) || (result&1l))return ~0u;
    return unsigned(result);
}
struct Result {
    unsigned nativeDispatcher=0u,phaseZero=0u,script=0u,selector=0u;
    bool valid()const{return nativeDispatcher!=0u && phaseZero!=0u;}
};
static inline Result resolve(const std::uint8_t* code,unsigned nCode,
                             const std::uint8_t* rom,unsigned nRom,
                             unsigned fighter,unsigned d6) {
    Result result;
    static const std::uint8_t d6Sig[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t phaseD0[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t phaseD1[]={
        0x30,0x2E,0x00,0x08,0x32,0x3B,0x00,0x06,0x4E,0xFB,0x10,0x02
    };
    unsigned n=0u;
    const Case* table=cases(&n);
    for(unsigned i=0u;i<n;++i) {
        const Case& item=table[i];
        if(item.fighter!=fighter||item.d6!=d6)continue;
        if(!at(code,nCode,item.state,d6Sig,unsigned(sizeof(d6Sig))))return result;
        const unsigned handler=relTarget(code,nCode,item.state+sizeof(d6Sig),d6);
        if(handler!=item.handler)return result;
        if(!at(code,nCode,handler,phaseD0,unsigned(sizeof(phaseD0))) &&
           !at(code,nCode,handler,phaseD1,unsigned(sizeof(phaseD1))))
            return result;
        const unsigned phase=relTarget(code,nCode,handler+sizeof(phaseD0),0u);
        if(phase!=item.phaseZero || item.launch<phase ||
           item.launch-phase>=240u)return result;
        // Verify that the character's original phase-zero code really
        // launches the requested stock 2E selector via the stock dispatcher.
        if(fighter==0x12u && d6==0x0Au) {
            static const std::uint8_t conditional[]={
                0x70,0x2E,0x72,0x08,0x0C,0x2E,0x00,0x04,0x00,0xA4,
                0x66,0x02,0x72,0x0C,0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            if(!at(code,nCode,item.launch,conditional,unsigned(sizeof(conditional))))
                return result;
        }else{
            const std::uint8_t launch[]={
                0x70,0x2E,0x72,std::uint8_t(item.selector),
                0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            if(!at(code,nCode,item.launch,launch,unsigned(sizeof(launch))))
                return result;
        }
        const auto script=XM87142::resolve(rom,nRom,fighter,0x2Eu,item.selector);
        if(!script.valid || script.script!=item.script)return result;
        result.nativeDispatcher=item.state;
        result.phaseZero=phase;
        result.selector=item.selector;
        result.script=item.script;
        return result;
    }
    return result;
}
} // namespace XM87336Dispatch
#endif
