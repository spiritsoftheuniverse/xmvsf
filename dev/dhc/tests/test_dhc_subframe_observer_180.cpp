// ROM-free C++11 test: subframe input logger never writes native actor RAM.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87142 {
static unsigned ramB(const std::uint8_t* r,unsigned a){return r[(a&65535u)^1u];}
static unsigned ramW(const std::uint8_t* r,unsigned a){
    return (ramB(r,a)<<8u)|ramB(r,a+1u);
}
}
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_180_subframe_input_observer.h"
static void wb(std::uint8_t* r,unsigned a,unsigned v){r[(a&65535u)^1u]=(std::uint8_t)v;}
static void ww(std::uint8_t* r,unsigned a,unsigned v){wb(r,a,v>>8u);wb(r,a+1u,v);}
int main(int argc,char**argv){
    const char* path=argc>1?argv[1]:"test_dhc_subframe_observer_180.tsv";
#ifdef _WIN32
    assert(_putenv_s("XM87180_NATIVE_SUBFRAME_LOG",path)==0);
#else
    assert(setenv("XM87180_NATIVE_SUBFRAME_LOG",path,1)==0);
#endif
    std::uint8_t ram[65536]={};
    wb(ram,0x5000,8);ww(ram,0x4052,0x12);ww(ram,0x4852,0x14);
    ww(ram,0x4060,0x2E08);ww(ram,0x40A0,0x2A28);
    std::uint8_t original[65536];
    std::memcpy(original,ram,sizeof(original));
    auto& obs=XM87180Subframe::state();
    obs.sample(ram,123,100,"BEFORE_CPU",11,true,true,2,3,0x12,0x34,0x56);
    obs.sample(ram,123,200,"AFTER_PRE_DISPLAY_CPU",11,true,true,2,3,0x12,0x34,0x56);
    obs.sample(ram,123,300,"AFTER_SCAN_0",11,true,true,2,3,0x12,0x34,0x56);
    // Simulated original 68000 input words change between CPU slices.
    ww(ram,0x40C0,0x0031);
    ww(ram,0x40C2,0x0031);
    ww(ram,0x40C6,0x0032);
    ww(ram,0x40CA,0x0000);
    obs.sample(ram,123,400,"AFTER_SCAN_1",11,true,true,2,3,0x12,0x34,0x56);
    obs.sample(ram,123,500,"AFTER_VBLANK_CPU",11,true,true,2,3,0x12,0x34,0x56);
    obs.sample(ram,123,510,"AFTER_DHC_HOST",2,true,true,2,3,0x12,0x34,0x56);
    assert(obs.enabled && obs.log);
    std::fflush(obs.log);
    std::FILE* f=std::fopen(path,"rb"); assert(f);
    char buffer[12000]={};
    size_t bytes=std::fread(buffer,1,sizeof(buffer)-1,f);
    std::fclose(f);
    assert(bytes>500);
    assert(std::strstr(buffer,"BEFORE_CPU"));
    assert(std::strstr(buffer,"AFTER_PRE_DISPLAY_CPU"));
    assert(std::strstr(buffer,"AFTER_SCAN_1"));
    assert(std::strstr(buffer,"AFTER_VBLANK_CPU"));
    assert(std::strstr(buffer,"AFTER_DHC_HOST"));
    assert(std::strstr(buffer,"\t0031\t0031\t"));
    assert(std::strstr(buffer,"\t0032\t"));
    // Only the test's own emulated ROM writes above may alter RAM.
    for(unsigned i=0;i<65536;i++){
        const bool explicitlyChanged=i==((0x40C0u)^1u) ||
            i==((0x40C1u)^1u) || i==((0x40C2u)^1u) ||
            i==((0x40C3u)^1u) || i==((0x40C6u)^1u) ||
            i==((0x40C7u)^1u);
        if(!explicitlyChanged) assert(ram[i]==original[i]);
    }
    std::puts("PASS 87.180: read-only, intra-frame CPS2 signal staging and log persistence");
}
