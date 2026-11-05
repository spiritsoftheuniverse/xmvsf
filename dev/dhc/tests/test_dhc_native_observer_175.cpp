// ROM-free smoke test of Stage 87.175 observer. Does not link or run FBNeo.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
namespace XM87142 {
static inline unsigned ramB(const std::uint8_t* r,unsigned p){return r[(p&65535u)^1u];}
static inline unsigned ramW(const std::uint8_t* r,unsigned p){return (ramB(r,p)<<8u)|ramB(r,p+1u);}
static inline unsigned ramL(const std::uint8_t* r,unsigned p){return (ramW(r,p)<<16u)|ramW(r,p+2u);}
}
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_175_native_accept_observer.h"
static void wb(std::uint8_t*r,unsigned p,unsigned v){r[p^1u]=(std::uint8_t)v;}
static void ww(std::uint8_t*r,unsigned p,unsigned v){wb(r,p,v>>8u);wb(r,p+1u,v);}
static void putEnv(const char*key,const char*value){
#ifdef _WIN32
    assert(_putenv_s(key,value)==0);
#else
    assert(setenv(key,value,1)==0);
#endif
}
int main(int argc,char**argv){
    const char* out=argc>1?argv[1]:"test_dhc_native_observer_175.tsv";
    putEnv("XM87175_DHC_OBSERVER","1");
    putEnv("XM87175_DHC_OBSERVER_LOG",out);
    std::uint8_t ram[65536]={};
    wb(ram,0x5000u,8u);
    ww(ram,0x4052u,0x12u); ww(ram,0x4852u,0x14u);
    ww(ram,0x4452u,0x1Eu); ww(ram,0x4C52u,0x0Au);
    for(unsigned i=0;i<4u;++i){
        const auto a=XM87175Observer::read(ram,XM87175Observer::slotBase(i));
        assert(a.fighter==(i==0u?0x12u:i==1u?0x14u:i==2u?0x1Eu:0x0Au));
        assert(std::strcmp(XM87175Observer::slotName(i),(i&1u)?"RESERVE":"POINT")==0);
    }
    assert(XM87175Observer::slotBase(0)==0x4000u);
    assert(XM87175Observer::slotBase(1)==0x4800u);
    assert(XM87175Observer::slotBase(2)==0x4400u);
    assert(XM87175Observer::slotBase(3)==0x4C00u);
    auto& s=XM87175Observer::state();
    s.postCpu(ram,100u,200u,"IDLE",false,0u,0u);
    s.postDhc(ram,100u,200u,"IDLE",false,0u,0u);
    assert(s.enabled && !s.pending);
    s.postCpu(ram,101u,201u,"NATIVE_KEN_INPUT_SEQUENCE",true,3u,3u);
    ww(ram,0x4060u,0x2E00u);
    wb(ram,0x40D6u,0x0Cu);
    s.postDhc(ram,101u,201u,"NATIVE_KEN_INPUT_SEQUENCE",true,3u,3u);
    assert(!s.pending);
    s.postCpu(ram,102u,202u,"NATIVE_KEN_INPUT_SEQUENCE",true,1u,3u);
    s.postDhc(ram,102u,202u,"IDLE",true,1u,3u);
    std::FILE* f=std::fopen(out,"rb");
    assert(f);
    char b[16384]={};
    const auto n=std::fread(b,1,sizeof(b)-1,f);
    std::fclose(f);
    assert(n>300);
    assert(std::strstr(b,"POST_CPU_PRE_DHC"));
    assert(std::strstr(b,"POST_HOST_DHC_CHANGED"));
    assert(std::strstr(b,"POST_HOST_DHC_PHASE_ONLY"));
    assert(std::strstr(b,"\tPOINT\t0012\t"));
    assert(std::strstr(b,"\tRESERVE\t0014\t"));
    assert(std::strstr(b,"\t2E00\t"));
    std::puts("PASS: four actor offsets, opt-in append-only trace, host-write and phase-only attribution");
    return 0;
}
