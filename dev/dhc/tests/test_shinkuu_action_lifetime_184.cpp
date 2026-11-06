// C++11 ROM-free regression from six uninterrupted original 87.183 supers.
// No fixed frame counts are used by game logic. These verify measurements.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_184_super_action_lifetime.h"
using namespace XM87184Life;
static Sample sample(unsigned action,unsigned step,unsigned script,
                     unsigned mode=0x2A28u,unsigned fighter=0x12u,
                     unsigned d6=0x0Au){
    return Sample(fighter,action,mode,step,script,d6);
}
struct Episode{unsigned root,active,recovery,exit;};
static const Episode natural[]={
    {1802,1869,1925,1966},
    {2105,2171,2227,2268},
    {2327,2393,2449,2490},
    {2543,2609,2665,2706},
    {2834,2901,2957,2999},
    {3515,3582,3638,3679}
};
int main(){
    for(const auto& e:natural){
        Tracker t;
        assert(t.update(e.root,sample(0x2E08,2,0x0D3070))==ENTER);
        assert(t.live && t.generation==1 && t.startAt==e.root);
        assert(t.update(e.root+3,sample(0x2E08,2,0x0D3080))==NOTHING);
        assert(!canTag(sample(0x2E08,2,0x0D3090),true));
        assert(t.update(e.active,sample(0x2E08,4,0x0D30B0))==ATTACK_BEGIN);
        assert(t.attackAt==e.active);
        assert(canTag(sample(0x2E08,4,0x0D30D0),false));
        assert(canTag(sample(0x2E08,4,0x0D30E0),true));
        assert(t.update(e.recovery,sample(0x2E0A,6,0x0D30F4))==RECOVERY_BEGIN);
        assert(t.recoveryAt==e.recovery);
        assert(identify(sample(0x2E0A,6,0x0D3104))==RECOVERY);
        assert(!canTag(sample(0x2E0A,6,0x0D3184),false));
        assert(canTag(sample(0x2E0A,6,0x0D3184),true));
        assert(!canTag(sample(0x2E0A,6,0x0D3194),true));
        assert(t.update(e.exit,sample(0,0,0x0CFD08,0x002A))==ACTION_EXIT);
        assert(!t.live && t.endAt==e.exit);
        assert(e.active-e.root>=66 && e.active-e.root<=67);
        assert(e.recovery-e.active==56);
        assert(e.exit-e.root>=163 && e.exit-e.root<=165);
        assert(t.update(e.exit+1,sample(0,0,0x0CFD18,0x002A))==NOTHING);
        t.reset();assert(t.generation==0 && !t.live);
    }
    assert(identify(sample(0x2E08,4,0x0D30D0,0x2A28,0x14u))==OUTSIDE);
    assert(identify(sample(0x2E0A,6,0x0D3124,0x2A28,0x12u,0x0Cu))==OUTSIDE);
    assert(!canTag(sample(0x2E08,2,0x0D3070),true));
    std::puts("PASS 87.184: 6 natural super action lifetimes, phase and terminal gating");
}
