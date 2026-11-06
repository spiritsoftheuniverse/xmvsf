// Real 87.187 capture: eight disjoint attack probes and two native modes.
// ROM-free C++11 predicate verification is NOT proof of ROM acceptance.
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_188_full_attack_gate.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_187_opening_front.h"
int main(){
 using XM87184Life::Sample;
 using namespace XM87188Attack;
 const Sample a28(0x12u,0x2E08u,0x2A28u,4u,0x0D30B0u,0x0Au);
 const Sample a02(0x12u,0x2E08u,0x2A02u,4u,0x0D30D0u,0x0Au);
 const Sample st(0x12u,0x2E08u,0x2A02u,2u,0x0D3070u,0x0Au);
 const Sample re(0x12u,0x2E0Au,0x2A02u,6u,0x0D3134u,0x0Au);
 for(unsigned n=1u;n<=0x3Cu;++n){
  assert(eligible(a28,true,n,1u,0u));
  assert(eligible(a02,true,n,1u,0u));
 }
 for(unsigned n:{0x2Fu,0x2Du,0x2Bu,0x0Bu,0x02u})
  assert(eligible(a02,true,n,1u,0u));
 assert(!eligible(a02,true,0x2Au,0x0021u,0x0020u)); // 3987 consumed
 assert(!eligible(a02,true,0x2Au,0x0011u,0u));
 assert(!eligible(a02,false,0x2Au,1u,0u));
 assert(!eligible(a02,true,0x40u,1u,0u)); // 87.186 separate opt-in
 assert(!eligible(a02,true,0x3Du,1u,0u));
 assert(!eligible(a02,true,0u,1u,0u));
 assert(!eligible(st,true,0x2Au,1u,0u));
 assert(!eligible(re,true,0x2Au,1u,0u));
 assert(XM87187Front::check(a02,5348u,5348u)==XM87187Front::KEEP);
 assert(XM87187Front::check(a02,5348u,5347u)==XM87187Front::STALE_ATTACK_INPUT);
 std::puts("PASS 87.188: full original attack, both 2A modes, no stale/consumed edge");
}
