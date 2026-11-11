#ifndef XMVSF_STAGE87_274_FINAL_DESTINATION_H
#define XMVSF_STAGE87_274_FINAL_DESTINATION_H
// Final Destination: original _art PNG in a 768px native stage world,
// camera tracked inside the normal 384px viewport, with nearest-neighbor
// artwork and deterministic 32-bit RGBA blue/red parallax space behind it.
// This is an SDL presentation-only stage; no ROM physics or hitboxes change.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdlib> // std::abs
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace XMFD {
static const int W=384,H=224,WORLD_W=768,SPACE_W=1152;
static const int DECK_SCREEN_Y=194; // align the platform surface with fighter feet
static inline int nativeCamera(int x){return std::max(0,std::min(WORLD_W-W,x));}
struct Star {
 int x,y;
 unsigned char layer,color,phase;
};
struct Scene {
 SDL_Texture* stars=nullptr;
 SDL_Texture* platform=nullptr;
 SDL_Texture* fighters=nullptr;
 SDL_Texture* flash=nullptr;
 int platformW=0,platformH=0,deckSourceY=0;
 bool failed=false;
 unsigned long long lastFrame=~0ull;
 int lastCameraX=-9999,lastCameraY=-9999;
 std::vector<unsigned char> rgba;
 std::vector<Star> particles;
 void close(){
  if(stars)SDL_DestroyTexture(stars);
  if(platform)SDL_DestroyTexture(platform);
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  stars=platform=fighters=flash=nullptr;
  platformW=platformH=deckSourceY=0;
  rgba.clear();particles.clear();
  failed=false;lastFrame=~0ull;
  lastCameraX=lastCameraY=-9999;
 }
 static unsigned next(unsigned& seed){
  seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;
 }
 bool load(SDL_Renderer* renderer){
  if(failed)return false;
  if(stars && platform && fighters && flash)return true;
  if(!renderer)return false;
  std::vector<std::string> folders={"_art/","../_art/","../../_art/"};
  char* base=SDL_GetBasePath();
  if(base){
   const std::string prefix(base);SDL_free(base);
   folders.push_back(prefix+"../../_art/");
   folders.push_back(prefix+"../_art/");
   folders.push_back(prefix+"_art/");
  }
  std::string found;
  SDL_Surface* image=nullptr;
  for(const auto& dir:folders){
   const std::string filename=dir+"finaldestination.png";
   image=IMG_Load(filename.c_str());
   if(image){found=filename;break;}
  }
  if(!image){
   std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_ASSET_MISSING original=_art/finaldestination.png; native fallback\n");
   failed=true;return false;
  }
  platformW=image->w;platformH=image->h;
  // Find the first substantial opaque part of the platform's central deck.
  // This inspects alpha once but does not change any of the source pixels.
  SDL_Surface* rgbaImage=SDL_ConvertSurfaceFormat(image,SDL_PIXELFORMAT_RGBA32,0);
  if(rgbaImage){
   deckSourceY=platformH-1;
   for(int y=0;y<platformH;++y){
    const Uint8* row=(const Uint8*)rgbaImage->pixels+(size_t)y*rgbaImage->pitch;
    int count=0;
    for(int x=platformW/4;x<3*platformW/4;++x)
     if(row[(size_t)x*4u+3u]>=64u)++count;
    if(count>=std::max(1,platformW/40)){deckSourceY=y;break;}
   }
   SDL_FreeSurface(rgbaImage);
  }else{
   deckSourceY=platformH*77/100; // fallback for the 600x368 source
  }
  // Upload the original PNG directly, preserving its pixels and alpha.
  platform=SDL_CreateTextureFromSurface(renderer,image);
  SDL_FreeSurface(image);
  stars=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
       SDL_TEXTUREACCESS_STREAMING,W,H);
  fighters=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
       SDL_TEXTUREACCESS_STREAMING,W,H);
  flash=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
       SDL_TEXTUREACCESS_STREAMING,W,H);
  if(!platform || !stars || !fighters || !flash || platformW<1 || platformH<1){
   std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_TEXTURE_FAILED %s\n",SDL_GetError());
   close();failed=true;return false;
  }
  SDL_SetTextureBlendMode(platform,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(platform,SDL_ScaleModeNearest);
  SDL_SetTextureBlendMode(stars,SDL_BLENDMODE_NONE);
  SDL_SetTextureScaleMode(stars,SDL_ScaleModeNearest);
  SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(fighters,SDL_ScaleModeNearest);
  SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(flash,SDL_ScaleModeNearest);
  rgba.resize((size_t)W*H*4);
  // Constant seed: the same scene is repeatable during replays/rematches.
  unsigned seed=0xF1A1D357u;
  for(int depth=0;depth<3;++depth){
   const int count=depth==0?180:(depth==1?125:85);
   for(int i=0;i<count;++i){
    Star s;
    s.x=(int)(next(seed)%SPACE_W);
    s.y=(int)(next(seed)%H);
    s.layer=(unsigned char)depth;
    const unsigned choice=next(seed)%100u;
    s.color=(unsigned char)(choice<48u?0u:(choice<91u?1u:2u));
    s.phase=(unsigned char)(next(seed)%251u);
    particles.push_back(s);
   }
  }
  const int worldH=(platformH*WORLD_W+platformW/2)/platformW;
  std::fprintf(stderr,
   "XM87.275 FINAL_DESTINATION_READY asset=%s source=%dx%d world=%dx%d deckSourceY=%d deckNativeY=%d filter=nearest RGBA32 stars=%zu layers=3\n",
   found.c_str(),platformW,platformH,WORLD_W,worldH,deckSourceY,
   DECK_SCREEN_Y,particles.size());
  return true;
 }
 void pixel(int x,int y,unsigned r,unsigned g,unsigned b,unsigned strength){
  if(x<0 || y<0 || x>=W || y>=H)return;
  unsigned char* p=rgba.data()+((size_t)y*W+(size_t)x)*4u;
  p[0]=(unsigned char)std::min(255u,(unsigned)p[0]+r*strength/255u);
  p[1]=(unsigned char)std::min(255u,(unsigned)p[1]+g*strength/255u);
  p[2]=(unsigned char)std::min(255u,(unsigned)p[2]+b*strength/255u);
  p[3]=255u;
 }
 bool renderStars(SDL_Renderer* renderer,unsigned long long frame,int cameraX,int cameraY){
  if(lastFrame==frame && cameraX==lastCameraX && cameraY==lastCameraY)return true;
  lastFrame=frame;lastCameraX=cameraX;lastCameraY=cameraY;
  for(int y=0;y<H;++y){
   for(int x=0;x<W;++x){
    unsigned char* p=rgba.data()+((size_t)y*W+(size_t)x)*4u;
    // Dark, full 32-bit RGB sky, with barely visible blue/red depth.
    const int nearBlue=std::max(0,100-std::abs(x-238)*100/270);
    const int nearRed=std::max(0,100-std::abs(x-82)*100/190);
    p[0]=(unsigned char)(3+nearRed/15+y/110);
    p[1]=(unsigned char)(4+nearBlue/25);
    p[2]=(unsigned char)(13+nearBlue/9+(H-y)/90);
    p[3]=255u;
   }
  }
  for(const Star& s:particles){
   const int drift=(int)(frame/(s.layer==0?480u:(s.layer==1?290u:175u)));
   const int parallax=s.layer==0?1:(s.layer==1?2:4); // 1/8, 1/4, 1/2
   int x=(s.x-cameraX*parallax/8-drift)%SPACE_W;
   if(x<0)x+=SPACE_W;
   if(x>=W)continue;
   int y=(s.y-(cameraY-256)*parallax/16)%H;
   if(y<0)y+=H;
   const unsigned period=46u+(s.phase%91u);
   const unsigned t=(unsigned)((frame+s.phase*11u)%period);
   const unsigned triangular=(t<period/2u)?t*2u:(period-t)*2u;
   const unsigned intensity=56u+triangular*185u/period;
   const unsigned red=s.color==1?255u:(s.color==0?87u:224u);
   const unsigned green=s.color==1?68u:(s.color==0?143u:222u);
   const unsigned blue=s.color==1?119u:255u;
   const unsigned core=std::min(255u,intensity+(s.layer==2?40u:0u));
   if(s.layer>=1){
    pixel(x-1,y,red,green,blue,core/5u);
    pixel(x+1,y,red,green,blue,core/5u);
    pixel(x,y-1,red,green,blue,core/5u);
    pixel(x,y+1,red,green,blue,core/5u);
   }
   if(s.layer==2){
    pixel(x-2,y,red,green,blue,core/12u);
    pixel(x+2,y,red,green,blue,core/12u);
   }
   pixel(x,y,red,green,blue,core);
  }
  if(SDL_UpdateTexture(stars,nullptr,rgba.data(),W*4)!=0){
   std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_STAR_UPLOAD_FAILED %s\n",SDL_GetError());
   return false;
  }
  return true;
 }
};
inline Scene& scene(){static Scene s;return s;}
inline void cleanup(){scene().close();}
inline bool paint(SDL_Renderer* render,const SDL_Rect& view){
 // Final Destination is a dedicated native-stage05 scenery replacement.
 if(xm77StageResolved!=2 || !xm8649Visual.sceneActive ||
    xm8649Visual.stage!=5 || !xm77SpriteValid ||
    xm77SpriteWidth!=W || xm77SpriteHeight!=H || !xm77SpritePixels())
  return false;
 Scene& s=scene();
 if(!s.load(render))return false; // keep original native stage on asset failure
 const unsigned long long frame=xm8649Visual.frames;
 if(!s.renderStars(render,frame,xm8649Visual.nativeCameraX,xm8649Visual.nativeCameraY))
  return false;
 if(SDL_UpdateTexture(s.fighters,nullptr,xm77SpritePixels(),W*4)!=0){
  std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_FIGHTER_UPLOAD_FAILED %s\n",SDL_GetError());
  return false;
 }
 const unsigned char* nativeFlash=xm8757FlashPixels();
 if(nativeFlash && SDL_UpdateTexture(s.flash,nullptr,nativeFlash,W*4)!=0)nativeFlash=nullptr;
 const bool hadClip=SDL_RenderIsClipEnabled(render)==SDL_TRUE;
 SDL_Rect oldClip={0,0,0,0};
 if(hadClip)SDL_RenderGetClipRect(render,&oldClip);
 SDL_RenderSetClipRect(render,&view);
 // World layer: preserve the PNG's original aspect ratio at a 768px
 // native world width, pan it by the same camera as the native fighters.
 // The existing viewport clip hides the out-of-view world art; it is
 // never squeezed down to a single 384px-wide full-stage picture.
 const int camera=nativeCamera(xm8649Visual.nativeCameraX);
 const int worldH=(s.platformH*WORLD_W+s.platformW/2)/s.platformW;
 const int deckY=(s.deckSourceY*WORLD_W+s.platformW/2)/s.platformW;
 const int worldTop=DECK_SCREEN_Y-deckY;
 const SDL_Rect platformWorld={
    view.x-camera*view.w/W, view.y+worldTop*view.h/H,
    WORLD_W*view.w/W, worldH*view.h/H
 };
 // Event-style evidence for every changed camera position.
 static int loggedCamera=-1;
 if(camera!=loggedCamera){
  std::fprintf(stderr,
   "XM87.275 FINAL_DESTINATION_CAMERA cam=%d world=%dx%d viewport=%dx%d dst=%d,%d,%d,%d filter=nearest\n",
   camera,WORLD_W,worldH,W,H,
   platformWorld.x,platformWorld.y,platformWorld.w,platformWorld.h);
  loggedCamera=camera;
 }
 const bool good=SDL_RenderCopy(render,s.stars,nullptr,&view)==0 &&
                 SDL_RenderCopy(render,s.platform,nullptr,&platformWorld)==0;
 if(good){
  xm8733Overlay(render,view); // match the established Wily stun-bar Z order
  SDL_RenderCopy(render,s.fighters,nullptr,&view);
  if(nativeFlash)SDL_RenderCopy(render,s.flash,nullptr,&view);
 }
 SDL_RenderSetClipRect(render,hadClip?&oldClip:nullptr);
 if(!good){
  std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_DRAW_FAILED %s\n",SDL_GetError());
  return false;
 }
 return true;
}
} // namespace XMFD
#endif
