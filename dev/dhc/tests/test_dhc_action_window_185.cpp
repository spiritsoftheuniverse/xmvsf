// 87.185 C++11 ROM-free regression from uploaded 87.184 movie/PC trace.
// Source-level predicates are NOT native Windows DHC acceptance proof.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_185_action_gate.h"
using XM87184Life::Sample;
using namespace XM87185Gate;
int main(){
 const Sample startup(0x12u,0x2E08u,0x2A28u,2u,0x0D3070u,0x0Au);
 const Sample attack(0x12u,0x2E08u,0x2A28u,4u,0x0D30D0u,0x0Au);
 const Sample recovery(0x12u,0x2E0Au,0x2A28u,6u,0x0D3134u,0x0Au);
 const Sample terminal(0x12u,0x2E0Au,0x2A28u,6u,0x0D3194u,0x0Au);
 const Sample after(0x12u,0x0000u,0x0000u,0u,0x0CFD08u,0u);
 assert(canRecognize(startup)); // 87.184 incorrectly refused 3P/2P here
 assert(canRecognize(attack));
 assert(canRecognize(recovery));
 assert(!canRecognize(terminal));assert(!canRecognize(after));
 // Real 87.184 early queued active phase: +4E=24,1F,24,28,22,25,23.
 // Previously all waited until 28 and were discarded, losing input edge.
 for(unsigned timer:{0x24u,0x1Fu,0x28u,0x22u,0x25u,0x23u})
   assert(directActiveAttack(attack,true,timer,0x0001u,0u));
 assert(!directActiveAttack(attack,true,0x1Cu,0x0001u,0u));
 assert(!directActiveAttack(attack,true,0x3Du,0x0001u,0u));
 assert(!directActiveAttack(attack,true,0x24u,0x0011u,0u));
 assert(!directActiveAttack(attack,true,0x24u,0x0001u,0x0010u));
 assert(!directActiveAttack(startup,true,0x24u,0x0001u,0u));
 assert(!directActiveAttack(recovery,true,0x24u,0x0001u,0u));
 assert(!directActiveAttack(attack,false,0x24u,0x0001u,0u));
 // Exact five observed experimental recovery controller results:
 // original 2E0A/step06/script 0D3114 or 0D3134;
 // next frame native action 2E08 step02 script 0D3070,
 // A0 2A00, +256=00, +259=08, D6=0A.
 for(unsigned oldscript:{0x0D3114u,0x0D3134u})
   assert(exactRecoveryRebound(true,2u,0x12u,0x14u,
       0x2E0Au,6u,oldscript,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1778u));
 assert(!exactRecoveryRebound(false,2u,0x12u,0x14u,
       0x2E0Au,6u,0x0D3114u,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1778u));
 assert(!exactRecoveryRebound(true,2u,0x12u,0x14u,
       0x2E0Au,6u,0x0D3194u,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1778u));
 assert(!exactRecoveryRebound(true,2u,0x12u,0x14u,
       0x2E0Au,6u,0x0D3114u,0x2C04u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1778u));
 assert(!exactRecoveryRebound(true,2u,0x12u,0x14u,
       0x2E0Au,6u,0x0D3114u,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1780u));
 assert(!exactRecoveryRebound(true,2u,0x12u,0x14u,
       0x2E0Au,6u,0x0D3114u,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,8u,8u,1777u,1778u));
 assert(!exactRecoveryRebound(true,2u,0x14u,0x12u,
       0x2E0Au,6u,0x0D3114u,0x2E08u,0x2A00u,2u,
       0x0D3070u,0x0Au,0u,8u,1777u,1778u));
 std::puts("PASS 87.185: whole-action 2P/3P recognition, 7 early-stage guards, 5 recovery rebound signatures and strict exclusions");
}
