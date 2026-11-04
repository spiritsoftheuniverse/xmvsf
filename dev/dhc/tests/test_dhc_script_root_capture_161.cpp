// Stage 87.161: observation-only native root-entry capture must not
// count every animation-script pointer advance as an independent super.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_160_super_discovery.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace XM87160;
static void romW(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    r[p]=std::uint8_t(v);r[p+1]=std::uint8_t(v>>8u);
}
static void romL(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    romW(r,p,v>>16u);romW(r,p+2u,v);
}
static void ramB(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    r[(p&0xFFFFu)^1u]=std::uint8_t(v);
}
static void ramW(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    ramB(r,p,v>>8u);ramB(r,p+1u,v);
}
static void ramL(std::vector<std::uint8_t>& r,unsigned p,unsigned v){
    ramW(r,p,v>>16u);ramW(r,p+2u,v);
}
int main(){
    assert(setenv("XM87160_DISCOVER","1",1)==0);
    std::vector<std::uint8_t> rom(0xE1000u,0u),ram(65536u,0u);
    const unsigned fighter=0x14u,root=0xD378Eu,
       section=0xD3966u,script=0xD6722u;
    romL(rom,0x8A946u+fighter*2u,root);
    romW(rom,root+0x2Eu,section-root);
    romW(rom,section,script-section);
    romW(rom,script+2u,4u);
    romW(rom,script+6u,0xC130u);
    ramB(ram,0x5000u,8u);
    ramW(ram,0x4052u,fighter);
    ramW(ram,0x4852u,0x12u);
    ramB(ram,0x404Bu,1u);
    ramW(ram,0x4060u,0u);
    State state;
    // Input DOWN -> DOWN/FORWARD -> FORWARD, 2-punch edge.
    state.frame(ram.data(),rom.data(),unsigned(rom.size()),
                97u,4u,0u,0u,0u,false);
    state.frame(ram.data(),rom.data(),unsigned(rom.size()),
                98u,5u,0u,0u,0u,false);
    ramW(ram,0x4060u,0x2E00u);
    ramL(ram,0x4034u,script);
    state.frame(ram.data(),rom.data(),unsigned(rom.size()),
                99u,1u,0u,3u,0u,false);
    assert(state.observedCount==1u);
    assert(state.lastObservedAction[0]==0x2E00u);
    assert(state.lastObservedScript[0]==script);
    char path[96]={};
    state.directionPath(0u,99u,path,sizeof(path));
    assert(std::strcmp(path,"2>3>6")==0);
    // A native actor commonly increments the sprite script pointer
    // by 0x10 every animation step while action 2E00 remains unchanged.
    for(unsigned frame=100u;frame<113u;++frame){
        ramL(ram,0x4034u,script+16u+((frame-100u)%6u)*16u);
        state.frame(ram.data(),rom.data(),unsigned(rom.size()),
                    frame,1u,0u,0u,0u,false);
        assert(state.observedCount==1u);
    }
    // A genuine visit to the canonical script root is independently
    // detectable, but still does NOT prove a fresh input command.
    ramL(ram,0x4034u,script);
    state.frame(ram.data(),rom.data(),unsigned(rom.size()),
                113u,1u,0u,0u,0u,false);
    assert(state.observedCount==2u);
    return 0;
}
