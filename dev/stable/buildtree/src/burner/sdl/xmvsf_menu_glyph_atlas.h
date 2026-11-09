#ifndef XMVSF_MENU_GLYPH_ATLAS_H
#define XMVSF_MENU_GLYPH_ATLAS_H
// Pixel-exact UI glyph system sourced from _art/sf2font[small].png.
// Sheets are a 16-column ASCII grid (space through DEL), six rows.
// Large: 190x70, 12x12 stride (rightmost two pixels intentionally absent).
// Small: 128x48, 8x8 stride. Never resample with linear filtering.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "xmvsf_menu_color_fx.h"

namespace XMGlyph {
class Atlas {
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* large_ = nullptr;
    SDL_Texture* small_ = nullptr;
    SDL_Texture* largeReveal_=nullptr;
    SDL_Texture* smallReveal_=nullptr;
    SDL_Texture* logo_ = nullptr;
    SDL_Texture* logoFx_ = nullptr; // streaming color-group pulse layer
    std::vector<Uint8> originalLogo_;
    std::vector<Uint8> animatedLogo_,introLogo_;
    std::vector<Uint8> originalLargeRGBA_,originalSmallRGBA_;
    std::vector<Uint8> revealedLargeRGBA_,revealedSmallRGBA_;
    bool revealOn_=false;
    int revealStep_=-1,lastIntroLogoStep_=-1;
    // A CPU copy of the REAL SF2 small PNG for the 384x224 native
    // game-render popup layer. Never synthesize a substitute bitmap.
    std::vector<Uint8> nativeSmallRGBA_;
    Uint32 lastLogoFX_=0;
    int logoWidth_=0,logoHeight_=0;
    bool logoFxWarning_=false;
    SDL_Rect logoSource_ = {0,0,0,0};
    // A fixed atlas grid is for locating sprites, not laying out text.
    // Per-glyph bounds discard empty side columns, keeping colored pixels intact.
    struct Glyph {
        SDL_Rect src = {0,0,0,0};
        int advance = 0;
    };
    Glyph metrics_[2][128] = {};
    bool attempted_ = false;
    static void measure(SDL_Surface* surface,Glyph* glyphs,int cell) {
        if(!surface)return;
        SDL_Surface* rgba=SDL_ConvertSurfaceFormat(surface,SDL_PIXELFORMAT_RGBA32,0);
        if(!rgba)return;
        if(SDL_LockSurface(rgba)!=0){SDL_FreeSurface(rgba);return;}
        for(int ch=32;ch<=126;ch++) {
            const int baseX=((ch-32)%16)*cell;
            const int baseY=((ch-32)/16)*cell;
            const int span=std::max(0,std::min(cell,rgba->w-baseX));
            const int height=std::max(0,std::min(cell,rgba->h-baseY));
            const int tracking=cell==12?2:1; // pixel-space gap, never scaled twice
            Glyph& g=glyphs[ch];
            g.advance=(ch==' ' ? cell/2 : std::max(1,cell/2))+tracking;
            if(span<=0||height<=0)continue;
            int left=span,right=-1;
            for(int y=0;y<height;y++)for(int x=0;x<span;x++){
                const Uint8* p=(const Uint8*)rgba->pixels+
                    (baseY+y)*rgba->pitch+(baseX+x)*4;
                if(p[3]>12){left=std::min(left,x);right=std::max(right,x);}
            }
            if(right>=left){
                g.src={baseX+left,baseY,right-left+1,height};
                g.advance=g.src.w+tracking;
            }
        }
        SDL_UnlockSurface(rgba);
        SDL_FreeSurface(rgba);
    }
    static std::vector<std::string> paths(const char* filename) {
        std::vector<std::string> folders;
        folders.push_back("_art/");
        folders.push_back("../_art/");
        folders.push_back("../../_art/");
        char* base = SDL_GetBasePath();
        if(base) {
            folders.push_back(std::string(base) + "../../_art/");
            folders.push_back(std::string(base) + "../_art/");
            folders.push_back(std::string(base) + "_art/");
            SDL_free(base);
        }
        std::vector<std::string> result;
        for(const auto& folder:folders) result.push_back(folder + filename);
        return result;
    }
    static SDL_Surface* open(const char* filename) {
        for(const auto& path:paths(filename)) {
            SDL_Surface* surface = IMG_Load(path.c_str());
            if(surface) return surface;
        }
        std::fprintf(stderr,"XM MENU ATLAS missing _art/%s; original bitmap fallback retained\n",filename);
        return nullptr;
    }
    static SDL_Texture* texture(SDL_Renderer* renderer, SDL_Surface* surface) {
        if(!surface) return nullptr;
        SDL_Texture* result = SDL_CreateTextureFromSurface(renderer,surface);
        if(result) SDL_SetTextureBlendMode(result,SDL_BLENDMODE_BLEND);
        return result;
    }
    static bool white(const Uint8* p) {
        return p[0]>=250 && p[1]>=250 && p[2]>=250 && p[3]>=240;
    }
    static void normalizeLogo(SDL_Surface* s,SDL_Rect& bounds) {
        // A PNG with an opaque white canvas is treated like a keyed sprite,
        // but ONLY border-connected white is erased; enclosed white highlights
        // in the lettering are kept. Transparent PNGs are untouched.
        if(!s || SDL_LockSurface(s)!=0)return;
        const int w=s->w,h=s->h;
        Uint8* base=(Uint8*)s->pixels;
        auto pixel=[&](int x,int y)->Uint8*{return base+y*s->pitch+x*4;};
        const bool opaqueWhiteBorder=white(pixel(0,0));
        if(opaqueWhiteBorder){
            std::vector<Uint8> seen((size_t)w*h,0);
            std::vector<int> queue;queue.reserve((size_t)w*h/2);
            auto add=[&](int x,int y){
                if(x<0||y<0||x>=w||y>=h)return;
                const int i=y*w+x;
                if(!seen[i] && white(pixel(x,y))){seen[i]=1;queue.push_back(i);}
            };
            for(int x=0;x<w;x++){add(x,0);add(x,h-1);}
            for(int y=0;y<h;y++){add(0,y);add(w-1,y);}
            for(size_t i=0;i<queue.size();++i){
                const int x=queue[i]%w,y=queue[i]/w;
                pixel(x,y)[3]=0;
                add(x-1,y);add(x+1,y);add(x,y-1);add(x,y+1);
            }
        }
        int left=w,top=h,right=-1,bottom=-1;
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            if(pixel(x,y)[3]>12){
                left=std::min(left,x);top=std::min(top,y);
                right=std::max(right,x);bottom=std::max(bottom,y);
            }
        }
        if(right>=left && bottom>=top)bounds={left,top,right-left+1,bottom-top+1};
        SDL_UnlockSurface(s);
    }
public:
    void release(){
        if(large_)SDL_DestroyTexture(large_);
        if(small_)SDL_DestroyTexture(small_);
        if(largeReveal_)SDL_DestroyTexture(largeReveal_);
        if(smallReveal_)SDL_DestroyTexture(smallReveal_);
        if(logoFx_)SDL_DestroyTexture(logoFx_);
        if(logo_)SDL_DestroyTexture(logo_);
        large_=small_=logoFx_=logo_=nullptr;
        largeReveal_=smallReveal_=nullptr;
        originalLogo_.clear();animatedLogo_.clear();introLogo_.clear();nativeSmallRGBA_.clear();
        originalLargeRGBA_.clear();originalSmallRGBA_.clear();
        revealedLargeRGBA_.clear();revealedSmallRGBA_.clear();
        revealOn_=false;revealStep_=lastIntroLogoStep_=-1;
        lastLogoFX_=0;logoWidth_=logoHeight_=0;logoFxWarning_=false;
        renderer_=nullptr;logoSource_={0,0,0,0};attempted_=false;
        for(int i=0;i<2;i++)for(int c=0;c<128;c++)metrics_[i][c]=Glyph{};
    }
    void ensure(SDL_Renderer* renderer){
        if(renderer==renderer_ && attempted_)return;
        release();
        if(!renderer)return;
        renderer_=renderer;attempted_=true;
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
        SDL_Surface* l=open("sf2font.png");
        if(l){
            if(l->w==190 && l->h==70){
                measure(l,metrics_[0],12);
                large_=texture(renderer,l);
                if(large_)XMMenuColor::rgbaPixels(l,originalLargeRGBA_);
            }
            else std::fprintf(stderr,"XM MENU ATLAS unexpected large sheet %dx%d\n",l->w,l->h);
            SDL_FreeSurface(l);
        }
        SDL_Surface* s=open("sf2fontsmall.png");
        if(s){
            if(s->w==128 && s->h==48){
                measure(s,metrics_[1],8);
                small_=texture(renderer,s);
                if(small_)XMMenuColor::rgbaPixels(s,originalSmallRGBA_);
                SDL_Surface* native=SDL_ConvertSurfaceFormat(
                    s,SDL_PIXELFORMAT_RGBA32,0);
                if(native && SDL_LockSurface(native)==0){
                    nativeSmallRGBA_.resize(128u*48u*4u);
                    for(int y=0;y<48;++y)
                        std::memcpy(nativeSmallRGBA_.data()+y*128*4,
                                    (const Uint8*)native->pixels+y*native->pitch,
                                    128u*4u);
                    SDL_UnlockSurface(native);
                }
                if(native)SDL_FreeSurface(native);
            }
            else std::fprintf(stderr,"XM MENU ATLAS unexpected small sheet %dx%d\n",s->w,s->h);
            SDL_FreeSurface(s);
        }
        // One character is unpainted in the big source atlas ('/').
        // Borrow its width from the small sprite to prevent the next glyph
        // from colliding with the substituted art.
        if(metrics_[0]['/'].src.w==0 && metrics_[1]['/'].src.w>0)
            metrics_[0]['/'].advance=
                (metrics_[1]['/'].src.w*12+7)/8+2;
        SDL_Surface* original=open("logo.png");
        if(original){
            SDL_Surface* rgba=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
            SDL_FreeSurface(original);
            if(rgba){
                normalizeLogo(rgba,logoSource_);
                if(logoSource_.w && logoSource_.h){
                    logo_=texture(renderer,rgba); // original art fallback
                    logoWidth_=rgba->w;logoHeight_=rgba->h;
                    if(logo_ && XMMenuColor::rgbaPixels(rgba,originalLogo_)){
                        logoFx_=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING,logoWidth_,logoHeight_);
                        if(logoFx_)SDL_SetTextureBlendMode(logoFx_,SDL_BLENDMODE_BLEND);
                        else std::fprintf(stderr,
                            "XM MENU LOGO FX unsupported, retaining original PNG: %s\n",
                            SDL_GetError());
                    }
                }
                SDL_FreeSurface(rgba);
            }
        }
        std::fprintf(stderr,
            "XM MENU ATLAS loaded big=%d small=%d title=%d logo_src=%d,%d,%d,%d widths big[A]=%d big[I]=%d small[a]=%d small[i]=%d\n",
            large_!=nullptr,small_!=nullptr,logo_!=nullptr,
            logoSource_.x,logoSource_.y,logoSource_.w,logoSource_.h,
            metrics_[0]['A'].advance,metrics_[0]['I'].advance,
            metrics_[1]['a'].advance,metrics_[1]['i'].advance);
    }
    // Intrinsic source pixels determine advances. This is the normalization
    // pass: narrow I/i/l/1 and punctuation no longer consume a full cell.
    // 87.235: dither the SF2 sprite atlas, not the background or the logo.
    // A source-pixel 4x4 Bayer mask updates in sixteen steps over 500ms.
    void setReveal(SDL_Renderer* renderer,float progress){
        ensure(renderer);
        progress=std::max(0.0f,std::min(1.0f,progress));
        const int level=std::max(0,std::min(16,int(progress*16.0f)));
        revealOn_=level<16;
        if(!revealOn_||revealStep_==level)return;
        revealStep_=level;
        static const Uint8 bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
        auto stage=[&](SDL_Texture*& target,const std::vector<Uint8>& original,
                       std::vector<Uint8>& output,int w,int h){
            if(original.size()!=(size_t)w*h*4u)return;
            if(!target){
                target=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STREAMING,w,h);
                if(target)SDL_SetTextureBlendMode(target,SDL_BLENDMODE_BLEND);
            }
            if(!target)return;
            output=original;
            for(int y=0;y<h;++y)for(int x=0;x<w;++x)
                if(bayer[(y&3)*4+(x&3)]>=level)
                    output[((size_t)y*w+x)*4u+3u]=0;
            if(SDL_UpdateTexture(target,nullptr,output.data(),w*4)!=0){
                SDL_DestroyTexture(target);target=nullptr;
            }
        };
        stage(largeReveal_,originalLargeRGBA_,revealedLargeRGBA_,190,70);
        stage(smallReveal_,originalSmallRGBA_,revealedSmallRGBA_,128,48);
    }
    bool text(SDL_Renderer* renderer,const char* s,int x,int y,int maxWidth,
              bool centered,bool small,int scale,Uint8 brightness=255){
        ensure(renderer);
        SDL_Texture* font=small?
            (revealOn_&&smallReveal_?smallReveal_:small_):
            (revealOn_&&largeReveal_?largeReveal_:large_);
        if(!font||!s||!renderer)return false;
        const Glyph* glyphs=metrics_[small?1:0];
        const int cell=small?8:12;
        scale=std::max(1,scale);
        int measured=0;
        for(const unsigned char* p=(const unsigned char*)s;*p;p++) {
            const unsigned c=*p;
            measured+=(c>=32 && c<=126 && glyphs[c].advance>0)
                ?glyphs[c].advance:cell/2;
        }
        if(maxWidth>0 && measured*scale>maxWidth && scale>1)
            scale=std::max(1,maxWidth/std::max(1,measured));
        // Optical widths are measured BEFORE centering, not a character count.
        if(centered)x-=(measured*scale)/2;
        SDL_SetTextureColorMod(font,brightness,brightness,brightness);
        SDL_SetTextureAlphaMod(font,255);
        int written=0;
        for(const unsigned char* p=(const unsigned char*)s;*p;p++) {
            const unsigned c=*p;
            const Glyph& g=glyphs[c<128?c:32];
            const int advance=(c>=32 && c<=126 && g.advance>0)
                ?g.advance:cell/2;
            if(maxWidth>0 && written+std::max(advance,g.src.w)*scale>maxWidth)break;
            if(c>=33 && c<=126 && g.src.w>0){
                SDL_Rect dst={x+written,y,g.src.w*scale,g.src.h*scale};
                SDL_RenderCopy(renderer,font,&g.src,&dst);
            } else if(c=='/' && !small && small_ && metrics_[1][c].src.w>0) {
                // The large source sheet has no slash pixels; the small sheet
                // contains the original mark. Render it at matching height.
                const Glyph& fallback=metrics_[1][c];
                SDL_SetTextureColorMod(small_,brightness,brightness,brightness);
                SDL_Rect dst={x+written,y,
                    fallback.src.w*cell*scale/8,cell*scale};
                SDL_RenderCopy(renderer,small_,&fallback.src,&dst);
                SDL_SetTextureColorMod(small_,255,255,255);
            }
            written+=advance*scale;
        }
        SDL_SetTextureColorMod(font,255,255,255);
        return true;
    }
    // Caller uses this SOURCE RGBA32 data only for the native 384x224
    // game-pixel raster. Never draw popup text through the SDL window
    // logical canvas or via scaled glyph rectangles.
    bool nativeSmallPixels(SDL_Renderer* renderer,const Uint8*& pixels,
                           int& pitch){
        ensure(renderer);
        pixels=nativeSmallRGBA_.empty()?nullptr:nativeSmallRGBA_.data();
        pitch=pixels?128*4:0;
        return pixels!=nullptr;
    }
    // Draw the SOURCE ART, not the transparent 400x300 export canvas.
    // Its visible aspect ratio is maintained inside the requested target
    // rectangle; the original source PNG is never resized or rewritten.
    bool logoInRect(SDL_Renderer* renderer,const SDL_Rect& target,
                    float introRemaining=0.0f){
        ensure(renderer);
        if(!renderer||!logo_||logoSource_.w<=0||logoSource_.h<=0 ||
           target.w<=0||target.h<=0)return false;
        const Uint32 now=SDL_GetTicks();
        introRemaining=std::max(0.0f,std::min(1.0f,introRemaining));
        const int introStep=int(introRemaining*16.0f+0.5f);
        // Refresh the FX texture at 12.5 Hz, only for the on-screen logo.
        // Build from the original PNG every time: pulses never accumulate.
        if(logoFx_ && !originalLogo_.empty() &&
           (lastLogoFX_==0 || (Uint32)(now-lastLogoFX_)>=80u ||
            introStep!=lastIntroLogoStep_)){
            XMMenuColor::logoPalettePulse(originalLogo_,animatedLogo_,
                                           logoWidth_,logoHeight_,now);
            const std::vector<Uint8>* pixels=&animatedLogo_;
            if(introRemaining>0.0f){
                XMMenuColor::introLogoDither(animatedLogo_,introLogo_,
                                            logoWidth_,logoHeight_,now,introRemaining);
                pixels=&introLogo_;
            }
            if(SDL_UpdateTexture(logoFx_,nullptr,pixels->data(),
                                 logoWidth_*4)!=0){
                if(!logoFxWarning_){
                    std::fprintf(stderr,"XM MENU LOGO FX upload failed: %s\n",
                                 SDL_GetError());
                    logoFxWarning_=true;
                }
                // Rendering must continue with the untouched original PNG.
                SDL_DestroyTexture(logoFx_);logoFx_=nullptr;
            }else {lastLogoFX_=now;lastIntroLogoStep_=introStep;}
        }
        const double scale=std::min(double(target.w)/logoSource_.w,
                                    double(target.h)/logoSource_.h);
        const int width=std::max(1,int(logoSource_.w*scale+0.5));
        const int height=std::max(1,int(logoSource_.h*scale+0.5));
        SDL_Rect dst={target.x+(target.w-width)/2,
                      target.y+(target.h-height)/2,width,height};
        SDL_RenderCopy(renderer,logoFx_?logoFx_:logo_,&logoSource_,&dst);
        return true;
    }
    // Shared 400x300 source-pixel alignment for independently painted II
    // strokes. The main PNG's alpha crop/fit remains the sole authority.
    // No per-stroke auto-fit: every exported part uses the same art canvas.
    bool logoFullCanvas(SDL_Renderer* renderer,const SDL_Rect& target,
                        SDL_Rect& fullCanvas){
        ensure(renderer);
        if(!renderer||!logo_||logoSource_.w<=0||logoSource_.h<=0 ||
           logoWidth_<=0||logoHeight_<=0)return false;
        const double scale=std::min(double(target.w)/logoSource_.w,
                                    double(target.h)/logoSource_.h);
        const int width=std::max(1,int(logoSource_.w*scale+0.5));
        const int height=std::max(1,int(logoSource_.h*scale+0.5));
        const SDL_Rect cropped={target.x+(target.w-width)/2,
                                target.y+(target.h-height)/2,width,height};
        fullCanvas={cropped.x-int(logoSource_.x*scale+0.5),
                    cropped.y-int(logoSource_.y*scale+0.5),
                    std::max(1,int(logoWidth_*scale+0.5)),
                    std::max(1,int(logoHeight_*scale+0.5))};
        return true;
    }
    // Pixel-art nearest-neighbor rendering with fractional visual scale.
    // The glyph geometry remains derived from the actual PNG atlas and the
    // advance is proportional. Destination edges snap to integer pixels.
    bool textScaled(SDL_Renderer* renderer,const char* string,
                    int anchorX,int top,bool small,double scale,
                    Uint8 brightness,int alignment){
        ensure(renderer);
        SDL_Texture* font=small?
            (revealOn_&&smallReveal_?smallReveal_:small_):
            (revealOn_&&largeReveal_?largeReveal_:large_);
        if(!renderer||!string||!font||scale<=0.0)return false;
        const Glyph* glyphs=metrics_[small?1:0];
        const int nominal=small?8:12;
        int units=0;
        for(const unsigned char* p=(const unsigned char*)string;*p;++p){
            const int ch=*p;
            units+=(ch>=32&&ch<=126&&glyphs[ch].advance>0)
                  ?glyphs[ch].advance:nominal/2;
        }
        const int fullWidth=int(units*scale+0.5);
        int x=anchorX;
        if(alignment==1)x-=fullWidth/2; // center
        else if(alignment==2)x-=fullWidth; // right edge
        SDL_SetTextureColorMod(font,brightness,brightness,brightness);
        SDL_SetTextureAlphaMod(font,255);
        int pen=0;
        for(const unsigned char* p=(const unsigned char*)string;*p;++p){
            const int ch=*p;
            const Glyph& g=glyphs[ch<128?ch:32];
            const int advance=(ch>=32&&ch<=126&&g.advance>0)
                              ?g.advance:nominal/2;
            if(ch>=33&&ch<=126&&g.src.w>0){
                SDL_Rect dst={x+int(pen*scale+0.5),top,
                    std::max(1,int(g.src.w*scale+0.5)),
                    std::max(1,int(g.src.h*scale+0.5))};
                SDL_RenderCopy(renderer,font,&g.src,&dst);
            }else if(ch=='/'&&!small&&small_&&metrics_[1][ch].src.w>0){
                const Glyph& fallback=metrics_[1][ch];
                SDL_SetTextureColorMod(small_,brightness,brightness,brightness);
                SDL_Rect dst={x+int(pen*scale+0.5),top,
                    std::max(1,int(fallback.src.w*nominal*scale/8.0+0.5)),
                    std::max(1,int(nominal*scale+0.5))};
                SDL_RenderCopy(renderer,small_,&fallback.src,&dst);
                SDL_SetTextureColorMod(small_,255,255,255);
            }
            pen+=advance;
        }
        SDL_SetTextureColorMod(font,255,255,255);
        return true;
    }

};
// Explicitly released on every boot->game boundary and native game exit.
static inline Atlas& atlas(){static Atlas value;return value;}
static inline void release(){atlas().release();}
static inline void preload(SDL_Renderer* renderer){atlas().ensure(renderer);}
static inline void menuReveal(SDL_Renderer* renderer,float progress){atlas().setReveal(renderer,progress);}
static inline bool center(SDL_Renderer* r,const char* s,int cx,int y,int pixel,
                          Uint8 red,Uint8 green,Uint8 blue){
    const bool useSmall=pixel<=2;
    const int scale=useSmall?(pixel<=1?1:2):(pixel>=6?3:2);
    const Uint8 light=(Uint8)((unsigned(red)+green+blue)/3);
    return atlas().text(r,s,cx,y,0,true,useSmall,scale,light);
}
static inline bool left(SDL_Renderer* r,const char* s,int x,int y,int maxWidth,
                        int scale,Uint8 brightness){
    return atlas().text(r,s,x,y,maxWidth,false,true,scale,brightness);
}
// The user's 1811x1358 (4:3) layout reference mapped uniformly into a
// centered 720x540 safe frame on our 960x540 SDL logical canvas.
// This retains source-pixel proportions; no image stretching is necessary.
struct MainLayout {
    enum {
        LogoX=235, LogoY=51, LogoW=544, LogoH=289,
        TextCenter=489, FirstRowY=347, RowStep=31,
        HintY=495, ExitY=522, ExitLeft=18, VersionRight=942
    };
};
static inline bool menuLogoFullCanvas(SDL_Renderer* renderer,SDL_Rect& fullCanvas){
    const SDL_Rect target={MainLayout::LogoX,MainLayout::LogoY,
                           MainLayout::LogoW,MainLayout::LogoH};
    return atlas().logoFullCanvas(renderer,target,fullCanvas);
}
static inline bool menuLogo(SDL_Renderer* r,float introRemaining=0.0f){
    const SDL_Rect dst={MainLayout::LogoX,MainLayout::LogoY,
                        MainLayout::LogoW,MainLayout::LogoH};
    return atlas().logoInRect(r,dst,introRemaining);
}
static inline bool menuChoice(SDL_Renderer* r,const char* s,int row,bool selected){
    return atlas().textScaled(r,s,MainLayout::TextCenter,
        MainLayout::FirstRowY+row*MainLayout::RowStep,
        false,1.35,selected?255:80,1);
}
static inline bool menuHint(SDL_Renderer* r,const char* s,Uint8 brightness){
    return atlas().textScaled(r,s,MainLayout::TextCenter,
        MainLayout::HintY,true,1.375,brightness,1);
}
static inline bool menuExit(SDL_Renderer* r,const char* s){
    return atlas().textScaled(r,s,MainLayout::ExitLeft,
        MainLayout::ExitY,true,1.08,185,0);
}
static inline bool menuVersion(SDL_Renderer* r,const char* s){
    return atlas().textScaled(r,s,MainLayout::VersionRight,
        MainLayout::ExitY,true,1.08,185,2);
}
static inline bool logo(SDL_Renderer* r,int cx=480,int top=0,int w=400,int h=300){
    const SDL_Rect target={cx-w/2,top,w,h};
    return atlas().logoInRect(r,target);
}
} // namespace XMGlyph
#endif
