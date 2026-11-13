// 87.352 local-only C++11 regression. Not a BUILD_TEST gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_352_cyclops_sob_cleanup.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XM87352CyclopsFx;
static void wb(std::uint8_t* r,unsigned a,unsigned v){sb(r,a,v>>8u);sb(r,a+1u,v);}
static void lb(std::uint8_t* r,unsigned a,unsigned v){wb(r,a,v>>16u);wb(r,a+2u,v);}
int main(){
 std::vector<std::uint8_t> ram(65536u,0u);
 std::uint8_t* r=ram.data();
 const unsigned cyclops=0xFF5100u,other=0xFF5600u;
 lb(r,0xFFE3D4u,cyclops);lb(r,0xFFE3D0u,other);
 wb(r,cyclops+0x52u,2u);lb(r,cyclops+0x54u,0xFF4000u);
 sb(r,cyclops+3u,1u);sb(r,cyclops+0x82u,1u);
 wb(r,other+0x52u,0x16u);lb(r,other+0x54u,0xFF4400u);
 sb(r,other+3u,1u);
 Side s;s.begin(r,100u,0x4000u,0x4800u);
 assert(s.armed && s.n==1u && s.fx[0].base==cyclops);
 assert(s.update(r,101u,false,false,false)==0u);
 lb(r,cyclops+0x54u,0xFF4800u);
 assert(s.update(r,102u,true,false,false)==0u);
 assert(s.update(r,309u,false,false,false)==0u);
 assert(b(r,cyclops+3u)==1u);
 assert(s.update(r,310u,false,false,false)==1u);
 assert(b(r,cyclops+3u)==0u && b(r,cyclops+0x82u)==0u);
 assert(b(r,other+3u)==1u);
 // A recycled pointer is never killed after the ROM unlinks it.
 sb(r,cyclops+3u,1u);lb(r,cyclops+0x54u,0xFF4000u);
 Side recycled;recycled.begin(r,1000u,0x4000u,0x4800u);
 lb(r,0xFFE3D4u,0u);recycled.update(r,1001u,true,false,false);
 lb(r,0xFFE3D4u,cyclops);
 assert(recycled.update(r,1210u,false,false,false)==0u);
 assert(b(r,cyclops+3u)==1u);
 Side failed;failed.begin(r,2000u,0x4000u,0x4800u);
 failed.update(r,2001u,false,true,false);
 assert(!failed.armed);
 Side returning;returning.begin(r,3000u,0x4000u,0x4800u);
 returning.update(r,3001u,true,false,false);
 returning.update(r,3002u,false,false,true);
 assert(!returning.armed && b(r,cyclops+3u)==1u);
 std::puts("XMVSF 87.352 native Cyclops helper lifecycle PASS");
}
