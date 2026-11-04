#ifndef XMVSF_STAGE86_64_STRONG_NOIR_H
#define XMVSF_STAGE86_64_STRONG_NOIR_H
// Stage86.64: ordinary CPS-2 scenery receives 30%-luma cap and 70% HSV
// saturation, rotating through the 35-second cycle. Actor/FX/HUD use their
// separate sprite palettes. The 70% is actual HSV saturation, not a weak
// amplitude applied to the already darkened grayscale channel.
#include "xmvsf_stage86_59_grade.h"
#include <cmath>
#include <cstdint>
struct XM8664RGB { unsigned r,g,b; };
static inline XM8664RGB xm8664HueRGB(double phase,unsigned value){
    const double h=(phase-std::floor(phase))*6.0;
    const double c=double(value)*0.70;
    const double x=c*(1.0-std::fabs(std::fmod(h,2.0)-1.0));
    const double m=double(value)-c;
    double r=0,g=0,b=0;
    if(h<1.0){r=c;g=x;}
    else if(h<2.0){r=x;g=c;}
    else if(h<3.0){g=c;b=x;}
    else if(h<4.0){g=x;b=c;}
    else if(h<5.0){r=x;b=c;}
    else {r=c;b=x;}
    return XM8664RGB{xm8657Clamp(r+m),xm8657Clamp(g+m),xm8657Clamp(b+m)};
}
static inline std::uint32_t xm8664Background(std::uint32_t from,unsigned frame){
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    // Absolute peak <= 77/255 in EVERY hue. Shades remain proportional,
    // avoiding the original additive/clipping behavior.
    const unsigned peak=(y*30u+50u)/100u;
    const auto rgb=xm8664HueRGB(double(frame%2100u)/2100.0,peak);
    return xm8657Pack(from,rgb.r,rgb.g,rgb.b);
}
#endif
