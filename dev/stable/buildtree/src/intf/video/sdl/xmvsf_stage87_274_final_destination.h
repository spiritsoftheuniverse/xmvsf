#ifndef XMVSF_STAGE87_274_FINAL_DESTINATION_H
#define XMVSF_STAGE87_274_FINAL_DESTINATION_H
// Final Destination: original _art PNG on the scenery/platform layer,
// behind native CPS2 fighters/FX. The background is a deterministic,
// full-color RGBA32, three-depth, blinking blue/red procedural starfield.
// This is an SDL presentation-only stage; no ROM physics or hitboxes change.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace XMFD {
static const int W=384,H=224,SPACE_W=1152;
struct Star {
 int x,y;
 unsigned char layer,color,phase;
};
struct Scene {
 SDL_Texture* stars=nullptr;
 SDL_Texture* platform=nullptr;
 SDL_Texture* fighters=nullptr;
 SDL_Texture* flash=nullptr;
 int platformW=0,platformH=0;
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
  platformW=platformH=0;
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
  // Upload the PNG exactly as supplied; preserve the original alpha pixels.
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
  SDL_SetTextureScaleMode(platform,SDL_ScaleModeLinear);
  SDL_SetTextureBlendMode(stars,SDL_BLENDMODE_NONE);
  SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
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
  std::fprintf(stderr,"XM87.274 FINAL_DESTINATION_READY asset=%s source=%dx%d RGBA32 stars=%zu layers=3\n",
       found.c_str(),platformW,platformH,particles.size());
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
 // Scenery depth: space -> supplied platform -> native HUD underlay -> fighters.
 const int drawnW=(s.platformW*H+s.platformH/2)/s.platformH;
 const SDL_Rect platformView={view.x+(view.w*(W-drawnW))/(2*W),
    view.y,view.w*drawnW/W,view.h};
 const bool good=SDL_RenderCopy(render,s.stars,nullptr,&view)==0 &&
                 SDL_RenderCopy(render,s.platform,nullptr,&platformView)==0;
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
