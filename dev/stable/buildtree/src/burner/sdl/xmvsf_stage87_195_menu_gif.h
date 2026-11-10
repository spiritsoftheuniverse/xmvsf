#ifndef XMVSF_STAGE87_195_MENU_GIF_H
#define XMVSF_STAGE87_195_MENU_GIF_H
// Native XMVSF boot menu background: _art/menuanimation.gif.
// Keep its authentic GIF frame delays and cover-crop aspect ratio. Rotate
// the RGB hue independently over 36 seconds, retaining exact source alpha.
// The hue transform is produced on the CPU only when the displayed GIF frame
// changes (or at a throttled interval for old SDL2_image static fallback).
// Foreground logo, font glyphs and gameplay rendering are never tinted.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "xmvsf_menu_color_fx.h"
namespace XM87195Menu {
class Background {
    Background(const Background&)=delete;
    Background&operator=(const Background&)=delete;
    SDL_Renderer* renderer_=nullptr;
    SDL_Texture* texture_=nullptr;
    SDL_Surface* single_=nullptr;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    IMG_Animation* animation_=nullptr;
#endif
    int lastIndex_=-1;
    Uint32 start_=0,totalMs_=0,lastUpload_=0;
    std::vector<Uint32> ends_;
    std::vector<Uint8> originalPixels_;
    std::vector<Uint8> huePixels_;
    bool stream_=false;
    bool reported_=false;
    int width_=0,height_=0;
    static std::vector<std::string> paths(const char* file){
        std::vector<std::string> folders;
        folders.push_back("_art/");
        folders.push_back("../_art/");
        folders.push_back("../../_art/");
        char* base=SDL_GetBasePath();
        if(base){
            folders.push_back(std::string(base)+"../../_art/");
            folders.push_back(std::string(base)+"../_art/");
            folders.push_back(std::string(base)+"_art/");
            SDL_free(base);
        }
        std::vector<std::string> paths;
        for(const auto& folder:folders)paths.push_back(folder+file);
        return paths;
    }
    SDL_Surface* currentFrame(int index) const {
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_ && animation_->frames && index>=0 &&
           index<animation_->count)return animation_->frames[index];
#endif
        return single_;
    }
    void initTexture(SDL_Renderer* render,SDL_Surface* frame){
        if(texture_ || !render || !frame)return;
        texture_=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING,width_,height_);
        stream_=texture_!=nullptr;
        if(!texture_){
            // On older SDL GPU drivers preserve playback, just skip hue FX.
            texture_=SDL_CreateTextureFromSurface(render,frame);
            if(!texture_ && !reported_){
                std::fprintf(stderr,"XM MENU GIF texture failed: %s\n",
                             SDL_GetError());reported_=true;
            }
        }
        if(texture_){
            SDL_SetTextureBlendMode(texture_,SDL_BLENDMODE_BLEND);
            SDL_SetTextureColorMod(texture_,77,77,77); // 30% behind logo
        }
    }
