#ifndef XMVSF_STAGE87_401_BOOMER_ENEMIES_H
#define XMVSF_STAGE87_401_BOOMER_ENEMIES_H
// TEST 87.401: source-GIF animated Boomer Kuwanger scenery.
// Purely visual; no ROM, CPS2 hitbox, player or collision mutations.
// All movement uses the native stage's emulated-frame clock, not SDL paints.
#include <SDL.h>
#include <SDL_image.h>
#if !SDL_IMAGE_VERSION_ATLEAST(2,6,0)
#error "TEST 87.401 needs SDL2_image 2.6.0+ for animated GIF frames. Update MSYS2 UCRT64 SDL2_image."
#endif
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include "xmvsf_stage87_401_boomer_motion.h"

namespace XMBoomerExtra {
static constexpr int ScreenW=384, ScreenH=224;
static constexpr int CenterCameraX=192;
static constexpr int RotorMinX=XM87401Motion::RotorXMin;
static constexpr int RotorMaxX=XM87401Motion::RotorXMax;
static constexpr unsigned WallFramesPerPixel=XM87401Motion::NearFramesPerPixel;
static constexpr unsigned RotorFramesPerPixel=XM87401Motion::RotorFramesPerPixel;
static constexpr unsigned MaxShots=24u;

struct Frame {
    SDL_Texture* texture=nullptr;
    unsigned duration=6u; // native 60-Hz game frames
};
struct Sprite {
    std::vector<Frame> frames;
    std::vector<unsigned> starts;
    int w=0,h=0;
    unsigned cycleFrames=0u;
    void close(){
        for(Frame& f:frames)if(f.texture)SDL_DestroyTexture(f.texture);
        frames.clear();starts.clear();w=h=0;cycleFrames=0;
    }
    bool load(SDL_Renderer* renderer,const std::string& path){
        close();
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
        SDL_Animation* animation=IMG_LoadAnimation(path.c_str());
        if(!animation || animation->count<=0 || !animation->frames){
            std::fprintf(stderr,"XM87.401 GIF_LOAD_FAILED file=%s reason=%s\n",
                         path.c_str(),IMG_GetError());
            if(animation)IMG_FreeAnimation(animation);
            return false;
        }
        w=animation->w;h=animation->h;
        if(w<1||h<1||w>384||h>384||animation->count>512){
            std::fprintf(stderr,"XM87.401 GIF_INVALID file=%s dims=%dx%d frames=%d\n",
                         path.c_str(),w,h,animation->count);
            IMG_FreeAnimation(animation);return false;
        }
        frames.reserve((unsigned)animation->count);
        starts.reserve((unsigned)animation->count);
        bool ok=true;
        for(int i=0;i<animation->count;++i){
            SDL_Surface* surface=animation->frames[i];
            if(!surface){ok=false;break;}
            SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,surface);
            if(!t){ok=false;break;}
            SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
            SDL_SetTextureScaleMode(t,SDL_ScaleModeNearest);
#endif
            const unsigned milliseconds=animation->delays &&
                animation->delays[i]>0 ? (unsigned)animation->delays[i]:100u;
            const unsigned gameFrames=std::max(1u,std::min(300u,
                                      (milliseconds*60u+500u)/1000u));
            starts.push_back(cycleFrames);
            frames.push_back(Frame{t,gameFrames});
            cycleFrames+=gameFrames;
        }
        const int count=animation->count;
        IMG_FreeAnimation(animation);
        if(!ok || !cycleFrames){
            close();
            std::fprintf(stderr,"XM87.401 GIF_TEXTURE_FAILED file=%s reason=%s\n",
                         path.c_str(),SDL_GetError());return false;
        }
        std::fprintf(stderr,
            "XM87.401 GIF_READY file=%s frame_count=%d size=%dx%d cycle_game_frames=%u\n",
            path.c_str(),count,w,h,cycleFrames);
        return true;
#else
        // The game does not silently pretend a static GIF is an animation.
        std::fprintf(stderr,
            "XM87.401 GIF_UNSUPPORTED SDL_image_2.6_or_newer_required file=%s\n",
            path.c_str());
        (void)renderer;(void)path;
        return false;
#endif
    }
    bool ready()const{return !frames.empty()&&cycleFrames!=0u;}
    unsigned index(unsigned long long age)const{
        if(!ready())return 0u;
        const unsigned phase=(unsigned)(age%cycleFrames);
        for(unsigned i=(unsigned)frames.size();i>0u;--i)
            if(phase>=starts[i-1u])return i-1u;
        return 0u;
    }
    unsigned long long cycle(unsigned long long age)const{
        return cycleFrames?age/cycleFrames:0u;
    }
    bool draw(SDL_Renderer* renderer,const SDL_Rect& view,
              int x,int y,SDL_RendererFlip flip,unsigned long long age)const{
        if(!ready())return false;
        const Frame& f=frames[index(age)];
        const int x0=view.x+int(std::lround(double(x)*view.w/ScreenW));
        const int y0=view.y+int(std::lround(double(y)*view.h/ScreenH));
        const int x1=view.x+int(std::lround(double(x+w)*view.w/ScreenW));
        const int y1=view.y+int(std::lround(double(y+h)*view.h/ScreenH));
        const SDL_Rect target={x0,y0,x1-x0,y1-y0};
        if(target.w<=0||target.h<=0)return true;
        return SDL_RenderCopyEx(renderer,f.texture,nullptr,&target,0.0,nullptr,flip)==0;
    }
};

