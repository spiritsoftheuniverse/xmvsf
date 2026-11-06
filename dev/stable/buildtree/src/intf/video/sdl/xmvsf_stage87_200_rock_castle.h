#ifndef XMVSF_STAGE87_200_ROCK_CASTLE_H
#define XMVSF_STAGE87_200_ROCK_CASTLE_H
// XM87.200: MVC1 Rock Castle, rendered from the ORIGINAL _art GIF.
// This is a presentation-only sibling of the existing Stage86.49 Wily painter.
// SDL_image >= 2.6 decodes the GIF frames; source timings are driven by the
// native game-frame clock (no GIF frame extraction or replacement art required).
// Native fighters/FX and the source game's flash stay above the full-color art.
#include <SDL.h>
#include <SDL_image.h>
#include "xmvsf_stage87_201_gif_camera.h"
#include "../../../burner/sdl/xmvsf_stage86_55_options.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace XM87200 {
class RockCastle {
    RockCastle(const RockCastle&)=delete;
    RockCastle& operator=(const RockCastle&)=delete;
    SDL_Renderer* renderer_=nullptr;
    SDL_Texture* image_=nullptr;
    SDL_Texture* fighters_=nullptr;
    SDL_Texture* flash_=nullptr;
    SDL_Surface* single_=nullptr;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    IMG_Animation* animation_=nullptr;
#endif
    std::vector<Uint32> ends_;
    Uint32 duration_=0;
    unsigned long long firstFrame_=0;
    unsigned generation_=0;
    int imageIndex_=-1;
    int width_=0, height_=0;
    bool attempted_=false;
    bool reportedFailure_=false;

    bool load(SDL_Renderer* r, unsigned generation, unsigned long long frame) {
        attempted_=true;
        renderer_=r;
        generation_=generation;
        firstFrame_=frame;
        std::vector<std::string> roots;
        std::string relative;
        for(int i=0;i<9;++i){
            roots.push_back(relative+"_art/");
            relative+="../";
        }
        char* base=SDL_GetBasePath();
        if(base){
            relative=base;
            for(int i=0;i<9;++i){
                roots.push_back(relative+"_art/");
                relative+="../";
            }
            SDL_free(base);
        }
        const char* filename="mvc1-rock-castle-stage.gif";
        for(const std::string& root:roots){
            const std::string path=root+filename;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
            animation_=IMG_LoadAnimation(path.c_str());
            if(animation_ && animation_->count>0 && animation_->frames &&
               animation_->w>0 && animation_->h>0 &&
               animation_->w<=8192 && animation_->h<=8192 &&
               animation_->count<=4096){
                width_=animation_->w;
                height_=animation_->h;
                duration_=0;
                ends_.clear();
                for(int i=0;i<animation_->count;++i){
                    int delay=animation_->delays?animation_->delays[i]:100;
                    // Zero-delay GIF frames must not lock the animation loop.
                    delay=std::max(20,std::min(60000,delay));
                    if(duration_>60000000u-static_cast<Uint32>(delay))delay=20;
                    duration_+=static_cast<Uint32>(delay);
                    ends_.push_back(duration_);
                }
            } else {
                if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
                continue;
            }
#else
            single_=IMG_Load(path.c_str());
            if(!single_ || single_->w<=0 || single_->h<=0 ||
               single_->w>8192 || single_->h>8192){
                if(single_){SDL_FreeSurface(single_);single_=nullptr;}
                continue;
            }
            width_=single_->w;
            height_=single_->h;
            duration_=100;
            ends_.push_back(duration_);
#endif
            fighters_=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,384,224);
            flash_=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,384,224);
            if(!fighters_ || !flash_){
                std::fprintf(stderr,"XM87.200 ROCK_CASTLE_OVERLAY_TEXTURE_FAILED %s\n",SDL_GetError());
                release();
                attempted_=true;
                return false;
            }
            SDL_SetTextureBlendMode(fighters_,SDL_BLENDMODE_BLEND);
            SDL_SetTextureBlendMode(flash_,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
            SDL_SetTextureScaleMode(fighters_,SDL_ScaleModeNearest);
            SDL_SetTextureScaleMode(flash_,SDL_ScaleModeNearest);
#endif
            std::fprintf(stderr,"XM87.200 ROCK_CASTLE_READY source=%s size=%dx%d frames=%d clock=game_frames color=original\n",
                path.c_str(),width_,height_,
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
                animation_->count
#else
                1
#endif
            );
            return true;
        }
        std::fprintf(stderr,"XM87.200 ROCK_CASTLE_MISSING source=_art/%s; native stage fallback\n",filename);
        return false;
    }

