#include <cassert>
#include <cstring>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_187_opening_front.h"
int main(){
 using XM87184Life::Sample;
 using namespace XM87187Front;
 const Sample startup(0x12u,0x2E08u,0x2A28u,2u,0x0D3090u,0x0Au);
 const Sample attack(0x12u,0x2E08u,0x2A28u,4u,0x0D30B0u,0x0Au);
 const Sample badMode(0x12u,0x2E08u,0x2A02u,4u,0x0D30E0u,0x0Au);
 const Sample recovery(0x12u,0x2E0Au,0x2A02u,6u,0x0D30F4u,0x0Au);
 assert(check(startup,2524u,2524u)==STARTUP_NOT_ARMABLE);
 assert(check(startup,2565u,2565u)==STARTUP_NOT_ARMABLE);
 assert(check(attack,2544u,2524u)==STALE_ATTACK_INPUT);
 assert(check(attack,2625u,2565u)==STALE_ATTACK_INPUT);
 assert(check(attack,2544u,2544u)==KEEP);
 assert(check(attack,1640u,1640u)==KEEP);
 assert(check(badMode,10629u,10629u)==KEEP); // actual 2A02 ATTACK
 assert(check(badMode,10629u,10628u)==STALE_ATTACK_INPUT);
 assert(check(recovery,10126u,10126u)==KEEP);
 assert(check(recovery,10126u,10091u)==KEEP);
 assert(std::strstr(wrongAction(0x0002u),"0002"));
 assert(std::strstr(wrongAction(0x2C02u),"2C02"));
 assert(std::strstr(wrongAction(0x2C04u),"2C04"));
 std::puts("PASS 87.187: startup/stale/active/recovery and abort labels");
}
