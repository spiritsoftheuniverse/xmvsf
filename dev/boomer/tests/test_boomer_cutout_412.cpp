#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_412_boomer_cutout.h"
#include <cassert>
#include <cstdio>
int main(){
 XM87412::Controls c;
 assert(c.actors&&c.stun);
 for(unsigned k=0;k<8;++k)assert(!c.toggle(k));
 assert(c.actors&&c.stun);
 assert(c.toggle(9)&&!c.actors&&c.stun);
 assert(c.toggle(8)&&!c.stun&&!c.actors);
 assert(c.toggle(9)&&c.actors&&!c.stun);
 c.reset();assert(c.actors&&c.stun);
 assert(XM87412::needsMatchReset(~0u,3u,~0ull,0ull));
 assert(!XM87412::needsMatchReset(3u,3u,50ull,51ull));
 assert(XM87412::needsMatchReset(3u,4u,51ull,52ull));
 assert(XM87412::needsMatchReset(3u,3u,500ull,1ull)); // next match rewind
 assert(!XM87412::needsMatchReset(3u,3u,500ull,500ull));
 assert(XM87412::nativeHud(7,2,88501,120,20,9,1));
 assert(XM87412::nativeHud(7,4,88418,228,32,6,2));
 assert(XM87412::nativeHud(0,1,87808,176,8,2,2));
 assert(!XM87412::nativeHud(6,6,50001,120,20,9,1));
 assert(!XM87412::nativeHud(7,26,109950,180,90,4,4));
 assert(!XM87412::nativeHud(7,3,50000,20,0,3,2)); // never restore tree
 assert(XM87412::nativeHud(7,3,89500,20,16,11,2)); // long name
 assert(!XM87412::nativeHud(0,1,87808,176,90,2,2));
 std::puts("87.414 Boomer session rearm and source HUD safety tests PASS");
}
