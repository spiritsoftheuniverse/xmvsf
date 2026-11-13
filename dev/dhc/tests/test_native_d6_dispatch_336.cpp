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
    // Cammy's original D6 dispatch is fighter-owned, but the native
    // phase helper may be in a shared code region (outside the inferred
    // source boundary); a strict local-bounds test is a false veto.
    const unsigned handler=fighter==0x22u?
        0x47000u:base+0x500u+index*0x100u;
    const unsigned entry=handler+0x70u;
    be(code,state,d6Sig,sizeof(d6Sig));
    w(code,table+d6,handler-table);
    be(code,handler,phaseSig,sizeof(phaseSig));
    w(code,handler+sizeof(phaseSig),entry-(handler+sizeof(phaseSig)));
    // Three legitimate original-68000 actor action-2E launch encodings:
    // MOVEQ/JMP (Ken), MOVEQ/JSR (Cyclops), MOVE.W/JSR (Storm).
    if(fighter==0x04u){
        const std::uint8_t action[]={
            0x30,0x3C,0x00,0x2E,0x32,0x3C,0x00,std::uint8_t(selector),
            0x4E,0xB9,0x00,0x01,0x03,0x8E
        };
        be(code,entry+0x20u,action,sizeof(action));
    }else{
        const std::uint8_t action[]={
            0x70,0x2E,0x72,std::uint8_t(selector),
            0x4E,std::uint8_t(fighter==0x02u?0xB9u:0xF9u),
            0x00,0x01,0x03,0x8E
        };
        be(code,entry+0x20u,action,sizeof(action));
    }
    makeRom(rom,fighter,selector);
}
int main(){
    std::vector<std::uint8_t> code(0x60000u,0u);
    std::vector<std::uint8_t> rom(0x200000u,0u);
    static const std::uint8_t idx[]={
        0x30,0x2E,0x00,0x52,0xD0,0x40,0x20,0x7B
    };
    // 87.337's all-or-nothing gate refused EVERY incoming fighter
    // unless ten full native dispatcher maps included every character.
    // A genuine fighter-owned original indexer can omit unrelated IDs.
    // Construct one 15/18 original-layout indexer with three absent IDs
    // (none of them the target fighters below). All target moves must
    // remain independently checked by their real D6/phase-zero/2E bytes.
    const unsigned pc=0x1000u;
    const unsigned table=0x3000u;
    be(code,pc,idx,sizeof(idx));
    w(code,pc+8u,table-(pc+8u));
    w(code,pc+10u,0x4E90u);
    for(unsigned f=0u;f<19u;++f)
        l(code,table+f*4u,(f==4u||f==5u||f==6u)?0u:
          0x20000u+f*0x1000u);
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
    assert(bounds.ready && bounds.indexers==1u);
    assert(bounds.begin[4u]==kNotFound);
    assert(bounds.begin[5u]==kNotFound);
    assert(bounds.begin[6u]==kNotFound);
    for(unsigned i=0u;i<sizeof(cases)/sizeof(cases[0]);++i){
        const auto& p=cases[i];
        const auto result=resolveGeneric(
                                  code.data(),unsigned(code.size()),
                                  rom.data(),unsigned(rom.size()),
                                  p.fighter,p.d6);
        assert(result.valid());
        assert(result.matchingHandlers==1u);
        assert(result.hasNativeMatcherSource);
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
    assert(!resolveGeneric(code.data(),unsigned(code.size()),
                           rom.data(),unsigned(rom.size()),
                           0x14u,0x04u).valid()); // unowned Ken D6
    const unsigned cyclops=0x20000u+0x1000u+0x100u;
    code[cyclops+1u]^=1u; // alter canonical source matcher
    const auto unusual=resolveGeneric(code.data(),unsigned(code.size()),
                                      rom.data(),unsigned(rom.size()),0x02u,0x04u);
    assert(unusual.valid() && !unusual.hasNativeMatcherSource);
    // A fighter's D6 handler and original 2E launch must still be proven.
    const unsigned launch=0x20000u+0x1000u+0x500u+0x70u+0x20u;
    code[launch+1u]^=0x10u; // mutate original 2E action bytes
    assert(!resolveGeneric(code.data(),unsigned(code.size()),
                           rom.data(),unsigned(rom.size()),
                           0x02u,0x04u).valid());
    // 87.345: a genuine native input source writes D6/A0, and an
    // ORIGINAL indexed D6 + phase-zero state may launch 2E through
    // an indirect path that is NOT Ryu's immediate MOVEQ/JMP bytes.
    // Restore Cyclops' original matcher after mutating his compact
    // action launch above: native-state path, not a synthetic 2E setter.
    code[cyclops+1u]^=1u;
    const auto cyclopsState=resolveGeneric(
        code.data(),unsigned(code.size()),
        rom.data(),unsigned(rom.size()),0x02u,0x04u);
    assert(cyclopsState.valid() && cyclopsState.proof==2u);
    assert(cyclopsState.hasNativeMatcherSource);
    assert(cyclopsState.nativeDispatcher==0x21400u);

    // Verify exact character-pointer ownership from a SECOND actual
    // 19-entry original dispatcher table. Charlie's D6 dispatcher is
    // at a code address OUTSIDE the inferred Charlie private region,
    // and its phase zero deliberately has no recognized 2E launch.
    // The original indexer entry for Charlie itself supplies ownership.
    auto indexed=code; // new backing pointer refreshes the region cache
    const unsigned alternatePC=0x1200u,alternateTable=0x3200u;
    be(indexed,alternatePC,idx,sizeof(idx));
    w(indexed,alternatePC+8u,alternateTable-(alternatePC+8u));
    w(indexed,alternatePC+10u,0x4E90u);
    for(unsigned i=0u;i<19u;++i)
        l(indexed,alternateTable+i*4u,
          (i==4u||i==5u||i==6u)?0u:
          (i==16u?0x46000u:0x20000u+i*0x1000u));
    static const std::uint8_t originalD6[]={
        0x70,0x00,0x10,0x2E,0x00,0xD6,0x30,0x3B,
        0x00,0x06,0x4E,0xFB,0x00,0x02
    };
    static const std::uint8_t originalPhase[]={
        0x30,0x2E,0x00,0x08,0x30,0x3B,0x00,0x06,
        0x4E,0xFB,0x00,0x02
    };
    const unsigned charlieStatePC=0x46000u;
    const unsigned charlieHandler=0x46500u;
    const unsigned charliePhase=0x46700u;
    be(indexed,charlieStatePC,originalD6,sizeof(originalD6));
    w(indexed,charlieStatePC+sizeof(originalD6)+0x04u,
      charlieHandler-(charlieStatePC+sizeof(originalD6)));
    be(indexed,charlieHandler,originalPhase,sizeof(originalPhase));
    w(indexed,charlieHandler+sizeof(originalPhase),
      charliePhase-(charlieHandler+sizeof(originalPhase)));
    // No action-2E call is invented at charliePhase.
    makeRom(rom,0x20u,0u);
    const auto indexedRegions=identify(indexed.data(),unsigned(indexed.size()));
    assert(indexedRegions.ready && indexedRegions.indexers==2u);
    assert(indexedRegions.begin[16u]<charlieStatePC);
    assert(indexedRegions.end[16u]<charlieStatePC);
    assert(originalIndexedOwner(indexedRegions,0x20u,charlieStatePC));
    assert(!originalIndexedOwner(indexedRegions,0x02u,charlieStatePC));
    const auto dynamic=resolveGeneric(
        indexed.data(),unsigned(indexed.size()),
        rom.data(),unsigned(rom.size()),0x20u,0x04u);
    assert(dynamic.valid() && dynamic.proof==2u);
    assert(dynamic.nativeDispatcher==charlieStatePC);
    assert(dynamic.phaseZero==charliePhase);
    assert(!dynamic.launch && !dynamic.script);
    assert(!resolveGeneric(indexed.data(),unsigned(indexed.size()),
              rom.data(),unsigned(rom.size()),0x20u,0x06u).valid());

    // Changing the exact original indexed owner MUST invalidate the
    // proof. A reserve fighter with a generic ROM 2E table alone is not
    // eligible; the original M68K fighter->D6 edge must exist.
    auto wrongOwner=indexed;
    l(wrongOwner,alternateTable+16u*4u,0x47000u);
    assert(!resolveGeneric(wrongOwner.data(),unsigned(wrongOwner.size()),
              rom.data(),unsigned(rom.size()),0x20u,0x04u).valid());
    auto wrongRom=rom;
    const unsigned charlieRomTable=0x100000u+0x20u*0x1000u;
    romW(wrongRom,charlieRomTable+0x2Eu,0u);
    assert(!resolveGeneric(indexed.data(),unsigned(indexed.size()),
              wrongRom.data(),unsigned(wrongRom.size()),0x20u,0x04u).valid());

    std::puts("XMVSF 87.345 original indexed fighter D6 phase route PASS");
}
