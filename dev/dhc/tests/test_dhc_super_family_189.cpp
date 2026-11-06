// 87.189: ROM-free C++11 original 68000 2E-family lifecycle.
// No aerial special cases, hardcoded move names, or fabricated controller.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_189_native_super_family.h"
using XM87184Life::Sample;
using namespace XM87189Family;
static void word(std::vector<std::uint8_t>& r,unsigned a,unsigned v){
    r[a]=std::uint8_t(v);r[a+1u]=std::uint8_t(v>>8u);
}
static void dword(std::vector<std::uint8_t>& r,unsigned a,unsigned v){
    word(r,a,v>>16u);word(r,a+2u,v);
}
static void add(std::vector<std::uint8_t>&rom,unsigned fighter,
                unsigned section,unsigned selector,unsigned script){
    const unsigned table=fighter==0x12u?0xCFAD4u:0xD378Eu;
    dword(rom,0x8A946u+fighter*2u,table);
    word(rom,table+0x2Eu,section-table);
    word(rom,section+selector,script-section);
}
int main(){
    std::vector<std::uint8_t> rom(0xE1000u,0u);
    add(rom,0x12u,0xCFCA2u,0x08u,0xD3070u);
    add(rom,0x12u,0xCFCA2u,0x0Au,0xD30F4u);
    add(rom,0x12u,0xCFCA2u,0x0Cu,0xD31A8u);
    add(rom,0x12u,0xCFCA2u,0x0Eu,0xD322Cu);
    add(rom,0x12u,0xCFCA2u,0x10u,0xD32B0u);
    add(rom,0x14u,0xD3966u,0x00u,0xD6722u);
    add(rom,0x14u,0xD3966u,0x02u,0xD6976u);
    add(rom,0x14u,0xD3966u,0x04u,0xD6A00u);
    Tracker g;
    Sample ground(0x12u,0x2E08u,0x2A28u,2u,0xD3070u,0x0Au);
    assert(g.update(100u,ground,rom.data(),rom.size())==ENTER);
    assert(g.phase==STARTUP && !g.activeAt(100u,ground));
    ground.step=4u;ground.script=0xD30C0u;
    assert(g.update(102u,ground,rom.data(),rom.size())==ATTACK_BEGIN);
    assert(g.activeAt(102u,ground));
    assert(!g.activeAt(103u,ground)); // same-frame freshness
    ground.action=0x2E0Au;ground.step=6u;ground.script=0xD30F4u;
    assert(g.update(130u,ground,rom.data(),rom.size())==RECOVERY_BEGIN);
    assert(!g.activeAt(130u,ground));
    ground.action=0u;ground.mode=0u;
    assert(g.update(150u,ground,rom.data(),rom.size())==ACTION_EXIT);
    assert(!g.live && !g.activeAt(150u,ground));
    Tracker air;
    Sample a(0x12u,0x2E0Cu,0x2A1Cu,2u,0xD31A8u,0x0Au);
    assert(air.update(5213u,a,rom.data(),rom.size())==ENTER);
    assert(air.rootAction==0x2E0Cu && air.rootScript==0xD31A8u);
    a.script=0xD31B8u;
    assert(air.update(5214u,a,rom.data(),rom.size())==NOTHING);
    a.step=4u;a.script=0xD31D8u;
    assert(air.update(5260u,a,rom.data(),rom.size())==ATTACK_BEGIN);
    assert(air.activeAt(5260u,a));
    a.script=0xD3208u;
    assert(air.update(5292u,a,rom.data(),rom.size())==NOTHING);
    assert(air.activeAt(5292u,a));
    a.action=0x2E0Eu;a.step=6u;a.script=0xD322Cu;
    assert(air.update(5326u,a,rom.data(),rom.size())==RECOVERY_BEGIN);
    assert(!air.activeAt(5326u,a));
    a.action=0x0002u;a.mode=0x0200u;a.d6=0u;
    assert(air.update(5400u,a,rom.data(),rom.size())==ACTION_EXIT);
    assert(!air.recognizes());
    // Second observed air version had 2A28, not 2A1C.
    Tracker otherAir;
    a=Sample(0x12u,0x2E0Cu,0x2A28u,2u,0xD31A8u,0x0Au);
    assert(otherAir.update(5486u,a,rom.data(),rom.size())==ENTER);
    a.step=4u;a.script=0xD3208u;
    assert(otherAir.update(5550u,a,rom.data(),rom.size())==ATTACK_BEGIN);
    assert(otherAir.activeAt(5550u,a));
    // Ken's genuine native 2E super uses D6=0C and stays in same action.
    Tracker ken;
    a=Sample(0x14u,0x2E00u,0x2A02u,2u,0xD6722u,0x0Cu);
    assert(ken.update(1971u,a,rom.data(),rom.size())==ENTER);
    a.step=4u;a.script=0xD6752u;
    assert(ken.update(1972u,a,rom.data(),rom.size())==ATTACK_BEGIN);
    assert(ken.activeAt(1972u,a));
    a.step=8u;a.script=0xD6812u;
    assert(ken.update(2020u,a,rom.data(),rom.size())==NOTHING);
    assert(ken.activeAt(2020u,a));
    // Do not infer super roots from 2A mode, D6, or an approximate script.
    Tracker falseRoot;
    a=Sample(0x12u,0x2E0Cu,0x2A1Cu,2u,0xD31B8u,0x0Au);
    assert(falseRoot.update(100u,a,rom.data(),rom.size())==NOTHING);
    a.script=0xD31A8u;a.d6=0u;
    assert(falseRoot.update(101u,a,rom.data(),rom.size())==NOTHING);
    a.d6=0x0Au;a.action=0x2C0Cu;
    assert(falseRoot.update(102u,a,rom.data(),rom.size())==NOTHING);
    std::puts("PASS 87.189 original ROM 2E family: ground, both air modes, Ken controller, exact action exit");
}
