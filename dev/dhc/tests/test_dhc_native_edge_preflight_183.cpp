// 87.183 ROM-free C++11 regression, grounded in all 23 original 87.182
// armed DHC attempts. "Decline" means no controller 8/A0 writes, NOT success.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_183_native_edge_preflight.h"
using XM87183Preflight::freshNativeTagEdge;
using XM87183Preflight::shouldDecline;
struct Case {unsigned frame,c0,ca;bool nativeSucceeded;};
static const Case cases[] = {
 {1735,0x0001,0x0000,true},{2787,0x0001,0x0000,true},
 {3245,0x0011,0x0010,false},{3631,0x0001,0x0000,true},
 {4088,0x0001,0x0000,true},{4559,0x0001,0x0000,true},
 {5111,0x0011,0x0010,false},{5330,0x0001,0x0000,true},
 {5814,0x0001,0x0000,true},{6783,0x0001,0x0000,true},
 {7232,0x0001,0x0000,true},{8339,0x0001,0x0000,true},
 {10178,0x0001,0x0000,true},{10756,0x0031,0x0030,false},
 {11109,0x0001,0x0000,true},{11644,0x0001,0x0000,true},
 {12280,0x0001,0x0000,true},{12745,0x0001,0x0000,true},
 {13276,0x0001,0x0000,true},{14268,0x0001,0x0000,true},
 {14934,0x0001,0x0000,true},{15406,0x0001,0x0000,true},
 {15884,0x0001,0x0000,true}
};
int main(){
 unsigned preserve=0,skip=0;
 for(const auto& x:cases){
   const bool unsafe=shouldDecline(true,false,x.c0,x.ca);
   assert(unsafe==!x.nativeSucceeded);
   assert(freshNativeTagEdge(x.c0,x.ca)==x.nativeSucceeded);
   if(unsafe)++skip;else ++preserve;
   // Offline replay must reproduce the exact prior action arbitration.
   assert(!shouldDecline(true,true,x.c0,x.ca));
   // Never add a guard outside active experimental DHC.
   assert(!shouldDecline(false,false,x.c0,x.ca));
 }
 assert(preserve==20u && skip==3u);
 // Direction bits or other unrelated native buttons are not punch bits.
 for(unsigned mask=0;mask<0x10u;++mask)
    assert(freshNativeTagEdge(mask,0u));
 assert(!freshNativeTagEdge(0x0010u,0u));
 assert(!freshNativeTagEdge(0u,0x0020u));
 assert(!freshNativeTagEdge(0x0040u,0u));
 assert(!freshNativeTagEdge(0u,0x0070u));
 std::puts("PASS 87.183: preserve 20/20 native successes, decline all 3 original bad arms; replay bypass explicit");
}
