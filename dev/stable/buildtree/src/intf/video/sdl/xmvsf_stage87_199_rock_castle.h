#ifndef XMVSF_STAGE87_199_ROCK_CASTLE_H
#define XMVSF_STAGE87_199_ROCK_CASTLE_H
// 87.199 first-pass Rock Castle: plug into the existing Wily/native
// 384x224 stage compositor. Native ROM stage05, camera, fighters,
// HUD, foreground sprites, collisions, audio and FX are unchanged.
// The artwork is a full-color animated GIF, not a simulated CPS2
// palette/tile bank. Preserve original delays; static first-frame
// fallback when SDL_image predates animation support.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
namespace XM87199Rock {
struct Renderer {
    SDL_Renderer*owner=nullptr;
    SDL_Texture*image=nullptr;
    SDL_Texture*fighters=nullptr;
    SDL_Texture*flash=nullptr;
    SDL_Surface*still=nullptr;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    IMG_Animation*anim=nullptr;
#endif
    unsigned long long start=0;
    unsigned frameIndex=~0u;
    unsigned frameCount=0u,totalMs=0u;
    int imageW=0,imageH=0;
    std::vector<unsigned> ends;
    bool missing=false,reported=false;
    void reset(){
        if(image)SDL_DestroyTexture(image);
        if(fighters)SDL_DestroyTexture(fighters);
        if(flash)SDL_DestroyTexture(flash);
        if(still)SDL_FreeSurface(still);
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(anim)IMG_FreeAnimation(anim);
        anim=nullptr;
#endif
        image=fighters=flash=nullptr;still=nullptr;
        owner=nullptr;start=0u;frameIndex=~0u;
        frameCount=totalMs=0u;imageW=imageH=0;
        ends.clear();missing=reported=false;
    }
    ~Renderer(){reset();}
    bool load(SDL_Renderer*render,unsigned long long nativeFrame){
        reset(); owner=render; start=nativeFrame;
        // A game executable lives under runtime/stable. Resolve the
        // user's ORIGINAL local art without embedding ROM/GIF bytes or
        // assuming the process was started from the repo directory.
        std::vector<std::string> candidates;
        const char* names[]={
            "mvc1-rock-castle-stage.gif","rock-castle.gif"
        };
        char*base=SDL_GetBasePath();
        std::vector<std::string> dirs={"_art/","../_art/","../../_art/",
            "assets/xmvsf/rockcastle/","F:/xmvsf/_art/"};
        if(base){
            std::string b(base);SDL_free(base);
            dirs.push_back(b+"../../_art/");
            dirs.push_back(b+"../_art/");
            dirs.push_back(b+"assets/xmvsf/rockcastle/");
        }
        for(const auto&dir:dirs){
            for(const char*name:names){
                const std::string file=dir+name;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
                anim=IMG_LoadAnimation(file.c_str());
                if(anim && anim->count>0 && anim->frames &&
                   anim->w>0 && anim->h>0){
                    imageW=anim->w;imageH=anim->h;
                    frameCount=(unsigned)anim->count;
                    for(unsigned n=0u;n<frameCount;++n){
                        int delay=anim->delays?anim->delays[n]:100;
                        if(delay<=0)delay=100;
                        delay=std::max(20,std::min(10000,delay));
                        totalMs+=(unsigned)delay;
                        ends.push_back(totalMs);
                    }
                }else if(anim){IMG_FreeAnimation(anim);anim=nullptr;}
#endif
                if(!frameCount){
                    still=IMG_Load(file.c_str());
                    if(still && still->w>0 && still->h>0){
                        imageW=still->w;imageH=still->h;
                        frameCount=1u;totalMs=100u;
                        ends.push_back(totalMs);
                    }else if(still){SDL_FreeSurface(still);still=nullptr;}
                }
                if(frameCount){
                    fighters=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
                         SDL_TEXTUREACCESS_STREAMING,384,224);
                    flash=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
                         SDL_TEXTUREACCESS_STREAMING,384,224);
                    if(!fighters||!flash){reset();return false;}
                    SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
                    SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
                    std::fprintf(stderr,
                        "XM87.199 ROCK_CASTLE artwork=%s size=%dx%d frames=%u nativeCamera=1 color=RGBA\n",
                        file.c_str(),imageW,imageH,frameCount);
                    return true;
                }
            }
        }
        missing=true;
        std::fprintf(stderr,
            "XM87.199 ROCK_CASTLE asset absent: place original mvc1-rock-castle-stage.gif in F:/xmvsf/_art; native ROM background retained\n");
        return false;
    }
    bool refreshFrame(SDL_Renderer*render,unsigned long long nativeFrame){
        if(!frameCount||!totalMs)return false;
        // Native emulation clock, not SDL wall time: pause, replay FFWD
        // and return-to-menu don't desynchronize the original GIF loop.
        const unsigned long long elapsed=
            nativeFrame>=start?(nativeFrame-start)*1000ull/60ull:0ull;
        const unsigned ms=(unsigned)(elapsed%totalMs);
        const auto it=std::upper_bound(ends.begin(),ends.end(),ms);
        unsigned idx=(unsigned)(it-ends.begin());
        if(idx>=frameCount)idx=frameCount-1;
        if(image && frameIndex==idx)return true;
        SDL_Surface*surface=still;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(anim)surface=anim->frames[idx];
#endif
        if(!surface)return false;
        SDL_Texture*newTexture=SDL_CreateTextureFromSurface(render,surface);
        if(!newTexture)return false;
        SDL_SetTextureBlendMode(newTexture,SDL_BLENDMODE_NONE);
        if(image)SDL_DestroyTexture(image);
        image=newTexture;frameIndex=idx;
        return true;
    }
    bool paint(SDL_Renderer*render,const SDL_Rect&view,
               unsigned long long nativeFrame,
               const unsigned char*spritePixels,const unsigned char*flashPixels){
        if(!render || !spritePixels)return false;
        if(owner!=render)reset();
        if(!owner && !load(render,nativeFrame))return false;
        if(missing || !refreshFrame(render,nativeFrame))return false;
        // Keep the artwork's aspect ratio. At 636x479 the native
        // 384x224 viewport samples a 636x371 area, bottom anchored
        // to retain the actual stage floor and characters' grounding.
        const int cropH=std::min(imageH,
                       std::max(1,int((long long)imageW*224/384)));
        const int cropW=std::min(imageW,
                       std::max(1,int((long long)imageH*384/224)));
        SDL_Rect crop;
        if(cropH<imageH){crop={0,imageH-cropH,imageW,cropH};}
        else{crop={(imageW-cropW)/2,0,cropW,imageH};}
        if(SDL_RenderCopy(render,image,&crop,&view)!=0)return false;
        if(SDL_UpdateTexture(fighters,nullptr,spritePixels,384*4)!=0)return false;
        // Same ordering as Wily: stage RGBA then original native HUD
        // then original filtered CPS2 actors/effects, all full color.
        xm8733Overlay(render,view);
        if(SDL_RenderCopy(render,fighters,nullptr,&view)!=0)return false;
        if(flashPixels){
            if(SDL_UpdateTexture(flash,nullptr,flashPixels,384*4)!=0 ||
               SDL_RenderCopy(render,flash,nullptr,&view)!=0)return false;
        }
        if(!reported){
            std::fprintf(stderr,
                "XM87.199 ROCK_CASTLE presented original_RGBA=1 native_fighters=1 camera_stage05=1 GIF_frames=%u\n",
                frameCount);
            reported=true;
        }
        return true;
    }
};
inline Renderer&state(){static Renderer r;return r;}
} // XM87199Rock
#endif
