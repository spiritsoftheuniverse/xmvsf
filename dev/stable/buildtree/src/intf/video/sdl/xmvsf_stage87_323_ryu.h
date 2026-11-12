#ifndef XMVSF_STAGE87_323_RYU_H
#define XMVSF_STAGE87_323_RYU_H
// Ryu Stage: original PSD coordinates, including art beyond 768x471.
// The native 384x224 viewport only CLIPS presentation; no source crop,
// scaling, retiming, layer recentering or pixel reindexing.
// Ground = world stage. Roof > Roof2 > Roof3 in visual depth.
// Every layer shares the ground's Y translation; Roof3 alone is vertically
// locked to the viewport as requested. Sky Top/Bottom tile in opposite X.
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
static inline int limit(int x,int lo,int hi){return std::max(lo,std::min(hi,x));}
struct Layer {
 SDL_Texture* tex=nullptr;
 std::string role,name;
 int order=0,x=0,y=0,w=0,h=0;
 void close(){if(tex)SDL_DestroyTexture(tex);tex=nullptr;}
};
struct Scene {
 std::vector<Layer> layers;
 SDL_Texture *fighters=nullptr,*flash=nullptr;
 bool loaded=false,failed=false;
 std::string root;
 void close(){
  for(Layer& l:layers)l.close();
  layers.clear();
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  fighters=flash=nullptr;loaded=failed=false;root.clear();
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
      "XM87.323 RYU_ASSETS_MISSING run tools/extract_ryu_stage.py\n");
   failed=true;return false;
  }
  std::ifstream manifest((root+"manifest.tsv").c_str());
  if(!manifest){failed=true;return false;}
  bool header=false,hasGround=false,hasRoof=false,hasRoof2=false,
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
   Layer l;l.role=role;l.order=std::atoi(index.c_str());
   l.x=std::atoi(x.c_str());l.y=std::atoi(y.c_str());
   l.w=std::atoi(w.c_str());l.h=std::atoi(h.c_str());l.name=name;
   if(l.w<1||l.h<1||l.w>4096||l.h>4096){close();failed=true;return false;}
   SDL_Surface* surface=IMG_Load((root+file).c_str());
   if(!surface){std::fprintf(stderr,"XM87.323 RYU_PNG_MISSING %s%s\n",
                            root.c_str(),file.c_str());
    close();failed=true;return false;
   }
   if(surface->w!=l.w||surface->h!=l.h){
    std::fprintf(stderr,"XM87.323 RYU_PNG_BOUNDS_MISMATCH %s\n",name.c_str());
    SDL_FreeSurface(surface);close();failed=true;return false;
   }
   l.tex=SDL_CreateTextureFromSurface(render,surface);SDL_FreeSurface(surface);
   if(!l.tex){close();failed=true;return false;}
   SDL_SetTextureBlendMode(l.tex,SDL_BLENDMODE_BLEND);
   SDL_SetTextureScaleMode(l.tex,SDL_ScaleModeNearest);
   if(role=="ground")hasGround=true;
   else if(role=="roof")hasRoof=true;
   else if(role=="roof2")hasRoof2=true;
   else if(role=="roof3")hasRoof3=true;
   else if(role=="skytop")hasTop=true;
   else if(role=="skybottom")hasBottom=true;
   layers.push_back(l);
  }
  if(!header||!hasGround||!hasRoof||!hasRoof2||!hasRoof3||
     !hasTop||!hasBottom){
   std::fprintf(stderr,
     "XM87.323 RYU_MANIFEST_BAD canvas=%d ground=%d roofs=%d/%d/%d skies=%d/%d\n",
     (int)header,(int)hasGround,(int)hasRoof,(int)hasRoof2,(int)hasRoof3,
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
   "XM87.323 RYU_READY layers=%zu world=768x471 clip=384x224 overflow=preserved roofZ=1,2,3 roof3Y=fixed skyTile=opposite\n",
   layers.size());
  for(const Layer& l:layers)
   std::fprintf(stderr,"XM87.323 RYU_LAYER %s %s (%d,%d) %dx%d\n",
       l.role.c_str(),l.name.c_str(),l.x,l.y,l.w,l.h);
  return true;
 }
 // Match the established Guile/FD registration: native opening camera
 // X=296 corresponds to the authored 768px world's centered 384px view.
 // At that opening reference ALL layers remain in their original PSD
 // coordinates (X-192), including negative X or overflow past 768.
 // Parallax begins only when native camera moves away from that anchor.
 static int xAt(const Layer& l,int camera,int percent){
  const int delta=limit(camera-296,-192,192);
  return l.x-192-delta*percent/100;
 }
 static SDL_Rect rect(const SDL_Rect& view,const Layer& l,
                      int camera,int cameraY,int percent,bool fixedY=false){
  const int x=xAt(l,camera,percent);
  const int y=H-WORLD_H+l.y+(fixedY?0:GROUND_CAMERA_Y-cameraY);
  const int left=view.x+(int)std::lround(double(x)*view.w/W);
  const int right=view.x+(int)std::lround(double(x+l.w)*view.w/W);
  const int top=view.y+(int)std::lround(double(y)*view.h/H);
  const int bottom=view.y+(int)std::lround(double(y+l.h)*view.h/H);
  return SDL_Rect{left,top,std::max(1,right-left),std::max(1,bottom-top)};
 }
 static bool drawTile(SDL_Renderer* r,const SDL_Rect& view,const Layer& l,
                      int camera,int cameraY,unsigned long long frame,
                      int percent,int direction){
  if(l.w<1||!l.tex)return false;
  // Continuous edge-to-edge X tiling. No Y repeats, independent wobble,
  // breathing, alpha fade or pixel removal.
  const int drift=(int)((frame/12ull)%(unsigned)l.w);
  const int origin=xAt(l,camera,percent)+direction*drift;
  const int first=((origin%l.w)+l.w)%l.w-l.w;
  const int y=H-WORLD_H+l.y+GROUND_CAMERA_Y-cameraY;
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
  // Background -> foreground, independent of the PSD's layer indexing.
  if(role=="skytop")return 0;
  if(role=="skybottom")return 1;
  if(role=="other")return 2;
  if(role=="roof3")return 3;
  if(role=="roof2")return 4;
  if(role=="ground")return 5;
  if(role=="roof")return 6;
  return 2;
 }
 bool paint(SDL_Renderer* render,const SDL_Rect& view){
  if(!load(render))return false;
  if(SDL_UpdateTexture(fighters,nullptr,xm77SpritePixels(),W*4)!=0)return false;
  const int camera=limit(xm8649Visual.nativeCameraX,0,4096);
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
   if(l.role=="skytop")ok=drawTile(render,view,l,camera,cameraY,frame,20,-1);
   else if(l.role=="skybottom")ok=drawTile(render,view,l,camera,cameraY,frame,28,+1);
   else{
    const int rate=l.role=="roof"?90:l.role=="roof2"?65:
                   l.role=="roof3"?40:l.role=="other"?60:100;
    const SDL_Rect dst=rect(view,l,camera,cameraY,rate,l.role=="roof3");
    ok=SDL_RenderCopy(render,l.tex,nullptr,&dst)==0;
   }
   if(!ok)break;
  }
  if(ok){
   xm8733Overlay(render,view);
   ok=SDL_RenderCopy(render,fighters,nullptr,&view)==0;
   const UINT8* nativeFlash=xm8757FlashPixels();
   if(nativeFlash&&SDL_UpdateTexture(flash,nullptr,nativeFlash,W*4)==0)
    ok=SDL_RenderCopy(render,flash,nullptr,&view)==0&&ok;
  }
  SDL_RenderSetClipRect(render,clipBefore?&previous:nullptr);
  if(!ok)std::fprintf(stderr,"XM87.323 RYU_DRAW_FAILED %s\n",SDL_GetError());
  return ok;
 }
};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* r,const SDL_Rect& view){
 if(xm77StageResolved!=6||!xm8649Visual.sceneActive||xm8649Visual.stage!=5)
  return false;
 if(!xm77SpriteValid||xm77SpriteWidth!=W||xm77SpriteHeight!=H||
    !xm77SpritePixels())return false;
 return scene().paint(r,view);
}
} // namespace XMRyu
#endif
