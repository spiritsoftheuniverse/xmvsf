// ROM-free C++11 smoke: original action lifetime and opening PPC schedule.
// The test changes synthetic RAM as the ROM would; the observer may not.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_186_opening_observer.h"
static void wb(std::uint8_t*r,unsigned p,unsigned v){r[p^1u]=(std::uint8_t)v;}
static void ww(std::uint8_t*r,unsigned p,unsigned v){wb(r,p,v>>8u);wb(r,p+1u,v);}
static void prepare(std::uint8_t*r,unsigned action,unsigned step,unsigned script) {
    ww(r,0x4060u,action);ww(r,0x4008u,step);
    ww(r,0x4034u,script>>16u);ww(r,0x4036u,script&0xffffu);
}
static void probe(std::uint8_t*r,unsigned frame,unsigned buttons=0u) {
    std::uint8_t snapshot[65536];
    std::memcpy(snapshot,r,sizeof(snapshot));
    XM87186Opening::state().sample(r,frame,"PRE_CPU",0u,2u,buttons);
    assert(std::memcmp(snapshot,r,sizeof(snapshot))==0);
}
int main(int argc,char**argv) {
    const char*path=argc>1?argv[1]:"test_dhc_opening_186.tsv";
#ifdef _WIN32
    assert(_putenv_s("XM87186_OPENING_OBSERVER","1")==0);
    assert(_putenv_s("XM87186_OPENING_OBSERVER_LOG",path)==0);
#else
    assert(setenv("XM87186_OPENING_OBSERVER","1",1)==0);
    assert(setenv("XM87186_OPENING_OBSERVER_LOG",path,1)==0);
#endif
    std::uint8_t ram[65536]={};
    wb(ram,0x5000u,8u);ww(ram,0x4052u,0x12u);
    ww(ram,0x4852u,0x14u);ww(ram,0x40A0u,0x2A28u);
    wb(ram,0x40D6u,0x0Au);ww(ram,0x40C0u,0x0001u);
    prepare(ram,0x2E08u,2u,0x0D3070u);
    probe(ram,100u);assert(XM87186Opening::state().wantsPcTrace(100u));
    for(unsigned f=101u;f<104u;++f){
        probe(ram,f);assert(XM87186Opening::state().wantsPcTrace(f));
    }
    probe(ram,104u);assert(!XM87186Opening::state().wantsPcTrace(104u));
    // Real super startup lasts 66-67 frames; split punch edges
    // while still in startup must request forensic executed-PC capture.
    ww(ram,0x40C0u,0x0031u);
    probe(ram,140u);assert(XM87186Opening::state().wantsPcTrace(140u));
    ww(ram,0x40C0u,0x0001u);
    prepare(ram,0x2E08u,4u,0x0D30B0u);
    probe(ram,166u);assert(XM87186Opening::state().wantsPcTrace(166u));
    for(unsigned f=167u;f<=190u;++f){
        probe(ram,f);assert(XM87186Opening::state().wantsPcTrace(f));
    }
    probe(ram,191u);assert(!XM87186Opening::state().wantsPcTrace(191u));
    prepare(ram,0x2E0Au,6u,0x0D30F4u);
    probe(ram,222u);assert(!XM87186Opening::state().wantsPcTrace(222u));
    prepare(ram,0x0000u,0u,0x0CFD08u);
    probe(ram,263u);assert(!XM87186Opening::state().lifetime.live);
    assert(XM87186Opening::state().lifetime.endAt==263u);
    std::fflush(XM87186Opening::state().log);
    std::FILE*f=std::fopen(path,"rb");assert(f);
    char data[18000]={};const size_t n=std::fread(data,1,sizeof(data)-1,f);
    std::fclose(f);assert(n>1200u);
    assert(std::strstr(data,"ACTUAL_NATIVE_ROOT"));
    assert(std::strstr(data,"ACTUAL_ATTACK_START"));
    assert(std::strstr(data,"ACTUAL_RECOVERY_START"));
    assert(std::strstr(data,"ORIGINAL_ACTION_EXIT_OR_HOST_INTERRUPT"));
    std::puts("PASS 87.186: root/attack/recovery/exit and selective original-PC capture, no RAM mutation");
}
