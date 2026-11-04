#ifndef XMVSF_STAGE86_57_PALETTE_H
#define XMVSF_STAGE86_57_PALETTE_H
// Double Feature: palette-domain separation before CPS2 layer composition.
// Exact framebuffer formats: 32bpp BurnHighCol output; original palette is
// restored after every draw. No RAM or CPS2 draw-order changes.
#include <cstdint>
#include <cmath>
struct XM8657RGB { unsigned r,g,b; };
static inline unsigned xm8657Luma(unsigned r,unsigned g,unsigned b) {
    return (77u*r+150u*g+29u*b+128u)>>8;
}
static inline std::uint32_t xm8657Pack(std::uint32_t from,unsigned r,unsigned g,unsigned b) {
    return (from&0xff000000u) | ((b&255u)<<16) | ((g&255u)<<8) | (r&255u);
}
static inline XM8657RGB xm8657Unpack(std::uint32_t from) {
    return XM8657RGB{from&255u,(from>>8)&255u,(from>>16)&255u};
}
static inline std::uint32_t xm8657Gray(std::uint32_t from) {
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    return xm8657Pack(from,y,y,y);
}
static inline std::uint32_t xm8657HalfSaturation(std::uint32_t from) {
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    return xm8657Pack(from,(c.r+y+1u)/2,(c.g+y+1u)/2,(c.b+y+1u)/2);
}
struct XM8657Spectral { double r,g,b; };
// Stage86.29, 35-second three-phase sinusoidal color drift, normalized
// to preserve average luminance. Amplitude 36 is the approved preset 9.
static inline XM8657Spectral xm8657SpectralAt(unsigned frame) {
    constexpr double pi=3.14159265358979323846;
    const double theta=2.0*pi*double(frame%2100u)/2100.0;
    const double r=230.0+36.0*std::sin(theta);
    const double g=230.0+36.0*std::sin(theta-2.0*pi/3.0);
    const double b=230.0+36.0*std::sin(theta+2.0*pi/3.0);
    const double lum=(77.0*r+150.0*g+29.0*b)/256.0;
    const double norm=230.0/lum;
    return XM8657Spectral{r*norm/230.0,g*norm/230.0,b*norm/230.0};
}
static inline unsigned xm8657Clamp(double v) {
    return v<0.0?0u:(v>=255.0?255u:static_cast<unsigned>(v+0.5));
}
static inline std::uint32_t xm8657Background(std::uint32_t from,XM8657Spectral s) {
    const auto c=xm8657Unpack(from);
    const double y=double(xm8657Luma(c.r,c.g,c.b))*0.5;
    return xm8657Pack(from,xm8657Clamp(y*s.r),xm8657Clamp(y*s.g),xm8657Clamp(y*s.b));
}
// Conservative subset of the previous Stage86.29 effects discovery.
// This is not proof that all attacks use these palettes. Do not color whole
// priority 6, as it also contains the fighters.
static inline bool xm8657LikelyFx(unsigned priority,unsigned palette) {
    return priority==6u && palette>=16u && palette<=23u;
}
// Verified HUD numeral atlas; Stage86.29 also identified priority-7 as
// the main HUD band. Unlike FX, HUD is 50% RGB saturation, not full RGB.
static inline bool xm8657Hud(unsigned priority,unsigned palette,unsigned tile,
                              unsigned width,unsigned height) {
    if(priority==0u && palette==1u && width==2u && height==2u &&
       tile>=87808u && tile<=87826u && ((tile-87808u)%2u)==0u)return true;
    return priority==7u;
}
#endif
