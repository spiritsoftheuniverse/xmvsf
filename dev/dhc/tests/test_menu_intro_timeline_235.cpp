// Native Windows build runs this ROM-free timing test before linking FBNeo.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage87_235_menu_intro_timeline.h"
using namespace XM87235Intro;
int main(){
    static_assert(BackgroundMs==1000u && BackgroundFadeMs==3000u && LogoMs==2000u &&
                  MenuMs==500u && StrikeCount==4u && FramesPerStrike==10u &&
                  StrikeFrames==40u && StrikeDurationMs==667u &&
                  StrikeStart==3000u && AnnouncerStart==3000u && MenuStart==3667u && TotalMs==4167u,"87.235 timings changed");
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
    assert(logoStrength(2000u)==0.5f);
    assert(logoStrength(3000u)==0.0f);
    assert(strokeFrame(2999u)==0u);
    assert(strokeFrame(3000u)==0u);
    assert(strokeFrame(3167u)==10u); // first 10-frame A stroke done
    assert(strokeFrame(3334u)==20u); // B done
    assert(strokeFrame(3500u)==30u); // C done
    assert(strokeFrame(3666u)==39u);
    assert(strokeFrame(3667u)==40u); // D done; menu begins
    assert(AnnouncerStart-LogoStart==2000u);
    assert(AnnouncerStart-StrikeStart==0u);
    assert(!menuVisible(3666u));
    assert(menuVisible(3667u));
    assert(menuReveal(3667u)==0.0f);
    assert(menuReveal(3917u)==0.5f);
    assert(menuReveal(4167u)==1.0f);
    assert(!inputAllowed(4166u));
    assert(inputAllowed(4167u));
    assert(!inputAllowed(0u));
    const std::uint32_t wrapStart=0xffffff00u;
    assert((std::uint32_t)(wrapStart+TotalMs-wrapStart)==TotalMs);
    std::puts("XMVSF 87.235 3s background/2s logo/4x10 strike frames/0.5s menu gate: PASS");
    return 0;
}