struct Random {
    std::uint32_t state=0x9e3779b9u;
    void reset(unsigned generation){
        state=0x6d2b79f5u^(generation*0x9e3779b9u);
        if(!state)state=1u;
    }
    unsigned next(){
        // No std::rand or wall-clock seed: stable in a replay/session.
        state^=state<<13;state^=state>>17;state^=state<<5;
        return state;
    }
    unsigned range(unsigned lo,unsigned hi){
        return lo+(next()%(hi-lo+1u));
    }
};
struct Rotor {
    bool active=false;
    unsigned long long born=0u,nextSpawn=0u;
    int x=185,yStart=-64,hoverY=70;
    int bobPhase=0;
};
struct Triblaster {
    bool active=false;
    unsigned long long born=0u,nextSpawn=0u,lastAge=0u;
    int x=0,yStart=-96;
    bool initializedEvents=false;
};
enum ProjectileKind {Shot=0,Bullet=1};
struct Projectile {
    ProjectileKind kind=Shot;
    bool left=false;
    bool vertical=false;
    unsigned long long born=0u;
    int startX=0,startY=0;
};

struct State {
    Sprite rotor,triblaster,shot,bullet;
    Random rng;
    std::array<Rotor,3> rotors{};
    std::array<Triblaster,2> guns{};
    std::vector<Projectile> projectiles;
    bool ready=false;
    unsigned generation=~0u;
    unsigned long long lastFrame=~0ull,choiceAt=0u;
    unsigned desiredRotors=2u;
    void close(){
        rotor.close();triblaster.close();shot.close();bullet.close();
        projectiles.clear();
        ready=false;generation=~0u;lastFrame=~0ull;
        rotors={};guns={};
    }
    bool load(SDL_Renderer* renderer,const std::string& root){
        if(ready)return true;
        if(!rotor.load(renderer,root+"rotor.gif") ||
           !triblaster.load(renderer,root+"triblaster.gif") ||
           !shot.load(renderer,root+"triblaster_shot.gif") ||
           !bullet.load(renderer,root+"triblaster_bullet.gif")){
            close();
            std::fprintf(stderr,
                "XM87.401 BOOMER_ENEMY_GIFS_REQUIRED root=%s\n",root.c_str());
            return false;
        }
        ready=true;
        if(triblaster.frames.size()<=10u || triblaster.frames.size()<=1u)
            std::fprintf(stderr,
                "XM87.401 TRIBLASTER_FRAME_WARNING count=%u need_frame_11_and_2\n",
                (unsigned)triblaster.frames.size());
        std::fprintf(stderr,
            "XM87.401 BOOMER_ENEMIES_READY rotor=1..3 x=150..220 "
            "triblaster_x_right=277 left=105 gif_frames=%u "
            "bullet_frame=2 shot_frame=11 "
            "wall_px_per_second=30 rotor=20 projectile=60\n",
            (unsigned)triblaster.frames.size());
        return true;
    }
    static int nearPan(int cameraX){return CenterCameraX-cameraX;}
    static int rotorPan(int cameraX){
        return (CenterCameraX-cameraX)*60/100; // C uses 30%, B uses 100%
    }
    void reset(unsigned generationNow,unsigned long long gameFrame){
        generation=generationNow;lastFrame=gameFrame;
        rng.reset(generationNow);
        desiredRotors=rng.range(1u,3u);
        choiceAt=gameFrame+240u;
        for(unsigned i=0;i<rotors.size();++i){
            Rotor& r=rotors[i];r=Rotor{};
            r.nextSpawn=gameFrame+(i<desiredRotors?rng.range(0u,110u):360u);
        }
        for(unsigned i=0;i<guns.size();++i){
            Triblaster& g=guns[i];g=Triblaster{};
            g.x=i==0u?XM87401Motion::DefaultRightX:
                       XM87401Motion::MirroredLeftX;
            g.nextSpawn=gameFrame+rng.range(0u,80u)+(i==1u?45u:0u);
        }
        projectiles.clear();
        std::fprintf(stderr,
            "XM87.401 BOOMER_ENEMIES_RESET generation=%u frame=%llu "
            "rotor_slots=%u\n",generationNow,
            (unsigned long long)gameFrame,desiredRotors);
    }
    static int gunY(const Triblaster& gun,unsigned long long frame){
        return gun.yStart+XM87401Motion::wallShift(gun.born,frame);
    }
    static int rotorY(const Rotor& rotor,unsigned long long frame){
        const unsigned long long elapsed=frame>=rotor.born?frame-rotor.born:0ull;
        // Drop from above, settle into a slow hover, then descend
        // at an intermediate parallax speed with the moving elevator.
        const int approach=std::min(rotor.hoverY,rotor.yStart+int(elapsed));
        const int drift=XM87401Motion::rotorShift(rotor.born,frame);
        const int bob=int(std::lround(3.0*std::sin(
             double(elapsed+unsigned(rotor.bobPhase))*0.08)));
        return approach+drift+bob;
    }
    void spawnProjectile(bool left,bool flipV,ProjectileKind kind,
                         const Triblaster& gun,unsigned long long moment){
        if(projectiles.size()>=MaxShots)return;
        const Sprite& effect=kind==Shot?shot:bullet;
        // Coordinates are measured from the original, unflipped right-side
        // Triblaster. Horizontal and vertical mirroring transform the
        // *attachment point* as well as the artwork.
        const int localX=kind==Shot?8:27;
        const int localY=kind==Shot?26:47;
        const int x=XM87401Motion::offsetX(left,triblaster.w,effect.w,localX);
        const int y=XM87401Motion::offsetY(flipV,triblaster.h,effect.h,localY);
        const int originX=gun.x+x;
        const int originY=gunY(gun,moment)+y;
        projectiles.push_back(Projectile{kind,left,flipV,moment,
                                         originX,originY});
        std::fprintf(stderr,
            "XM87.401 TRIBLASTER_FIRE side=%s kind=%s frame=%llu "
            "mirror_vertical=%d anchor=%d,%d\n",
            left?"left":"right",kind==Shot?"shot":"bullet",
            (unsigned long long)moment,int(flipV),originX,originY);
    }
    void fireEvents(Triblaster& gun,bool left,unsigned long long frame){
        if(!triblaster.ready()||frame<gun.born)return;
        const unsigned long long now=frame-gun.born;
        // Every GIF cycle triggers on source frames 2 and 11 (1-based).
        // Walk animation frame ENTER events, not paint calls. That catches
        // triggers even if an SDL frame is skipped to meet an audio deadline.
        const unsigned long long prev=gun.initializedEvents?gun.lastAge:~0ull;
        const unsigned long long previousCycle=prev==~0ull?0ull:prev/triblaster.cycleFrames;
        const unsigned long long currentCycle=now/triblaster.cycleFrames;
        // Bounded after a pause/load: don't burst many old projectiles.
        const unsigned long long firstCycle=currentCycle>3ull &&
            previousCycle<currentCycle-3ull?currentCycle-3ull:previousCycle;
        for(unsigned long long cycle=firstCycle;cycle<=currentCycle;++cycle){
            for(unsigned index: {1u,10u}){
                if(index>=triblaster.starts.size())continue;
                const unsigned long long trigger=cycle*triblaster.cycleFrames+
                                                 triblaster.starts[index];
                if(trigger>now || (prev!=~0ull && trigger<=prev))continue;
                if(prev==~0ull && trigger!=0ull && trigger<now)continue;
                const bool flipV=(cycle&1ull)!=0ull;
                spawnProjectile(left,flipV,index==1u?Bullet:Shot,
                                gun,gun.born+trigger);
            }
        }
        gun.initializedEvents=true;gun.lastAge=now;
    }
    void update(unsigned long long frame,unsigned gen){
        if(!ready)return;
        if(gen!=generation || lastFrame==~0ull || frame<lastFrame)
            reset(gen,frame);
        lastFrame=frame;
        if(frame>=choiceAt){
            desiredRotors=rng.range(1u,3u);
            choiceAt=frame+unsigned(rng.range(240u,420u));
        }
        unsigned activeCount=0u;
        for(const Rotor& r:rotors)if(r.active)++activeCount;
        for(Rotor& r:rotors){
            if(r.active){
                if(rotorY(r,frame)>ScreenH+rotor.h+12){
                    r.active=false;
                    r.nextSpawn=frame+rng.range(25u,130u);
                    --activeCount;
                }
                continue;
            }
            if(frame<r.nextSpawn || activeCount>=desiredRotors)continue;
            r.active=true;r.born=frame;
            r.x=int(rng.range(RotorMinX,RotorMaxX));
            r.yStart=-rotor.h-int(rng.range(12u,64u));
            r.hoverY=int(rng.range(28u,118u));
            r.bobPhase=int(rng.range(0u,80u));
            ++activeCount;
            std::fprintf(stderr,
                "XM87.401 ROTOR_SPAWN x=%d start_y=%d hover_y=%d "
                "frame=%llu active=%u\n",
                r.x,r.yStart,r.hoverY,(unsigned long long)frame,activeCount);
        }
        for(unsigned side=0;side<guns.size();++side){
            Triblaster& gun=guns[side];
            if(gun.active && gunY(gun,frame)>ScreenH+triblaster.h){
                gun.active=false;
                gun.nextSpawn=frame+rng.range(60u,150u);
            }
            if(!gun.active && frame>=gun.nextSpawn){
                gun.active=true;gun.born=frame;
                gun.yStart=-triblaster.h-int(rng.range(8u,90u));
                gun.lastAge=0u;gun.initializedEvents=false;
                std::fprintf(stderr,
                    "XM87.401 TRIBLASTER_SPAWN side=%s x=%d y=%d "
                    "frame=%llu\n",
                    side?"left":"right",gun.x,gun.yStart,
                    (unsigned long long)frame);
            }
            if(gun.active)fireEvents(gun,side==1u,frame);
        }
        projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),
            [&](const Projectile& p){
                const unsigned long long elapsed=frame>=p.born?frame-p.born:0ull;
                const int horizontal=XM87401Motion::projectileDx(
                    p.left,p.kind==Shot,elapsed);
                const int vertical=XM87401Motion::projectileDy(
                    p.vertical,p.kind==Bullet,p.born,frame);
                const Sprite& art=p.kind==Shot?shot:bullet;
                const int x=p.startX+horizontal,y=p.startY+vertical;
                return x< -art.w-64||x>ScreenW+64||
                       y< -art.h-160||y>ScreenH+art.h+96||
                       elapsed>900ull;
            }),projectiles.end());
    }
    bool drawRotors(SDL_Renderer* renderer,const SDL_Rect& view,
                    unsigned long long frame,int camX){
        if(!ready)return true;
        bool ok=true;
        for(const Rotor& r:rotors)if(r.active){
            const unsigned long long age=frame>=r.born?frame-r.born:0ull;
            ok=rotor.draw(renderer,view,r.x+rotorPan(camX),
                          rotorY(r,frame),SDL_FLIP_NONE,age)&&ok;
        }
        return ok;
    }
    bool drawGuns(SDL_Renderer* renderer,const SDL_Rect& view,
                  unsigned long long frame,int camX,int wallSpread){
        if(!ready)return true;
        bool ok=true;
        for(unsigned side=0;side<guns.size();++side){
            const Triblaster& gun=guns[side];
            if(!gun.active)continue;
            const bool left=side==1u;
            const unsigned long long age=frame>=gun.born?frame-gun.born:0ull;
            const bool flipV=XM87401Motion::verticalFlip(triblaster.cycle(age));
            SDL_RendererFlip flip=left?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE;
            if(flipV)flip=SDL_RendererFlip(flip|SDL_FLIP_VERTICAL);
            const int x=gun.x+nearPan(camX)+(left?-wallSpread:wallSpread);
            ok=triblaster.draw(renderer,view,x,gunY(gun,frame),
                               flip,age)&&ok;
        }
        for(const Projectile& p:projectiles){
            const unsigned long long elapsed=frame>=p.born?frame-p.born:0ull;
            // Right-side default: leftward. Mirrored left-side: rightward.
            const int vx=XM87401Motion::projectileDx(
                p.left,p.kind==Shot,elapsed);
            const int vy=XM87401Motion::projectileDy(
                p.vertical,p.kind==Bullet,p.born,frame);
            const Sprite& effect=p.kind==Shot?shot:bullet;
            SDL_RendererFlip flip=p.left?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE;
            if(p.vertical)flip=SDL_RendererFlip(flip|SDL_FLIP_VERTICAL);
            const int sideSpread=p.left?-wallSpread:wallSpread;
            ok=effect.draw(renderer,view,
                p.startX+nearPan(camX)+sideSpread+vx,
                p.startY+vy,flip,elapsed)&&ok;
         }
        return ok;
    }
};
} // namespace XMBoomerExtra
#endif
