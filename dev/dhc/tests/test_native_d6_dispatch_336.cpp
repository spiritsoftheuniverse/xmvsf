// 87.336 standalone C++11 ORIGINAL M68K D6 dispatch verification.
// Synthetic, legally ROM-free fixtures. Not added to BUILD_TEST gates.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_336_native_d6_dispatch.h"
#include <cassert>
#include <cstdio>
#include <vector>

static void be(std::vector<std::uint8_t>& mem,unsigned at,
               const std::uint8_t* bytes,unsigned n){
    for(unsigned i=0;i<n;++i)mem[at+(i^1u)]=bytes[i];
}
static void beWord(std::vector<std::uint8_t>& mem,unsigned at,unsigned w){
    mem[at]=std::uint8_t(w);mem[at+1u]=std::uint8_t(w>>8u);
}
static void romWord(std::vector<std::uint8_t>& rom,unsigned at,unsigned w){
    rom[at]=std::uint8_t(w);rom[at+1u]=std::uint8_t(w>>8u);
}
static void makeRom(std::vector<std::uint8_t>& rom,unsigned fighter,
                    unsigned table,unsigned selector,unsigned script){
    const unsigned at=0x8A946u+fighter*2u;
    romWord(rom,at,table>>16u);romWord(rom,at+2u,table&0xFFFFu);
    romWord(rom,table+0x2Eu,0x40u);
    const unsigned section=table+0x40u;
    assert(script>section && script-section<65536u);
    romWord(rom,section+selector,script-section);
}
int main(){
    std::vector<std::uint8_t> code(0x50000u,0u);
    std::vector<std::uint8_t> rom(0x100000u,0u);
    static const std::uint8_t stateSig[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t phaseSig[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    unsigned cases=0u;
    const auto* paths=XM87336Dispatch::cases(&cases);
    assert(cases==4u);
    for(unsigned i=0u;i<cases;++i){
        const auto& p=paths[i];
        be(code,p.state,stateSig,sizeof(stateSig));
        const unsigned table=p.state+sizeof(stateSig);
        beWord(code,table+p.d6,p.handler-table);
        be(code,p.handler,phaseSig,sizeof(phaseSig));
        const unsigned phaseTable=p.handler+sizeof(phaseSig);
        beWord(code,phaseTable,p.phaseZero-phaseTable);
        if(p.fighter==0x12u && p.d6==0x0Au){
            static const std::uint8_t ryu[]={
                0x70,0x2E,0x72,0x08,0x0C,0x2E,0x00,0x04,0x00,0xA4,
                0x66,0x02,0x72,0x0C,0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            be(code,p.launch,ryu,sizeof(ryu));
        }else {
            const std::uint8_t launch[]={
                0x70,0x2E,0x72,std::uint8_t(p.selector),
                0x4E,0xF9,0x00,0x01,0x03,0x8E
            };
            be(code,p.launch,launch,sizeof(launch));
        }
        makeRom(rom,p.fighter,p.fighter==0x12u?0xD2C00u:0xD6600u,
                p.selector,p.script);
    }
    for(unsigned i=0u;i<cases;++i){
        const auto& p=paths[i];
        const auto actual=XM87336Dispatch::resolve(
            code.data(),unsigned(code.size()),rom.data(),unsigned(rom.size()),
            p.fighter,p.d6);
        assert(actual.valid());
        assert(actual.nativeDispatcher==p.state);
        assert(actual.phaseZero==p.phaseZero);
        assert(actual.script==p.script);
    }
    assert(!XM87336Dispatch::resolve(
        code.data(),unsigned(code.size()),rom.data(),unsigned(rom.size()),
        0x0Au,0x16u).valid()); // no guessed Sabretooth code
    assert(!XM87336Dispatch::resolve(
        code.data(),unsigned(code.size()),rom.data(),unsigned(rom.size()),
        0x14u,0x08u).valid()); // no guessed Ken code
    const auto& ken=paths[3u];
    code[ken.launch]^=0x10u;
    assert(!XM87336Dispatch::resolve(
        code.data(),unsigned(code.size()),rom.data(),unsigned(rom.size()),
        ken.fighter,ken.d6).valid()); // refuse changed native action bytes
    std::puts("XMVSF 87.336 guarded original D6-to-2E native dispatcher PASS");
}
