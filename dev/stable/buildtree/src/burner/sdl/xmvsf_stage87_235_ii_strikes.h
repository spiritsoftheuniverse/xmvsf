#ifndef XMVSF_STAGE87_235_II_STRIKES_H
#define XMVSF_STAGE87_235_II_STRIKES_H
// Draw the user-exported, perfectly aligned 400x300 II strokes in A/B/C/D
// order. Each stroke is revealed along its own SOURCE-pixel direction for
// exactly ten 60Hz frames. Particle spray follows the actual visible tip.
// No screen-space rectangle wipes, no image re-scaling and no new audio path.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "xmvsf_menu_color_fx.h"
#include "xmvsf_menu_glyph_atlas.h"
#include "xmvsf_stage87_235_menu_intro_timeline.h"

namespace XM87235II {
static constexpr int CanvasWidth=400;
static constexpr int CanvasHeight=300;
static constexpr int FramesPerStroke=10;
static const int Bayer4[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};

struct Stroke {
    SDL_Texture* texture=nullptr;
    std::vector<Uint8> pixels,masked;
    SDL_Rect bounds={0,0,0,0};
    SDL_Point tip[FramesPerStroke]={};
    int dx=0,dy=0,minDot=0,maxDot=0,lastStep=-1;
    bool valid=false;
    void release(){
        if(texture)SDL_DestroyTexture(texture);
        texture=nullptr;pixels.clear();masked.clear();
        bounds={0,0,0,0};minDot=maxDot=lastStep=0;
        valid=false;
    }
};

class Painter {
    SDL_Renderer* renderer_=nullptr;
    Stroke parts_[4];
    SDL_Texture* final_=nullptr;
    SDL_Rect finalBounds_={0,0,0,0};
    bool attempted_=false;

