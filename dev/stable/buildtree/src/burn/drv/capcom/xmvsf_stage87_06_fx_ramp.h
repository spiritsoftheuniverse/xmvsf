#ifndef XMVSF_STAGE87_06_FX_RAMP_H
#define XMVSF_STAGE87_06_FX_RAMP_H
// Stage87.06: recover faded highlight pens belonging to an already accepted FX
// palette gradient. NEVER used to classify a sprite or override lighting veto.
// Only restore upper-group pens 8..15 near a previously verified chromatic
// anchor in THAT same 16-pen palette; other pens retain Stage86.67 treatment.
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include "xmvsf_stage86_67_fx_isolation.h"
namespace XM8706 {
static inline bool RampEnabled(){
    static const bool on=[](){
        const char* v=std::getenv("XM8706_RAMP_MASK");
        return !v || v[0]!='0' || v[1]!='\0';
    }();
    return on;
}
static inline unsigned AbsDiff(unsigned a,unsigned b){return a>b?a-b:b-a;}
static inline bool NeighborGradient(std::uint32_t c,std::uint32_t anchor){
    const auto a=xm8657Unpack(c),b=xm8657Unpack(anchor);
    const unsigned dR=AbsDiff(a.r,b.r),dG=AbsDiff(a.g,b.g),dB=AbsDiff(a.b,b.b);
    const unsigned brightest=std::max(a.r,std::max(a.g,a.b));
    return brightest>=155u && std::max(dR,std::max(dG,dB))<=115u &&
           dR+dG+dB<=180u;
}
// 'rgb' contains live ROM colors, 'ink' contains existing character grayscale.
// 'dst' starts as the existing masked palette, so this only adds colors.
static inline unsigned RestoreUpperRamp(const std::uint32_t* rgb,
        const std::uint32_t* ink,std::uint32_t* dst){
    unsigned added=0;
    for(unsigned pen=8u;pen<16u;++pen){
        if(xm8667ChromaticEffectPen(rgb[pen]))continue;
        for(unsigned anchor=8u;anchor<16u;++anchor){
            if(anchor==pen || AbsDiff(anchor,pen)>3u ||
               !xm8667ChromaticEffectPen(rgb[anchor]))continue;
            if(!NeighborGradient(rgb[pen],rgb[anchor]))continue;
            if(dst[pen]==ink[pen] && rgb[pen]!=ink[pen]){
                dst[pen]=rgb[pen];
                ++added;
            }
            break;
        }
    }
    return added;
}
}
#endif
