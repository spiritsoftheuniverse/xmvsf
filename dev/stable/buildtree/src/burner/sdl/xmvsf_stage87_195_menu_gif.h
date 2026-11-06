#ifndef XMVSF_STAGE87_195_MENU_GIF_H
#define XMVSF_STAGE87_195_MENU_GIF_H
// 87.195 animated background, main SDL boot-menu only.
// Source stays in the user's LOCAL _art folder, NOT a recreated frame
// or committed ROM asset. ColorMod (77/255 ~= 30%) dims RGB while
// preserving GIF alpha and the unchanged foreground menu controls.
// SDL2_image >= 2.6.0 implements animated GIF via IMG_LoadAnimation.
// Older SDL_image builds use the first frame as a graceful fallback.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
namespace XM87195Menu {
class Background {
    Background(const Background&)=delete;
    Background&operator=(const Background&)=delete;
    SDL_Renderer*renderer_=nullptr;
    SDL_Texture*texture_=nullptr;
    SDL_Surface*single_=nullptr;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
    IMG_Animation*animation_=nullptr;
#endif
    int lastIndex_=-1;
    Uint32 start_=0,totalMs_=0;
    std::vector<Uint32> ends_;
    bool reported_=false;
    int width_=0,height_=0;
public:
    Background()=default;
    ~Background(){release();}
    bool load(SDL_Renderer*renderer){
        release();
        renderer_=renderer;
        if(!renderer)return false;
        // Normal runtime/stable/XMVSF_Test...exe -> F:/xmvsf/_art.
        // Support launch-from-repo as well as SDL_GetBasePath, so
        // changing the working directory cannot silently hide the GIF.
        const char* filename="marvel-vs-capcom1-super-effect2.gif";
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
        for(const std::string &folder:folders){
            const std::string file=folder+filename;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
            animation_=IMG_LoadAnimation(file.c_str());
            if(animation_ && animation_->count>0 && animation_->frames &&
               animation_->w>0 && animation_->h>0){
                width_=animation_->w;height_=animation_->h;
                ends_.clear();totalMs_=0;
                for(int i=0;i<animation_->count;++i){
                    // Respect original GIF delays. Protect zero-delay
                    // frames from tight animation loops.
                    int ms=animation_->delays?animation_->delays[i]:100;
                    if(ms<20)ms=20;
                    if(totalMs_>60000000u-unsigned(ms))ms=20;
                    totalMs_+=unsigned(ms);
                    ends_.push_back(totalMs_);
                }
                start_=SDL_GetTicks();
                std::fprintf(stderr,
                    "XM87.195 MENU GIF frames=%d size=%dx%d bright=30%% file=%s\\n",
                    animation_->count,width_,height_,file.c_str());
                return true;
            }
            if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
#else
            single_=IMG_Load(file.c_str());
            if(single_){
                width_=single_->w;height_=single_->h;
                start_=SDL_GetTicks();totalMs_=100u;ends_.push_back(100u);
                std::fprintf(stderr,
                    "XM87.195 MENU GIF static fallback: SDL2_image <2.6 file=%s\\n",
                    file.c_str());
                return true;
            }
#endif
        }
        // Missing local asset is not a reason to block the game.
        // Never substitute another art or create fake GIF frames.
        std::fprintf(stderr,
            "XM87.195 MENU GIF absent: put %s in F:/xmvsf/_art (or repo _art). Menu remains normal.\\n",
            filename);
        return false;
    }
    void draw(SDL_Renderer*renderer,Uint32 now){
        if(!renderer||renderer!=renderer_||width_<=0||height_<=0)return;
        int index=0;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_ && totalMs_ && animation_->count>0){
            const Uint32 elapsed=(now-start_)%totalMs_;
            const auto found=std::upper_bound(ends_.begin(),ends_.end(),elapsed);
            index=found==ends_.end()?animation_->count-1:
                  int(found-ends_.begin());
        }
#endif
        if(index!=lastIndex_ || !texture_){
            if(texture_){SDL_DestroyTexture(texture_);texture_=nullptr;}
            SDL_Surface*surface=single_;
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
            if(animation_)surface=animation_->frames[index];
#endif
            if(!surface)return;
            texture_=SDL_CreateTextureFromSurface(renderer,surface);
            if(!texture_ && !reported_){
                std::fprintf(stderr,
                    "XM87.195 MENU GIF texture failed: %s\\n",SDL_GetError());
                reported_=true;
            }
            if(!texture_)return;
            SDL_SetTextureBlendMode(texture_,SDL_BLENDMODE_BLEND);
            SDL_SetTextureColorMod(texture_,77,77,77); // 30.2% RGB
            lastIndex_=index;
        }
        // Cover the entire logical 960x540 menu without deforming GIF
        // aspect ratio. Center crop only; foreground UI is drawn later.
        const double scale=std::max(960.0/double(width_),540.0/double(height_));
        const int w=int(width_*scale+0.5),h=int(height_*scale+0.5);
        SDL_Rect dst={(960-w)/2,(540-h)/2,w,h};
        SDL_RenderCopy(renderer,texture_,nullptr,&dst);
    }
    void release(){
        if(texture_){SDL_DestroyTexture(texture_);texture_=nullptr;}
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        if(animation_){IMG_FreeAnimation(animation_);animation_=nullptr;}
#endif
        if(single_){SDL_FreeSurface(single_);single_=nullptr;}
        renderer_=nullptr;lastIndex_=-1;
        width_=height_=0;start_=totalMs_=0;
        ends_.clear();reported_=false;
    }
};
} // XM87195Menu
#endif