    static SDL_Surface* open(const char* name){
        std::vector<std::string> dirs={"_art/","../_art/","../../_art/"};
        char* base=SDL_GetBasePath();
        if(base){
            const std::string prefix(base);
            dirs.push_back(prefix+"../../_art/");
            dirs.push_back(prefix+"../_art/");
            dirs.push_back(prefix+"_art/");
            SDL_free(base);
        }
        for(const std::string& dir:dirs){
            SDL_Surface* surface=IMG_Load((dir+name).c_str());
            if(surface)return surface;
        }
        std::fprintf(stderr,"XM87.235 II_STROKE_MISSING _art/%s\n",name);
        return nullptr;
    }
    static bool white(const Uint8* p){
        return p[3]>=240&&p[0]>=248&&p[1]>=248&&p[2]>=248;
    }
    // Some Photoshop exports have a white, opaque surrounding canvas.
    // Clear only white connected to the SOURCE IMAGE BORDER: preserve
    // all interior white highlights, black edges and stroke coloring.
    static void normalizeBorder(std::vector<Uint8>& rgba){
        if(rgba.size()!=size_t(CanvasWidth*CanvasHeight*4))return;
        if(!white(rgba.data()))return;
        std::vector<Uint8> visited(CanvasWidth*CanvasHeight,0);
        std::vector<int> queue;
        queue.reserve(CanvasWidth*CanvasHeight/2);
        const auto add=[&](int x,int y){
            if(x<0||x>=CanvasWidth||y<0||y>=CanvasHeight)return;
            const int at=y*CanvasWidth+x;
            if(visited[at]||!white(&rgba[size_t(at)*4]))return;
            visited[at]=1;queue.push_back(at);
        };
        for(int x=0;x<CanvasWidth;++x){add(x,0);add(x,CanvasHeight-1);}
        for(int y=0;y<CanvasHeight;++y){add(0,y);add(CanvasWidth-1,y);}
        for(size_t i=0;i<queue.size();++i){
            const int at=queue[i],x=at%CanvasWidth,y=at/CanvasWidth;
            rgba[size_t(at)*4+3]=0;
            add(x-1,y);add(x+1,y);add(x,y-1);add(x,y+1);
        }
    }
    static SDL_Rect occupied(const std::vector<Uint8>& pixels){
        int left=CanvasWidth,top=CanvasHeight,right=-1,bottom=-1;
        for(int y=0;y<CanvasHeight;++y)for(int x=0;x<CanvasWidth;++x){
            if(pixels[(size_t(y)*CanvasWidth+x)*4+3]<=12)continue;
            left=std::min(left,x);top=std::min(top,y);
            right=std::max(right,x);bottom=std::max(bottom,y);
        }
        if(right<left||bottom<top)return {0,0,0,0};
        return {left,top,right-left+1,bottom-top+1};
    }
    static bool source(const char* filename,std::vector<Uint8>& pixels,
                       SDL_Rect& bounds){
        SDL_Surface* raw=open(filename);
        if(!raw)return false;
        const bool same=raw->w==CanvasWidth&&raw->h==CanvasHeight;
        const bool ok=same&&XMMenuColor::rgbaPixels(raw,pixels);
        SDL_FreeSurface(raw);
        if(!ok){
            std::fprintf(stderr,"XM87.235 II_CANVAS_INVALID %s expected=%dx%d\n",
                         filename,CanvasWidth,CanvasHeight);
            return false;
        }
        normalizeBorder(pixels);
        bounds=occupied(pixels);
        return bounds.w>0&&bounds.h>0;
    }
    static SDL_Texture* upload(SDL_Renderer* render,
                               const std::vector<Uint8>& pixels){
        SDL_Texture* tex=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          CanvasWidth,CanvasHeight);
        if(!tex)return nullptr;
        SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
        if(SDL_UpdateTexture(tex,nullptr,pixels.data(),CanvasWidth*4)!=0){
            SDL_DestroyTexture(tex);return nullptr;
        }
        return tex;
    }
    static int dot(const Stroke& p,int x,int y){
        return x*p.dx+y*p.dy;
    }
    // The exports include dark shadows/backing around their neon strokes.
    // Follow the colored/bright ink for projection and tip particles, not
    // the rectangular dark matte; the COMPLETE RGBA is still revealed.
    static bool coloredInk(const Stroke& p,int x,int y){
        const size_t n=(size_t(y)*CanvasWidth+x)*4u;
        if(p.pixels[n+3]<=12)return false;
        const Uint8 r=p.pixels[n],g=p.pixels[n+1],b=p.pixels[n+2];
        const Uint8 hi=std::max(r,std::max(g,b));
        const Uint8 lo=std::min(r,std::min(g,b));
        return hi>=90u&&(hi-lo>=25u||hi>=190u);
    }
    static float cut(const Stroke& p,int step){
        return float(p.minDot)+float(p.maxDot-p.minDot)*float(step)/FramesPerStroke;
    }
    static void calculateTips(Stroke& p){
        p.minDot=2147483647;p.maxDot=-2147483647;
        bool hasColor=false;
        for(int y=p.bounds.y;y<p.bounds.y+p.bounds.h;++y)
        for(int x=p.bounds.x;x<p.bounds.x+p.bounds.w;++x)
            if(coloredInk(p,x,y))hasColor=true;
        const auto tipMaterial=[&](int x,int y){
            return hasColor?coloredInk(p,x,y):
                p.pixels[(size_t(y)*CanvasWidth+x)*4+3]>12;
        };
        for(int y=p.bounds.y;y<p.bounds.y+p.bounds.h;++y)
        for(int x=p.bounds.x;x<p.bounds.x+p.bounds.w;++x){
            if(!tipMaterial(x,y))continue;
            const int n=dot(p,x,y);
            p.minDot=std::min(p.minDot,n);
            p.maxDot=std::max(p.maxDot,n);
        }
        // A projection's leading boundary can touch several mask pixels.
        // Use their center, not a guessed rectangular corner, as the spark tip.
        for(int frame=1;frame<=FramesPerStroke;++frame){
            const float leading=cut(p,frame);
            float closest=1e9f;
            for(int y=p.bounds.y;y<p.bounds.y+p.bounds.h;++y)
            for(int x=p.bounds.x;x<p.bounds.x+p.bounds.w;++x){
                if(!tipMaterial(x,y))continue;
                closest=std::min(closest,std::fabs(float(dot(p,x,y))-leading));
            }
            float px=0.f,py=0.f;int count=0;
            for(int y=p.bounds.y;y<p.bounds.y+p.bounds.h;++y)
            for(int x=p.bounds.x;x<p.bounds.x+p.bounds.w;++x){
                if(!tipMaterial(x,y))continue;
                if(std::fabs(float(dot(p,x,y))-leading)>closest+1.f)continue;
                px+=x;py+=y;++count;
            }
            p.tip[frame-1]={count?int(px/count+0.5f):p.bounds.x,
                            count?int(py/count+0.5f):p.bounds.y};
        }
    }
    void loadStroke(int i,const char* name,int dx,int dy){
        Stroke& p=parts_[i];p.dx=dx;p.dy=dy;
        if(!source(name,p.pixels,p.bounds))return;
        p.masked=p.pixels;
        calculateTips(p);
        p.texture=upload(renderer_,p.pixels);
        p.valid=p.texture!=nullptr;
        p.lastStep=FramesPerStroke;
        std::fprintf(stderr,
            "XM87.235 II_STROKE_%c loaded=%d bounds=%d,%d,%d,%d direction=%d,%d dot=%d..%d\n",
            char('A'+i),int(p.valid),p.bounds.x,p.bounds.y,p.bounds.w,p.bounds.h,
            dx,dy,p.minDot,p.maxDot);
    }
    void mask(Stroke& p,int step){
        step=std::max(0,std::min(FramesPerStroke,step));
        if(!p.valid||step==p.lastStep)return;
        if(step==FramesPerStroke){
            p.masked=p.pixels;
        }else{
            const float threshold=cut(p,step);
            p.masked=p.pixels;
            for(int y=p.bounds.y;y<p.bounds.y+p.bounds.h;++y)
            for(int x=p.bounds.x;x<p.bounds.x+p.bounds.w;++x){
                const size_t n=(size_t(y)*CanvasWidth+x)*4u+3u;
                if(!p.masked[n])continue;
                const float distance=threshold-float(dot(p,x,y));
                // Two source pixels of ordered Bayer edge breakup. Outside
                // the leading tip no pixels can leak from future frames.
                const float dither=float(Bayer4[(y&3)*4+(x&3)])/16.0f;
                if(distance<0.f||(distance<2.f&&distance<dither*2.f))
                    p.masked[n]=0;
            }
        }
        if(SDL_UpdateTexture(p.texture,nullptr,p.masked.data(),CanvasWidth*4)==0)
            p.lastStep=step;
    }
    static SDL_Rect place(const SDL_Rect& canvas,const SDL_Rect& bounds){
        return {canvas.x+int(double(bounds.x)*canvas.w/CanvasWidth+0.5),
                canvas.y+int(double(bounds.y)*canvas.h/CanvasHeight+0.5),
                std::max(1,int(double(bounds.w)*canvas.w/CanvasWidth+0.5)),
                std::max(1,int(double(bounds.h)*canvas.h/CanvasHeight+0.5))};
    }
    static unsigned hash(unsigned value){
        value^=value>>16;value*=0x7feb352du;
        value^=value>>15;value*=0x846ca68bu;value^=value>>16;
        return value;
    }
    void particles(SDL_Renderer* render,const SDL_Rect& canvas,
                   std::uint32_t elapsed)const{
        if(elapsed<XM87235Intro::StrikeStart)return;
        const std::uint32_t globalFrame=
            ((elapsed-XM87235Intro::StrikeStart)*XM87235Intro::StrikeFps)/1000u;
        if(!globalFrame||globalFrame>XM87235Intro::StrikeFrames+13u)return;
        SDL_BlendMode oldBlend=SDL_BLENDMODE_NONE;
        SDL_GetRenderDrawBlendMode(render,&oldBlend);
        SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_BLEND);
        Uint8 oldR=0,oldG=0,oldB=0,oldA=255;
        SDL_GetRenderDrawColor(render,&oldR,&oldG,&oldB,&oldA);
        const Uint8 palette[5][3]={
            {255,255,255},{120,255,80},{60,242,255},
            {255,70,242},{255,218,90}
        };
        const unsigned end=std::min(globalFrame,XM87235Intro::StrikeFrames);
        const unsigned begin=globalFrame>12u?globalFrame-12u:1u;
        for(unsigned birth=begin;birth<=end;++birth){
            const int part=int((birth-1u)/FramesPerStroke);
            const int step=int((birth-1u)%FramesPerStroke);
            const Stroke& stroke=parts_[part];
            if(!stroke.valid)continue;
            const int age=int(globalFrame-birth);
            const SDL_Point tip=stroke.tip[step];
            const float length=std::sqrt(float(stroke.dx*stroke.dx+stroke.dy*stroke.dy));
            const float forwardX=float(stroke.dx)/length;
            const float forwardY=float(stroke.dy)/length;
            const float sideX=-forwardY,sideY=forwardX;
            for(unsigned k=0;k<7u;++k){
                const unsigned r=hash((birth+11u)*0x9e3779b9u+k*131u);
                const float spread=float(int((r>>9)&15u)-7)/4.0f;
                const float speed=0.55f+float((r>>17)&7u)/5.0f;
                const float outward=float(age)*speed;
                const float px=float(tip.x)+forwardX*outward+
                               sideX*spread*(1.0f+float(age)*0.8f);
                const float py=float(tip.y)+forwardY*outward+
                               sideY*spread*(1.0f+float(age)*0.8f);
                const int x=canvas.x+int(px*canvas.w/CanvasWidth+0.5f);
                const int y=canvas.y+int(py*canvas.h/CanvasHeight+0.5f);
                const Uint8* rgb=palette[(r>>25)%5u];
                const Uint8 opacity=Uint8(std::max(0,255-age*23));
                SDL_SetRenderDrawColor(render,rgb[0],rgb[1],rgb[2],opacity);
                const int sz=(k&3u)==0u?3:2;
                const SDL_Rect spark={x,y,sz,sz};
                SDL_RenderFillRect(render,&spark);
            }
        }
        SDL_SetRenderDrawColor(render,oldR,oldG,oldB,oldA);
        SDL_SetRenderDrawBlendMode(render,oldBlend);
    }
