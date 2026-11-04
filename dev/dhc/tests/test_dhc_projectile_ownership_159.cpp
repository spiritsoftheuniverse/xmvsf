// Stage 87.159 independent projectile collision ownership regression.
// This is not an emulated game hit test: validates native linked-actor
// accounting and writes only to the outgoing project's offensive fields.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_159_projectile_cutoff.h"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace XM87159Projectile;
static void putW(std::vector<std::uint8_t>& a,unsigned p,unsigned value){
    setB(a.data(),p,value>>8u);setB(a.data(),p+1u,value);
}
static void putL(std::vector<std::uint8_t>& a,unsigned p,unsigned value){
    putW(a,p,value>>16u);putW(a,p+2u,value);
}
static void boxes(std::vector<std::uint8_t>& ram,unsigned base){
    putW(ram,base+0x70u,0x1234u);
    putW(ram,base+0x72u,0x5678u);
    setB(ram.data(),base+0x82u,0x80u);
    setB(ram.data(),base+0x83u,0x01u); // non-offense must survive
    putW(ram,base+0x74u,0xFEDCu); // defensive hurt box must survive
    putW(ram,base+0x60u,0x2E08u); // animation/action not touched
}
static void assertOff(std::vector<std::uint8_t>& ram,unsigned base){
    assert(w(ram.data(),base+0x70u)==0u);
    assert(w(ram.data(),base+0x72u)==0u);
    assert(b(ram.data(),base+0x82u)==0u);
    assert(b(ram.data(),base+0x83u)==1u);
    assert(w(ram.data(),base+0x74u)==0xFEDCu);
    assert(w(ram.data(),base+0x60u)==0x2E08u);
}
int main(){
    std::vector<std::uint8_t> ram(65536u,0u);
    setB(ram.data(),0x5000u,8u);
    // Established XM23 native hitbox table:
    // owner P1: FFE3D8 - (slot*4); owner P2: FFE3D8 + 150 - slot*4.
    const unsigned oldRyu=0xFF6000u,lateRyu=0xFF6200u;
    const unsigned newKen=0xFF6400u,p2=0xFF6600u;
    putL(ram,0xE3D4u,oldRyu);
    putL(ram,0xE524u,p2);
    boxes(ram,oldRyu);boxes(ram,lateRyu);boxes(ram,newKen);boxes(ram,p2);
    State s;
    s.arm(ram.data(),100u);
    assert(s.armed && !s.cutoff && s.count==1u);
    assert(w(ram.data(),oldRyu+0x70u)==0x1234u);
    // Only an accepted Ryu outgoing tag authorizes offense removal.
    s.activate(ram.data(),101u);
    assertOff(ram,oldRyu);
    assert(w(ram.data(),p2+0x70u)==0x1234u);
    assert(w(ram.data(),lateRyu+0x70u)==0x1234u);
    // ROM produces a new Ryu Shinkuu projectile while Ryu is still point.
    putL(ram,0xE3D0u,lateRyu);
    s.afterCPU(ram.data(),102u,true);
    assert(s.count==2u);
    assertOff(ram,lateRyu);
    // ROM can reassert the attack descriptors; block the NEXT native CPU.
    boxes(ram,oldRyu);boxes(ram,lateRyu);
    s.beforeCPU(ram.data(),103u);
    assertOff(ram,oldRyu);assertOff(ram,lateRyu);
    // Now Ken becomes the point fighter. Freeze Ryu's old projectile set.
    s.swapped(104u);
    putL(ram,0xE3CCu,newKen);
    s.afterCPU(ram.data(),105u,false);
    assert(s.count==2u);
    assert(w(ram.data(),newKen+0x70u)==0x1234u);
    assert(w(ram.data(),p2+0x70u)==0x1234u);
    // Ken can spawn and damage with his own projectile/hyper boxes.
    boxes(ram,newKen);
    s.beforeCPU(ram.data(),106u);
    assert(w(ram.data(),newKen+0x70u)==0x1234u);
    // Ryu's old projectile leaves the list; do not suppress reused slot.
    putL(ram,0xE3D4u,newKen);
    putL(ram,0xE3D0u,0u);
    putL(ram,0xE3CCu,0u);
    s.afterCPU(ram.data(),107u,false);
    assert(!s.armed && s.count==0u);
    boxes(ram,newKen);s.beforeCPU(ram.data(),108u);
    assert(w(ram.data(),newKen+0x70u)==0x1234u);
    // Never mutate on a bad match phase.
    setB(ram.data(),0x5000u,1u);
    State noCombat;noCombat.arm(ram.data(),109u);
    assert(!noCombat.armed);
    noCombat.activate(ram.data(),110u);
    assert(w(ram.data(),newKen+0x70u)==0x1234u);
    return 0;
}
