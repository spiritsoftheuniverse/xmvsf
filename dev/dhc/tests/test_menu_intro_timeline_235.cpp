// Native Windows build runs this ROM-free timing test before linking FBNeo.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_235_menu_intro_timeline.h"
using namespace XM87235Intro;
int main(){
    static_assert(BackgroundMs==1000u && BackgroundFadeMs==3000u && LogoMs==3500u &&
                  MenuMs==500u && TotalMs==5000u,"87.235 timings changed");
    assert(backgroundStrength(0u)==1.0f);
    assert(backgroundStrength(500u)>0.0f);
    assert(backgroundStrength(500u)<1.0f);
    assert(backgroundStrength(1000u)>0.0f); // background overlaps logo
    assert(backgroundStrength(1000u)<backgroundStrength(500u));
    assert(backgroundStrength(2000u)>0.0f);
    assert(backgroundStrength(2999u)>0.0f);
    assert(backgroundStrength(3000u)==0.0f);
    assert(!logoVisible(999u));
    assert(logoVisible(1000u));
    assert(logoStrength(1000u)==1.0f);
    assert(logoStrength(2750u)==0.5f);
    assert(logoStrength(4500u)==0.0f);
    assert(!menuVisible(4499u));
    assert(menuVisible(4500u));
    assert(menuReveal(4500u)==0.0f);
    assert(menuReveal(4750u)==0.5f);
    assert(menuReveal(5000u)==1.0f);
    assert(!inputAllowed(4999u));
    assert(inputAllowed(5000u));
    assert(!inputAllowed(0u));
    const std::uint32_t wrapStart=0xffffff00u;
    assert((std::uint32_t)(wrapStart+5000u-wrapStart)==5000u);
    std::puts("XMVSF 87.235 3s background overlap/1s logo cue/3.5s logo/0.5s menu gate: PASS");
    return 0;
}
