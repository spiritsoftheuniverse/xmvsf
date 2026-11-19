#ifndef XMVSF_RECOMP_LOGO_INTRO_TIMELINE_H
#define XMVSF_RECOMP_LOGO_INTRO_TIMELINE_H
#include <cstdint>
#include <algorithm>
// Recomp V2 port of the original 87.235 timeline. Cold boot only. The replay harness and game->menu return skip the intro.
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
static constexpr std::uint32_t AnnouncerStart=0u; // one-second WAV lead-in before logo first appears
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

struct BootClock {
    bool started = false;
    std::uint64_t startedAt = 0;
    void Start(std::uint64_t at) { if (!started) { started = true; startedAt = at; } }
    std::uint32_t Elapsed(std::uint64_t now) const {
        if (!started) return TotalMs;
        const std::uint64_t delta = now >= startedAt ? now - startedAt : 0u;
        return static_cast<std::uint32_t>(std::min<std::uint64_t>(delta, 0xffffffffu));
    }
    bool InputAllowed(std::uint64_t now) const { return !started || Elapsed(now) >= TotalMs; }
};
inline BootClock& State() { static BootClock clock; return clock; }
} // namespace XM87235Intro
#endif
