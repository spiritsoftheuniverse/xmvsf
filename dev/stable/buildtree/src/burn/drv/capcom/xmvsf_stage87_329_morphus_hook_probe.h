#ifndef XMVSF_STAGE87_329_MORPHUS_HOOK_PROBE_H
#define XMVSF_STAGE87_329_MORPHUS_HOOK_PROBE_H
// Read-only opcode comparison with Morphus EURO 961004 native 2P patch hooks.
// CpsCode is the decrypted 68000 OPCODE view, not CpsRom encrypted data.
// Never patch an XMVSFU hook unless its instruction stream is verified.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87329HookProbe {
struct Site { unsigned address; const char* name; std::uint8_t expected[6]; };
static inline const Site* sites() {
    static const Site data[] = {
        {0x13AA8u,"DHC_CHAIN",{0x0C,0x2E,0x00,0x04,0x00,0xA4}},
        {0x11DB0u,"INCOMING_HYPER_COMMIT",{0x30,0x2E,0x00,0x52,0xD0,0x40}}
    };
    return data;
}
static inline bool matches(const std::uint8_t* data,unsigned size,const Site& site) {
    return data && site.address<=size && size-site.address>=6u &&
           std::memcmp(data+site.address,site.expected,6u)==0;
}
static inline void hex6(const std::uint8_t* p,unsigned size,unsigned address,char* out) {
    if (!p || address>size || size-address<6u) { std::strcpy(out,"OUT_OF_BOUNDS"); return; }
    for(unsigned i=0;i<6u;i++) std::sprintf(out+2*i,"%02X",unsigned(p[address+i]));
    out[12]=0;
}
struct State {
    bool done=false;
    void frame(const std::uint8_t* op,unsigned len,const char* driver) {
        if(done || !op || !driver || std::strncmp(driver,"xmvsf",5)!=0) return;
        const char* enabled=std::getenv("XM87329_DHC_HOOK_PROBE");
        if(!enabled || std::strcmp(enabled,"1")!=0) return;
        const char* path=std::getenv("XM87329_DHC_HOOK_PROBE_LOG");
        if(!path || !*path) return;
        std::FILE* f=std::fopen(path,"wb");
        if(!f) return;
        std::fprintf(f,"driver\topcode_size\tsite\taddress\texpected\tactual\tmatch_euro\n");
        for(unsigned i=0;i<2u;i++) {
            const Site& site=sites()[i];
            char actual[32]={}, expected[32]={};
            hex6(op,len,site.address,actual);
            for(unsigned k=0;k<6u;k++) std::sprintf(expected+k*2,"%02X",unsigned(site.expected[k]));
            std::fprintf(f,"%s\t%u\t%s\t%06X\t%s\t%s\t%s\n",
                driver,len,site.name,site.address,expected,actual,
                matches(op,len,site)?"MATCH":"DIFFERENT");
        }
        std::fclose(f);
        done=true;
    }
};
static inline State& state() { static State s; return s; }
} // namespace XM87329HookProbe
#endif
