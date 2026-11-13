#ifndef XMVSF_STAGE87_331_MORPHUS_ENDIAN_LOCATOR_H
#define XMVSF_STAGE87_331_MORPHUS_ENDIAN_LOCATOR_H
// XMVSF 87.331 — read-only endian-corrected European-to-US M68K hook audit.
// The CPS2 decrypted CpsCode buffer holds each native 68000 16-bit word
// in byte-reversed order. Source "expected" guard bytes are canonical 68k
// big-endian, so comparisons must use source[k] == CpsCode[addr+(k^1)].
// The 87.330 locator compared raw bytes and therefore gave false zero hits.
// A hit is *not* proof of a safe patch or equivalent execution semantics.
#include "xmvsf_stage87_330_morphus_locator.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87331Endian {
using XM87330Locator::Site;
using XM87330Locator::Hits;
// These 24 sites complete the 46 exact source signatures in the supplied
// Morphus56k 3v3 v1.1.1 / source/engine/native-hooks-2p.json.
// The other 22 remain in XM87330Locator::sites().
static inline const Site* extra(unsigned* count=nullptr) {
    static const Site a[] = {
        {0x016954u,"xm3_hook_reserve_recovery","4a2dd00a663a4dedc800"},
        {0x00AA60u,"xm3_hook_css_init","1cbc00007000"},
        {0x00B36Au,"xm3_hook_css_wait","536c00126472"},
        {0x00B316u,"xm3_hook_css_confirm","700161000890"},
        {0x000C20u,"xm3_hook_css_sample","1b79008040b0d424"},
        {0x00A504u,"xm3_hook_snap_frame","4eb90007c600"},
        {0x000682u,"xm3_hook_snap_palette","33edd44e0080410a3b6dd446d44e33fc003f00804170"},
        {0x01DBDEu,"xm3_hook_hud_copy_a","207c00924000"},
        {0x01DC1Au,"xm3_hook_hud_copy_b","207c00924000"},
        {0x01DCAEu,"xm3_hook_hud_close","28fc80008000"},
        {0x01DB9Cu,"xm3_hook_hud_start","49f900708000"},
        {0x01DCBAu,"xm3_hook_hud_publish","0a6dffff7a9a"},
        {0x009FEEu,"xm3_col_9fee","4a2dc217670441e80060"},
        {0x00A138u,"xm3_col_a138","4a2dc617670441e80060"},
        {0x00A178u,"xm3_col_a178","4a2dce17670441e80060"},
        {0x04B762u,"xm3_col_4b762","1d5100a66704d0fc0060"},
        {0x02F170u,"xm3_col_2f170","4a2e02176704d0fc0060"},
        {0x039C94u,"xm3_col_39c94","4a2e02176706d1fc00000060"},
        {0x038AD2u,"xm3_col_38ad2","4a2e0217670441e80060"},
        {0x0296CEu,"xm3_col_296ce","4a2e0217670441e80060"},
        {0x00BBAAu,"xm3_hook_css_costume","41edc00043edc800"},
        {0x00B1C4u,"xm3_hook_css_confirm1","0240077f670000600240077067000058"},
        {0x00B2F2u,"xm3_hook_css_confirm2","0240077f67000054024007706700004c"},
        {0x04C0F2u,"xm3_hook_ko_ghost","302e0004323b00064efb1002"}
    };
    if(count) *count=unsigned(sizeof(a)/sizeof(a[0]));
    return a;
}
static inline bool matchesAt(const std::uint8_t* code,unsigned size,
                             unsigned address,
                             const std::uint8_t* bigEndian,unsigned n) {
    if (!code || !bigEndian || !n || (n&1u) ||
        (address&1u) || address>size || size-address<n)
        return false;
    // CpsCode is arranged byte-swapped within each Motorola word.
    for(unsigned j=0u;j<n;++j)
        if(code[address+(j^1u)]!=bigEndian[j])return false;
    return true;
}
static inline Hits scan(const std::uint8_t* code,unsigned size,
                        const std::uint8_t* bigEndian,unsigned n) {
    Hits hits;
    if(!code || !bigEndian || !n || (n&1u) || size<n)return hits;
    for(unsigned addr=0u;addr<=size-n;addr+=2u){
        if(code[addr]!=bigEndian[1u])continue;
        if(!matchesAt(code,size,addr,bigEndian,n))continue;
        if(hits.stored<8u)hits.address[hits.stored++]=addr;
        ++hits.total;
    }
    return hits;
}
static inline void normalizedSix(const std::uint8_t* code,unsigned size,
                                  unsigned addr,char output[32]) {
    if(!code || (addr&1u) || addr>size || size-addr<6u) {
        std::strcpy(output,"OUT_OF_BOUNDS"); return;
    }
    for(unsigned j=0u;j<6u;++j)
        std::sprintf(output+2u*j,"%02X",unsigned(code[addr+(j^1u)]));
    output[12]=0;
}
struct State {
    bool done=false;
    void frame(const std::uint8_t* code,unsigned size,const char* driver){
        if(done||!code||!driver||std::strncmp(driver,"xmvsf",5)!=0)return;
        const char* flag=std::getenv("XM87331_MORPHUS_LOCATOR");
        if(!flag||std::strcmp(flag,"1")!=0)return;
        const char* path=std::getenv("XM87331_MORPHUS_LOCATOR_LOG");
        if(!path||!*path)return;
        std::FILE* log=std::fopen(path,"wb");
        if(!log)return;
        std::fputs("driver\topcode_size\tsymbol\teuro_address\texpected_be\t"
                   "actual_at_euro_be\tmatch_euro\ttotal_hits\t"
                   "candidate_addresses\tdelta_if_unique\n",log);
        unsigned counts[2]={};
        const Site* tables[2]={
            XM87330Locator::sites(&counts[0]),extra(&counts[1])
        };
        for(unsigned group=0u;group<2u;++group)
            for(unsigned i=0u;i<counts[group];++i){
                const Site& site=tables[group][i];
                std::uint8_t bytes[32]={};
                const unsigned n=XM87330Locator::decode(site.hex,bytes,32u);
                if(!n || (n&1u))continue;
                const Hits hits=scan(code,size,bytes,n);
                char actual[32]={};
                normalizedSix(code,size,site.euroAddress,actual);
                std::fprintf(log,"%s\t%u\t%s\t%06X\t%s\t%s\t%s\t%u\t",
                             driver,size,site.symbol,site.euroAddress,site.hex,
                             actual,
                             matchesAt(code,size,site.euroAddress,bytes,n)?"MATCH":"DIFFERENT",
                             hits.total);
                for(unsigned k=0u;k<hits.stored;++k)
                    std::fprintf(log,"%s%06X",k?",":"",hits.address[k]);
                std::fputc('\t',log);
                if(hits.total==1u)
                    std::fprintf(log,"%+d",int(hits.address[0])-int(site.euroAddress));
                std::fputc('\n',log);
            }
        std::fclose(log);
        done=true;
    }
};
static inline State& state(){static State result;return result;}
} // namespace XM87331Endian
#endif
