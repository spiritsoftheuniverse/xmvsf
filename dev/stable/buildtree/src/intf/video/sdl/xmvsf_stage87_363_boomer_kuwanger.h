#ifndef XMVSF_STAGE87_363_BOOMER_KUWANGER_H
#define XMVSF_STAGE87_363_BOOMER_KUWANGER_H
// Boomer Kuwanger Stage. Authored A/B/C PNGs, original native stage-05
// camera/combat, zero modification to native CPS2 stage collision.
// Draw order: C distant shaft -> rotor mid-parallax -> B near shaft sides
// -> Triblasters/shot/bullet -> A fixed platform -> native stun HUD
// -> native fighters -> native flash.
// B and C move DOWN at different real-game-frame speeds. B is split
// into independently translated left/right source slices, tiled vertically
// and outward. C tiles in both axes with slight X parallax.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "xmvsf_stage87_401_boomer_enemies.h" // rotor / Triblaster visual GIFs
#include "xmvsf_stage87_404_boomer_layers.h" // stage-only 1-0 layer control

namespace XMBoomer {
static const int VIEW_W=384,VIEW_H=224,WORLD_W=768,WORLD_H=471;
static const int GROUND_CAMERA_Y=256;
// Elevator: near shaft 30px/sec, distant shaft 12px/sec at 60fps.
// These are *game* frames, never render calls / SDL wall time.
static const unsigned NEAR_FRAMES_PER_PX=2u,FAR_FRAMES_PER_PX=5u;
static const int FAR_X_PERCENT=30;
static inline int clamp(int v,int lo,int hi){
 return std::max(lo,std::min(hi,v));
}
static inline int positiveMod(long long value,int period){
 if(period<=0)return 0;
 const long long r=value%period;
 return (int)(r<0?r+period:r);
}
static inline SDL_Rect rect(const SDL_Rect& view,int x,int y,int w,int h){
 const int l=view.x+(int)std::lround(double(x)*view.w/VIEW_W);
 const int r=view.x+(int)std::lround(double(x+w)*view.w/VIEW_W);
 const int t=view.y+(int)std::lround(double(y)*view.h/VIEW_H);
 const int b=view.y+(int)std::lround(double(y+h)*view.h/VIEW_H);
 return SDL_Rect{l,t,r-l,b-t};
}
struct Layer {
 SDL_Texture* tex=nullptr;
 int w=0,h=0;
 void close(){if(tex)SDL_DestroyTexture(tex);tex=nullptr;w=h=0;}
};
struct Scene {
 Layer a,b,c;
 XMBoomerExtra::State enemies;
 SDL_Texture *fighters=nullptr,*flash=nullptr;
 bool failed=false;
 unsigned failedGeneration=~0u;
 // Final authored offsets are loaded read-only; no arrow key adjustment.
 int stageOffsetY=0,wallSpread=0;
 bool placementLoaded=false;
 // 87.404: mirror wall B's CAMERA-Y change for world-attached sprites.
 // Sampling the first frame keeps initial top-edge spawn screen-relative;
 // later jumps move sprites WITH the painted wall, not with the fighter.
 bool cameraAnchorKnown=false;
 unsigned cameraAnchorGeneration=~0u;
 unsigned long long cameraLastFrame=~0ull;
 int cameraAnchorY=GROUND_CAMERA_Y;

