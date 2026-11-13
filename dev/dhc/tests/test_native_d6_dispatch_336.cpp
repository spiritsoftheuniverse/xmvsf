// XMVSF 87.337: generic, ORIGINAL-68000 fighter-owned D6 test.
// Synthetic ROM-free structures for seven different character sections.
// Standalone C++11; NOT a new gate in BUILD_TEST.bat.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_336_native_d6_dispatch.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XM87336Dispatch;

static void be(std::vector<std::uint8_t>& c,unsigned at,
               const std::uint8_t* src,unsigned n){
    assert(at+n<=c.size() && !(at&1u) && !(n&1u));
    for(unsigned i=0;i<n;++i)c[at+(i^1u)]=src[i];
}
static void w(std::vector<std::uint8_t>& c,unsigned at,unsigned value){
    assert(at+1u<c.size());
    c[at]=std::uint8_t(value);c[at+1u]=std::uint8_t(value>>8u);
}
static void l(std::vector<std::uint8_t>& c,unsigned at,unsigned value){
    w(c,at,value>>16u);w(c,at+2u,value);
}
static void romW(std::vector<std::uint8_t>& c,unsigned at,unsigned value){
    c[at]=std::uint8_t(value);c[at+1u]=std::uint8_t(value>>8u);
}
static void makeRom(std::vector<std::uint8_t>& rom,unsigned fighter,
                    unsigned selector){
    const unsigned table=0x100000u+fighter*0x1000u;
    const unsigned at=0x8A946u+fighter*2u;
    romW(rom,at,table>>16u);
    romW(rom,at+2u,table);
    romW(rom,table+0x2Eu,0x80u);
    romW(rom,table+0x80u+selector,0x100u);
}
static void makeNative(std::vector<std::uint8_t>& code,
                       std::vector<std::uint8_t>& rom,
                       unsigned fighter,unsigned d6,unsigned selector,
                       unsigned index){
    const unsigned base=0x20000u+(fighter/2u)*0x1000u;
    // Source matcher call + condition + original literal D6/A0 writes.
    const unsigned m=base+0x100u+index*0x60u;
    static const std::uint8_t match[]={
        0x4E,0xB9,0x00,0x01,0x3C,0x30,0x67,0x06
    };
    be(code,m,match,sizeof(match));
    const std::uint8_t setD6[]={
        0x1D,0x7C,0x00,std::uint8_t(d6),0x00,0xD6
    };
    static const std::uint8_t setA0[]={
        0x1D,0x7C,0x00,0x2A,0x00,0xA0
    };
    be(code,m+0x10u,setD6,sizeof(setD6));
    be(code,m+0x18u,setA0,sizeof(setA0));

    static const std::uint8_t d6Sig[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,
        0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t phaseSig[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    const unsigned state=base+0x400u;
    const unsigned table=state+sizeof(d6Sig);
    const unsigned handler=base+0x500u+index*0x100u;
    const unsigned entry=handler+0x70u;
    be(code,state,d6Sig,sizeof(d6Sig));
    w(code,table+d6,handler-table);
    be(code,handler,phaseSig,sizeof(phaseSig));
    w(code,handler+sizeof(phaseSig),entry-(handler+sizeof(phaseSig)));
    const std::uint8_t action[]={
        0x70,0x2E,0x72,std::uint8_t(selector),
        0x4E,0xF9,0x00,0x01,0x03,0x8E
    };
    be(code,entry+0x20u,action,sizeof(action));
    makeRom(rom,fighter,selector);
}
int main(){
    std::vector<std::uint8_t> code(0x60000u,0u);
    std::vector<std::uint8_t> rom(0x200000u,0u);
    static const std::uint8_t idx[]={
        0x30,0x2E,0x00,0x52,0xD0,0x40,0x20,0x7B
    };
    // Ten independently structured ORIGINAL-style fighter indexers,
    // each with 19 point slots. The native 87.170 scanner requires ten.
    for(unsigned j=0u;j<10u;++j){
        const unsigned pc=0x1000u+j*0x80u;
        const unsigned table=0x3000u+j*0x80u;
        be(code,pc,idx,sizeof(idx));
        w(code,pc+8u,table-(pc+8u));
        w(code,pc+10u,0x4E90u);
        for(unsigned f=0u;f<19u;++f)
            l(code,table+f*4u,0x20000u+f*0x1000u);
    }
    static const struct {unsigned fighter,d6,selector,index;} cases[]={
        {0x00u,0x08u,0x00u,0u}, // Wolverine
        {0x02u,0x04u,0x00u,0u}, // Cyclops
        {0x04u,0x02u,0x00u,0u}, // Storm
        {0x14u,0x0Cu,0x00u,0u}, // Ken Shoryureppa
        {0x14u,0x0Eu,0x02u,1u}, // Ken Shinryuken, separate D6
        {0x1Au,0x0Au,0x00u,0u}, // Zangief assigned DHC alias
        {0x1Eu,0x10u,0x0Cu,0u}, // Akuma special D6
        {0x22u,0x0Eu,0x00u,0u}, // Cammy
        {0x24u,0x06u,0x00u,0u}  // Alt Chun-Li, optional native slot
    };
    for(unsigned i=0u;i<sizeof(cases)/sizeof(cases[0]);++i)
        makeNative(code,rom,cases[i].fighter,cases[i].d6,
                   cases[i].selector,cases[i].index);
    const auto bounds=identify(code.data(),unsigned(code.size()));
    assert(bounds.ready && bounds.indexers==10u);
    for(unsigned i=0u;i<sizeof(cases)/sizeof(cases[0]);++i){
        const auto& p=cases[i];
        const auto result=resolve(code.data(),unsigned(code.size()),
                                  rom.data(),unsigned(rom.size()),
                                  p.fighter,p.d6);
        assert(result.valid());
        assert(result.matchingHandlers==1u);
        assert(result.selector==p.selector);
        assert(result.nativeDispatcher==
               0x20000u+(p.fighter/2u)*0x1000u+0x400u);
    }
    assert(!resolve(code.data(),unsigned(code.size()),
                    rom.data(),unsigned(rom.size()),
                    0x10u,0x0Cu).valid()); // Apocalypse
    assert(!resolve(code.data(),unsigned(code.size()),
                    rom.data(),unsigned(rom.size()),
                    0x04u,0x0Au).valid()); // wrong D6 for Storm
    assert(!resolve(code.data(),unsigned(code.size()),
                    rom.data(),unsigned(rom.size()),
                    0x14u,0x04u).valid()); // unowned Ken D6
    const unsigned cyclops=0x20000u+0x1000u+0x100u;
    code[cyclops+1u]^=1u; // damage original matcher JSR
    assert(!resolve(code.data(),unsigned(code.size()),
                    rom.data(),unsigned(rom.size()),
                    0x02u,0x04u).valid());
    std::puts("XMVSF 87.337 universal native fighter-owned D6 test PASS");
}
