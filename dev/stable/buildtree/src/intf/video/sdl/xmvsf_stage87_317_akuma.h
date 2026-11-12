#ifndef XMVSF_STAGE87_317_AKUMA_H
#define XMVSF_STAGE87_317_AKUMA_H
// Akuma Stage: native stage05 camera, original-sized PNG pairs.
// For each role, B is under A in the SAME depth/transform. At first
// combat frame +600, reveal B and dither A over the following 2700
// emulated frames (~45 seconds @60fps). No first-death/KO trigger.
// Four-bit SDL alpha blending is NOT used as a substitute for dithering:
// a fixed world-pixel 8x8 Bayer mask punches actual alpha holes.
// An A texture is uploaded only when its 0..64 dither step changes.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

namespace XMAkuma {
static const int VIEW_W=384,VIEW_H=224,STAGE_W=768,STAGE_H=471;
static const int COMBAT_WAIT_FRAMES=600,TRANSITION_FRAMES=2700;
static inline int clampi(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}
static const unsigned char BAYER_8[64]={
 0,48,12,60,3,51,15,63,32,16,44,28,35,19,47,31,
 8,56,4,52,11,59,7,55,40,24,36,20,43,27,39,23,
 2,50,14,62,1,49,13,61,34,18,46,30,33,17,45,29,
 10,58,6,54,9,57,5,53,42,26,38,22,41,25,37,21
};
struct Layer {
 SDL_Texture *a=nullptr,*b=nullptr;
 std::vector<Uint8> source,pixels;
 int w=0,h=0,bw=0,bh=0,lastCut=-1;
 void close(){
  if(a)SDL_DestroyTexture(a);if(b)SDL_DestroyTexture(b);
  a=b=nullptr;source.clear();pixels.clear();
  w=h=bw=bh=0;lastCut=-1;
 }
 bool load(SDL_Renderer* renderer,const std::string& root,
           const char* nameA,const char* nameB){
  SDL_Surface* rawA=IMG_Load((root+nameA).c_str());
  SDL_Surface* rawB=IMG_Load((root+nameB).c_str());
  if(!rawA||!rawB){
   std::fprintf(stderr,"XM87.317 AKUMA_ART_MISSING A=%s B=%s root=%s error=%s\n",
                nameA,nameB,root.c_str(),IMG_GetError());
   if(rawA)SDL_FreeSurface(rawA);
   if(rawB)SDL_FreeSurface(rawB);
   return false;
  }
  SDL_Surface* rgba=SDL_ConvertSurfaceFormat(rawA,SDL_PIXELFORMAT_RGBA32,0);
  w=rawA->w;h=rawA->h;bw=rawB->w;bh=rawB->h;
  b=SDL_CreateTextureFromSurface(renderer,rawB);
  SDL_FreeSurface(rawA);SDL_FreeSurface(rawB);
  if(!rgba||!b){if(rgba)SDL_FreeSurface(rgba);return false;}
  if(w<1||h<1||w>4096||h>4096||bw<1||bh<1||bw>4096||bh>4096){
   SDL_FreeSurface(rgba);return false;
  }
  source.resize((size_t)w*h*4u);
  for(int y=0;y<h;++y)
   std::memcpy(source.data()+(size_t)y*w*4u,
               (const Uint8*)rgba->pixels+(size_t)y*rgba->pitch,(size_t)w*4u);
  SDL_FreeSurface(rgba);
  pixels=source;
  a=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                      SDL_TEXTUREACCESS_STREAMING,w,h);
  if(!a)return false;
  SDL_SetTextureBlendMode(a,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(b,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(a,SDL_ScaleModeNearest);
  SDL_SetTextureScaleMode(b,SDL_ScaleModeNearest);
  if(SDL_UpdateTexture(a,nullptr,source.data(),w*4)!=0)return false;
  lastCut=0;
  std::fprintf(stderr,"XM87.317 AKUMA_LAYER A=%s %dx%d B=%s %dx%d\n",
               nameA,w,h,nameB,bw,bh);
  return true;
 }
 bool updateCut(int cut){
  cut=clampi(cut,0,64);
  if(lastCut==cut)return true;
  if(cut==0)pixels=source;
  else{
   for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    const size_t k=((size_t)y*w+x)*4u;
    // Stable mask anchored in source pixels: no moving checkerboard
    // when the native camera pans, and the original RGB is preserved.
    pixels[k+3]=BAYER_8[(y&7)*8+(x&7)]<cut?0u:source[k+3];
   }
  }
  if(SDL_UpdateTexture(a,nullptr,pixels.data(),w*4)!=0)return false;
  lastCut=cut;return true;
 }
};
struct Scene {
 // Layer 0=stage floor, 1=moon, 2=clouds; painter draws 2,1,0.
 Layer layers[3];
 SDL_Texture *fighters=nullptr,*flash=nullptr;
 unsigned generation=~0u;
 unsigned long long combatStart=~0ull;
 unsigned previousStatus=0;
 bool loaded=false,failed=false;
 int lastProgress=-1;
 void close(){
  for(Layer& layer:layers)layer.close();
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  fighters=flash=nullptr;
  loaded=failed=false;generation=~0u;combatStart=~0ull;
  previousStatus=0;lastProgress=-1;
 }
 static std::string locate(){
  std::vector<std::string> dirs={"_art/sf3akuma/","../_art/sf3akuma/",
                                 "../../_art/sf3akuma/"};
  char* b=SDL_GetBasePath();
  if(b){
   const std::string base(b);SDL_free(b);
   dirs.push_back(base+"../../_art/sf3akuma/");
   dirs.push_back(base+"../_art/sf3akuma/");
   dirs.push_back(base+"_art/sf3akuma/");
  }
  for(const std::string& dir:dirs){
   SDL_RWops* probe=SDL_RWFromFile((dir+"sf3akuma1_a1.png").c_str(),"rb");
   if(probe){SDL_RWclose(probe);return dir;}
  }
  return "";
 }
 bool load(SDL_Renderer* renderer){
  if(loaded)return true;
  if(failed)return false;
  const std::string root=locate();
  if(root.empty()){
   std::fprintf(stderr,"XM87.317 AKUMA_ASSETS_MISSING _art/sf3akuma/sf3akuma1_a1.png\n");
   failed=true;return false;
  }
  const char* a[]={"sf3akuma1_a1.png","sf3akuma1_a2.png","sf3akuma1_a3.png"};
  const char* b[]={"sf3akuma1_b.png","sf3akuma1_b2.png","sf3akuma1_b3.png"};
  bool good=true;
  for(int i=0;i<3&&good;++i)good=layers[i].load(renderer,root,a[i],b[i]);
  if(good && (layers[0].w!=STAGE_W||layers[0].h!=STAGE_H||
              layers[0].bw!=STAGE_W||layers[0].bh!=STAGE_H)){
   std::fprintf(stderr,"XM87.317 AKUMA_GROUND_SIZE expected=%dx%d gotA=%dx%d gotB=%dx%d\n",
                STAGE_W,STAGE_H,layers[0].w,layers[0].h,layers[0].bw,layers[0].bh);
   good=false;
  }
  if(good){
   fighters=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                              SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
   flash=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                           SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
   good=fighters&&flash;
   if(good){
    SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(fighters,SDL_ScaleModeNearest);
    SDL_SetTextureScaleMode(flash,SDL_ScaleModeNearest);
   }
  }
  if(!good){
   std::fprintf(stderr,"XM87.317 AKUMA_TEXTURE_FAILED %s\n",SDL_GetError());
   close();failed=true;return false;
  }
  loaded=true;
  std::fprintf(stderr,"XM87.317 AKUMA_READY root=%s stage=%dx%d delay=%d transition=%d nativeCamera=stage05\n",
               root.c_str(),STAGE_W,STAGE_H,COMBAT_WAIT_FRAMES,TRANSITION_FRAMES);
  return true;
 }
 void newSession(unsigned gen){
  generation=gen;combatStart=~0ull;previousStatus=0;lastProgress=-1;
  // When restarting a match, restore all A alpha without reloading
  // sprites/assets. No scene or framebuffer destruction.
  for(Layer& layer:layers)if(layer.a)layer.updateCut(0);
 }
 int progress(unsigned long long frame,unsigned status,unsigned gen){
  if(generation!=gen)newSession(gen);
  if(status>=2u && status<8u && combatStart!=~0ull &&
     previousStatus>=8u)newSession(gen); // rematch/pre-combat
  if(status==8u && combatStart==~0ull){
   combatStart=frame;
   std::fprintf(stderr,"XM87.317 AKUMA_COMBAT_START frame=%llu ditherStart=+%d complete=+%d\n",
                frame,COMBAT_WAIT_FRAMES,COMBAT_WAIT_FRAMES+TRANSITION_FRAMES);
  }
  previousStatus=status;
  if(combatStart==~0ull||frame<=combatStart+COMBAT_WAIT_FRAMES)return 0;
  const unsigned long long age=frame-combatStart-COMBAT_WAIT_FRAMES;
  const int cut=(int)std::min(64ull,
                  (age*64ull+TRANSITION_FRAMES-1u)/TRANSITION_FRAMES);
  if(lastProgress!=cut){
   if(cut==1||cut==64||cut==16||cut==32||cut==48)
    std::fprintf(stderr,"XM87.317 AKUMA_DITHER cut=%d/64 combatFrames=%llu\n",
                 cut,frame-combatStart);
   lastProgress=cut;
  }
  return cut;
 }
 static SDL_Rect placed(const SDL_Rect& view,int w,int h,
                        int cameraX,int cameraY,int parallaxPercent){
  // World origin is authored 768x471, at native grounded Y=256.
  // Oversized moon/cloud layers are CENTRED in both axes at original
  // source pixel sizes (including the 1491x1055 B3, not resized).
  const int dx=(STAGE_W-w)/2-cameraX*parallaxPercent/100;
  const int dy=(STAGE_H-h)/2+VIEW_H-STAGE_H+256-cameraY;
  return SDL_Rect{view.x+dx*view.w/VIEW_W,view.y+dy*view.h/VIEW_H,
                  std::max(1,w*view.w/VIEW_W),std::max(1,h*view.h/VIEW_H)};
 }
 bool paint(SDL_Renderer* renderer,const SDL_Rect& view){
  if(!loaded || !renderer)return false;
  const int camera=clampi(xm8649Visual.nativeCameraX,0,STAGE_W-VIEW_W);
  const int camY=clampi(xm8649Visual.nativeCameraY,-14,256);
  const int cut=progress(xm8649Visual.frames,xm8649Visual.status,
                         xm77StageRollGeneration());
  const bool clipBefore=SDL_RenderIsClipEnabled(renderer)==SDL_TRUE;
  SDL_Rect oldClip={0,0,0,0};
  if(clipBefore)SDL_RenderGetClipRect(renderer,&oldClip);
  SDL_RenderSetClipRect(renderer,&view);
  bool good=true;
  for(int i=2;i>=0&&good;--i){
   Layer& l=layers[i];
   // Clouds 30%, moon 55%, stage floor 100% native horizontal movement.
   const int rate=i==2?30:(i==1?55:100);
   if(cut>0){
    const SDL_Rect b=placed(view,l.bw,l.bh,camera,camY,rate);
    good=SDL_RenderCopy(renderer,l.b,nullptr,&b)==0;
   }
   if(cut<64 && good){
    if(!l.updateCut(cut)){good=false;break;}
    const SDL_Rect a=placed(view,l.w,l.h,camera,camY,rate);
    good=SDL_RenderCopy(renderer,l.a,nullptr,&a)==0;
   }
  }
  if(good){
   xm8733Overlay(renderer,view); // native stun UI under fighters
   good=SDL_RenderCopy(renderer,fighters,nullptr,&view)==0;
   const UINT8* nativeFlash=xm8757FlashPixels();
   if(nativeFlash &&
      SDL_UpdateTexture(flash,nullptr,nativeFlash,VIEW_W*4)==0)
    good=SDL_RenderCopy(renderer,flash,nullptr,&view)==0&&good;
  }
  SDL_RenderSetClipRect(renderer,clipBefore?&oldClip:nullptr);
  if(!good)std::fprintf(stderr,"XM87.317 AKUMA_DRAW_FAILED %s\n",SDL_GetError());
  return good;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* renderer,const SDL_Rect& view){
 if(xm77StageResolved!=5||!xm8649Visual.sceneActive||xm8649Visual.stage!=5)
  return false;
 if(!xm77SpriteValid||xm77SpriteWidth!=VIEW_W||xm77SpriteHeight!=VIEW_H||
    !xm77SpritePixels())return false;
 Scene& s=scene();
 if(!s.load(renderer))return false;
 if(SDL_UpdateTexture(s.fighters,nullptr,xm77SpritePixels(),VIEW_W*4)!=0)
  return false;
 return s.paint(renderer,view);
}
} // namespace XMAkuma
#endif
