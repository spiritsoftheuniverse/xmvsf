// Standalone C++11 ROM-free test of 87.182 executed-PC callback isolation.
// Compile within the existing repo. No FBNeo runtime or proprietary ROM.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
unsigned int SekGetPPC(int);
unsigned int SekGetPC(int);
namespace XM87142 {
static unsigned ramB(const std::uint8_t* r,unsigned p){return r[(p&65535u)^1u];}
static unsigned ramW(const std::uint8_t* r,unsigned p){
    return (ramB(r,p)<<8u)|ramB(r,p+1u);
}
}
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_182_native_pc_trace.h"
static unsigned int fakePPC=0,fakePC=0;
static unsigned int priorCalled=0;
static int originalCallback(int){++priorCalled;return 0;}
static int (*currentCallback)(int)=originalCallback;
unsigned int SekGetPPC(int){return fakePPC;}
unsigned int SekGetPC(int){return fakePC;}
extern "C" {
int (*m68k_get_insn_cb(void))(int){return currentCallback;}
void m68k_set_insn_cb(int (*f)(int)){currentCallback=f;}
unsigned int m68k_get_reg(void*,m68k_register_t r){
    if(r==M68K_REG_A6)return 0xFF4000u;
    if(r==M68K_REG_D0)return 0x0000002Cu;
    if(r==M68K_REG_D1)return 0x00000004u;
    return 0u;
}
}
static void wb(std::uint8_t*r,unsigned p,unsigned v){r[p^1u]=(std::uint8_t)v;}
static void ww(std::uint8_t*r,unsigned p,unsigned v){wb(r,p,v>>8u);wb(r,p+1u,v);}
int main(int argc,char**argv){
    const char* path=argc>1?argv[1]:"test_dhc_native_pc_trace_182.tsv";
#ifdef _WIN32
    assert(_putenv_s("XM87182_NATIVE_PC_TRACE","1")==0);
    assert(_putenv_s("XM87182_NATIVE_PC_TRACE_LOG",path)==0);
#else
    assert(setenv("XM87182_NATIVE_PC_TRACE","1",1)==0);
    assert(setenv("XM87182_NATIVE_PC_TRACE_LOG",path,1)==0);
#endif
    std::uint8_t ram[65536]={};
    wb(ram,0x5000u,8u);ww(ram,0x4052u,0x12u);
    ww(ram,0x4852u,0x14u);ww(ram,0x4060u,0x2E08u);
    ww(ram,0x40A0u,0x002Au);wb(ram,0x4256u,8u);wb(ram,0x4259u,8u);
    auto& obs=XM87182PC::state();
    obs.begin(ram,7016u,2u,true,false);
    assert(obs.active && obs.enabled);
    assert(currentCallback==&XM87182PC::State::bridge);
    fakePPC=0x01042Cu;fakePC=0x010430u;
    currentCallback(8);
    assert(obs.count==1u);
    // Native input producer generated new MP+HP edge.
    ww(ram,0x40CAu,0x0060u);
    fakePPC=0x010448u;fakePC=0x01044Eu;
    currentCallback(16);
    assert(obs.count==2u);
    assert(obs.entries[1].changed&64u);
    // Original ROM selects action 0002 after the punch edge.
    ww(ram,0x4060u,0x0002u);ww(ram,0x40A0u,0x0200u);
    fakePPC=0x033000u;fakePC=0x033004u;
    currentCallback(12);
    assert(obs.count==3u);
    assert((obs.entries[2].changed&3u)==3u);
    // Even outside the initial watch regions, the exact PC that changed
    // native action must be captured instead of silently discarded.
    ww(ram,0x4060u,0x2C04u);
    fakePPC=0x123456u;fakePC=0x123458u;
    currentCallback(12);
    assert(obs.count==4u && obs.entries[3].pc==0x123456u);
    assert(obs.entries[3].changed&1u);
    assert(obs.entries[3].fromPc==0x033000u);
    assert(obs.entries[3].d0==0x2Cu && obs.entries[3].d1==0x04u);
    assert(priorCalled==4u); // wrapped previous callback still ran
    obs.end();
    assert(!obs.active && currentCallback==originalCallback);
    assert(obs.count==4u && obs.visited==4u);
    std::FILE*f=std::fopen(path,"rb");assert(f);
    char buffer[12000]={};
    const size_t n=std::fread(buffer,1,sizeof(buffer)-1,f);
    std::fclose(f);
    assert(n>400u);
    assert(std::strstr(buffer,"XM87.182 NATIVE PC SESSION"));
    assert(std::strstr(buffer,"123456"));
    assert(std::strstr(buffer,"2C04"));
    assert(std::strstr(buffer,"# DHC_FRAME frame=7016 watched=4"));
    // Gate rejects idle/other-character frames without touching callback.
    obs.begin(ram,7017u,0u,true,false);
    assert(!obs.active && currentCallback==originalCallback);
    std::puts("PASS 87.182: exact original PPC state changes, callback preserved, no intrusive CPU hooks");
}
