// ROM-free 87.181 C++11 check: health log appears before combat and never edits RAM.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87142 {
static inline unsigned ramB(const std::uint8_t* r,unsigned a){
    return r[(a&65535u)^1u];
}
}
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_181_observer_health.h"
int main(int argc,char**argv){
    const char* path=(argc>1)?argv[1]:"test_dhc_observer_health_181.log";
#ifdef _WIN32
    assert(_putenv_s("XM87181_DIAGNOSTIC_STATUS",path)==0);
#else
    assert(setenv("XM87181_DIAGNOSTIC_STATUS",path,1)==0);
#endif
    std::uint8_t ram[65536]={};
    auto& h=XM87181Health::state();
    h.frame(100,"othergame",ram,ram,false,false);
    h.frame(101,"xmvsfu",ram,ram,true,true); // XMVSF boot, NOT combat
    assert(h.log && h.frames==2 && h.gameFrames==1 && h.combatFrames==0);
    ram[0x5000u^1u]=8u;
    h.frame(102,"xmvsfu",ram,ram,true,true);
    assert(h.frames==3 && h.gameFrames==2 && h.combatFrames==1);
    assert(h.firstCps && h.firstXmvsf && h.firstCombat);
    std::fflush(h.log);
    std::FILE* f=std::fopen(path,"rb");assert(f);
    char buffer[8192]={};
    const size_t n=std::fread(buffer,1,sizeof(buffer)-1,f);
    std::fclose(f);
    assert(n>200u);
    assert(std::strstr(buffer,"ENGINE_HEALTH_INITIALIZED"));
    assert(std::strstr(buffer,"CPS2_FIRST_FRAME"));
    assert(std::strstr(buffer,"XMVSF_FIRST_FRAME"));
    assert(std::strstr(buffer,"XMVSF_FIRST_COMBAT"));
    assert(std::strstr(buffer,"oldObserverReady=1"));
    assert(std::strstr(buffer,"combat=1"));
    assert(ram[0x5000u^1u]==8u);
    std::puts("PASS 87.181: boot/combat markers, observer readiness, no gameplay writes");
    return 0;
}
