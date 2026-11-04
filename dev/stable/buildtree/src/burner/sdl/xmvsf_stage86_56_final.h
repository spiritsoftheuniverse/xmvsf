#ifndef XMVSF_STAGE86_56_FINAL_H
#define XMVSF_STAGE86_56_FINAL_H
// XM86.56 global presentation LUT. SDL2 renderer backend independent (D3D/OpenGL/software).
// Present-time readback is used deliberately: every rendered layer is included.
// Never post-process individual emulation sprite buffers; no gameplay/ROM changes.
#include <SDL.h>
#include <vector>
#include <cstdint>
#include <cstdio>
#include "xmvsf_stage86_55_options.h"
struct XM8656FrameState {
    SDL_Renderer* renderer=nullptr;
    SDL_Texture* texture=nullptr;
    int width=0,height=0;
    std::vector<Uint32> pixels;
    bool warned=false;
    unsigned frames=0;
};
// Single resource state for the application, shared across all SDL2 translation units.
// Video exit releases it before SDL renderer destruction.
inline XM8656FrameState& xm8656State(){static XM8656FrameState state;return state;}
static inline void xm8656Cleanup(SDL_Renderer* renderer=nullptr){
    auto& s=xm8656State();
    if(renderer && s.renderer!=renderer)return;
    if(s.texture){SDL_DestroyTexture(s.texture);s.texture=nullptr;}
    s.pixels.clear();s.renderer=nullptr;s.width=s.height=0;s.warned=false;s.frames=0;
}
// Pure function, independently testable using synthetic frame data.
static inline void xm8656Grayscale(Uint32* pixels,size_t count){
    for(size_t i=0;i<count;++i){
        const Uint32 px=pixels[i];
        const unsigned r=(px>>16)&255u,g=(px>>8)&255u,b=px&255u;
        const Uint32 gray=(77u*r+150u*g+29u*b+128u)>>8;
        pixels[i]=(px&0xff000000u)|(gray<<16)|(gray<<8)|gray;
    }
}
// XM86.62: the character-select phase uses the SAME threshold-96 ink
// transfer as gameplay fighter palettes. Other frontend menus retain
// their accepted neutral grayscale; gameplay retains HUD/FX colors.
static inline void xm8662SelectionInk(Uint32* pixels,size_t count){
    for(size_t i=0;i<count;++i){
        const Uint32 px=pixels[i];
        const unsigned r=(px>>16)&255u,g=(px>>8)&255u,b=px&255u;
        const unsigned y=(77u*r+150u*g+29u*b+128u)>>8;
        const unsigned ink=y<=96u?0u:y;
        pixels[i]=(px&0xff000000u)|(ink<<16)|(ink<<8)|ink;
    }
}
static inline bool xm8656FinalPass(SDL_Renderer* render,bool selectionInk=false){
    if(!render || !xm8655DoubleFeature())return true;
    auto& s=xm8656State();
    int w=0,h=0;
    if(SDL_GetRendererOutputSize(render,&w,&h)!=0 || w<1 || h<1 ||
       w>8192 || h>8192 || (size_t)w*(size_t)h>16000000u){
        if(!s.warned){std::fprintf(stderr,"XM86.56 FINAL_PASS_INVALID_OUTPUT %dx%d %s\n",w,h,SDL_GetError());s.warned=true;}
        return false;
    }
    if(s.renderer!=render || s.width!=w || s.height!=h || !s.texture){
        xm8656Cleanup();
        s.renderer=render;s.width=w;s.height=h;
        s.pixels.resize((size_t)w*(size_t)h);
        s.texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
        if(!s.texture){
            if(!s.warned){std::fprintf(stderr,"XM86.56 FINAL_PASS_TEXTURE_ERROR %s\n",SDL_GetError());s.warned=true;}
            return false;
        }
        SDL_SetTextureBlendMode(s.texture,SDL_BLENDMODE_NONE);
        std::fprintf(stderr,"XM87.131 DF_READBACK_FULL_OUTPUT ready=%dx%d\n",w,h);
    }
    // XM87.131 DF_READBACK_FULL_OUTPUT: The source game uses a centered 960x540 logical viewport,
    // while the DF effect stores a full 1686x1064 physical-output texture.
    // SDL_RenderReadPixels(nullptr) reads in the CURRENT VIEWPORT coordinate
    // system, not a guaranteed (0,0)-anchored physical backbuffer.  With
    // vertical letterboxing it starts 58 physical pixels down, so restoring
    // those readback pixels into a full-output texture at (0,0) moves the
    // picture UP 58 pixels and clips the top.  Fix the coordinate mismatch at
    // its source by switching to the physical output BEFORE the readback.
    // Every render state is restored on both success and failure paths.
    int lw=0,lh=0;SDL_RenderGetLogicalSize(render,&lw,&lh);
    SDL_Rect viewport={0,0,0,0};SDL_RenderGetViewport(render,&viewport);
    SDL_Rect clip={0,0,0,0};const bool hadClip=SDL_RenderIsClipEnabled(render)==SDL_TRUE;
    if(hadClip)SDL_RenderGetClipRect(render,&clip);
    SDL_BlendMode oldMode=SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(render,&oldMode);
    const auto restore=[&](){
        if(lw>0 && lh>0)SDL_RenderSetLogicalSize(render,lw,lh);
        else {SDL_RenderSetLogicalSize(render,0,0);SDL_RenderSetViewport(render,&viewport);}
        SDL_RenderSetClipRect(render,hadClip?&clip:nullptr);
        SDL_SetRenderDrawBlendMode(render,oldMode);
    };
    SDL_RenderSetLogicalSize(render,0,0);
    SDL_RenderSetViewport(render,nullptr);
    SDL_RenderSetClipRect(render,nullptr);
    // The texture now contains the actual full physical output, including
    // BOTH top and bottom letterbox bars, rather than a viewport-cropped view.
    if(SDL_RenderReadPixels(render,nullptr,SDL_PIXELFORMAT_ARGB8888,
                            s.pixels.data(),w*int(sizeof(Uint32)))!=0){
        if(!s.warned){std::fprintf(stderr,"XM86.56 FINAL_PASS_READ_ERROR %s\n",SDL_GetError());s.warned=true;}
        restore();return false;
    }
    xm8656Grayscale(s.pixels.data(),s.pixels.size());
    if(SDL_UpdateTexture(s.texture,nullptr,s.pixels.data(),w*int(sizeof(Uint32)))!=0){
        if(!s.warned){std::fprintf(stderr,"XM86.56 FINAL_PASS_UPLOAD_ERROR %s\n",SDL_GetError());s.warned=true;}
        restore();return false;
    }
    const int result=SDL_RenderCopy(render,s.texture,nullptr,nullptr);
    // Preserve SDL's centered logical letterbox, and never override its
    // computed viewport with a stale coordinate-space rectangle.
    restore();
    if(result!=0 && !s.warned){std::fprintf(stderr,"XM86.56 FINAL_PASS_COPY_ERROR %s\n",SDL_GetError());s.warned=true;}
    if(++s.frames==1)std::fprintf(stderr,"XM86.56 FINAL_PASS_ACTIVE %dx%d\n",w,h);
    return result==0;
}
static inline void xm8662SelectionPresent(SDL_Renderer* render){
    if(!render)return;
    static bool reported=false;
    if(!reported && xm8655DoubleFeature()){
        std::fprintf(stderr,"XM86.65 SELECTION_LUMA no_full_screen_ink96=true\n");
        reported=true;
    }
    xm8656FinalPass(render,false); // XM86.65 no whole-screen fighter ink cutoff
    SDL_RenderPresent(render);
}
static inline void xm8656Present(SDL_Renderer* render){
    if(!render)return;
    xm8656FinalPass(render);
    SDL_RenderPresent(render);
}
#endif
