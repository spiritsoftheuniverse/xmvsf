#ifndef XMVSF_STAGE87_314_SIGMA_H
#define XMVSF_STAGE87_314_SIGMA_H
// Sigma Stage (87.314). One authored PNG in the same 768px world as FD.
// Original native stage 05 still owns the camera, collision and sprites.
// Three masks are created ONCE per renderer: no video decoding, per-frame
// texture uploads, palette writes, extra audio output, or per-pixel CPU work.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace XMSigma {
static const int W=384,H=224,WORLD_W=768,GROUND_CAMERA_Y=256;
static inline int clampi(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}
struct Scene {
 SDL_Texture *base=nullptr,*orange=nullptr,*white=nullptr,*warm=nullptr;
 SDL_Texture *fighters=nullptr,*flash=nullptr;
 int imageW=0,imageH=0;
 bool failed=false;
 unsigned failedGeneration=~0u;
 void close(){
  if(base)SDL_DestroyTexture(base);
  if(orange)SDL_DestroyTexture(orange);
  if(white)SDL_DestroyTexture(white);
  if(warm)SDL_DestroyTexture(warm);
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  base=orange=white=warm=fighters=flash=nullptr;
  imageW=imageH=0;failed=false;failedGeneration=~0u;
 }
 static bool roots(SDL_Renderer* renderer,SDL_Surface*& source,std::string&found){
  (void)renderer;
  const char* names[]={
   "sigmastage.png","sigma.png","sigma_stage.png",
   "Sigma Stage.png","SigmaStage.png","sigma-stage.png",
   "sigmastage/sigmastage.png","sigmastage/sigma.png",
   "sigma/sigma.png","sigmastage/Sigma Stage.png",
   "sigma stage/sigma stage.png","sigma stage/stage.png",
   "Sigma Stage/Sigma Stage.png","Sigma Stage/stage.png",
   "assets/sigmastage.png","assets/sigma.png"
  };
  std::vector<std::string> dirs={"_art/","../_art/","../../_art/"};
  char* basePath=SDL_GetBasePath();
  if(basePath){
   const std::string b(basePath);SDL_free(basePath);
   dirs.push_back(b+"../../_art/");
   dirs.push_back(b+"../_art/");
   dirs.push_back(b+"_art/");
  }
  for(const std::string& dir:dirs)for(const char* name:names){
   const std::string path=dir+name;
   source=IMG_Load(path.c_str());
   if(source){found=path;return true;}
  }
#if defined(_WIN32)
  // Guile's original failure showed why hard-coded filenames are fragile:
  // inspect local _art/ for an unmistakably Sigma-named PNG or one PNG
  // inside a Sigma-named directory. Do NOT choose arbitrary level art.
  // The result is sorted for deterministic selection on Windows filesystems.
  for(const std::string& dir:dirs){
   std::vector<std::string> candidates;
   WIN32_FIND_DATAA entry={};
   HANDLE h=FindFirstFileA((dir+"*").c_str(),&entry);
   if(h==INVALID_HANDLE_VALUE)continue;
   do{
    const std::string name=entry.cFileName;
    if(name=="."||name=="..")continue;
    std::string lowered=name;
    std::transform(lowered.begin(),lowered.end(),lowered.begin(),
                   [](unsigned char c){return (char)std::tolower(c);});
    if(lowered.find("sigma")==std::string::npos)continue;
    if(entry.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
     WIN32_FIND_DATAA inside={};
     const std::string nested=dir+name+"/";
     HANDLE inner=FindFirstFileA((nested+"*.png").c_str(),&inside);
     if(inner!=INVALID_HANDLE_VALUE){
      do{
       if(!(inside.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
        candidates.push_back(nested+inside.cFileName);
      }while(FindNextFileA(inner,&inside));
      FindClose(inner);
     }
    }else if(lowered.size()>4 &&
             lowered.compare(lowered.size()-4,4,".png")==0)
     candidates.push_back(dir+name);
   }while(FindNextFileA(h,&entry));
   FindClose(h);
   std::sort(candidates.begin(),candidates.end());
   for(const std::string& candidate:candidates){
    source=IMG_Load(candidate.c_str());
    if(source){found=candidate;return true;}
   }
  }
#endif
  return false;
 }
 bool load(SDL_Renderer* renderer){
  if(failed){
   // Allow a new stage session to retry after assets are added, rather
   // than permanently poisoning the SDL renderer until application exit.
   if(failedGeneration==xm77StageRollGeneration())return false;
   failed=false;
  }
  if(base&&orange&&white&&warm&&fighters&&flash)return true;
  if(!renderer)return false;
  SDL_Surface* original=nullptr;std::string found;
  if(!roots(renderer,original,found)){
   std::fprintf(stderr,
    "XM87.315 SIGMA_ART_MISSING stage=4 expected=_art/sigmastage.png, _art/sigma.png, or Sigma-named PNG/folder under _art; native art fallback; verify git ls-files _art\n");
   failed=true;failedGeneration=xm77StageRollGeneration();return false;
  }
  imageW=original->w;imageH=original->h;
  // The image must remain a full image, never cropped or pixel-stripped.
  if(imageW<384||imageH<128||imageW>4096||imageH>4096 ||
     (uint64_t)imageW*imageH>8000000u){
   std::fprintf(stderr,"XM87.314 SIGMA_ART_SIZE_INVALID size=%dx%d\n",imageW,imageH);
   SDL_FreeSurface(original);failed=true;failedGeneration=xm77StageRollGeneration();return false;
  }
  SDL_Surface* image=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
  SDL_FreeSurface(original);
  if(!image){failed=true;failedGeneration=xm77StageRollGeneration();return false;}
  SDL_Surface* o=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  SDL_Surface* w=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  SDL_Surface* b=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  if(!o||!w||!b){
   if(o)SDL_FreeSurface(o);if(w)SDL_FreeSurface(w);if(b)SDL_FreeSurface(b);
   SDL_FreeSurface(image);failed=true;failedGeneration=xm77StageRollGeneration();return false;
  }
  SDL_FillRect(o,nullptr,SDL_MapRGBA(o->format,0,0,0,0));
  SDL_FillRect(w,nullptr,SDL_MapRGBA(w->format,0,0,0,0));
  SDL_FillRect(b,nullptr,SDL_MapRGBA(b->format,0,0,0,0));
  uint64_t reds=0,browns=0,orangeDots=0,whiteDots=0;
  // One-time Manhattan distance from each red highlight to its dark
  // border. This creates a genuine spatial heat gradient: red edges,
  // glowing orange mids, white-hot cores. It does not flatten the
  // original pixel-art shading into a solid white rectangle.
  const size_t n=(size_t)imageW*(size_t)imageH;
  std::vector<Uint8> distance(n,0u);
  for(int y=0;y<imageH;++y)for(int x=0;x<imageW;++x){
   const Uint8* p=(const Uint8*)image->pixels+(size_t)y*image->pitch+4u*x;
   if(p[3] && p[0]>=160 && p[0]>=p[1]+48 && p[0]>=p[2]+48)
    distance[(size_t)y*imageW+x]=32u;
  }
  for(int y=0;y<imageH;++y)for(int x=0;x<imageW;++x){
   Uint8& d=distance[(size_t)y*imageW+x];
   if(!d)continue;
   if(x>0)d=std::min<int>(d,distance[(size_t)y*imageW+x-1]+1);
   if(y>0)d=std::min<int>(d,distance[(size_t)(y-1)*imageW+x]+1);
  }
  for(int y=imageH-1;y>=0;--y)for(int x=imageW-1;x>=0;--x){
   Uint8& d=distance[(size_t)y*imageW+x];
   if(!d)continue;
   if(x+1<imageW)d=std::min<int>(d,distance[(size_t)y*imageW+x+1]+1);
   if(y+1<imageH)d=std::min<int>(d,distance[(size_t)(y+1)*imageW+x]+1);
  }
  // Ordered 8x8 dither is baked into the two immutable highlight masks.
  // Rendering uses only two global opacity modulations per frame.
  static const Uint8 bayer[64]={
   0,48,12,60,3,51,15,63,32,16,44,28,35,19,47,31,
   8,56,4,52,11,59,7,55,40,24,36,20,43,27,39,23,
   2,50,14,62,1,49,13,61,34,18,46,30,33,17,45,29,
   10,58,6,54,9,57,5,53,42,26,38,22,41,25,37,21
  };
  // RGBA32 surface data has byte order R,G,B,A on Windows.
  for(int y=0;y<imageH;++y)for(int x=0;x<imageW;++x){
   const Uint8* p=(const Uint8*)image->pixels+(size_t)y*image->pitch+4u*x;
   const int r=p[0],g=p[1],bl=p[2],a=p[3];
   if(!a)continue;
   Uint8* po=(Uint8*)o->pixels+(size_t)y*o->pitch+4u*x;
   Uint8* pw=(Uint8*)w->pixels+(size_t)y*w->pitch+4u*x;
   Uint8* pb=(Uint8*)b->pixels+(size_t)y*b->pitch+4u*x;
   // Hue/brightness ramp depends on ORIGINAL RED and distance from
   // a silhouette edge. Bayer thresholds vary its coverage by pixel;
   // red survives at edges, orange fills midtones, sparse near-white
   // points intensify toward the core (max white coverage 58/64).
   if(distance[(size_t)y*imageW+x]){
    const int heat=clampi((int(distance[(size_t)y*imageW+x])-1)*27+
                            (r-160)*2+g/4,0,255);
    const int threshold=bayer[((y&7)<<3)|(x&7)];
    const int orangeCoverage=clampi((heat-30)*64/170,0,64);
    const int whiteCoverage=clampi((heat-120)*58/135,0,58);
    if(threshold<orangeCoverage){
     po[0]=255;po[1]=(Uint8)(145+heat/12);po[2]=(Uint8)(20+heat/22);
     po[3]=(Uint8)((unsigned)a*(unsigned)(150+heat/3)/255u);
     ++orangeDots;
    }
    if(threshold<whiteCoverage){
     pw[0]=255;pw[1]=255;pw[2]=255;
     pw[3]=(Uint8)((unsigned)a*(unsigned)(160+heat/4)/255u);
     ++whiteDots;
    }
    ++reds;
   }else if(r>=63 && g>=28 && r>bl+26 && g>bl+21 &&
            g*4>=r && g<=r+40){
    // Yellow / brown retain their hue. A subtle prebrightened mask
    // follows the red light with positive phase lag in BOTH directions.
    const int lift=6+(r+g)/19;
    pb[0]=(Uint8)std::min(255,r+lift);
    pb[1]=(Uint8)std::min(255,g+lift);
    pb[2]=(Uint8)std::min(255,bl+lift/2);
    pb[3]=(Uint8)a;
    ++browns;
   }
  }
  base=SDL_CreateTextureFromSurface(renderer,image);
  orange=SDL_CreateTextureFromSurface(renderer,o);
  white=SDL_CreateTextureFromSurface(renderer,w);
  warm=SDL_CreateTextureFromSurface(renderer,b);
  SDL_FreeSurface(image);SDL_FreeSurface(o);SDL_FreeSurface(w);SDL_FreeSurface(b);
  fighters=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                            SDL_TEXTUREACCESS_STREAMING,W,H);
  flash=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                         SDL_TEXTUREACCESS_STREAMING,W,H);
  if(!base||!orange||!white||!warm||!fighters||!flash){
   std::fprintf(stderr,"XM87.314 SIGMA_TEXTURE_FAILED %s\n",SDL_GetError());
   close();failed=true;failedGeneration=xm77StageRollGeneration();return false;
  }
  SDL_Texture* all[]={base,orange,white,warm,fighters,flash};
  for(SDL_Texture* t:all){
   SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
   SDL_SetTextureScaleMode(t,SDL_ScaleModeNearest);
  }
  std::fprintf(stderr,
    "XM87.321 SIGMA_GRADIENT_READY path=%s image=%dx%d worldWidth=%d red=%llu orange=%llu white=%llu yellowBrown=%llu bayer=8x8 maximum=red-orange-white\n",
    found.c_str(),imageW,imageH,WORLD_W,(unsigned long long)reds,
    (unsigned long long)orangeDots,(unsigned long long)whiteDots,
    (unsigned long long)browns);
  return true;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* render,const SDL_Rect& view){
 if(xm77StageResolved!=4)return false;
 // Once per stage-session diagnostic pinpoints a missing native camera
 // gate, sprite capture or disk art without flooding the render loop.
 static unsigned loggedGeneration=~0u;
 static bool oldNativeGate=false,oldSpriteGate=false;
 const unsigned generation=xm77StageRollGeneration();
 const bool nativeGate=xm8649Visual.sceneActive&&xm8649Visual.stage==5;
 const bool spriteGate=xm77SpriteValid&&xm77SpriteWidth==W&&
                      xm77SpriteHeight==H&&xm77SpritePixels()!=nullptr;
 if(loggedGeneration!=generation || nativeGate!=oldNativeGate ||
    spriteGate!=oldSpriteGate){
  loggedGeneration=generation;
  oldNativeGate=nativeGate;oldSpriteGate=spriteGate;
  std::fprintf(stderr,
    "XM87.315 SIGMA_GATE generation=%u choice=%d resolved=%d nativeActive=%d nativeStage=%d status=%u sprites=%d spriteSize=%dx%d\n",
    generation,xm77StageChoice,xm77StageResolved,(int)nativeGate,
    xm8649Visual.stage,xm8649Visual.status,(int)spriteGate,
    xm77SpriteWidth,xm77SpriteHeight);
 }
 if(!nativeGate||!spriteGate)return false;
 Scene& s=scene();
 if(!s.load(render))return false;
 if(SDL_UpdateTexture(s.fighters,nullptr,xm77SpritePixels(),W*4)!=0)
  return false;
 const unsigned char* nativeFlash=xm8757FlashPixels();
 if(nativeFlash&&SDL_UpdateTexture(s.flash,nullptr,nativeFlash,W*4)!=0)
  nativeFlash=nullptr;
 const int camera=clampi(xm8649Visual.nativeCameraX,0,WORLD_W-W);
 const int cameraY=clampi(xm8649Visual.nativeCameraY,-14,GROUND_CAMERA_Y);
 const int worldH=(s.imageH*WORLD_W+s.imageW/2)/s.imageW;
 // Exact FD bottom-up registration, matching the original native
 // camera's vertical movement and horizontally cropped 768px world.
 const int top=H-worldH+(GROUND_CAMERA_Y-cameraY);
 const SDL_Rect dst={
    view.x-camera*view.w/W,view.y+top*view.h/H,
    WORLD_W*view.w/W,worldH*view.h/H
 };
 const bool clipped=SDL_RenderIsClipEnabled(render)==SDL_TRUE;
 SDL_Rect oldClip={0,0,0,0};
 if(clipped)SDL_RenderGetClipRect(render,&oldClip);
 SDL_RenderSetClipRect(render,&view);
 // Eight-second pulse, but its maximum is a spatial Bayer-dithered
 // heat gradient, NOT an all-over white filter. Smoothstep softens both
 // the temporal ramp and the crossover between orange and white.
 const double frame=(double)xm8649Visual.frames;
 const double cycle=6.2831853071795864769/480.0;
 const double red=(1.0-std::cos(frame*cycle))*0.5;
 const double orangeT=std::min(1.0,red/0.62);
 const double whiteT=std::max(0.0,std::min(1.0,(red-0.20)/0.80));
 const double orangeAlpha=orangeT*orangeT*(3.0-2.0*orangeT);
 const double whiteAlpha=whiteT*whiteT*(3.0-2.0*whiteT);
 // The warm mask trails the reds by 54 game frames (~0.9 seconds)
 // both on its rise and on its fall, with a deliberately smaller lift.
 const double delayed=(1.0-std::cos((frame-54.0)*cycle))*0.5;
 SDL_SetTextureAlphaMod(s.orange,(Uint8)(orangeAlpha*255.0+0.5));
 SDL_SetTextureAlphaMod(s.white,(Uint8)(whiteAlpha*255.0+0.5));
 SDL_SetTextureAlphaMod(s.warm,(Uint8)(delayed*255.0+0.5));
 bool good=SDL_RenderCopy(render,s.base,nullptr,&dst)==0;
 if(good)good=SDL_RenderCopy(render,s.orange,nullptr,&dst)==0;
 if(good)good=SDL_RenderCopy(render,s.white,nullptr,&dst)==0;
 if(good)good=SDL_RenderCopy(render,s.warm,nullptr,&dst)==0;
 if(good){
  xm8733Overlay(render,view); // existing native stun bar depth
  good=SDL_RenderCopy(render,s.fighters,nullptr,&view)==0;
  if(nativeFlash)good=SDL_RenderCopy(render,s.flash,nullptr,&view)==0 && good;
 }
 SDL_RenderSetClipRect(render,clipped?&oldClip:nullptr);
 if(!good)std::fprintf(stderr,"XM87.321 SIGMA_DRAW_FAILED %s\n",SDL_GetError());
 return good;
}
} // namespace XMSigma
#endif