 // 1..9 are local Boomer painter layer toggles, like the existing Wily
 // number-key audit. 0 toggles truly clean scenery-only F6 captures.
 // Indices: C,rotor,B,gun,shot,bullet,A,stun,fighter/flash.
 bool layerOn[9]={true,true,true,true,true,true,true,true,true};
 bool sceneryOnly=false;
 unsigned layerGeneration=~0u;
 bool visible(int layer)const{
  return XM87404Layers::shown(layer,
          layer>=0&&layer<9?layerOn[layer]:false,sceneryOnly);
 }
 void syncLayerGeneration(){
  const unsigned gen=xm77StageRollGeneration();
  if(layerGeneration==gen)return;
  layerGeneration=gen;
  for(bool& on:layerOn)on=true;
  sceneryOnly=false;
  std::fprintf(stderr,
   "XM87.410 BOOMER_LAYERS_RESET generation=%u all_visible=1\n",gen);
 }
 // One physical SDL KEYDOWN per toggle; no paint-loop key polling.
 void toggleDigit(unsigned k){
  if(k>9u)return;
  syncLayerGeneration();
  static const char* const names[9]={
   "distant C","rotors","B walls","Triblasters","shots",
   "bullets","A platform","stun HUD","fighters and flash"};
  if(k==0u){
   sceneryOnly=!sceneryOnly;
   std::fprintf(stderr,
     "XM87.410 BOOMER_LAYERS_0 scenery_only=%d F6_captures_clean_stage=1\n",
     int(sceneryOnly));
  }else{
   layerOn[k-1u]=!layerOn[k-1u];
   std::fprintf(stderr,
     "XM87.410 BOOMER_LAYER_%u name=%s enabled=%d clean=%d\n",
     k,names[k-1u],int(layerOn[k-1u]),int(sceneryOnly));
  }
 }
 void loadPlacement(){
  if(placementLoaded)return;
  placementLoaded=true;
  char* pref=SDL_GetPrefPath("XMVSF","Stages");
  if(pref){
   const std::string placementPath=std::string(pref)+"boomer_placement.txt";
   SDL_free(pref);
   std::ifstream in(placementPath.c_str());
   int y=0,spread=0;
   if(in>>y>>spread){
    stageOffsetY=clamp(y,-256,256);
    wallSpread=clamp(spread,-384,384);
   }
  }
  std::fprintf(stderr,"XM87.366 BOOMER_TARGET_LOCKED a_y=%d wall_spread=%d\n",
               stageOffsetY,wallSpread);
 }
 void close(){
  enemies.close();
  cameraAnchorKnown=false;cameraAnchorGeneration=~0u;
  cameraLastFrame=~0ull;
  for(bool& on:layerOn)on=true;
  layerGeneration=~0u;
  sceneryOnly=false;
  a.close();b.close();c.close();
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  fighters=flash=nullptr;
  failed=false;failedGeneration=~0u;
 }
 static bool exists(const std::string& p){
  std::ifstream f(p.c_str(),std::ios::binary);return f.good();
 }
 static std::string locate(){
  std::vector<std::string> roots={
   "_art/boomerkuwanger/","../_art/boomerkuwanger/",
   "../../_art/boomerkuwanger/"};
  char* base=SDL_GetBasePath();
  if(base){
   std::string p(base);SDL_free(base);
   roots.push_back(p+"../../_art/boomerkuwanger/");
   roots.push_back(p+"../_art/boomerkuwanger/");
   roots.push_back(p+"_art/boomerkuwanger/");
  }
  for(const std::string& p:roots)
   if(exists(p+"a.png")&&exists(p+"b.png")&&exists(p+"c.png"))
    return p;
  return "";
 }
 static bool loadLayer(SDL_Renderer* r,const std::string& path,Layer& out){
  SDL_Surface* s=IMG_Load(path.c_str());
  if(!s)return false;
  out.w=s->w;out.h=s->h;
  if(out.w<=0||out.h<=0){
   SDL_FreeSurface(s);return false;
  }
  out.tex=SDL_CreateTextureFromSurface(r,s);
  SDL_FreeSurface(s);
  if(!out.tex)return false;
  SDL_SetTextureBlendMode(out.tex,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(out.tex,SDL_ScaleModeNearest);
  return true;
 }
 bool load(SDL_Renderer* r){
  if(a.tex&&b.tex&&c.tex&&fighters&&flash)return true;
  if(failed && failedGeneration==xm77StageRollGeneration())return false;
  if(!r)return false;
  const std::string root=locate();
  if(root.empty()){
   failed=true;failedGeneration=xm77StageRollGeneration();
   std::fprintf(stderr,"XM87.363 BOOMER_ASSETS_MISSING _art/boomerkuwanger/{a,b,c}.png; native stage fallback\n");
   return false;
  }
  if(!loadLayer(r,root+"a.png",a)||
     !loadLayer(r,root+"b.png",b)||
     !loadLayer(r,root+"c.png",c)){
   a.close();b.close();c.close();
   failed=true;failedGeneration=xm77StageRollGeneration();
   std::fprintf(stderr,"XM87.363 BOOMER_PNG_LOAD_FAILED %s %s\n",
                root.c_str(),IMG_GetError());
   return false;
  }
  fighters=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,
                             SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
  flash=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
  if(!fighters||!flash){
   if(fighters)SDL_DestroyTexture(fighters);
   if(flash)SDL_DestroyTexture(flash);
   fighters=flash=nullptr;
   a.close();b.close();c.close();
   failed=true;failedGeneration=xm77StageRollGeneration();
   return false;
  }
  SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(fighters,SDL_ScaleModeNearest);
  SDL_SetTextureScaleMode(flash,SDL_ScaleModeNearest);
  failed=false;failedGeneration=~0u;
  loadPlacement();
  // GIF assets are an isolated decorative layer: if animated sprites are
  // unavailable, preserve the existing working A/B/C stage and combat.
  enemies.load(r,root);
  std::fprintf(stderr,
   "XM87.363 BOOMER_READY A=%dx%d B=%dx%d C=%dx%d world=768x471 scrollDown B=1/2f C=1/5f C_x=30%%\n",
   a.w,a.h,b.w,b.h,c.w,c.h);
  return true;
 }
 // Render only the tiles intersecting the native 384x224 viewport.
 // Texture and source surfaces are never generated inside this loop.
 static bool tile(SDL_Renderer* r,const SDL_Rect& view,
                  const Layer& l,int cameraX,int top,
                  unsigned long long frame,unsigned framesPerPx,
                  int cameraPercent){
  if(!l.tex||l.w<=0||l.h<=0)return false;
  const long long drift=(long long)((frame/framesPerPx)%unsigned(l.h));
  const int baseX=(WORLD_W-l.w)/2-cameraX*cameraPercent/100;
  const long long baseY=(long long)top+WORLD_H-l.h+drift;
  const int firstX=positiveMod(baseX,l.w)-l.w;
  const int firstY=positiveMod(baseY,l.h)-l.h;
  for(int y=firstY;y<VIEW_H;y+=l.h){
   if(y+l.h<=0)continue;
   for(int x=firstX;x<VIEW_W;x+=l.w){
    if(x+l.w<=0)continue;
    const SDL_Rect d=rect(view,x,y,l.w,l.h);
    if(d.w<=0||d.h<=0||SDL_RenderCopy(r,l.tex,nullptr,&d)!=0)
     return false;
   }
  }
  return true;
 }
 // B remains split at runtime using two SDL source rectangles. Every
 // half tiles vertically and outward only, keeping a real central gap.
 // No PNG rewriting, per-frame decode or duplicate GPU textures.
 static bool drawWallHalf(SDL_Renderer* r,const SDL_Rect& view,
                          const Layer& wall,int cameraX,int stageTop,
                          unsigned long long frame,int spread,bool left){
  if(!wall.tex||wall.w<2||wall.h<1)return false;
  const int mid=wall.w/2;
  const SDL_Rect src=left?SDL_Rect{0,0,mid,wall.h}:
                          SDL_Rect{mid,0,wall.w-mid,wall.h};
  const int w=src.w;
  const int drift=int((frame/NEAR_FRAMES_PER_PX)%unsigned(wall.h));
  const int firstY=positiveMod(
       (long long)stageTop+WORLD_H-wall.h+drift,wall.h)-wall.h;
  const int baseX=(WORLD_W-wall.w)/2-cameraX;
  const int edgeX=baseX+(left?-spread:mid+spread);
  for(int y=firstY;y<VIEW_H;y+=wall.h){
   if(y+wall.h<=0)continue;
   if(left){
    for(int x=edgeX;x+w>0;x-=w){
     if(x>=VIEW_W)continue;
     const SDL_Rect dst=rect(view,x,y,w,wall.h);
     if(dst.w<1||dst.h<1||
        SDL_RenderCopy(r,wall.tex,&src,&dst)!=0)return false;
    }
   }else{
    for(int x=edgeX;x<VIEW_W;x+=w){
     if(x+w<=0)continue;
     const SDL_Rect dst=rect(view,x,y,w,wall.h);
     if(dst.w<1||dst.h<1||
        SDL_RenderCopy(r,wall.tex,&src,&dst)!=0)return false;
    }
   }
  }
  return true;
 }
 bool paint(SDL_Renderer* r,const SDL_Rect& view){
  if(!load(r))return false;
  if(SDL_UpdateTexture(fighters,nullptr,xm77SpritePixels(),VIEW_W*4)!=0)
   return false;
  const int camX=clamp(xm8649Visual.nativeCameraX,0,WORLD_W-VIEW_W);
  const int camY=clamp(xm8649Visual.nativeCameraY,-14,GROUND_CAMERA_Y);
  const int stageTop=VIEW_H-WORLD_H+GROUND_CAMERA_Y-camY;
  const unsigned long long frame=xm8649Visual.frames;
  const unsigned gen=xm77StageRollGeneration();
  syncLayerGeneration();
  if(!cameraAnchorKnown || cameraAnchorGeneration!=gen ||
     (cameraLastFrame!=~0ull && frame<cameraLastFrame)){
   // Never carry hidden fighters or scenery-only screenshot mode to a
   // different match or restored replay timeline.
   cameraAnchorKnown=true;cameraAnchorGeneration=gen;
   cameraAnchorY=camY;
   std::fprintf(stderr,
       "XM87.404 BOOMER_CAMERA_ANCHOR frame=%llu camera_y=%d\n",
       (unsigned long long)frame,cameraAnchorY);
  }
  cameraLastFrame=frame;
  // Wall B's screen Y is stageTop; this is exactly the change in
  // stageTop since first appearance, so props stay fixed to B on jump.
  const int stageYShift=XM87401Motion::wallCameraYOffset(cameraAnchorY,camY);
  const bool oldClip=SDL_RenderIsClipEnabled(r)==SDL_TRUE;
  SDL_Rect old={0,0,0,0};
  if(oldClip)SDL_RenderGetClipRect(r,&old);
  SDL_RenderSetClipRect(r,&view);
  enemies.update(frame,gen,camX,stageYShift);
  bool ok=true;
  if(visible(0))ok=tile(r,view,c,camX,stageTop,frame,
                        FAR_FRAMES_PER_PX,FAR_X_PERCENT);
  // Mid-parallax rotors draw above C, behind split B.
  if(ok && visible(1))ok=enemies.drawRotors(r,view,frame,camX,stageYShift);
  if(ok && visible(2))ok=drawWallHalf(r,view,b,camX,stageTop,frame,wallSpread,true);
  if(ok && visible(2))ok=drawWallHalf(r,view,b,camX,stageTop,frame,wallSpread,false);
  // Original Triblasters and projectile sprites occupy the B wall plane.
  if(ok && (visible(3)||visible(4)||visible(5)))
   ok=enemies.drawGuns(r,view,frame,camX,stageYShift,
                       visible(3),visible(4),visible(5));
  // A remains registered to the original world and camera. Only
  // its saved debug Y displacement changes the visible PNG.
  if(ok && visible(6)){
   const SDL_Rect d=rect(view,(WORLD_W-a.w)/2-camX,
                         stageTop+WORLD_H-a.h+stageOffsetY,a.w,a.h);
   ok=d.w>0 && d.h>0 && SDL_RenderCopy(r,a.tex,nullptr,&d)==0;
  }
  if(ok && visible(7))xm8733Overlay(r,view);
  if(ok && visible(8)){
   ok=SDL_RenderCopy(r,fighters,nullptr,&view)==0;
   const UINT8* nativeFlash=xm8757FlashPixels();
   if(nativeFlash && SDL_UpdateTexture(flash,nullptr,nativeFlash,
                                      VIEW_W*4)==0)
    ok=SDL_RenderCopy(r,flash,nullptr,&view)==0 && ok;
  }
  SDL_RenderSetClipRect(r,oldClip?&old:nullptr);
  if(!ok)std::fprintf(stderr,"XM87.363 BOOMER_DRAW_FAILED %s\n",
                      SDL_GetError());
  return ok;
 }
};
static inline Scene& scene(){static Scene s;return s;}
// Common cutout for every SDL-host HUD and notification pass.
static inline bool sdlHudVisible(){
 return xm77StageResolved!=7 || !xm8649Visual.sceneActive ||
        xm8649Visual.stage!=5 ||
        scene().visible(XM87404Layers::StunHUD);
}
static inline bool sceneryOnly(){
 return xm77StageResolved==7 && xm8649Visual.sceneActive &&
        xm8649Visual.stage==5 && scene().sceneryOnly;
}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* r,const SDL_Rect& view){
 if(xm77StageResolved!=7)return false;
 if(!xm8649Visual.sceneActive||xm8649Visual.stage!=5)return false;
 if(!xm77SpriteValid||xm77SpriteWidth!=VIEW_W||
    xm77SpriteHeight!=VIEW_H||!xm77SpritePixels())return false;
 return scene().paint(r,view);
}
} // namespace XMBoomer
#endif
