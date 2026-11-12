#ifndef XMVSF_STAGE87_317_AKUMA_H
#define XMVSF_STAGE87_317_AKUMA_H
// Akuma Stage: original-sized A/B scenery registered to native stage 05.
// Hold A for 600 combat frames, then alpha-fade A over 2700 frames.
// B remains behind A, first green-matched, gradually returning to red.
// Both moons share a user-adjustable horizontal offset, saved on key release.
// Clouds tile at half scale, scroll to the right at ~13px/second.
// No moon automation, foliage wiggle, cloud breathing or Bayer dithering.
// Ground/foreground images keep their original bottom-anchored world pixels.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>

namespace XMAkuma {
static const int VIEW_W=384,VIEW_H=224,STAGE_W=768,STAGE_H=471;
static const int COMBAT_WAIT_FRAMES=600,TRANSITION_FRAMES=2700;
static inline int clampi(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}

struct Layer {
 SDL_Texture *a=nullptr,*b=nullptr;
 std::vector<Uint8> source; // exact original A RGBA
 std::vector<Uint8> originalB,greenMatchedB,pixelsB; // B green -> red
 int w=0,h=0,bw=0,bh=0,lastTint=-1;
 void close(){
  if(a)SDL_DestroyTexture(a);if(b)SDL_DestroyTexture(b);
  a=b=nullptr;source.clear();originalB.clear();greenMatchedB.clear();pixelsB.clear();
  w=h=bw=bh=0;lastTint=-1;
 }
 bool load(SDL_Renderer* renderer,const std::string& root,
           const char* nameA,const char* nameB,bool cloud){
  SDL_Surface* rawA=IMG_Load((root+nameA).c_str());
  SDL_Surface* rawB=IMG_Load((root+nameB).c_str());
  if(!rawA||!rawB){
   std::fprintf(stderr,"XM87.320 AKUMA_ART_MISSING A=%s B=%s root=%s error=%s\n",
                nameA,nameB,root.c_str(),IMG_GetError());
   if(rawA)SDL_FreeSurface(rawA);
   if(rawB)SDL_FreeSurface(rawB);
   return false;
  }
  w=rawA->w;h=rawA->h;bw=rawB->w;bh=rawB->h;
  // Do not reindex/crop either source image. Authoring dimensions can
  // differ (the B clouds are wider/higher, and the stage gained sky rows).
  if(w<1||h<1||w>4096||h>4096||bw<1||bh<1||bw>4096||bh>4096){
   SDL_FreeSurface(rawA);SDL_FreeSurface(rawB);return false;
  }
  SDL_Surface* rgbaA=SDL_ConvertSurfaceFormat(rawA,SDL_PIXELFORMAT_RGBA32,0);
  SDL_Surface* rgbaB=SDL_ConvertSurfaceFormat(rawB,SDL_PIXELFORMAT_RGBA32,0);
  SDL_FreeSurface(rawA);SDL_FreeSurface(rawB);
  if(!rgbaA||!rgbaB){
   if(rgbaA)SDL_FreeSurface(rgbaA);
   if(rgbaB)SDL_FreeSurface(rgbaB);
   return false;
  }
  source.resize((size_t)w*h*4u);
  originalB.resize((size_t)bw*bh*4u);
  for(int y=0;y<h;++y)
   std::memcpy(source.data()+(size_t)y*w*4u,
               (const Uint8*)rgbaA->pixels+(size_t)y*rgbaA->pitch,
               (size_t)w*4u);
  for(int y=0;y<bh;++y)
   std::memcpy(originalB.data()+(size_t)y*bw*4u,
               (const Uint8*)rgbaB->pixels+(size_t)y*rgbaB->pitch,
               (size_t)bw*4u);
  SDL_FreeSurface(rgbaA);SDL_FreeSurface(rgbaB);
  greenMatchedB=originalB;
  // Establish B's FIRST-REVEAL RGB ONCE. A occupies the same world
  // coordinates; green-rich A foliage informs a strong environmental
  // green cast. Where A is transparent, hue-map B orange/red to green
  // rather than leaving obvious scarlet pixels behind the dither holes.
  // Keep each original B alpha exactly as authored.
  const int offsetX=(w-bw)/2;
  const int offsetY=cloud?(h-bh)/2:(h-bh);
  for(int y=0;y<bh;++y){
   const int ay=clampi(y+offsetY,0,h-1);
   for(int x=0;x<bw;++x){
    int ax=x+offsetX;
    if(cloud)ax=((ax%w)+w)%w;
    else ax=clampi(ax,0,w-1);
    const size_t ai=((size_t)ay*w+ax)*4u;
    const size_t bi=((size_t)y*bw+x)*4u;
    if(originalB[bi+3]==0)continue;
    const unsigned r=originalB[bi],g=originalB[bi+1],blue=originalB[bi+2];
    const unsigned greenRGB[3]={
     (18u*r+19u*g+11u*blue)/100u,
     std::min(255u,(57u*r+82u*g+20u*blue)/100u),
     (12u*r+26u*g+27u*blue)/100u
    };
    for(int ch=0;ch<3;++ch){
     const unsigned coverage=(unsigned)source[ai+3];
     // Even opaque A contributes only 30% to the greener B palette.
     // This avoids red rock or orange foliage popping through initially.
     const unsigned match=(greenRGB[ch]*(255u-coverage)+
                            source[ai+ch]*coverage+127u)/255u;
     const unsigned tinted=(greenRGB[ch]*179u+match*77u+128u)/256u;
     greenMatchedB[bi+ch]=(Uint8)(
         (tinted*232u+originalB[bi+ch]*24u+128u)/256u);
    }
   }
  }
  pixelsB=originalB;
  a=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                      SDL_TEXTUREACCESS_STREAMING,w,h);
  b=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                      SDL_TEXTUREACCESS_STREAMING,bw,bh);
  if(!a||!b)return false;
  SDL_SetTextureBlendMode(a,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(b,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(a,SDL_ScaleModeNearest);
  SDL_SetTextureScaleMode(b,SDL_ScaleModeNearest);
  if(SDL_UpdateTexture(a,nullptr,source.data(),w*4)!=0)return false;
  lastTint=-1;
  if(!updateTint(0))return false;
  std::fprintf(stderr,"XM87.320 AKUMA_LAYER A=%s %dx%d B=%s %dx%d initialGreenB=1 staticScenery=1\n",
               nameA,w,h,nameB,bw,bh);
  return true;
 }
 bool updateTint(int step){
  step=clampi(step,0,64);
  if(step==lastTint)return true;
  // B only: preserve authored alpha, interpolate green to native red.
  if(step==64)pixelsB=originalB;
  else{
   const int remaining=64-step;
   for(size_t k=0;k<originalB.size();k+=4u){
    for(int channel=0;channel<3;++channel){
     const int original=(int)originalB[k+channel];
     const int diff=(int)greenMatchedB[k+channel]-original;
     pixelsB[k+channel]=(Uint8)(original+diff*remaining/64);
    }
   }
  }
  if(SDL_UpdateTexture(b,nullptr,pixelsB.data(),bw*4)!=0)return false;
  lastTint=step;return true;
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
 // Saved and retained across rematches/renderer restarts.
 int moonOffset=240;
 bool moonSettingsLoaded=false,moonDirty=false,arrowWasHeld=false;
 unsigned long long lastArrowFrame=~0ull;
 std::string moonSettingsPath;
 void saveMoon(){
  if(!moonDirty||moonSettingsPath.empty())return;
  std::ofstream out(moonSettingsPath.c_str(),std::ios::trunc);
  if(!out){std::fprintf(stderr,"XM87.320 AKUMA_MOON_SAVE_FAILED path=%s\n",moonSettingsPath.c_str());return;}
  out<<moonOffset<<"\n";
  if(out.good()){moonDirty=false;
   std::fprintf(stderr,"XM87.320 AKUMA_MOON_SAVED offset=%d\n",moonOffset);
  }
 }
 void loadMoon(){
  if(moonSettingsLoaded)return;
  moonSettingsLoaded=true;
  char* pref=SDL_GetPrefPath("XMVSF","Stages");
  if(pref){
   moonSettingsPath=std::string(pref)+"akuma_moon_offset.txt";
   SDL_free(pref);
   std::ifstream in(moonSettingsPath.c_str());
   int v=0;if(in>>v)moonOffset=clampi(v,-768,768);
  }
  std::fprintf(stderr,"XM87.320 AKUMA_MOON_POSITION offset=%d\n",moonOffset);
 }
 void adjustMoon(unsigned long long frame){
  const Uint8* keys=SDL_GetKeyboardState(nullptr);
  if(!keys)return;
  const bool left=keys[SDL_SCANCODE_LEFT]!=0;
  const bool right=keys[SDL_SCANCODE_RIGHT]!=0;
  if(frame!=lastArrowFrame&&left!=right){
   const int next=clampi(moonOffset+(right?1:-1),-768,768);
   if(next!=moonOffset){moonOffset=next;moonDirty=true;}
  }
  lastArrowFrame=frame;
  if(arrowWasHeld&&!left&&!right)saveMoon();
  arrowWasHeld=left||right;
 }

 void close(){
  saveMoon();
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
   std::fprintf(stderr,"XM87.320 AKUMA_ASSETS_MISSING _art/sf3akuma/sf3akuma1_a1.png\n");
   failed=true;return false;
  }
  const char* a[]={"sf3akuma1_a1.png","sf3akuma1_a2.png","sf3akuma1_a3.png"};
  const char* b[]={"sf3akuma1_b.png","sf3akuma1_b2.png","sf3akuma1_b3.png"};
  bool good=true;
  for(int i=0;i<3&&good;++i)good=layers[i].load(renderer,root,a[i],b[i],i==2);
  // The horizontal authoring width is fixed. The art may extend above
  // 471 px after the artist adds sky pixels: accept the new ACTUAL height.
  // Both A and B independently anchor their last row to the same floor.
  if(good && (layers[0].w!=STAGE_W||layers[0].bw!=STAGE_W||
              layers[0].h<VIEW_H||layers[0].bh<VIEW_H)){
   std::fprintf(stderr,"XM87.320 AKUMA_GROUND_SIZE_INVALID expectedWidth=%d A=%dx%d B=%dx%d\n",
                STAGE_W,layers[0].w,layers[0].h,layers[0].bw,layers[0].bh);
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
   std::fprintf(stderr,"XM87.320 AKUMA_TEXTURE_FAILED %s\n",SDL_GetError());
   close();failed=true;return false;
  }
  loaded=true;
  loadMoon();
  std::fprintf(stderr,"XM87.320 AKUMA_READY root=%s groundA=%dx%d groundB=%dx%d moonA=%dx%d moonB=%dx%d delay=%d transition=%d bottom=1 cloudScale=50pct greenB=1 alphaFade=1\n",
               root.c_str(),layers[0].w,layers[0].h,layers[0].bw,layers[0].bh,
               layers[1].w,layers[1].h,layers[1].bw,layers[1].bh,
               COMBAT_WAIT_FRAMES,TRANSITION_FRAMES);
  return true;
 }
 void newSession(unsigned gen){
  generation=gen;combatStart=~0ull;previousStatus=0;lastProgress=-1;
  // When restarting a match, restore all A alpha without reloading
  // sprites/assets. No scene or framebuffer destruction.
  for(Layer& layer:layers)if(layer.a)SDL_SetTextureAlphaMod(layer.a,255);
 }
 int progress(unsigned long long frame,unsigned status,unsigned gen){
  if(generation!=gen)newSession(gen);
  if(status>=2u && status<8u && combatStart!=~0ull &&
     previousStatus>=8u)newSession(gen);
  if(status==8u && combatStart==~0ull){
   combatStart=frame;
   std::fprintf(stderr,"XM87.320 AKUMA_COMBAT_START frame=%llu fadeStart=+%d complete=+%d\n",
                frame,COMBAT_WAIT_FRAMES,COMBAT_WAIT_FRAMES+TRANSITION_FRAMES);
  }
  previousStatus=status;
  if(combatStart==~0ull||frame<=combatStart+COMBAT_WAIT_FRAMES)return 0;
  const unsigned long long age=frame-combatStart-COMBAT_WAIT_FRAMES;
  const int elapsed=(int)std::min((unsigned long long)TRANSITION_FRAMES,age);
  const int tick=elapsed*64/TRANSITION_FRAMES;
  if(tick!=lastProgress){
   if(tick==1||tick==16||tick==32||tick==48||tick==64)
    std::fprintf(stderr,"XM87.320 AKUMA_OPACITY_FADE step=%d/64 combatFrames=%llu\n",
                 tick,frame-combatStart);
   lastProgress=tick;
  }
  return elapsed;
 }
 // No per-layer rescaling or artificial top padding. The exact native
 // bottom boundary is the same for both 768-pixel stage images, even if
 // one image now has extra rows painted ABOVE the floor.
 static SDL_Rect bottomAnchored(const SDL_Rect& view,int w,int h,
                                int cameraX,int cameraY,int parallaxPercent,int moonOffset){
  const int x=(STAGE_W-w)/2-cameraX*parallaxPercent/100+moonOffset;
  const int y=VIEW_H-h+256-cameraY;
  const int left=view.x+(int)std::lround(double(x)*view.w/VIEW_W);
  const int right=view.x+(int)std::lround(double(x+w)*view.w/VIEW_W);
  const int top=view.y+(int)std::lround(double(y)*view.h/VIEW_H);
  const int bottom=view.y+(int)std::lround(double(y+h)*view.h/VIEW_H);
  return SDL_Rect{left,top,std::max(1,right-left),std::max(1,bottom-top)};
 }
 static bool drawCloudTiles(SDL_Renderer* renderer,SDL_Texture* tex,
                            int w,int h,int cameraX,int cameraY,
                            unsigned long long frame,const SDL_Rect& view){
  if(!tex||w<1||h<1)return false;
  // Fixed 50% scale, no breathing. One third of old cloud speed.
  const int tileW=std::max(1,(w+1)/2);
  const int tileH=std::max(1,(h+1)/2);
  const int drift=(int)((frame*2ull/9ull)%(unsigned)tileW); // ~13px/sec
  const int origin=(STAGE_W-tileW)/2-cameraX*30/100+drift;
  const int first=((origin%tileW)+tileW)%tileW-tileW;
  // Center resized clouds in the original 768x471 world, then move
  // the entire sheet DOWN by 30% of the 471-pixel authoring height.
  const int y=VIEW_H-STAGE_H+(STAGE_H-tileH)/2+
              (STAGE_H*30/100)+256-cameraY;
  const int top=view.y+(int)std::lround(double(y)*view.h/VIEW_H);
  const int bottom=view.y+(int)std::lround(double(y+tileH)*view.h/VIEW_H);
  if(bottom<=top)return true;
  for(int x=first;x<VIEW_W;x+=tileW){
   if(x+tileW<=0)continue;
   const int left=view.x+(int)std::lround(double(x)*view.w/VIEW_W);
   const int right=view.x+(int)std::lround(double(x+tileW)*view.w/VIEW_W);
   if(right>left){
    const SDL_Rect dst{left,top,right-left,bottom-top};
    if(SDL_RenderCopy(renderer,tex,nullptr,&dst)!=0)return false;
   }
  }
  return true;
 }
 bool paint(SDL_Renderer* renderer,const SDL_Rect& view){
  if(!loaded || !renderer)return false;
  const int camera=clampi(xm8649Visual.nativeCameraX,0,STAGE_W-VIEW_W);
  const int camY=clampi(xm8649Visual.nativeCameraY,-14,256);
  const unsigned long long frame=xm8649Visual.frames;
  adjustMoon(frame);
  const int elapsed=progress(frame,xm8649Visual.status,
                             xm77StageRollGeneration());
  const Uint8 aAlpha=(Uint8)(255-elapsed*255/TRANSITION_FRAMES);
  const int tintStep=elapsed*64/TRANSITION_FRAMES;
  const bool clipBefore=SDL_RenderIsClipEnabled(renderer)==SDL_TRUE;
  SDL_Rect oldClip={0,0,0,0};
  if(clipBefore)SDL_RenderGetClipRect(renderer,&oldClip);
  SDL_RenderSetClipRect(renderer,&view);
  bool good=true;
  for(int i=2;i>=0&&good;--i){
   Layer& l=layers[i];
   // Clouds render farthest back; moon above clouds; ground in front.
   // Each B is drawn immediately beneath its matching A at equal depth.
   const int rate=i==2?30:(i==1?55:100);
   if(elapsed>0){
    if(!l.updateTint(tintStep)){good=false;break;}
    if(i==2)good=drawCloudTiles(renderer,l.b,l.bw,l.bh,camera,camY,frame,view);
    else{
     const SDL_Rect b=bottomAnchored(view,l.bw,l.bh,camera,camY,rate,
                                     i==1?moonOffset:0);
     good=SDL_RenderCopy(renderer,l.b,nullptr,&b)==0;
    }
   }
   if(elapsed<TRANSITION_FRAMES&&good){
    SDL_SetTextureAlphaMod(l.a,aAlpha);
    if(i==2)good=drawCloudTiles(renderer,l.a,l.w,l.h,camera,camY,frame,view);
    else{
     const SDL_Rect a=bottomAnchored(view,l.w,l.h,camera,camY,rate,
                                     i==1?moonOffset:0);
     good=SDL_RenderCopy(renderer,l.a,nullptr,&a)==0;
    }
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
  if(!good)std::fprintf(stderr,"XM87.320 AKUMA_DRAW_FAILED %s\n",SDL_GetError());
  return good;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* renderer,const SDL_Rect& view){
 if(xm77StageResolved!=5||!xm8649Visual.sceneActive||xm8649Visual.stage!=5){
  scene().saveMoon();return false;
 }
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
