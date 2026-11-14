// 87.338: preserve the 87.336 live-playtested exact Ryu/Ken native
// D6->phase-zero->original action 2E Hyper routes while widening the cast.
// Synthetic ROM-free C++11 fixture. Not added to normal BUILD_TEST.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_336_native_d6_dispatch.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
using namespace XM87336Dispatch;
static void be(std::vector<std::uint8_t>& c,unsigned at,
               const std::uint8_t* src,unsigned n){
    assert(at+n<=c.size() && !(at&1u) && !(n&1u));
    for(unsigned i=0;i<n;++i)c[at+(i^1u)]=src[i];
}
static void w(std::vector<std::uint8_t>& c,unsigned at,unsigned value){
    c[at]=std::uint8_t(value);c[at+1u]=std::uint8_t(value>>8u);
}
static void rw(std::vector<std::uint8_t>& c,unsigned at,unsigned value){
    c[at]=std::uint8_t(value);c[at+1u]=std::uint8_t(value>>8u);
}
static void actorRom(std::vector<std::uint8_t>& rom,unsigned fighter,
                     unsigned fighterTable,unsigned section){
    const unsigned p=0x8A946u+fighter*2u;
    rw(rom,p,fighterTable>>16u);
    rw(rom,p+2u,fighterTable);
    assert(section>fighterTable && section-fighterTable<=65535u);
    rw(rom,fighterTable+0x2Eu,section-fighterTable);
}
static void script(std::vector<std::uint8_t>& rom,unsigned section,
                   unsigned sel,unsigned at){
    assert(at>section && at-section<=65535u);
    rw(rom,section+sel,at-section);
}
static void putState(std::vector<std::uint8_t>& c,
                     unsigned state,unsigned d6,unsigned handler,
                     unsigned phaseZero){
    static const std::uint8_t first[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t second[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    be(c,state,first,sizeof(first));
    const unsigned table=state+sizeof(first);
    assert(handler>table && handler-table<=65535u);
    w(c,table+d6,handler-table);
    be(c,handler,second,sizeof(second));
    const unsigned phaseTable=handler+sizeof(second);
    assert(phaseZero>phaseTable && phaseZero-phaseTable<=65535u);
    w(c,phaseTable,phaseZero-phaseTable);
}
static void putLaunch(std::vector<std::uint8_t>& c,
                      unsigned at,unsigned selector){
    const std::uint8_t bytes[]={
        0x70,0x2E,0x72,std::uint8_t(selector),
        0x4E,0xF9,0x00,0x01,0x03,0x8E
    };
    be(c,at,bytes,sizeof(bytes));
}
int main(){
    std::vector<std::uint8_t> code(0x50000u,0u);
    std::vector<std::uint8_t> rom(0x120000u,0u);
    // Exact per-character D6 dispatcher and ROM script footprints proven
    // by the previously accepted original US 87.174 control-flow audit.
    putState(code,0x0337BAu,0x0Au,0x033D92u,0x033DA6u);
    putState(code,0x0337BAu,0x0Cu,0x033F0Eu,0x033F28u);
    putState(code,0x035356u,0x0Cu,0x0359F6u,0x035A10u);
    putState(code,0x035356u,0x0Eu,0x035C42u,0x035C5Au);
    static const std::uint8_t ryuConditional[]={
        0x70,0x2E,0x72,0x08,0x0C,0x2E,0x00,0x04,
        0x00,0xA4,0x66,0x02,0x72,0x0C,
        0x4E,0xF9,0x00,0x01,0x03,0x8E
    };
    be(code,0x033E0Cu,ryuConditional,sizeof(ryuConditional));
    putLaunch(code,0x033F80u,0u);
    putLaunch(code,0x035A4Eu,0u);
    putLaunch(code,0x035CB0u,2u);
    actorRom(rom,0x12u,0x0CFE00u,0x0D0000u);
    actorRom(rom,0x14u,0x0CFF00u,0x0D5000u);
    script(rom,0x0D0000u,0u,0x0D2CFCu);
    script(rom,0x0D0000u,8u,0x0D3070u);
    script(rom,0x0D0000u,12u,0x0D31A8u);
    script(rom,0x0D5000u,0u,0x0D6722u);
    script(rom,0x0D5000u,2u,0x0D6976u);
    const struct {unsigned fighter,d6,state,script;} expected[]={
        {0x12u,0x0Au,0x0337BAu,0x0D3070u},
        {0x12u,0x0Cu,0x0337BAu,0x0D2CFCu},
        {0x14u,0x0Cu,0x035356u,0x0D6722u},
        {0x14u,0x0Eu,0x035356u,0x0D6976u}
    };
    // There are intentionally NO synthetic fighter indexers. An accidental
    // "all other fighters ready" dependency must not break known-good DHC.
    for(const auto& e:expected){
        const auto path=resolve(code.data(),unsigned(code.size()),
                                rom.data(),unsigned(rom.size()),e.fighter,e.d6);
        assert(path.valid());
        assert(path.nativeDispatcher==e.state);
        assert(path.script==e.script);
    }
    code[0x035CB0u]^=1u; // mutate Ken Shinryuken original launch opcode
    assert(!resolve(code.data(),unsigned(code.size()),
                    rom.data(),unsigned(rom.size()),0x14u,0x0Eu).valid());
    std::puts("XMVSF 87.338 original US Ryu/Ken Hyper baseline PASS");
}
