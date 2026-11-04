// Match-start queue. C++11 synthetic table test: no copyrighted ROM data.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_143_super_queue.h"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace XM87143Queue;
static void rb(std::vector<std::uint8_t>& ram,unsigned a,unsigned b){ram[a^1u]=std::uint8_t(b);}
static void rw(std::vector<std::uint8_t>& ram,unsigned a,unsigned w){
    rb(ram,a,w>>8u);rb(ram,a+1u,w);
}
static void w16(std::vector<std::uint8_t>& rom,unsigned a,unsigned w){
    rom[a]=std::uint8_t(w);rom[a+1u]=std::uint8_t(w>>8u);
}
static void w32(std::vector<std::uint8_t>& rom,unsigned a,unsigned d){
    w16(rom,a,d>>16u);w16(rom,a+2u,d);
}
static void one(std::vector<std::uint8_t>& rom,unsigned id,
                unsigned action,unsigned selector,unsigned script){
    const unsigned table=(id==0x12u)?0xCFAD4u:0xD378Eu;
    unsigned section=0;
    if(id==0x12u&&action==0x2Eu)section=0xCFCA2u;
    else if(id==0x14u&&action==0x2Cu)section=0xD3922u;
    else if(id==0x14u&&action==0x2Eu)section=0xD3966u;
    assert(section!=0u && script>section && section>table);
    w32(rom,0x8A946u+id*2u,table);
    w16(rom,table+action,section-table);
    w16(rom,section+selector,script-section);
    w16(rom,script+6u,0xAA55u);
}
int main(){
    std::vector<std::uint8_t> ram(65536u,0),rom(0xE1000u,0);
    rb(ram,0x5000u,8u);
    rw(ram,0x4052u,0x12u);rw(ram,0x4852u,0x14u);
    rw(ram,0x4452u,0x20u);rw(ram,0x4C52u,0x22u);
    one(rom,0x12u,0x2Eu,0x08u,0xD3070u);
    one(rom,0x14u,0x2Cu,0x00u,0xD5C46u);
    one(rom,0x14u,0x2Cu,0x04u,0xD5E2Eu);
    one(rom,0x14u,0x2Eu,0x00u,0xD6722u);
    MatchQueue q;
    assert(q.prepare(ram.data(),rom.data(),unsigned(rom.size()),100u));
    assert(q.prepared&&q.generation==1u && q.initializedAt==100u);
    assert(q.count(0u,0x12u)==2u&&q.count(0u,0x14u)==2u);
    const Entry* ryuPunch=q.find(0u,0x12u,QCF_PP);
    const Entry* ryuKick=q.find(0u,0x12u,QCB_KK);
    const Entry* kenPunch=q.find(0u,0x14u,QCF_PP);
    const Entry* kenKick=q.find(0u,0x14u,QCF_KK);
    assert(ryuPunch&&ryuPunch->count==1u&&ryuPunch->scripts[0].resolved);
    assert(ryuPunch->scripts[0].script==0xD3070u);
    assert(ryuKick&&ryuKick->count==0u); // known input, unknown ROM action
    assert(kenPunch&&kenPunch->count==2u);
    assert(kenPunch->scripts[0].resolved && kenPunch->scripts[1].resolved);
    assert(kenKick&&kenKick->count==1u&&kenKick->scripts[0].resolved);
    assert(kenKick->scripts[0].script==0xD6722u);
    // DHC input chooses a prepared entry, but does not activate the ROM.
    const auto* staged=q.select(0u,0x14u,QCF_KK,111u);
    assert(staged==kenKick && q.selected[0]==kenKick);
    assert(q.selectedFrame[0]==111u);
    q.discard(0u);assert(q.selected[0]==nullptr);
    assert(!q.select(0u,0x12u,QCF_KK,112u));
    // No arbitrary super is injected into a fighter with no proven list.
    assert(q.count(1u,0x20u)==0u && q.find(1u,0x20u,QCF_PP)==nullptr);
    assert(!q.prepare(ram.data(),rom.data(),unsigned(rom.size()),101u));
    // A real normal tag swaps point/reserve, NOT the selected match roster.
    rw(ram,0x4052u,0x14u);rw(ram,0x4852u,0x12u);
    assert(!q.prepare(ram.data(),rom.data(),unsigned(rom.size()),102u));
    assert(q.generation==1u && q.find(0u,0x12u,QCB_KK));
    // New selected roster is a new match and rebuilds prepared entries.
    rw(ram,0x4852u,0x20u);
    rw(ram,0x4C52u,0x24u);
    assert(q.prepare(ram.data(),rom.data(),unsigned(rom.size()),103u));
    assert(q.generation==2u && !q.find(0u,0x12u,QCF_PP));
    // No mutation of ROM or fighter state occurs during queue preparation.
    assert(XM87142::ramW(ram.data(),0x4052u)==0x14u);
    assert(XM87142::ramW(ram.data(),0x4852u)==0x20u);
    rb(ram,0x5000u,0u);
    assert(!q.prepare(ram.data(),rom.data(),unsigned(rom.size()),104u));
    assert(!q.prepared);
    return 0;
}
