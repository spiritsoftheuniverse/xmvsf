#ifndef XMVSF_STAGE87_400_NATIVE_SCREENSHOT_H
#define XMVSF_STAGE87_400_NATIVE_SCREENSHOT_H
// TEST 87.400: one-shot final SDL game-viewport PNG at 384x224.
// Read the finished renderer before Present: includes custom stages,
// native HUD, independent SDL stun and other composited game overlays.
// Never invoke the stage painter twice, touch CPS ROM/RAM or mutate timing.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
namespace XM87400 {
static constexpr int NativeW=384, NativeH=224;
struct Coordinate {
    double originX=0.0,originY=0.0,scale=1.0;
    bool valid=false;
};
inline Coordinate map(int outputW,int outputH,int logicalW,int logicalH){
    Coordinate result;
    if(outputW<=0||outputH<=0)return result;
    if(logicalW<=0||logicalH<=0){
        logicalW=outputW;logicalH=outputH;
    }
    result.scale=std::min(double(outputW)/logicalW,double(outputH)/logicalH);
    if(result.scale<=0.0)return result;
    result.originX=(double(outputW)-double(logicalW)*result.scale)*0.5;
    result.originY=(double(outputH)-double(logicalH)*result.scale)*0.5;
    result.valid=true;return result;
}
inline int sample(double physical,int bound){
    if(bound<=0)return 0;
    // Center of a source pixel. This resamples the COMPLETED renderer
    // rather than capturing the window frame, border or side pane.
    const int value=int(std::floor(physical));
    return value<0?0:(value>=bound?bound-1:value);
}
inline void extract(const unsigned char* physical,int outputW,int outputH,
                    const Coordinate& mapping,const SDL_Rect& gameView,
                    unsigned char* native){
    if(!physical||!native||!mapping.valid)return;
    for(int y=0;y<NativeH;++y){
        const double logicalY=gameView.y+(double(y)+0.5)*gameView.h/NativeH;
        const int sy=sample(mapping.originY+logicalY*mapping.scale,outputH);
        for(int x=0;x<NativeW;++x){
            const double logicalX=gameView.x+(double(x)+0.5)*gameView.w/NativeW;
            const int sx=sample(mapping.originX+logicalX*mapping.scale,outputW);
            const unsigned char* src=physical+
                (std::size_t(sy)*unsigned(outputW)+unsigned(sx))*4u;
            unsigned char* dst=native+
                (std::size_t(y)*NativeW+unsigned(x))*4u;
            std::memcpy(dst,src,4u);
            dst[3]=255u;
        }
    }
}
inline bool save(SDL_Renderer* renderer,const SDL_Rect& gameView,
                 unsigned frame,char* savedPath,std::size_t pathCapacity){
    if(savedPath && pathCapacity)savedPath[0]=0;
    if(!renderer || gameView.w<=0 || gameView.h<=0)return false;
    // Final painter always targets the window. Never disturb custom-stage
    // texture targets if that invariant is broken.
    if(SDL_GetRenderTarget(renderer)){
        std::fprintf(stderr,"XM87.400 F6_PNG_ERROR renderer still has texture target\n");
        return false;
    }
    int outputW=0,outputH=0,logicalW=0,logicalH=0;
    SDL_GetRendererOutputSize(renderer,&outputW,&outputH);
    SDL_RenderGetLogicalSize(renderer,&logicalW,&logicalH);
    const Coordinate mapping=map(outputW,outputH,logicalW,logicalH);
    if(!mapping.valid || outputW>8192 || outputH>8192 ||
       std::size_t(outputW)*unsigned(outputH)>16000000u){
        std::fprintf(stderr,"XM87.400 F6_PNG_ERROR invalid output %dx%d\n",
                     outputW,outputH);
        return false;
    }
    SDL_Rect viewport={0,0,0,0},clip={0,0,0,0};
    SDL_RenderGetViewport(renderer,&viewport);
    const bool clipped=SDL_RenderIsClipEnabled(renderer)==SDL_TRUE;
    if(clipped)SDL_RenderGetClipRect(renderer,&clip);
    SDL_BlendMode blend=SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer,&blend);
    std::vector<unsigned char> framePixels(
        std::size_t(outputW)*unsigned(outputH)*4u,0u);
    // Read the full PHYSICAL backbuffer. Reading while 960x540 logical
    // letterboxing is active can offset the captured image.
    SDL_RenderSetLogicalSize(renderer,0,0);
    SDL_RenderSetViewport(renderer,nullptr);
    SDL_RenderSetClipRect(renderer,nullptr);
    const int rc=SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_RGBA32,
                                     framePixels.data(),outputW*4);
    // Restore render state before any file I/O or error return. The
    // screenshot operation must not reposition the following game frame.
    if(logicalW>0 && logicalH>0)
        SDL_RenderSetLogicalSize(renderer,logicalW,logicalH);
    else{
        SDL_RenderSetLogicalSize(renderer,0,0);
        SDL_RenderSetViewport(renderer,&viewport);
    }
    SDL_RenderSetClipRect(renderer,clipped?&clip:nullptr);
    SDL_SetRenderDrawBlendMode(renderer,blend);
    if(rc!=0){
        std::fprintf(stderr,"XM87.400 F6_PNG_ERROR readback %s\n",SDL_GetError());
        return false;
    }

    std::vector<unsigned char> native(std::size_t(NativeW)*NativeH*4u,255u);
    extract(framePixels.data(),outputW,outputH,mapping,gameView,native.data());
    SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormatFrom(
        native.data(),NativeW,NativeH,32,NativeW*4,SDL_PIXELFORMAT_RGBA32);
    if(!surface){
        std::fprintf(stderr,"XM87.400 F6_PNG_ERROR surface %s\n",SDL_GetError());
        return false;
    }
    const char* folder=SDL_getenv("XMVSF_SCREENSHOT_DIR");
    if(!folder || !*folder){
        folder="screenshots";
#ifdef _WIN32
        _mkdir(folder);
#else
        mkdir(folder,0755);
#endif
    }
    std::time_t now=std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local,&now);
