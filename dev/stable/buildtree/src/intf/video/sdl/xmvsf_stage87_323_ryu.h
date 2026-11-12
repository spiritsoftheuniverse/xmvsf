#ifndef XMVSF_STAGE87_323_RYU_H
#define XMVSF_STAGE87_323_RYU_H
// TEST 87.325: _art/ryustage/ryustage.png is the authoritative ground,
// an unmodified 768x471 world/camera holder. The PSD contributes only
// original-position parallax (sky and roof); PSD Ground/Layer 2 are NOT
// composited. Back-to-front: Sky Top, Sky Bottom, Roof3, Roof2, Roof,
// ground PNG, native HUD, native fighters/FX. Roof3 uses the SAME native
// camera Y as the ground, while Roof/2 move vertically at slower rates.
// All horizontal motion is native camera-relative, no player midpoint.
// Original PSD out-of-canvas X/Y coordinates are preserved and clipped
// only to the 384x224 display; no scaling/smoothing of source pixels.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace XMRyu {
static const int W=384,H=224,WORLD_W=768,WORLD_H=471;
static const int GROUND_CAMERA_Y=256;
static const int OPENING_NATIVE_CAMERA_X=296;
static const int CENTER_WORLD_CROP_X=(WORLD_W-W)/2;
// Slower X shift toward the rear of the scene; tied to Ground's
// actual native camera displacement (never an independent clock).
static const int ROOF_X_PERCENT=45,ROOF2_X_PERCENT=27,ROOF3_X_PERCENT=13;
// Roof3 stays ground-locked in Y. Roof1 and Roof2 slide over it
// during native super-jumps. Sky remains locked in Y to Ground.
static const int ROOF_Y_PERCENT=50,ROOF2_Y_PERCENT=75,ROOF3_Y_PERCENT=100;
static inline int limit(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}
struct Layer {
 SDL_Texture* tex=nullptr;
 std::string role,name;
 int order=0,x=0,y=0,w=0,h=0;
 void close(){if(tex)SDL_DestroyTexture(tex);tex=nullptr;}
};
struct Scene {
 std::vector<Layer> layers;
 SDL_Texture *ground=nullptr,*fighters=nullptr,*flash=nullptr;
 bool loaded=false,failed=false;
 std::string root;
 void close(){
  for(Layer& l:layers)l.close();
  layers.clear();
  if(ground)SDL_DestroyTexture(ground);
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  ground=fighters=flash=nullptr;loaded=failed=false;root.clear();
 }
 static bool exists(const std::string& path){
  std::ifstream f(path.c_str(),std::ios::binary);return f.good();
 }
 static std::string locate(){
  std::vector<std::string> dirs={
    "_art/ryustage/extracted/","../_art/ryustage/extracted/",
    "../../_art/ryustage/extracted/"};
  char* base=SDL_GetBasePath();
  if(base){std::string p(base);SDL_free(base);
   dirs.push_back(p+"../../_art/ryustage/extracted/");
   dirs.push_back(p+"../_art/ryustage/extracted/");
   dirs.push_back(p+"_art/ryustage/extracted/");
  }
  for(const std::string& p:dirs)if(exists(p+"manifest.tsv"))return p;
  return "";
 }
 bool load(SDL_Renderer* render){
  if(loaded)return true;
  if(failed||!render)return false;
  root=locate();
  if(root.empty()){std::fprintf(stderr,
      "XM87.325 RYU_ASSETS_MISSING extracted manifest absent; run tools/extract_ryu_stage.py\n");
   failed=true;return false;
  }
  // The PSD's old Ground layer is deliberately not used: the authored
  // 768x471 PNG is the sole ground, making stage alignment deterministic.
  const std::string groundPath=root+"../ryustage.png";
  SDL_Surface* groundSurface=IMG_Load(groundPath.c_str());
  if(!groundSurface){
   std::fprintf(stderr,"XM87.325 RYU_GROUND_PNG_MISSING %s: %s\n",
                groundPath.c_str(),IMG_GetError());
   failed=true;return false;
  }
  if(groundSurface->w!=WORLD_W||groundSurface->h!=WORLD_H){
   std::fprintf(stderr,"XM87.325 RYU_GROUND_BOUNDS_BAD %s got=%dx%d need=768x471\n",
       groundPath.c_str(),groundSurface->w,groundSurface->h);
   SDL_FreeSurface(groundSurface);failed=true;return false;
  }
  ground=SDL_CreateTextureFromSurface(render,groundSurface);
  SDL_FreeSurface(groundSurface);
  if(!ground){failed=true;return false;}
  SDL_SetTextureBlendMode(ground,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(ground,SDL_ScaleModeNearest);
  std::ifstream manifest((root+"manifest.tsv").c_str());
  if(!manifest){close();failed=true;return false;}
  bool header=false,hasRoof=false,hasRoof2=false,
       hasRoof3=false,hasTop=false,hasBottom=false;
  std::string line;
  while(std::getline(manifest,line)){
   if(line=="CANVAS\t768\t471")header=true;
   if(line.compare(0,6,"LAYER\t")!=0)continue;
   std::istringstream ss(line);std::string tag,role,index,x,y,w,h,file,name;
   std::getline(ss,tag,'\t');std::getline(ss,role,'\t');
   std::getline(ss,index,'\t');std::getline(ss,x,'\t');
   std::getline(ss,y,'\t');std::getline(ss,w,'\t');
   std::getline(ss,h,'\t');std::getline(ss,file,'\t');
   std::getline(ss,name);
   // The unnamed PSD construction layer may be opaque black; never
   // cover the parallax or override the standalone ground with it.
   if(role!="roof" && role!="roof2" && role!="roof3" &&
      role!="skytop" && role!="skybottom")continue;
   Layer l;l.role=role;l.order=std::atoi(index.c_str());
   l.x=std::atoi(x.c_str());l.y=std::atoi(y.c_str());
   l.w=std::atoi(w.c_str());l.h=std::atoi(h.c_str());l.name=name;
   if(l.w<1||l.h<1||l.w>4096||l.h>4096){close();failed=true;return false;}
   SDL_Surface* surface=IMG_Load((root+file).c_str());
   if(!surface){std::fprintf(stderr,"XM87.325 RYU_PNG_MISSING %s%s\n",
                            root.c_str(),file.c_str());
    close();failed=true;return false;
   }
   if(surface->w!=l.w||surface->h!=l.h){
    std::fprintf(stderr,"XM87.325 RYU_PNG_BOUNDS_MISMATCH %s\n",name.c_str());
    SDL_FreeSurface(surface);close();failed=true;return false;
   }
   l.tex=SDL_CreateTextureFromSurface(render,surface);SDL_FreeSurface(surface);
   if(!l.tex){close();failed=true;return false;}
   SDL_SetTextureBlendMode(l.tex,SDL_BLENDMODE_BLEND);
   SDL_SetTextureScaleMode(l.tex,SDL_ScaleModeNearest);
   if(role=="roof")hasRoof=true;
   else if(role=="roof2")hasRoof2=true;
   else if(role=="roof3")hasRoof3=true;
   else if(role=="skytop")hasTop=true;
   else if(role=="skybottom")hasBottom=true;
   layers.push_back(l);
  }
  if(!header||!hasRoof||!hasRoof2||!hasRoof3||!hasTop||!hasBottom){
   std::fprintf(stderr,
     "XM87.325 RYU_MANIFEST_BAD canvas=%d roofs=%d/%d/%d skies=%d/%d\n",
     (int)header,(int)hasRoof,(int)hasRoof2,(int)hasRoof3,
     (int)hasTop,(int)hasBottom);
   close();failed=true;return false;
  }
  fighters=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_STREAMING,W,H);
  flash=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
      SDL_TEXTUREACCESS_STREAMING,W,H);
  if(!fighters||!flash){close();failed=true;return false;}
  SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(fighters,SDL_ScaleModeNearest);
  SDL_SetTextureScaleMode(flash,SDL_ScaleModeNearest);
  loaded=true;
  std::fprintf(stderr,
   "XM87.325 RYU_READY ground=ryustage.png world=768x471 layers=%zu clip=384x224 roofZ=3,2,1,GROUND roofXY=%d,%d roof2XY=%d,%d roof3XY=%d,%d skyTile=opposite\n",
   layers.size(),ROOF_X_PERCENT,ROOF_Y_PERCENT,ROOF2_X_PERCENT,ROOF2_Y_PERCENT,
   ROOF3_X_PERCENT,ROOF3_Y_PERCENT);
  for(const Layer& l:layers)
   std::fprintf(stderr,"XM87.325 RYU_LAYER %s %s (%d,%d) %dx%d\n",
       l.role.c_str(),l.name.c_str(),l.x,l.y,l.w,l.h);
  return true;
 }
 // Native stage-05 world X is authoritative. The PNG's 768px canvas
 // covers the full stage width; center a 384px window at opening X296
 // and follow each subsequent ROM camera displacement 1:1.
 static int groundCrop(int camera){
  return limit(CENTER_WORLD_CROP_X+(camera-OPENING_NATIVE_CAMERA_X),
               0,WORLD_W-W);
 }
 // Every PSD layer keeps its absolute authored X. Moving the camera
 // slows as the layer recedes: Roof=45%, Roof2=27%, Roof3=13%.
 static int xAt(const Layer& l,int crop,int xPercent){
  return l.x-CENTER_WORLD_CROP_X-
         (crop-CENTER_WORLD_CROP_X)*xPercent/100;
 }
 static int yAt(int layerTop,int cameraY,int yPercent){
  return H-WORLD_H+layerTop+(GROUND_CAMERA_Y-cameraY)*yPercent/100;
 }
 static SDL_Rect scaledRect(const SDL_Rect& view,int x,int y,int w,int h){
  const int left=view.x+(int)std::lround(double(x)*view.w/W);
  const int right=view.x+(int)std::lround(double(x+w)*view.w/W);
  const int top=view.y+(int)std::lround(double(y)*view.h/H);
  const int bottom=view.y+(int)std::lround(double(y+h)*view.h/H);
  return SDL_Rect{left,top,std::max(1,right-left),std::max(1,bottom-top)};
 }
 static SDL_Rect rect(const SDL_Rect& view,const Layer& l,
                      int crop,int cameraY,int xPercent,int yPercent){
  return scaledRect(view,xAt(l,crop,xPercent),
    yAt(l.y,cameraY,yPercent),l.w,l.h);
 }
 static bool drawTile(SDL_Renderer* r,const SDL_Rect& view,const Layer& l,
                      int crop,int cameraY,unsigned long long frame,
                      int percent,int direction){
  if(l.w<1||!l.tex)return false;
  // Continuous edge-to-edge X tiling. No Y repeats, independent wobble,
  // breathing, alpha fade or pixel removal.
  const int drift=(int)((frame/12ull)%(unsigned)l.w);
  const int origin=xAt(l,crop,percent)+direction*drift;
  const int first=((origin%l.w)+l.w)%l.w-l.w;
  const int y=yAt(l.y,cameraY,100);
  const int top=view.y+(int)std::lround(double(y)*view.h/H);
  const int bottom=view.y+(int)std::lround(double(y+l.h)*view.h/H);
  if(bottom<=top)return true;
  for(int x=first;x<W;x+=l.w){
   if(x+l.w<=0)continue;
   const int left=view.x+(int)std::lround(double(x)*view.w/W);
   const int right=view.x+(int)std::lround(double(x+l.w)*view.w/W);
   if(right<=left)continue;
   const SDL_Rect dst{left,top,right-left,bottom-top};
   if(SDL_RenderCopy(r,l.tex,nullptr,&dst)!=0)return false;
  }
  return true;
 }
 static int rank(const std::string& role){
  if(role=="skytop")return 0;
  if(role=="skybottom")return 1;
  if(role=="roof3")return 2;
  if(role=="roof2")return 3;
  if(role=="roof")return 4;
  return 0;
 }
 bool paint(SDL_Renderer* render,const SDL_Rect& view){
  if(!load(render))return false;
  if(SDL_UpdateTexture(fighters,nullptr,xm77SpritePixels(),W*4)!=0)return false;
  const int camera=xm8649Visual.nativeCameraX;
  const int crop=groundCrop(camera);
  const int cameraY=limit(xm8649Visual.nativeCameraY,-14,GROUND_CAMERA_Y);
  const unsigned long long frame=xm8649Visual.frames;
  const bool clipBefore=SDL_RenderIsClipEnabled(render)==SDL_TRUE;
  SDL_Rect previous={0,0,0,0};
  if(clipBefore)SDL_RenderGetClipRect(render,&previous);
  SDL_RenderSetClipRect(render,&view);
  // Stable rank: preserve original PSD order within the same depth group.
  std::vector<size_t> indices;
  for(size_t i=0;i<layers.size();++i)indices.push_back(i);
  std::stable_sort(indices.begin(),indices.end(),
    [&](size_t a,size_t b){return rank(layers[a].role)<rank(layers[b].role);});
  bool ok=true;
  for(size_t index:indices){
   const Layer& l=layers[index];
   if(l.role=="skytop")ok=drawTile(render,view,l,crop,cameraY,frame,20,-1);
   else if(l.role=="skybottom")ok=drawTile(render,view,l,crop,cameraY,frame,28,+1);
   else{
    const int rateX=l.role=="roof"?ROOF_X_PERCENT:
           l.role=="roof2"?ROOF2_X_PERCENT:ROOF3_X_PERCENT;
    const int rateY=l.role=="roof"?ROOF_Y_PERCENT:
           l.role=="roof2"?ROOF2_Y_PERCENT:ROOF3_Y_PERCENT;
    const SDL_Rect dst=rect(view,l,crop,cameraY,rateX,rateY);
    ok=SDL_RenderCopy(render,l.tex,nullptr,&dst)==0;
   }
   if(!ok)break;
  }
  // Ground PNG MUST overdraw all roof/sky layers, but native
  // fighter/FX and existing HUD remain on their established planes.
  if(ok){
   const SDL_Rect groundDst=scaledRect(view,-crop,yAt(0,cameraY,100),
                                       WORLD_W,WORLD_H);
   ok=SDL_RenderCopy(render,ground,nullptr,&groundDst)==0;
  }
  // Log actual native X/Y and resulting parallax positions: verifies
  // horizontal movement without a synthetic camera or fighter tracking.
  if(ok && (frame<2u || frame%120u==0u)){
   std::fprintf(stderr,
    "XM87.325 RYU_CAMERA native=%d,%d crop=%d groundX=%d roofX=%d roof2X=%d roof3X=%d roof3Y=%d\n",
     camera,cameraY,crop,-crop,
     -CENTER_WORLD_CROP_X-(crop-CENTER_WORLD_CROP_X)*ROOF_X_PERCENT/100,
     -CENTER_WORLD_CROP_X-(crop-CENTER_WORLD_CROP_X)*ROOF2_X_PERCENT/100,
     -CENTER_WORLD_CROP_X-(crop-CENTER_WORLD_CROP_X)*ROOF3_X_PERCENT/100,
     yAt(0,cameraY,ROOF3_Y_PERCENT));
  }
  if(ok){
   xm8733Overlay(render,view);
   ok=SDL_RenderCopy(render,fighters,nullptr,&view)==0;
   const UINT8* nativeFlash=xm8757FlashPixels();
   if(nativeFlash&&SDL_UpdateTexture(flash,nullptr,nativeFlash,W*4)==0)
    ok=SDL_RenderCopy(render,flash,nullptr,&view)==0&&ok;
  }
  SDL_RenderSetClipRect(render,clipBefore?&previous:nullptr);
  if(!ok)std::fprintf(stderr,"XM87.325 RYU_DRAW_FAILED %s\n",SDL_GetError());
  return ok;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* r,const SDL_Rect& view){
 if(xm77StageResolved!=6)return false;
 // Verify the SAME three read-only native gates required by README.
 // Report only on a gate transition, not once per rendered frame.
 const bool stageReady=xm8649Visual.sceneActive&&xm8649Visual.stage==5;
 const bool nativeReady=xm77SpriteValid&&xm77SpriteWidth==W&&
                        xm77SpriteHeight==H&&xm77SpritePixels()!=nullptr;
 static int lastGate=-1;
 const int gate=(stageReady?1:0)|(nativeReady?2:0);
 if(gate!=lastGate){
  std::fprintf(stderr,
   "XM87.325 RYU_GATE resolved=%d native_stage=%u phase=%02X camera_active=%d sprite_valid=%d sprite=%dx%d\n",
   xm77StageResolved,xm8649Visual.stage,xm8649Visual.status,
   (int)stageReady,(int)nativeReady,xm77SpriteWidth,xm77SpriteHeight);
  lastGate=gate;
 }
 if(!stageReady||!nativeReady)return false;
 return scene().paint(r,view);
}
} // namespace XMRyu
#endif
