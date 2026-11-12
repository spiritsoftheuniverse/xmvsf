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
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace XMSigma {
static const int W=384,H=224,WORLD_W=768,GROUND_CAMERA_Y=256;
static inline int clampi(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}
struct Scene {
 SDL_Texture *base=nullptr,*orange=nullptr,*white=nullptr,*warm=nullptr;
 SDL_Texture *fighters=nullptr,*flash=nullptr;
 int imageW=0,imageH=0;
 bool failed=false;
 void close(){
  if(base)SDL_DestroyTexture(base);
  if(orange)SDL_DestroyTexture(orange);
  if(white)SDL_DestroyTexture(white);
  if(warm)SDL_DestroyTexture(warm);
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  base=orange=white=warm=fighters=flash=nullptr;
  imageW=imageH=0;failed=false;
 }
 static bool roots(SDL_Renderer* renderer,SDL_Surface*& source,std::string&found){
  (void)renderer;
  const char* names[]={
   "sigmastage.png","sigma.png","sigma_stage.png",
   "Sigma Stage.png","SigmaStage.png","sigmastage/sigmastage.png",
   "sigma/sigma.png","sigmastage/Sigma Stage.png"
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
  return false;
 }
 bool load(SDL_Renderer* renderer){
  if(failed)return false;
  if(base&&orange&&white&&warm&&fighters&&flash)return true;
  if(!renderer)return false;
  SDL_Surface* original=nullptr;std::string found;
  if(!roots(renderer,original,found)){
   std::fprintf(stderr,
    "XM87.314 SIGMA_ART_MISSING expected=_art/sigmastage.png or _art/sigma.png; native art fallback\n");
   failed=true;return false;
  }
  imageW=original->w;imageH=original->h;
  // The image must remain a full image, never cropped or pixel-stripped.
  if(imageW<384||imageH<128||imageW>4096||imageH>4096 ||
     (uint64_t)imageW*imageH>8000000u){
   std::fprintf(stderr,"XM87.314 SIGMA_ART_SIZE_INVALID size=%dx%d\n",imageW,imageH);
   SDL_FreeSurface(original);failed=true;return false;
  }
  SDL_Surface* image=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
  SDL_FreeSurface(original);
  if(!image){failed=true;return false;}
  SDL_Surface* o=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  SDL_Surface* w=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  SDL_Surface* b=SDL_CreateRGBSurfaceWithFormat(0,imageW,imageH,32,SDL_PIXELFORMAT_RGBA32);
  if(!o||!w||!b){
   if(o)SDL_FreeSurface(o);if(w)SDL_FreeSurface(w);if(b)SDL_FreeSurface(b);
   SDL_FreeSurface(image);failed=true;return false;
  }
  SDL_FillRect(o,nullptr,SDL_MapRGBA(o->format,0,0,0,0));
  SDL_FillRect(w,nullptr,SDL_MapRGBA(w->format,0,0,0,0));
  SDL_FillRect(b,nullptr,SDL_MapRGBA(b->format,0,0,0,0));
  uint64_t reds=0,browns=0;
  // RGBA32 surface data has byte order R,G,B,A on Windows.
  for(int y=0;y<imageH;++y)for(int x=0;x<imageW;++x){
   const Uint8* p=(const Uint8*)image->pixels+(size_t)y*image->pitch+4u*x;
   const int r=p[0],g=p[1],bl=p[2],a=p[3];
   if(!a)continue;
   Uint8* po=(Uint8*)o->pixels+(size_t)y*o->pitch+4u*x;
   Uint8* pw=(Uint8*)w->pixels+(size_t)y*w->pitch+4u*x;
   Uint8* pb=(Uint8*)b->pixels+(size_t)y*b->pitch+4u*x;
   // Bright saturated red ONLY. Continuous strength softens the boundary
   // to orange and dark red; dark background colors remain unmodified.
   if(r>=160 && r>=g+48 && r>=bl+48){
    const unsigned strength=(unsigned)std::min(255,(r-145)*6);
    const Uint8 alpha=(Uint8)((unsigned)a*strength/255u);
    po[0]=255;po[1]=133;po[2]=20;po[3]=alpha;
    pw[0]=255;pw[1]=255;pw[2]=255;pw[3]=alpha;
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
   close();failed=true;return false;
  }
  SDL_Texture* all[]={base,orange,white,warm,fighters,flash};
  for(SDL_Texture* t:all){
   SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
   SDL_SetTextureScaleMode(t,SDL_ScaleModeNearest);
  }
  std::fprintf(stderr,
    "XM87.314 SIGMA_READY path=%s image=%dx%d worldWidth=%d brightestReds=%llu yellowBrown=%llu camera=native05\n",
    found.c_str(),imageW,imageH,WORLD_W,
    (unsigned long long)reds,(unsigned long long)browns);
  return true;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* render,const SDL_Rect& view){
 if(xm77StageResolved!=4 || !xm8649Visual.sceneActive ||
    xm8649Visual.stage!=5)return false;
 if(!xm77SpriteValid || xm77SpriteWidth!=W || xm77SpriteHeight!=H ||
    !xm77SpritePixels())return false;
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
 // 8-second full pulse: vivid red -> bright orange -> white -> orange -> red.
 // Brightening and darkening are identical smooth sinusoidal halves.
 const double frame=(double)xm8649Visual.frames;
 const double cycle=6.2831853071795864769/480.0;
 const double red=(1.0-std::cos(frame*cycle))*0.5;
 const double orangeAlpha=std::min(1.0,red*2.0);
 const double whiteAlpha=std::max(0.0,red*2.0-1.0);
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
 if(!good)std::fprintf(stderr,"XM87.314 SIGMA_DRAW_FAILED %s\n",SDL_GetError());
 return good;
}
} // namespace XMSigma
#endif