#else
    localtime_r(&now,&local);
#endif
    char stamp[32]={};
    std::strftime(stamp,sizeof(stamp),"%Y%m%d_%H%M%S",&local);
    static unsigned unique=0u;
    char file[2048]={};
    bool goodName=false;
    // Never replace a prior capture, including a screenshot from a previous
    // TEST session that coincidentally used the same timestamp/frame number.
    for(unsigned attempt=0u;attempt<10000u;++attempt){
        const unsigned serial=++unique;
        const int used=std::snprintf(file,sizeof(file),
            "%s%cXMVSF_native_%s_f%09u_%06u.png",folder,
#ifdef _WIN32
            '\\',
#else
            '/',
#endif
            stamp,frame,serial);
        if(used<=0 || std::size_t(used)>=sizeof(file))break;
        std::FILE* previous=std::fopen(file,"rb");
        if(previous){std::fclose(previous);continue;}
        goodName=true;break;
    }
    const int saved=goodName?IMG_SavePNG(surface,file):-1;
    SDL_FreeSurface(surface);
    if(saved!=0){
        std::fprintf(stderr,"XM87.400 F6_PNG_ERROR save path=%s reason=%s\n",
                     goodName?file:"(too long)",IMG_GetError());
        return false;
    }
    if(savedPath && pathCapacity){
        std::snprintf(savedPath,pathCapacity,"%s",file);
    }
    std::fprintf(stderr,
        "XM87.400 F6_NATIVE_PNG_SAVED path=%s size=384x224 "
        "game_frame=%u output=%dx%d logical=%dx%d view=%d,%d,%d,%d\n",
        file,frame,outputW,outputH,logicalW,logicalH,
        gameView.x,gameView.y,gameView.w,gameView.h);
    return true;
}
} // namespace XM87400
#endif
