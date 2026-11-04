#ifndef XMVSF_STAGE87_46_HUD_LAYERS_H
#define XMVSF_STAGE87_46_HUD_LAYERS_H
// Stage87.46: native meter is drawn once before native priority-5/6/7 sprites.
// This is a software HUD underlay, NOT an SDL post-composition layer.
// Selection Num5 uses the existing fighter black point=96 on candidate
// character-art objects only; never apply it to the whole selection screen.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "xmvsf_stage87_33_dizzy_hud.h"
namespace XM8746 {
static inline bool ShouldNativeMeter(int from,int to,int status,bool capture,bool enabled,
                                     int width,int height,int bpp){
    return enabled && !capture && status==8 && from<=5 && to>=5 &&
           width==384 && height==224 && bpp==4;
}
template<typename Packer>
static inline unsigned BlitToNative(const unsigned char* src,unsigned char* dst,int pitch,
                                   Packer packer){
    if(!src || !dst || pitch<384*4 || !packer)return 0;
    unsigned pixels=0;
    // Numerals and bar occupy only the top HUD. No game pixel outside these
    // scanlines is modified. Color packing is delegated to native BurnHighCol.
    for(int y=43;y<=53;++y){
        for(int x=0;x<384;++x){
            const unsigned char* p=src+((y*384+x)*4);
            if(!p[3])continue;
            const std::uint32_t rgb=packer((int)p[0],(int)p[1],(int)p[2],0);
            std::memcpy(dst+y*pitch+x*4,&rgb,4);
            ++pixels;
        }
    }
    return pixels;
}
// XM87.122: after native scroll/objects finish, paint the stun HUD over
// scenery but UNDER the true ROM fighter opacity (not a rectangular mask).
// Returns pixels drawn; actorPixelsSkipped is optional audit-only output.
template<typename Packer>
static inline unsigned BlitOverSceneryUnderActors(const unsigned char* src,
        unsigned char* dst,int pitch,const unsigned char* actorMask,
        unsigned maskWidth,unsigned maskHeight,Packer packer,
        unsigned* actorPixelsSkipped=0){
    if(!src || !dst || !actorMask || !packer || pitch<384*4 ||
       maskWidth!=384u || maskHeight!=224u)return 0;
    unsigned drawn=0,skipped=0;
    for(unsigned y=43u;y<=53u;++y){
        for(unsigned x=0;x<384u;++x){
            const unsigned char* pixel=src+((y*384u+x)*4u);
            if(!pixel[3])continue;
            if(actorMask[y*maskWidth+x]){++skipped;continue;}
            const std::uint32_t rgb=packer(pixel[0],pixel[1],pixel[2],0);
            std::memcpy(dst+(std::size_t)y*(std::size_t)pitch+x*4u,&rgb,4);
            ++drawn;
        }
    }
    if(actorPixelsSkipped)*actorPixelsSkipped=skipped;
    return drawn;
}
static inline bool SelectionArt(int phase,unsigned priority,int x,int y,
                                unsigned wide,unsigned tall,bool hud,bool scenery){
    return (phase==0 || phase==1) && priority==6 && !hud && !scenery &&
           wide>=2 && tall>=2 && x>=16 && x<352 && y>=16 && y<212;
}
static inline std::uint32_t Num5Ink(std::uint32_t from){
    // FBNeo native BurnHighCol format: R in low byte, then G, then B.
    const unsigned r=from&255u,g=(from>>8)&255u,b=(from>>16)&255u;
    const unsigned luma=(77u*r+150u*g+29u*b+128u)>>8;
    const unsigned ink=luma<=96u?0u:luma;
    return (from&0xff000000u)|(ink<<16)|(ink<<8)|ink;
}
}
#endif