public:
    RockCastle()=default;
    ~RockCastle(){release();}
    void release(){
        if(image_){SDL_DestroyTexture(image_);image_=nullptr;}
        if(fighters_){SDL_DestroyTexture(fighters_);fighters_=nullptr;}
        if(flash_){SDL_DestroyTexture(flash_);flash_=nullptr;}
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
#endif
        if(single_){SDL_FreeSurface(single_);single_=nullptr;}
        ends_.clear();
        renderer_=nullptr;
        duration_=0;firstFrame_=0;generation_=0;
        imageIndex_=-1;width_=height_=0;
        attempted_=reportedFailure_=false;
    }

    // Caller has already verified the stage-05 visual gate and the native
    // 384x224 sprite-capture layer. Never write to CPS2 memory or alter combat.
    bool paint(SDL_Renderer* r, const SDL_Rect& view, int cameraX, int cameraY,
               unsigned long long gameFrame, unsigned generation,
               const UINT8* spritePixels, const UINT8* flashPixels) {
        if(!r || !spritePixels)return false;
        if(r!=renderer_ || generation!=generation_){
            release();
        }
        if(!attempted_ && !load(r,generation,gameFrame))return false;
        if(width_<=0 || height_<=0 || duration_==0)return false;
        const unsigned long long elapsedFrames=gameFrame>=firstFrame_?
                                               gameFrame-firstFrame_:0;
        // 60 Hz CPS2: integer division maintains deterministic replay clock.
        const Uint32 elapsed=static_cast<Uint32>(((elapsedFrames%3600000ULL)*1000ULL/60ULL)%duration_);
        const int index=static_cast<int>(std::upper_bound(ends_.begin(),ends_.end(),elapsed)-ends_.begin());
        if(index!=imageIndex_ || !image_){
            SDL_Surface* surface=single_;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
            if(animation_ && index<animation_->count)surface=animation_->frames[index];
#endif
            if(!surface)return false;
            SDL_Texture* next=SDL_CreateTextureFromSurface(r,surface);
            if(!next){
                if(!reportedFailure_){
                    std::fprintf(stderr,"XM87.200 ROCK_CASTLE_FRAME_TEXTURE_FAILED %s\n",SDL_GetError());
                    reportedFailure_=true;
                }
                return false;
            }
            SDL_SetTextureBlendMode(next,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
            SDL_SetTextureScaleMode(next,SDL_ScaleModeNearest);
#endif
            if(image_)SDL_DestroyTexture(image_);
            image_=next;
            imageIndex_=index;
        }
        if(SDL_UpdateTexture(fighters_,nullptr,spritePixels,384*4)!=0 ||
           (flashPixels && SDL_UpdateTexture(flash_,nullptr,flashPixels,384*4)!=0)){
            if(!reportedFailure_){
                std::fprintf(stderr,"XM87.200 ROCK_CASTLE_NATIVE_OVERLAY_FAILED %s\n",SDL_GetError());
                reportedFailure_=true;
            }
            return false;
        }
        // Stage86.29/86.49 native camera registration, now in BOTH axes.
        // Source GIF is ONE complete world picture, never a Wily tile manifest:
        // 384x224 native view + 381px X travel + 270px Y travel.
        const XM87201GifCamera::Rect crop=
            XM87201GifCamera::crop(width_,height_,cameraX,cameraY);
        if(crop.w<1 || crop.h<1)return false;
        SDL_Rect src={crop.x,crop.y,crop.w,crop.h};
        // Wily-compatible DF: dim only the stage art (30%), preserving
        // untouched native fighters/HUD/FX. No expensive screen readback.
        const Uint8 backgroundLevel=xm8655DoubleFeature()?77:255;
        SDL_SetTextureColorMod(image_,backgroundLevel,backgroundLevel,backgroundLevel);
        if(gameFrame%300u==0u)
            std::fprintf(stderr,
                "XM87.201 GIF_CAMERA native=%d,%d world=%d,%d crop=%d,%d,%d,%d source=%dx%d df=%d\\n",
                cameraX,cameraY,XM87201GifCamera::worldCameraX(cameraX),cameraY,
                crop.x,crop.y,crop.w,crop.h,width_,height_,int(xm8655DoubleFeature()));
        if(SDL_RenderCopy(r,image_,&src,&view)!=0)return false;
        // Identical depth order to the existing Wily painter: custom stage,
        // dizzy underlay, native fighter/FX sprite extraction, native flashes.
        xm8733Overlay(r,view);
        if(SDL_RenderCopy(r,fighters_,nullptr,&view)!=0)return false;
        if(flashPixels && SDL_RenderCopy(r,flash_,nullptr,&view)!=0)return false;
        return true;
    }
};
inline RockCastle& state(){static RockCastle instance;return instance;}
} // namespace XM87200
#endif