public:
    Painter()=default;
    Painter(const Painter&)=delete;
    Painter&operator=(const Painter&)=delete;
    ~Painter(){release();}
    void release(){
        for(Stroke& p:parts_)p.release();
        if(final_)SDL_DestroyTexture(final_);
        final_=nullptr;finalBounds_={0,0,0,0};
        renderer_=nullptr;attempted_=false;
    }
    bool load(SDL_Renderer* render){
        if(attempted_ && render==renderer_)return final_!=nullptr;
        release();renderer_=render;attempted_=true;
        if(!render)return false;
        std::vector<Uint8> full;
        if(source("logo_II.png",full,finalBounds_))
            final_=upload(renderer_,full);
        // A: upright (up/right). B: up. C: down. D: up/right.
        loadStroke(0,"logo_II_A.png",1,-1);
        loadStroke(1,"logo_II_B.png",0,-1);
        loadStroke(2,"logo_II_C.png",0,1);
        loadStroke(3,"logo_II_D.png",1,-1);
        XMGlyph::setLogoIIAlphaBounds(render,final_?finalBounds_:SDL_Rect{0,0,0,0});
        std::fprintf(stderr,"XM87.235 II_FINAL loaded=%d bounds=%d,%d,%d,%d\n",
                     final_!=nullptr,finalBounds_.x,finalBounds_.y,
                     finalBounds_.w,finalBounds_.h);
        return final_!=nullptr;
    }
    void draw(SDL_Renderer* render,std::uint32_t elapsed,bool cinematic){
        if(!renderer_||renderer_!=render)return;
        SDL_Rect canvas{};
        if(!XMGlyph::menuLogoFullCanvas(render,canvas))return;
        if(!cinematic || elapsed>=XM87235Intro::StrikeStart+
                                  XM87235Intro::StrikeDurationMs){
            if(final_){
                const SDL_Rect placed=place(canvas,finalBounds_);
                SDL_RenderCopy(render,final_,&finalBounds_,&placed);
            }else for(Stroke& p:parts_)if(p.valid){
                mask(p,FramesPerStroke);
                const SDL_Rect placed=place(canvas,p.bounds);
                SDL_RenderCopy(render,p.texture,&p.bounds,&placed);
            }
        }else if(elapsed>=XM87235Intro::StrikeStart){
            const unsigned drawn=XM87235Intro::strokeFrame(elapsed);
            for(int i=0;i<4;i++){
                const int step=std::max(0,std::min(FramesPerStroke,
                                         int(drawn)-i*FramesPerStroke));
                Stroke& p=parts_[i];
                if(!step||!p.valid)continue;
                mask(p,step);
                const SDL_Rect placed=place(canvas,p.bounds);
                SDL_RenderCopy(render,p.texture,&p.bounds,&placed);
            }
        }
        if(cinematic)particles(render,canvas,elapsed);
    }
};
} // namespace XM87235II
#endif
