#ifndef XMVSF_MENU_COLOR_FX_H
#define XMVSF_MENU_COLOR_FX_H
// Native SDL menu color effects. Source GIF and PNG stay unmodified.
// Source-to-texture conversion is one-time per frame. Hue and pulse use
// original RGB/alpha pixels, not cumulatively processed screenshots.
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace XMMenuColor {
static inline Uint8 byte(float v) {
    return (Uint8)std::max(0,std::min(255,int(v+0.5f)));
}
static inline bool rgbaPixels(SDL_Surface* source,std::vector<Uint8>& pixels) {
    if(!source || source->w<=0 || source->h<=0)return false;
    SDL_Surface* image=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_RGBA32,0);
    if(!image)return false;
    if(SDL_LockSurface(image)!=0){SDL_FreeSurface(image);return false;}
    const size_t row=(size_t)image->w*4u;
    pixels.resize(row*(size_t)image->h);
    for(int y=0;y<image->h;y++)
        std::memcpy(&pixels[(size_t)y*row],
                    (Uint8*)image->pixels+(size_t)y*image->pitch,row);
    SDL_UnlockSurface(image);
    SDL_FreeSurface(image);
    return true;
}
// Hue is measured directly from a source pixel, not its screen position.
static inline float hueDegrees(Uint8 red,Uint8 green,Uint8 blue) {
    const float r=float(red),g=float(green),b=float(blue);
    const float hi=std::max(r,std::max(g,b)),lo=std::min(r,std::min(g,b));
    const float chroma=hi-lo;
    if(chroma<1.0f)return -1.0f;
    float hue;
    if(hi==r)hue=(g-b)/chroma;
    else if(hi==g)hue=2.0f+(b-r)/chroma;
    else hue=4.0f+(r-g)/chroma;
    if(hue<0.0f)hue+=6.0f;
    return hue*60.0f;
}
// One full, continuous hue turn per 36 seconds, while each original GIF
// animation still advances at its authored frame delays.
static inline void backgroundHue(const std::vector<Uint8>& source,
                                  std::vector<Uint8>& output,Uint32 now) {
    if(source.empty())return;
    output.resize(source.size());
    const float turn=float(now%36000u)/100.0f; // degrees [0,360)
    for(size_t n=0;n+3<source.size();n+=4){
        const Uint8 r=source[n],g=source[n+1],b=source[n+2];
        output[n+3]=source[n+3];
        const Uint8 hi=std::max(r,std::max(g,b)),lo=std::min(r,std::min(g,b));
        const float chroma=float(hi)-float(lo);
        if(chroma<1.0f || source[n+3]==0){
            output[n]=r;output[n+1]=g;output[n+2]=b;continue;
        }
        float h=hueDegrees(r,g,b)+turn;
        if(h>=360.0f)h-=360.0f;
        const float sector=h/60.0f;
        const float x=chroma*(1.0f-std::fabs(std::fmod(sector,2.0f)-1.0f));
        const float m=float(lo);
        const int region=int(sector);
        float rr=0.0f,gg=0.0f,bb=0.0f;
        switch(region){
        case 0: rr=chroma;gg=x;break;
        case 1: rr=x;gg=chroma;break;
        case 2: gg=chroma;bb=x;break;
        case 3: gg=x;bb=chroma;break;
        case 4: rr=x;bb=chroma;break;
        default: rr=chroma;bb=x;break;
        }
        output[n]=byte(rr+m);
        output[n+1]=byte(gg+m);
        output[n+2]=byte(bb+m);
    }
}
// Four-by-four Bayer thresholds lock gas grain to SOURCE pixels. Moving
// low-frequency waves animate the value underneath, so blue and purple
// shades breathe as dither instead of shimmering through RGB channel blur.
static inline void logoPalettePulse(const std::vector<Uint8>& source,
                                    std::vector<Uint8>& output,
                                    int width,int height,Uint32 now) {
    if(width<=0||height<=0||source.size()<(size_t)width*(size_t)height*4u)return;
    output.resize(source.size());
    static const Uint8 bayer[16]={
        0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5
    };
    const float seconds=float(now%120000u)/1000.0f;
    const float orangePulse=0.5f+0.5f*std::sin(seconds*2.0f);
    const float greenPulse=0.5f+0.5f*std::sin(seconds*1.7f+1.1f);
    const float yellowPulse=0.5f+0.5f*std::sin(seconds*2.25f+0.55f);
    for(int y=0;y<height;y++)for(int x=0;x<width;x++){
        const size_t p=((size_t)y*(size_t)width+(size_t)x)*4u;
        const Uint8 red=source[p],green=source[p+1],blue=source[p+2];
        const Uint8 alpha=source[p+3];
        output[p]=red;output[p+1]=green;output[p+2]=blue;output[p+3]=alpha;
        if(alpha<8)continue;
        const Uint8 hi=std::max(red,std::max(green,blue));
        const Uint8 lo=std::min(red,std::min(green,blue));
        if(hi<32 || int(hi)-int(lo)<22)continue;
        const float hue=hueDegrees(red,green,blue);
        float gain=1.0f;
        if(hue>=16.0f && hue<80.0f){
            // Yellow and orange have their own overlapping pulse envelopes.
            const float mix=(hue-16.0f)/64.0f;
            gain=1.02f+0.27f*(orangePulse*(1.0f-mix)+yellowPulse*mix);
        }else if(hue>=80.0f && hue<178.0f){
            gain=1.03f+0.27f*greenPulse;
        }else if(hue>=184.0f && hue<=338.0f){
            const float curl=std::sin(float(x)*0.047f+float(y)*0.023f+
                                seconds*0.60f+
                                std::sin(float(y)*0.035f-seconds*0.23f));
            const float vapor=std::sin(float(y)*0.054f-float(x)*0.019f-
                                 seconds*0.44f);
            const float cloud=std::max(0.0f,std::min(1.0f,
                                        0.5f+0.29f*curl+0.21f*vapor));
            const float threshold=(float(bayer[(y&3)*4+(x&3)])+0.5f)/16.0f;
            gain=0.82f+0.28f*cloud+(cloud>threshold?0.11f:-0.07f);
        }else continue; // red badge, neutrals and other colors unchanged
        output[p]=byte(float(red)*gain);
        output[p+1]=byte(float(green)*gain);
        output[p+2]=byte(float(blue)*gain);
    }
}
} // namespace XMMenuColor
#endif
