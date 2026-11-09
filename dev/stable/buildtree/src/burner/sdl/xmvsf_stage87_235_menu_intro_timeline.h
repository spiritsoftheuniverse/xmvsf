#ifndef XMVSF_STAGE87_235_MENU_INTRO_TIMELINE_H
#define XMVSF_STAGE87_235_MENU_INTRO_TIMELINE_H
#include <cstdint>
#include <algorithm>
// Cold boot only. The replay harness and game->menu return skip the intro.
// Timer differences are unsigned and safe across SDL_GetTicks wraparound.
namespace XM87235Intro {
static constexpr std::uint32_t BackgroundMs=1000u; // logo cue remains at 1s
static constexpr std::uint32_t BackgroundFadeMs=3000u; // visual fade overlaps logo
static constexpr std::uint32_t LogoMs=1000u; // logo resolves over one second
static constexpr std::uint32_t MenuMs=500u;
static constexpr std::uint32_t StrikeCount=4u;
static constexpr std::uint32_t FramesPerStrike=10u;
static constexpr std::uint32_t StrikeFps=60u;
static constexpr std::uint32_t StrikeFrames=StrikeCount*FramesPerStrike;
static constexpr std::uint32_t StrikeDurationMs=(StrikeFrames*1000u+StrikeFps-1u)/StrikeFps;
static constexpr std::uint32_t LogoStart=BackgroundMs;
static constexpr std::uint32_t StrikeStart=LogoStart+LogoMs;
static constexpr std::uint32_t AnnouncerStart=LogoStart; // welcome starts with logo at t=1.0s
static constexpr std::uint32_t MenuStart=StrikeStart+StrikeDurationMs;
static constexpr std::uint32_t TotalMs=MenuStart+MenuMs;
static inline float progress(std::uint32_t elapsed,std::uint32_t start,
                             std::uint32_t duration){
    if(elapsed<=start)return 0.0f;
    if(elapsed>=start+duration)return 1.0f;
    return float(elapsed-start)/float(duration);
}
static inline float backgroundStrength(std::uint32_t elapsed){
    const float remaining=1.0f-progress(elapsed,0u,BackgroundFadeMs);
    return remaining*remaining; // pulse ramps DOWN to normal GIF hue/color
}
static inline float logoStrength(std::uint32_t elapsed){
    return 1.0f-progress(elapsed,LogoStart,LogoMs);
}
// Frame-quantized at 60Hz: four source-pixel masks, exactly ten steps each.
static inline std::uint32_t strokeFrame(std::uint32_t elapsed){
    if(elapsed<=StrikeStart)return 0u;
    const std::uint32_t delta=elapsed-StrikeStart;
    if(delta>=StrikeDurationMs)return StrikeFrames;
    return std::min(StrikeFrames,(delta*StrikeFps)/1000u);
}
static inline float menuReveal(std::uint32_t elapsed){
    return progress(elapsed,MenuStart,MenuMs);
}
static inline bool logoVisible(std::uint32_t elapsed){
    return elapsed>=LogoStart;
}
static inline bool menuVisible(std::uint32_t elapsed){
    return elapsed>=MenuStart;
}
static inline bool inputAllowed(std::uint32_t elapsed){
    return elapsed>=TotalMs;
}
} // namespace XM87235Intro
#endif
