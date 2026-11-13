#ifndef XMVSF_STAGE87_330_MORPHUS_LOCATOR_H
#define XMVSF_STAGE87_330_MORPHUS_LOCATOR_H
// XMVSF 87.330 — read-only program-revision locator for Morphus56k's
// EURO 961004 native-hook signatures. Input is CpsCode (decrypted M68K
// opcode view), NOT CpsRom ciphertext. No writes to ROM, RAM or CPU state.
// A signature hit is a candidate address, NEVER authorization to patch it.
// Only runs when XM87330_MORPHUS_LOCATOR=1 and a log path is provided.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87330Locator {
struct Site { unsigned euroAddress; const char* symbol; const char* hex; };
static inline const Site* sites(unsigned* count=nullptr) {
    // Exact guard bytes from Morphus' native-hooks-2p.json, including
    // nearby anchors. Some anchors are duplicated by design.
    static const Site a[] = {
      {0x009EE6u,"collision_9ee6","4a2e0217670441e80060"},
      {0x009F74u,"collision_9f74","4a2dc217670441e80060"},
      {0x009FB6u,"collision_9fb6","4a2dca176700000641e80060"},
      {0x00A02Eu,"collision_a02e","4a2dca176700000641e80060"},
      {0x00A0C0u,"collision_a0c0","4a2dc617670441e80060"},
      {0x00A102u,"collision_a102","4a2dce17670441e80060"},
      {0x00AE20u,"cpu_team","61000eb061000e36"},
      {0x00D588u,"ko_prepare","206e02244a680210"},
      {0x00F352u,"team_defeat","302dc210d06dca10322dc610d26dce10"},
      {0x00F3E8u,"team_timeout","302dc210d06dca10322dc610d26dce10"},
      {0x00F9ACu,"team_compare","302dc210d06dca10322dc610d26dce10"},
      {0x00FCFCu,"side0_init","2f0e4dedc000"},
      {0x00FD4Au,"side1_init","2f0e4dedc400"},
      {0x00FE04u,"match_init","4dedc000422e0002"},
      {0x00FFFAu,"resolver","4a2e02206702c141"},
      {0x0101C6u,"round_init","4dedc00061000016"},
      {0x010250u,"swap","41edc00043edc800"},
      {0x010410u,"input","7000102e0002d040d040207b00503010"},
      {0x011DB0u,"incoming_hyper_commit","302e0052d040"},
      {0x013AA8u,"dhc_chain","0c2e000400a4"},
      {0x013B3Au,"variable_combo","0c2d000ad000"},
      {0x013C22u,"snap_motion","4a2e0003661a"}
    };
    if(count) *count=unsigned(sizeof(a)/sizeof(a[0]));
    return a;
}
static inline int digit(char c) {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
static inline unsigned decode(const char* text,std::uint8_t* out,unsigned cap) {
    if(!text || !out || !cap)return 0u;
    const size_t m=std::strlen(text);
    if((m&1u)!=0u || m/2u>cap)return 0u;
    for(size_t k=0;k<m;k+=2u) {
        const int hi=digit(text[k]),lo=digit(text[k+1u]);
        if(hi<0 || lo<0)return 0u;
        out[k/2u]=std::uint8_t((hi<<4)|lo);
    }
    return unsigned(m/2u);
}
struct Hits {
    unsigned total=0u, stored=0u, address[8]={};
};
static inline Hits scan(const std::uint8_t* code,unsigned len,
                        const std::uint8_t* sequence,unsigned n) {
    Hits result;
    if(!code || !sequence || !n || len<n)return result;
    // Valid 68000 instruction entry addresses are even.
    for(unsigned at=0u;at<=len-n;at+=2u) {
        if(code[at]!=sequence[0])continue;
        if(std::memcmp(code+at,sequence,n)!=0)continue;
        if(result.stored<8u)result.address[result.stored++]=at;
        ++result.total;
    }
    return result;
}
static inline bool atOriginal(const std::uint8_t* code,unsigned len,
                              const std::uint8_t* signature,unsigned n,
                              unsigned address) {
    return code && signature && n && address<=len && len-address>=n &&
           std::memcmp(code+address,signature,n)==0;
}
static inline void six(const std::uint8_t* code,unsigned len,
                       unsigned address,char* text) {
    if(!code || address>len || len-address<6u) {
        std::strcpy(text,"OUT_OF_BOUNDS");
        return;
    }
    for(unsigned j=0u;j<6u;++j)
        std::sprintf(text+j*2u,"%02X",unsigned(code[address+j]));
    text[12]=0;
}
struct State {
    bool done=false;
    void frame(const std::uint8_t* code,unsigned len,const char* driver) {
        if(done || !code || !driver || std::strncmp(driver,"xmvsf",5)!=0)
            return;
        const char* flag=std::getenv("XM87330_MORPHUS_LOCATOR");
        if(!flag || std::strcmp(flag,"1")!=0)return;
        const char* path=std::getenv("XM87330_MORPHUS_LOCATOR_LOG");
        if(!path || !*path)return;
        std::FILE* log=std::fopen(path,"wb");
        if(!log)return;
        std::fputs("driver\topcode_size\tsymbol\teuro_address\texpected\t"
                   "actual_at_euro\tmatch_euro\ttotal_hits\t"
                   "candidate_addresses\tdelta_if_unique\n",log);
        unsigned count=0u;const Site* table=sites(&count);
        for(unsigned i=0u;i<count;++i) {
            const Site& s=table[i];
            std::uint8_t pattern[32]={};
            const unsigned n=decode(s.hex,pattern,32u);
            if(!n)continue;
            const Hits hits=scan(code,len,pattern,n);
            char actual[32]={};
            six(code,len,s.euroAddress,actual);
            std::fprintf(log,"%s\t%u\t%s\t%06X\t%s\t%s\t%s\t%u\t",
                driver,len,s.symbol,s.euroAddress,s.hex,actual,
                atOriginal(code,len,pattern,n,s.euroAddress)?"MATCH":"DIFFERENT",
                hits.total);
            for(unsigned j=0u;j<hits.stored;++j)
                std::fprintf(log,"%s%06X",j?",":"",hits.address[j]);
            std::fprintf(log,"\t");
            if(hits.total==1u)
                std::fprintf(log,"%+d",int(hits.address[0])-int(s.euroAddress));
            std::fputc('\n',log);
        }
        std::fclose(log);
        done=true;
    }
};
static inline State& state(){static State s;return s;}
} // namespace XM87330Locator
#endif
