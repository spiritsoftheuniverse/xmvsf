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
 assert(XM87412::nativeHud(7,2,88501,120,20,9,1));
 assert(XM87412::nativeHud(7,4,88418,228,32,6,2));
 assert(XM87412::nativeHud(0,1,87808,176,8,2,2));
 assert(!XM87412::nativeHud(6,6,50001,120,20,9,1));
 assert(!XM87412::nativeHud(7,26,109950,180,90,4,4));
 assert(!XM87412::nativeHud(0,1,87808,176,90,2,2));
 std::puts("87.412 Boomer native HUD and SDL stun toggle tests PASS");
}
