// 87.189: ROM-free C++11 original 68000 2E-family lifecycle.
// No aerial special cases, hardcoded move names, or fabricated controller.
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_189_native_super_family.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h"
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
    // Original 87.189 capture of Ryu's 2E00 Tatsu sequence.
    // Selector address order is NOT script address order: 2E10
    // points to D2DF4 before 2E04 points to D2E38.
    add(rom,0x12u,0xCFCA2u,0x00u,0xD2CFCu);
    add(rom,0x12u,0xCFCA2u,0x02u,0xD2DB0u);
    add(rom,0x12u,0xCFCA2u,0x10u,0xD2DF4u);
    add(rom,0x12u,0xCFCA2u,0x04u,0xD2E38u);
    add(rom,0x12u,0xCFCA2u,0x06u,0xD2E7Cu);
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
    assert(g.update(130u,ground,rom.data(),rom.size())==CONTINUATION_BEGIN);
    assert(g.phase==CONTINUATION && g.recognizes());
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
    assert(air.preCpuActive(5260u,a));
    assert(air.preCpuActive(5261u,a));
    assert(!air.preCpuActive(5262u,a));
    assert(XM87177Chords::shieldPrequeue3P(
        true,true,0u,true,0x12u,0x14u,0x2E0Cu,0x2A1Cu,
        0x07u,false,air.preCpuActive(5261u,a)));
    assert(!XM87177Chords::shieldPrequeue3P(
        true,true,0u,true,0x12u,0x14u,0x2E0Cu,0x2A1Cu,
        0x07u,false,false));
    assert(!XM87177Chords::shieldPrequeue3P(
        true,true,0u,true,0x12u,0x14u,0x2E0Cu,0x2A1Cu,
        0x27u,false,true)); // mixed kick cannot be a punch DHC
    a.script=0xD3208u;
    assert(air.update(5292u,a,rom.data(),rom.size())==NOTHING);
    assert(air.activeAt(5292u,a));
    a.action=0x2E0Eu;a.step=6u;a.script=0xD322Cu;
    assert(air.update(5326u,a,rom.data(),rom.size())==CONTINUATION_BEGIN);
    assert(air.phase==CONTINUATION && air.recognizes());
    assert(!air.activeAt(5326u,a));
    assert(!air.preCpuActive(5327u,a));
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
    // Actual 87.189 Ryu Tatsu at 8286. 8352 did NOT exit:
    // root 2E00/step04 rewound to the original D2CFC ROM script,
    // then the original 68000 continued 2E02, 2E10, 2E04, 2E06.
    Tracker tatsu;
    a=Sample(0x12u,0x2E00u,0x2A28u,2u,0xD2CFCu,0x0Cu);
    assert(tatsu.update(8286u,a,rom.data(),rom.size())==ENTER);
    assert(tatsu.phase==STARTUP && tatsu.nextScript==0xD2DB0u);
    a.step=4u;a.script=0xD2D6Cu;
    assert(tatsu.update(8347u,a,rom.data(),rom.size())==ATTACK_BEGIN);
    assert(tatsu.activeAt(8347u,a));
    a.script=0xD2D9Cu;
    assert(tatsu.update(8350u,a,rom.data(),rom.size())==NOTHING);
    a.script=0xD2CFCu; // ROM loops back, still ATTACK.
    assert(tatsu.update(8352u,a,rom.data(),rom.size())==REPEATED_ROOT_SCRIPT);
    assert(tatsu.live && tatsu.phase==ATTACK && tatsu.activeAt(8352u,a));
    a.action=0x2E02u;a.step=6u;a.script=0xD2DB0u;
    assert(tatsu.update(8354u,a,rom.data(),rom.size())==CONTINUATION_BEGIN);
    assert(tatsu.phase==CONTINUATION && tatsu.recognizes());
    assert(!tatsu.activeAt(8354u,a)); // NO unverified tag in follow-up.
    a.script=0xD2DE0u;
    assert(tatsu.update(8364u,a,rom.data(),rom.size())==NOTHING);
    a.script=0xD2DB0u; // looping attack follow-up, NOT action EXIT.
    assert(tatsu.update(8366u,a,rom.data(),rom.size())==NOTHING);
    assert(tatsu.live && tatsu.phase==CONTINUATION);
    a.action=0x2E10u;a.step=8u;a.script=0xD2DF4u;
    const Event stage8=tatsu.update(8447u,a,rom.data(),rom.size());
    if(stage8!=CONTINUATION_STAGE_CHANGE){
       const auto x=XM87142::resolve(rom.data(),rom.size(),
                                   0x12u,0x2Eu,0x10u);
       std::fprintf(stderr,"87.190 TROUBLE 2E10: event=%u phase=%u live=%u stage=%04X"
         " stageRoot=%08X stageEnd=%08X decodedValid=%u decodedRoot=%08X"
         " nativeAction=%04X nativeScript=%08X D6=%02X\\n",
         unsigned(stage8),unsigned(tatsu.phase),unsigned(tatsu.live),
         tatsu.stageAction,tatsu.stageScript,tatsu.stageEnd,
         unsigned(x.valid),x.script,a.action,a.script,a.d6);
    }
    assert(stage8==CONTINUATION_STAGE_CHANGE);
    assert(tatsu.stageEnd==0xD2E38u);
    a.action=0x2E04u;a.step=0x0Au;a.script=0xD2E38u;
    assert(tatsu.update(8466u,a,rom.data(),rom.size())==CONTINUATION_STAGE_CHANGE);
    a.action=0x2E06u;a.step=0x0Cu;a.script=0xD2E7Cu;
    assert(tatsu.update(8472u,a,rom.data(),rom.size())==CONTINUATION_STAGE_CHANGE);
    assert(tatsu.live && tatsu.phase==CONTINUATION);
    a.action=0x0200u;a.mode=0x0410u;
    assert(tatsu.update(8476u,a,rom.data(),rom.size())==ACTION_EXIT);
    assert(!tatsu.live && !tatsu.recognizes());
    // Invalid ROM script or D6 ownership changes must close immediately,
    // not widen availability by approximate action-number ranges.
    Tracker altered;
    a=Sample(0x12u,0x2E00u,0x2A28u,2u,0xD2CFCu,0x0Cu);
    assert(altered.update(9628u,a,rom.data(),rom.size())==ENTER);
    a.step=4u;a.script=0xD2D7Cu;
    assert(altered.update(9690u,a,rom.data(),rom.size())==ATTACK_BEGIN);
    a.action=0x2E02u;a.step=6u;a.script=0xD3010u;
    assert(altered.update(9696u,a,rom.data(),rom.size())==ACTION_EXIT);
    assert(!altered.live);
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
    std::puts("PASS 87.190 ROM 2E lifetime: 87.189 Tatsu rewind and full follow-up stages, ground, air, Ken, exact exit");
}
