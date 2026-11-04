#ifndef XMVSF_STAGE87_29_SUPER_TRAIL_RED_H
#define XMVSF_STAGE87_29_SUPER_TRAIL_RED_H
// Stage87.29: verified secondary afterimage producers and *their actual CPS
// palette blue pens*. Do not alter source ROM, CPS palette DMA, other FX,
// fighter ink, Stage87.28 outline, or an entire palette bank.
#include "xmvsf_stage87_17_provenance.h"
#include "xmvsf_stage86_57_palette.h"
#include <cstdint>
#include <cstdlib>
namespace XM8729 {
inline bool Enabled(){
    static const bool enabled=[](){
        const char* p=std::getenv("XM8729_SUPER_TRAIL_RED");
        return p && p[0]=='1' && p[1]=='\0';
    }();
    return enabled;
}
inline bool Source(const XM8717::Source& s,unsigned frame,
                   unsigned priority,unsigned pal) {
    if (!Enabled() || priority!=6u || !XM8717::Recent(s,frame) || s.owner!=0u)
        return false;
    return (pal==21u && s.a0==0xFF9C00u) ||
           (pal==22u && (s.a0==0xFF9CC0u || s.a0==0xFF9D80u));
}
inline bool IsBlue(std::uint32_t px){
    const auto c=xm8657Unpack(px);
    // A hue test of the original native effect RGB, not the grayscale result.
    // Blue/cyan pens are eligible; purple-ish body costume pixels are not.
    const unsigned mx = c.b>c.g?c.b:c.g;
    return c.b>=96u && c.b>c.r+32u && mx>c.r+45u && c.b+24u>=c.g;
}
inline std::uint32_t ToRed(std::uint32_t px){
    const auto c=xm8657Unpack(px);
    // Keep the original brightest channel; create a saturated red trail.
    const unsigned peak=(c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b));
    return xm8657Pack(px,peak,peak/12u,peak/20u);
}
inline unsigned Make(const std::uint32_t* original,
                     const std::uint32_t* previouslyGraded,
                     std::uint32_t* out){
    unsigned changed=0;
    for(unsigned pen=0;pen<16u;++pen){
        out[pen]=previouslyGraded[pen];
        if(pen && IsBlue(original[pen])){
            out[pen]=ToRed(original[pen]);
            ++changed;
        }
    }
    return changed;
}
} // namespace XM8729
#endif
