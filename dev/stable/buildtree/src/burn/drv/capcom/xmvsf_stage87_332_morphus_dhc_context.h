#ifndef XMVSF_STAGE87_332_MORPHUS_DHC_CONTEXT_H
#define XMVSF_STAGE87_332_MORPHUS_DHC_CONTEXT_H
// XMVSF 87.332 — narrow, read-only US 68000 context for the Morphus DHC
// handoff. Input is the decrypted CpsCode opcode view. Capcom program words
// are byte-swapped in this buffer; all logged hex is normalized Motorola BE.
// No code, ROM, actor RAM, CPU registers or callbacks are changed.
// Opt in with XM87332_DHC_CONTEXT=1 and a writable output log path.
#include "xmvsf_stage87_331_morphus_endian_locator.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87332Context {
using XM87330Locator::Site;
struct Focus {
    const char* symbol;
    int expectedShift;
    const char* reason;
};
static inline const Focus* focus(unsigned* count=0) {
    // 87.331 unique / neighboring anchors establish shift groups;
    // the two native DHC sites are hypotheses only, never direct hooks.
    static const Focus rows[] = {
        {"incoming_hyper_commit",14,"DHC_HYPOTHESIS"},
        {"dhc_chain",14,"DHC_HYPOTHESIS"},
        {"variable_combo",14,"NEAR_DHC"},
        {"snap_motion",14,"NEAR_DHC"},
        {"ko_prepare",14,"NEAR_DHC"},
        {"match_init",14,"UNIQUE_CONTROL"},
        {"resolver",14,"UNIQUE_CONTROL"},
        {"input",14,"UNIQUE_CONTROL"},
        {"xm3_hook_reserve_recovery",14,"UNIQUE_CONTROL"},
        {"xm3_hook_css_wait",12,"OTHER_SHIFT_CONTROL"},
        {"xm3_hook_css_confirm",12,"OTHER_SHIFT_CONTROL"},
        {"xm3_col_4b762",18,"OTHER_SHIFT_CONTROL"},
        {"xm3_hook_css_sample",0,"NO_SHIFT_CONTROL"}
    };
    if(count) *count=unsigned(sizeof(rows)/sizeof(rows[0]));
    return rows;
}
static inline const Site* siteFor(const char* symbol) {
    unsigned count=0;
    const Site* table=XM87330Locator::sites(&count);
    for(unsigned i=0;i<count;++i)
        if(std::strcmp(table[i].symbol,symbol)==0)return table+i;
    table=XM87331Endian::extra(&count);
    for(unsigned i=0;i<count;++i)
        if(std::strcmp(table[i].symbol,symbol)==0)return table+i;
    return 0;
}
static inline unsigned normalizeLength(const Site* site,std::uint8_t out[32]) {
    return site?XM87330Locator::decode(site->hex,out,32u):0u;
}
static inline bool plausible(const std::uint8_t* code,unsigned len,
                             const Site* site,int offset) {
    std::uint8_t expected[32]={};
    const unsigned n=normalizeLength(site,expected);
    const long at=site?long(site->euroAddress)+long(offset):-1l;
    return n && at>=0 && at<=long(len) &&
           XM87331Endian::matchesAt(code,len,unsigned(at),expected,n);
}
static inline unsigned localHits(const std::uint8_t* code,unsigned len,
                                 const Site* site,int radius,
                                 unsigned* first=0) {
    if(!site || !code)return 0;
    std::uint8_t expected[32]={};
    const unsigned n=normalizeLength(site,expected);
    if(!n || (n&1u) || len<n)return 0;
    const unsigned lo=site->euroAddress>unsigned(radius)?
                         site->euroAddress-unsigned(radius):0u;
    const unsigned hi=(site->euroAddress+unsigned(radius)<len-n)?
                         site->euroAddress+unsigned(radius):len-n;
    unsigned hits=0u;
    for(unsigned at=(lo+1u)&~1u;at<=hi;at+=2u)
        if(XM87331Endian::matchesAt(code,len,at,expected,n)){
            if(first && !hits)*first=at;
            ++hits;
        }
    return hits;
}
// Append 16 bytes of previous context, the candidate 6 bytes, and 58 bytes
// following it (80 continuous bytes, all canonical 68000 big-endian).
// Do not interpret this as proof of instruction boundaries.
static inline void context80(const std::uint8_t* code,unsigned len,
                             long candidate,char output[161]) {
    if(!code || candidate<16l || candidate>long(len) ||
       long(len)-candidate<64l){
        std::strcpy(output,"OUT_OF_BOUNDS");return;
    }
    const unsigned start=unsigned(candidate-16l);
    for(unsigned i=0;i<80u;++i)
        std::sprintf(output+2u*i,"%02X",unsigned(code[start+(i^1u)]));
    output[160]=0;
}
struct State {
    bool done=false;
    void frame(const std::uint8_t* code,unsigned len,const char* driver) {
        if(done || !code || !driver || std::strncmp(driver,"xmvsf",5)!=0)
            return;
        const char* flag=std::getenv("XM87332_DHC_CONTEXT");
        if(!flag || std::strcmp(flag,"1")!=0)return;
        const char* path=std::getenv("XM87332_DHC_CONTEXT_LOG");
        if(!path || !*path)return;
        std::FILE* log=std::fopen(path,"wb");
        if(!log)return;
        std::fputs("driver\topcode_size\tsymbol\tpurpose\teuro\t"
                   "candidate\tdelta\texpected_be\tfull_match\t"
                   "local_hits_128\tfirst_local\t"
                   "context80_be_16before_64after\n",log);
        unsigned count=0;
        const Focus* fs=focus(&count);
        for(unsigned i=0;i<count;++i){
            const Focus& f=fs[i];
            const Site* s=siteFor(f.symbol);
            if(!s)continue;
            unsigned first=0u;
            const unsigned local=localHits(code,len,s,128,&first);
            const int shift[]={0,12,14,18};
            for(unsigned k=0;k<4u;++k){
                const long at=long(s->euroAddress)+long(shift[k]);
                char context[161]={};
                context80(code,len,at,context);
                std::fprintf(log,"%s\t%u\t%s\t%s\t%06X\t%06lX\t%+d\t"
                                 "%s\t%s\t%u\t",
                    driver,len,s->symbol,f.reason,s->euroAddress,
                    (unsigned long)at,shift[k],s->hex,
                    plausible(code,len,s,shift[k])?"MATCH":"DIFFERENT",
                    local);
                if(local)std::fprintf(log,"%06X",first);
                std::fprintf(log,"\t%s\n",context);
            }
        }
        std::fclose(log);
        done=true;
    }
};
static inline State& state(){static State s;return s;}
} // namespace XM87332Context
#endif
