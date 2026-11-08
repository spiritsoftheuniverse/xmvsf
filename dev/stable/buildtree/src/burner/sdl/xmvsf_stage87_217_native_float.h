#ifndef XMVSF_STAGE87_217_NATIVE_FLOAT_H
#define XMVSF_STAGE87_217_NATIVE_FLOAT_H
// XMVSF TEST 87.217 — FLOATING FRAME-DATA TEXT IN GAME PIXEL SPACE
// This is a CPU RASTERIZER for the game's 384x224 source image, NOT an
// SDL-window-coordinate text effect. The result is uploaded as a single
// transparent native-resolution layer before menus/external UI are drawn.
// Original SF2 small-font RGBA pixels only. No ROM writes or SDL dependency.
// Draws once into a freshly cleared image for each paint (no ghost trails).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "xmvsf_stage30_frame_data.h"

namespace XM87217 {
struct Glyph {
    int x=0,y=0,w=0,h=0,advance=4;
};
struct Font {
    const std::uint8_t* rgba=nullptr;
    int width=0,height=0,pitch=0;
    Glyph glyph[128];
    bool prepare(const std::uint8_t* pixels,int w,int h,int rowPitch){
        rgba=nullptr;width=height=pitch=0;
        for(int i=0;i<128;++i)glyph[i]=Glyph{};
        if(!pixels || w!=128 || h!=48 || rowPitch<w*4)return false;
        rgba=pixels;width=w;height=h;pitch=rowPitch;
        for(int ch=32;ch<=126;++ch){
            Glyph& g=glyph[ch];
            const int bx=((ch-32)%16)*8,by=((ch-32)/16)*8;
            const int span=std::max(0,std::min(8,w-bx));
            const int cellh=std::max(0,std::min(8,h-by));
            g.advance=(ch==' '?4:4)+1;
            int left=span,right=-1;
            for(int y=0;y<cellh;++y)for(int x=0;x<span;++x){
                const std::uint8_t* p=rgba+(by+y)*pitch+(bx+x)*4;
                if(p[3]>12){left=std::min(left,x);right=std::max(right,x);}
            }
            if(right>=left){
                g.x=bx+left;g.y=by;
                g.w=right-left+1;g.h=cellh;g.advance=g.w+1;
            }
        }
        return true;
    }
};
struct Image {
    std::uint8_t* rgba=nullptr;
    int w=0,h=0,pitch=0;
    bool valid() const {return rgba && w>0 && h>0 && pitch>=w*4;}
};
static inline void over(std::uint8_t* dst,const std::uint8_t* src,
                        int opacity){
    // Straight-alpha source-over; store RGBA32 byte order [R,G,B,A].
    // The SDL texture uses SDL_PIXELFORMAT_RGBA32 / SDL_BLENDMODE_BLEND.
    const int sa=(int(src[3])*opacity+127)/255;
    if(sa<=0)return;
    const int da=dst[3],outA=sa+(da*(255-sa)+127)/255;
    for(int c=0;c<3;++c){
        const int mixed=int(src[c])*sa+
            (int(dst[c])*da*(255-sa)+127)/255;
        dst[c]=(std::uint8_t)(outA?((mixed+outA/2)/outA):0);
    }
    dst[3]=(std::uint8_t)outA;
}
static inline void one(Image out,const Font& font,const XM30Float& fl){
    if(!out.valid()||!font.rgba||!fl.visible ||
       fl.age<0||fl.age>=XM30FrameData::FLOAT_LIFE)return;
    // SF2 small atlas: one 8px-high glyph per 384x224 source pixel.
    // World/native hitbox coordinates already use the SAME source canvas.
    const XM30FloatPose pose=xm30FloatPose(fl.age,8);
    if(pose.alpha<=0 || pose.horizontal<=0.0)return;
    char text[32];std::snprintf(text,sizeof(text),"S:%dF",fl.startup);
    int advance=0;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p){
        const unsigned char c=*p;
        advance+=font.glyph[c<128?c:32].advance;
    }
    const int span=int(double(advance)*pose.horizontal+0.5);
    if(span<=0)return;
    const int anchorX=std::max(0,std::min(out.w-1,fl.x));
    // Rise is measured in NATIVE pixels. No reference to SDL view size.
    const int anchorY=std::max(0,std::min(out.h-1,fl.y));
    const int top=anchorY-pose.rise-8;
    const int left=anchorX-span/2;
    int pen=0;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p){
        const unsigned char c=*p;
        const Glyph& g=font.glyph[c<128?c:32];
        if(g.w>0 && g.h>0){
            const int x0=left+int(double(pen)*pose.horizontal+0.5);
            const int x1=left+int(double(pen+g.w)*pose.horizontal+0.5);
            // Never leave a 1px ghost when width reaches zero.
            if(x1>x0)for(int y=0;y<g.h;++y){
                const int oy=top+y;
                if(oy<0||oy>=out.h)continue;
                for(int x=std::max(0,x0);x<std::min(out.w,x1);++x){
                    const int srcX=g.x+(x-x0)*g.w/(x1-x0);
                    const int srcY=g.y+y;
                    const std::uint8_t* src=font.rgba+srcY*font.pitch+srcX*4;
                    std::uint8_t* dst=out.rgba+oy*out.pitch+x*4;
                    over(dst,src,pose.alpha);
                }
            }
        }
        pen+=g.advance;
    }
}
static inline bool any(const XM30FrameData& data){
    if(!data.enabled||!data.inCombat)return false;
    for(int i=0;i<XM30FrameData::FLOATS;++i)
        if(data.floats[i].visible && data.floats[i].age>=0 &&
           data.floats[i].age<XM30FrameData::FLOAT_LIFE)return true;
    return false;
}
static inline void draw(Image target,const Font& font,
                        const XM30FrameData& data){
    if(!target.valid() || !font.rgba || !any(data))return;
    for(int i=0;i<XM30FrameData::FLOATS;++i)
        one(target,font,data.floats[i]);
}
} // namespace XM87217
#endif
