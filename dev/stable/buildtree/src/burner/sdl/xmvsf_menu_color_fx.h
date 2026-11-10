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
                                  std::vector<Uint8>& output,Uint32 now,float introStrength=0.0f,int width=0) {
    if(source.empty())return;
    output.resize(source.size());
    const float strength=std::max(0.0f,std::min(1.0f,introStrength));
    // Retain the original steady 36s hue orbit; add a fast startup strobe.
    const float normalTurn=float(now%36000u)/100.0f;
    const float strobe=std::fmod(normalTurn+
        235.0f+110.0f*std::sin(float(now%10000u)*0.035f),360.0f);
    static const Uint8 bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
    const auto rotated=[&](Uint8 r,Uint8 g,Uint8 b,float turn,Uint8* out){
        const Uint8 hi=std::max(r,std::max(g,b)),lo=std::min(r,std::min(g,b));
        const float chroma=float(hi)-float(lo);
        if(chroma<1.0f){out[0]=r;out[1]=g;out[2]=b;return;}
        const float h=std::fmod(hueDegrees(r,g,b)+turn,360.0f);
        const float sector=h/60.0f;
        const float cross=chroma*(1.0f-std::fabs(std::fmod(sector,2.0f)-1.0f));
        float rr=0.f,gg=0.f,bb=0.f;
        switch(int(sector)){
            case 0:rr=chroma;gg=cross;break;
            case 1:rr=cross;gg=chroma;break;
            case 2:gg=chroma;bb=cross;break;
            case 3:gg=cross;bb=chroma;break;
            case 4:rr=cross;bb=chroma;break;
            default:rr=chroma;bb=cross;break;
        }
        out[0]=byte(float(lo)+rr);
        out[1]=byte(float(lo)+gg);
        out[2]=byte(float(lo)+bb);
    };
    // Ordered thresholds are fixed in source-image pixel space. First
    // frame: pure white. Last frame: exact established GIF hues with no FX.
    for(size_t n=0;n+3<source.size();n+=4){
        output[n+3]=source[n+3];
        if(source[n+3]==0){output[n]=source[n];output[n+1]=source[n+1];
                             output[n+2]=source[n+2];continue;}
        const Uint8 r=source[n],g=source[n+1],b=source[n+2];
        rotated(r,g,b,normalTurn,&output[n]);
        if(strength<=0.0f)continue;
        const size_t pixel=n/4u;
        const size_t row=width>0?pixel/size_t(width):pixel/4u;
        const size_t col=width>0?pixel%size_t(width):pixel;
        const float threshold=(float(bayer[(row&3u)*4u+(col&3u)])+0.5f)/16.f;
        if(strength<threshold)continue;
        Uint8 flash[3]={r,g,b};
        rotated(r,g,b,strobe,flash);
        // Bright white -> bright spectral strobe -> authored, dim background.
        const float white=std::max(0.0f,std::min(1.0f,(strength-0.50f)*2.0f));
        for(int ch=0;ch<3;ch++)
            output[n+ch]=byte(float(flash[ch])*(1.0f-white)+255.0f*white);
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
        // Preserve very dark purple pixels in the gas/vibration mask.
        // They were previously clipped by hi<32 and chroma<22, creating
        // an artificial seam across the low-value magenta core.
        const float hue=hueDegrees(red,green,blue);
        const int chroma=int(hi)-int(lo);
        const bool purpleMaterial=
            hue>=240.0f && hue<360.0f &&
            (int(red)+int(blue)-2*int(green))>=16 &&
            (int(blue)>=int(green)+7 || int(red)>=int(green)+18);
        if(hi<16 || (!purpleMaterial && chroma<22) ||
           (purpleMaterial && chroma<10))continue;
        float gain=1.0f;
        bool warm=false;
        float pulse=0.0f;
        if(hue>=16.0f && hue<80.0f){
            // Yellow and orange have separate overlapping envelopes.
            const float mix=(hue-16.0f)/64.0f;
            pulse=orangePulse*(1.0f-mix)+yellowPulse*mix;
            gain=1.02f+0.18f*pulse;warm=true;
        }else if(hue>=80.0f && hue<178.0f){
            pulse=greenPulse;
            gain=1.02f+0.18f*pulse;warm=true;
        }else if((hue>=184.0f && hue<=338.0f) || purpleMaterial){
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
        // Vibrate the native purple material without rectangular region
        // masks. Old vsCore (y=69..144) and bottomShadow (y=139..220)
        // ended on two visible horizontal seamlines through the logo.
        // Original pixel hue/saturation determines membership, including
        // the dark core and full extruded Street Fighter shadow. Only
        // the vibration's *frequency* morphs continuously with height.
        if(purpleMaterial){
            const float cycle=std::fmod(seconds,2.65f);
            // 0.55-second burst; 2.1 seconds of breathing between bursts.
            const float active=cycle<0.55f ?
                std::sin(cycle*3.14159265f/0.55f) : 0.0f;
            const float envelope=active*active;
            const float mix=std::max(0.0f,std::min(1.0f,
                                (float(y)-94.0f)/114.0f));
            // Blend the VS core (~9.2 Hz) and lower extrusion (~7.4 Hz)
            // as overlapping fields, never using a step threshold.
            const float coreWave=std::sin(seconds*6.2831853f*9.2f+
                                float(x)*0.047f+float(y)*0.073f);
            const float shadowWave=std::sin(seconds*6.2831853f*7.4f+
                                float(x)*0.027f-float(y)*0.061f);
            const float buzz=coreWave*(1.0f-mix)+shadowWave*mix;
            // Modulate original RGB only; alpha/coordinates/pixel edges
            // are untouched, even at the outermost purple shadow.
            gain*=1.0f+envelope*(0.25f*buzz+0.07f);
        }
        if(warm){
            // Add a gentle light-to-white component as well as exposure.
            // Pure 255 greens/yellows otherwise cannot pulse by multiplication.
            const float glow=0.02f+0.16f*pulse;
            output[p]=byte(float(red)*gain+(255.0f-float(red))*glow);
            output[p+1]=byte(float(green)*gain+(255.0f-float(green))*glow);
            output[p+2]=byte(float(blue)*gain+(255.0f-float(blue))*glow);
        }else{
            output[p]=byte(float(red)*gain);
            output[p+1]=byte(float(green)*gain);
            output[p+2]=byte(float(blue)*gain);
        }
    }
}
// Dither the additional intro hue out of the ORIGINAL authored logo pixels.
// The final frame is the unmodified regular logoPalettePulse texture.
static inline void introLogoDither(const std::vector<Uint8>& regular,
                                   std::vector<Uint8>& result,
                                   int width,int height,Uint32 now,
                                   float remaining){
    if(width<=0||height<=0 || regular.size()<(size_t)width*(size_t)height*4u)return;
    remaining=std::max(0.0f,std::min(1.0f,remaining));
    result=regular;
    if(remaining<=0.0f)return;
    static const Uint8 bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
    const float turn=235.0f+110.0f*std::sin(float(now%10000u)*0.035f);
    const float whiten=std::max(0.0f,std::min(1.0f,(remaining-0.50f)*2.0f));
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){
        const size_t p=(size_t(y)*width+x)*4u;
        if(regular[p+3]<8u)continue;
        const float threshold=(float(bayer[(y&3)*4+(x&3)])+0.5f)/16.0f;
        // Source-space Bayer mask releases pixels to their original colors.
        if(remaining<threshold)continue;
        const Uint8 red=regular[p],green=regular[p+1],blue=regular[p+2];
        const float hi=std::max(float(red),std::max(float(green),float(blue)));
        const float lo=std::min(float(red),std::min(float(green),float(blue)));
        const float chroma=hi-lo;
        float rr=float(red),gg=float(green),bb=float(blue);
        if(chroma>=1.0f){
            const float hue=std::fmod(hueDegrees(red,green,blue)+turn,360.0f);
            const float sector=hue/60.0f;
            const float zig=chroma*(1.0f-std::fabs(std::fmod(sector,2.0f)-1.0f));
            switch(int(sector)){
                case 0: rr=lo+chroma;gg=lo+zig;bb=lo;break;
                case 1: rr=lo+zig;gg=lo+chroma;bb=lo;break;
                case 2: rr=lo;gg=lo+chroma;bb=lo+zig;break;
                case 3: rr=lo;gg=lo+zig;bb=lo+chroma;break;
                case 4: rr=lo+zig;gg=lo;bb=lo+chroma;break;
                default: rr=lo+chroma;gg=lo;bb=lo+zig;break;
            }
        }
        result[p]=byte(rr*(1.0f-whiten)+255.0f*whiten);
        result[p+1]=byte(gg*(1.0f-whiten)+255.0f*whiten);
        result[p+2]=byte(bb*(1.0f-whiten)+255.0f*whiten);
    }
}
} // namespace XMMenuColor
#endif
