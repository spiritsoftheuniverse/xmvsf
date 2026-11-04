// C++11 isolated native 68000 ROM-table dispatcher tests.
// Synthetic ROM contains ONLY recreated directory entries and 16-byte
// script headers; no copyrighted ROM bytes or decrypted executable are stored.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_142_rom_dispatch.h"
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
using namespace XM87142;
static void wb(std::vector<std::uint8_t>& v,unsigned at,unsigned b){v[at^1u]=std::uint8_t(b);}
static void ww(std::vector<std::uint8_t>& v,unsigned at,unsigned w){
    wb(v,at,w>>8u);wb(v,at+1u,w);
}
static void wl(std::vector<std::uint8_t>& v,unsigned at,unsigned l){
    ww(v,at,l>>16u);ww(v,at+2u,l);
}
static void rw(std::vector<std::uint8_t>& v,unsigned at,unsigned val){
    v[at]=std::uint8_t(val);v[at+1u]=std::uint8_t(val>>8u);
}
static void rl(std::vector<std::uint8_t>& v,unsigned at,unsigned val){
    rw(v,at,val>>16u);rw(v,at+2u,val);
}
static void fakeEntry(std::vector<std::uint8_t>& rom,unsigned id,
                      unsigned action,unsigned selector,unsigned expectedScript,
                      unsigned animation){
    const unsigned table=id==0x12u?0xCFAD4u:0xD378Eu;
    rl(rom,0x8A946u+id*2u,table);
    unsigned section=0;
    if(id==0x12u && action==0x2Eu)section=0xCFCA2u;
    if(id==0x12u && action==0x32u)section=0xCFCCAu;
    if(id==0x14u && action==0x2Au)section=0xD3910u;
    if(id==0x14u && action==0x2Cu)section=0xD3922u;
    if(id==0x14u && action==0x2Eu)section=0xD3966u;
    assert(section>table&&expectedScript>section);
    rw(rom,table+action,section-table);
    rw(rom,section+selector,expectedScript-section);
    rw(rom,expectedScript+6u,animation);
}
static void actor(std::vector<std::uint8_t>& ram,unsigned base,unsigned id,
                  unsigned action,unsigned selector,unsigned script){
    ww(ram,base+0x52u,id);
    wb(ram,base+0x60u,action);
    wb(ram,base+0x61u,selector);
    wl(ram,base+0x34u,script);
}
int main(){
    std::vector<std::uint8_t> rom(0xE1000u,0),ram(65536u,0);
    fakeEntry(rom,0x12u,0x2Eu,0x08u,0xD3070u,0xBDA0u);
    fakeEntry(rom,0x12u,0x32u,0x00u,0xD1DACu,0xA5C2u);
    fakeEntry(rom,0x14u,0x2Au,0x0Cu,0xD5702u,0xEB0Au);
    fakeEntry(rom,0x14u,0x2Cu,0x00u,0xD5C46u,0xEE56u);
    fakeEntry(rom,0x14u,0x2Cu,0x04u,0xD5E2Eu,0xEE56u);
    fakeEntry(rom,0x14u,0x2Eu,0x00u,0xD6722u,0xEF18u);
    struct Item{unsigned id,action,sub,script,anim;Kind kind;};
    const Item known[]={
      {0x12u,0x2Eu,0x08u,0xD3070u,0xBDA0u,NATIVE_SUPER_ENTRY},
      {0x12u,0x32u,0x00u,0xD1DACu,0xA5C2u,NATIVE_TAG_OUT},
      {0x14u,0x2Au,0x0Cu,0xD5702u,0xEB0Au,NATIVE_TAG_IN},
      {0x14u,0x2Cu,0x00u,0xD5C46u,0xEE56u,NATIVE_SUPER_ENTRY},
      {0x14u,0x2Cu,0x04u,0xD5E2Eu,0xEE56u,NATIVE_SUPER_ENTRY},
      {0x14u,0x2Eu,0x00u,0xD6722u,0xEF18u,NATIVE_SUPER_ENTRY}
    };
    for(const Item& x:known){
        const Lookup r=resolve(rom.data(),unsigned(rom.size()),x.id,x.action,x.sub);
        assert(r.valid && r.script==x.script && r.animation==x.anim);
        assert(classify(r,x.script)==x.kind);
        assert(classify(r,x.script+16u)==NO_EVENT); // not a fabricated start
    }
    assert(!resolve(rom.data(),unsigned(rom.size()),0x13u,0x2Eu,8u).valid);
    assert(!resolve(rom.data(),unsigned(rom.size()),0x12u,0x35u,8u).valid);
    wb(ram,0x5000u,8u);
    actor(ram,0x4000u,0x12u,0u,0u,0u);
    actor(ram,0x4800u,0x14u,0u,0u,0u);
    actor(ram,0x4400u,0x20u,0u,0u,0u);
    actor(ram,0x4C00u,0x22u,0u,0u,0u);
    wl(ram,0x4224u,0xFF4800u);
    State obs;
    assert(obs.frame(ram.data(),rom.data(),unsigned(rom.size()),100u,false)
                .verifiedSuperMask==0u);
    actor(ram,0x4000u,0x12u,0x2Eu,8u,0xD3070u);
    std::vector<std::uint8_t> original=ram;
    Report super=obs.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,false);
    assert(super.verifiedSuperMask==1u && super.count==1u);
    assert(super.event[0].kind==NATIVE_SUPER_ENTRY);
    assert(super.event[0].partnerPointer==0xFF4800u);
    assert(ram==original); // no gameplay modification
    assert(obs.frame(ram.data(),rom.data(),unsigned(rom.size()),101u,false).count==0u);
    actor(ram,0x4000u,0x12u,0x32u,0u,0xD1DACu);
    Report out=obs.frame(ram.data(),rom.data(),unsigned(rom.size()),105u,false);
    assert(!out.verifiedSuperMask && out.event[0].kind==NATIVE_TAG_OUT);
    actor(ram,0x4000u,0x14u,0x2Au,0x0Cu,0xD5702u);
    actor(ram,0x4800u,0x12u,0u,0u,0u);
    Report tag=obs.frame(ram.data(),rom.data(),unsigned(rom.size()),106u,false);
    assert(tag.tagMask==1u && !tag.verifiedSuperMask);
    assert(tag.count==2u && tag.event[0].kind==POINT_SWAP);
    assert(tag.event[1].kind==NATIVE_TAG_IN);
    // Incoming Ken's tag is explicitly NOT a super.
    actor(ram,0x4000u,0x14u,0x2Cu,4u,0xD5E2Eu);
    Report ken=obs.frame(ram.data(),rom.data(),unsigned(rom.size()),107u,false);
    assert(ken.verifiedSuperMask==1u && ken.event[0].kind==NATIVE_SUPER_ENTRY);
    actor(ram,0x4000u,0x14u,0x2Cu,0u,0xD5C56u);
    Report wrong=obs.frame(ram.data(),rom.data(),unsigned(rom.size()),108u,false);
    assert(!wrong.verifiedSuperMask && wrong.count==0u);
    return 0;
}
