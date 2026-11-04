#ifndef XMVSF_STAGE87_35_HUD_PALETTE_H
#define XMVSF_STAGE87_35_HUD_PALETTE_H
// Stage87.35. Only recolor verified HUD chroma from captured native CPS2
// sprite pixels. Overlay only changed pixels with alpha 255: Wily composition
// and the unmodified native/forest background remain untouched.
// Not a shared CPS2 palette edit, which would change attack effects too.
#include <cstdint>
#include <cstring>
namespace XM8735 {
struct Pixel { unsigned r,g,b,a; };
static inline Pixel Read(const unsigned char *p) {
    return Pixel{p[0],p[1],p[2],p[3]};
}
static inline unsigned char Bound(unsigned v) {return (unsigned char)(v>255u?255u:v);}
static inline bool HpRegion(unsigned x,unsigned y) {
    return y>=19u && y<=44u && ((x>=19u&&x<=164u) || (x>=220u&&x<=365u));
}
static inline bool NameRegion(unsigned x,unsigned y) {
    return y>=16u && y<=42u && ((x>=20u&&x<=112u) || (x>=271u&&x<=365u));
}
static inline bool IsHpGreen(const Pixel& c) {
    // Including native yellow-green depletion gradients, but not dark bezel,
    // emerald timer digits, cyan name glyphs or grey HUD background.
    return c.g>=75u && c.g>c.r*13u/10u && c.g>c.b*13u/10u &&
           c.r>=30u && c.b>=22u && (c.r>c.b?c.r-c.b:c.b-c.r)<48u;
}
// Tight name-highlight-only area for the independently blue Sabretooth glyph sheet.
// Do not widen the original green HP or cyan-name predicates. The original
// name shadows are purple: only blue-dominant, bright highlight ink qualifies.
static inline bool BlueNameHighlightRegion(unsigned x,unsigned y) {
    return y>=16u && y<=34u && ((x>=30u&&x<=112u) || (x>=271u&&x<=350u));
}
static inline bool IsNameBlueHighlight(const Pixel& c) {
    return c.b>=185u && c.g>=42u &&
           c.b*10u>=c.r*16u && c.b*10u>=c.g*13u &&
           c.b>=c.g+28u && c.b>=c.r+38u;
}
static inline bool IsNameCyan(const Pixel& c) {
    return c.g>=115u && c.b>=92u && c.g>c.r*13u/10u &&
           c.b>c.r*6u/5u && (c.g>c.b?c.g-c.b:c.b-c.g)<=65u;
}
// Converts the existing luma shading of the green bar into blue ramp, retaining
// the original native highlights and dark areas. No health reading is altered.
static inline Pixel Blue(const Pixel& c) {
    // Original dominant green is the amplitude; blue fill 20% R / 57% G / 100% B.
    const unsigned peak=c.g;
    return Pixel{Bound(peak*28u/100u), Bound(peak*57u/100u),
                 Bound(peak*115u/100u),255u};
}
static inline Pixel Yellow(const Pixel& c) {
    // Bright yellow highlight, purple shadow left as-is.
    unsigned intensity=c.g>c.b?c.g:c.b;
    return Pixel{Bound(intensity*135u/100u),Bound(intensity*120u/100u),
                 Bound(intensity*20u/100u),255u};
}
static inline void RenderRecolor(const unsigned char* source,unsigned char* dest,
                                 int stride, unsigned width=384u,unsigned height=224u) {
    if(!source || !dest || stride<(int)(width*4u) || width!=384u || height!=224u) return;
    // Overlay was already cleared by the existing SDL HUD path. Only change
    // pixels that belong to one of the two exact upper-HUD regions.
    for(unsigned y=16u;y<=44u;y++)for(unsigned x=19u;x<=365u;x++) {
        if(!HpRegion(x,y) && !NameRegion(x,y))continue;
        const unsigned char* src=source+y*384u*4u+x*4u;
        const Pixel original=Read(src);
        if(original.a==0u)continue;
        Pixel result{}; bool write=false;
        if(HpRegion(x,y) && IsHpGreen(original)) { result=Blue(original);write=true; }
        else if(NameRegion(x,y) && (IsNameCyan(original) || (BlueNameHighlightRegion(x,y) && IsNameBlueHighlight(original)))) {result=Yellow(original);write=true;}
        if(write) {
            unsigned char* p=dest+y*(unsigned)stride+x*4u;
            p[0]=(unsigned char)result.r;p[1]=(unsigned char)result.g;
            p[2]=(unsigned char)result.b;p[3]=255u;
        }
    }
}
// Native forest/non-Wily fallback uses the existing RGB888 SDL texture input:
// little-endian bytes B,G,R,unused. Wily's isolated sprites are RGBA32.
static inline void RenderRecolorRGB888(const unsigned char* source,unsigned char* dest,
                                       int sourcePitch,int destPitch) {
    if(!source || !dest || sourcePitch<384*4 || destPitch<384*4)return;
    for(unsigned y=16u;y<=44u;y++)for(unsigned x=19u;x<=365u;x++) {
        if(!HpRegion(x,y) && !NameRegion(x,y))continue;
        const unsigned char* src=source+y*(unsigned)sourcePitch+x*4u;
        const Pixel original=Pixel{src[2],src[1],src[0],255u};
        Pixel result{};bool write=false;
        if(HpRegion(x,y) && IsHpGreen(original)){result=Blue(original);write=true;}
        else if(NameRegion(x,y) && (IsNameCyan(original) || (BlueNameHighlightRegion(x,y) && IsNameBlueHighlight(original)))){result=Yellow(original);write=true;}
        if(write){
            unsigned char* p=dest+y*(unsigned)destPitch+x*4u;
            p[0]=(unsigned char)result.r;p[1]=(unsigned char)result.g;
            p[2]=(unsigned char)result.b;p[3]=255u;
        }
    }
}

}
#endif