public:
    Background()=default;
    ~Background(){release();}
    bool load(SDL_Renderer* render){
        release();
        renderer_=render;
        if(!render)return false;
        const char* name="menuanimation.gif";
        for(const auto& file:paths(name)){
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
            animation_=IMG_LoadAnimation(file.c_str());
            if(animation_ && animation_->count>0 && animation_->frames &&
               animation_->w>0 && animation_->h>0){
                width_=animation_->w;height_=animation_->h;
                ends_.clear();totalMs_=0;
                for(int i=0;i<animation_->count;i++){
                    const int raw=animation_->delays?animation_->delays[i]:100;
                    const Uint32 delay=(Uint32)std::max(20,raw);
                    if(totalMs_>60000000u-delay)break;
                    totalMs_+=delay;ends_.push_back(totalMs_);
                }
                if((int)ends_.size()==animation_->count && totalMs_>0){
                    start_=SDL_GetTicks();
                    std::fprintf(stderr,
                        "XM MENU GIF %s frames=%d size=%dx%d loop=%ums hue-cycle=36s bright=30%%\n",
                        file.c_str(),animation_->count,width_,height_,totalMs_);
                    return true;
                }
            }
            if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
#endif
            // Keep a single-frame image on SDL_image without animated GIF
            // support; do not substitute an unrelated menu GIF.
            single_=IMG_Load(file.c_str());
            if(single_){
                width_=single_->w;height_=single_->h;
                start_=SDL_GetTicks();totalMs_=100;ends_.push_back(100);
                std::fprintf(stderr,
                    "XM MENU GIF static fallback %s size=%dx%d hue-cycle=36s\n",
                    file.c_str(),width_,height_);
                return true;
            }
        }
        std::fprintf(stderr,
           "XM MENU GIF missing _art/%s: leave navy background, keep native menu usable\n",
           name);
        return false;
    }
    void draw(SDL_Renderer* render,Uint32 now,float introStrength=0.0f){
        if(!render||render!=renderer_||width_<=0||height_<=0)return;
        int index=0;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_ && totalMs_ && animation_->count>0){
            const Uint32 elapsed=(now-start_)%totalMs_;
            const auto found=std::upper_bound(ends_.begin(),ends_.end(),elapsed);
            index=found==ends_.end()?animation_->count-1:
                  int(found-ends_.begin());
        }
#endif
        SDL_Surface* frame=currentFrame(index);
        if(!frame)return;
        const bool changed=(index!=lastIndex_);
        if(!texture_ || (!stream_ && changed)){
            if(texture_){SDL_DestroyTexture(texture_);texture_=nullptr;}
            initTexture(render,frame);
        }
        if(!texture_)return;
        if(stream_ && (changed || originalPixels_.empty() ||
                       (Uint32)(now-lastUpload_)>=70u)){
            if(changed || originalPixels_.empty()){
                if(frame->w!=width_ || frame->h!=height_ ||
                   !XMMenuColor::rgbaPixels(frame,originalPixels_)){
                    if(!reported_){
                        std::fprintf(stderr,"XM MENU GIF RGBA conversion failed\n");
                        reported_=true;
                    }
                    return;
                }
            }
            XMMenuColor::backgroundHue(originalPixels_,huePixels_,now,introStrength,width_);
            if(SDL_UpdateTexture(texture_,nullptr,huePixels_.data(),
                                  width_*4)!=0){
                if(!reported_){
                    std::fprintf(stderr,"XM MENU GIF upload failed: %s\n",
                                 SDL_GetError());reported_=true;
                }
                return;
            }
            lastUpload_=now;
        }
        lastIndex_=index;
        // The first frame is white; Bayer-hue strobes and dims to the authored 30% backdrop
        // is restored at introStrength=0, with no persistent texture mutation.
        const float s=std::max(0.0f,std::min(1.0f,introStrength));
        const Uint8 brightness=static_cast<Uint8>(77+int(178.0f*s));
        SDL_SetTextureColorMod(texture_,brightness,brightness,brightness);
        // Original source aspect ratio, centered and cover-cropped in the
        // unchanged 960x540 logical canvas, with no animation slowdown.
        const double scale=std::max(960.0/double(width_),
                                    540.0/double(height_));
        const int w=int(width_*scale+0.5),h=int(height_*scale+0.5);
        SDL_Rect dst={(960-w)/2,(540-h)/2,w,h};
        SDL_RenderCopy(render,texture_,nullptr,&dst);
    }
    void release(){
        if(texture_){SDL_DestroyTexture(texture_);texture_=nullptr;}
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
#endif
        if(single_){SDL_FreeSurface(single_);single_=nullptr;}
        renderer_=nullptr;
        lastIndex_=-1;start_=totalMs_=lastUpload_=0;
        width_=height_=0;reported_=stream_=false;
        ends_.clear();originalPixels_.clear();huePixels_.clear();
    }
};
} // namespace XM87195Menu
#endif
