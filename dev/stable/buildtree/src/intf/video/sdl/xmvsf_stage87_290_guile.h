#ifndef XMVSF_STAGE87_290_GUILE_H
#define XMVSF_STAGE87_290_GUILE_H
// Guile Stage, PSD-authored scenery. Native CPS2 stage 05 is used only for
// camera, combat, object filtering and collision. All custom art is cosmetic.
// Source: _art/guilestage/guilestage.psd. One-time extracted layers are emitted
// by tools/extract_guile_stage.py to _art/guilestage/extracted/.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace XMGuile {
// Photoshop ground is 1016x471. Render a centered 768x471 crop; never
// derive stage scale from the PSD canvas or trimmed PNG layer dimensions.
static const int VIEW_W=384,VIEW_H=224,GROUND_W=1016,GROUND_H=471;
static const int AUTHORED_W=768,AUTHORED_H=471,GROUND_CAMERA_Y=256;
static inline int clamp(int n,int lo,int hi){return std::max(lo,std::min(hi,n));}
static inline unsigned rand32(unsigned& s) {
 s^=s<<13;s^=s>>17;s^=s<<5;return s;
}
struct Layer {
 SDL_Texture* tex=nullptr;
 SDL_Texture* edge=nullptr; // cloud-edge checkerboard shimmer, sky only
 std::string role,actor,name;
 int order=0,x=0,y=0,w=0,h=0,frameNumber=0;
 void close(){
  if(tex)SDL_DestroyTexture(tex);
  if(edge)SDL_DestroyTexture(edge);
  tex=edge=nullptr;
 }
};
struct Actor {
 std::string name;
 std::vector<size_t> frames;
 unsigned rng=0x5EA0D123u;
 size_t current=0;
 int hold=6;
};
struct Scene {
 std::vector<Layer> layers;
 std::vector<Actor> actors;
 SDL_Texture* fighters=nullptr;
 SDL_Texture* flash=nullptr;
 int canvasW=0,canvasH=0;
 int groundX=0,groundY=0,groundW=0,groundH=0;
 int cameraBaseX=0;
 bool cameraBaseSet=false;
 bool failed=false;
 bool active=false;
 unsigned long long lastFrame=~0ull;
 std::string root;

 void close(){
  for(Layer& l:layers)l.close();
  layers.clear();actors.clear();
  if(fighters)SDL_DestroyTexture(fighters);
  if(flash)SDL_DestroyTexture(flash);
  fighters=flash=nullptr;
  canvasW=canvasH=0;groundX=groundY=groundW=groundH=0;
  cameraBaseX=0;cameraBaseSet=false;
  failed=false;active=false;lastFrame=~0ull;
  root.clear();
 }
 void resetMotion(){
  active=false;lastFrame=~0ull;cameraBaseSet=false;
  for(Actor& a:actors){a.current=0;a.hold=6+(int)(rand32(a.rng)%7u);}
 }
 static std::vector<std::string> split(const std::string& line){
  std::vector<std::string> fields;
  std::istringstream in(line);std::string part;
  while(std::getline(in,part,'\t'))fields.push_back(part);
  return fields;
 }
 static bool fileExists(const std::string& p){
  std::ifstream f(p.c_str(),std::ios::binary);
  return f.good();
 }
 static std::string locate(){
  std::vector<std::string> prefixes;
  prefixes.push_back("_art/guilestage/extracted/");
  prefixes.push_back("../_art/guilestage/extracted/");
  prefixes.push_back("../../_art/guilestage/extracted/");
  char* base=SDL_GetBasePath();
  if(base){
   const std::string b(base);SDL_free(base);
   prefixes.push_back(b+"../../_art/guilestage/extracted/");
   prefixes.push_back(b+"../_art/guilestage/extracted/");
   prefixes.push_back(b+"_art/guilestage/extracted/");
  }
  for(const std::string& p:prefixes)
   if(fileExists(p+"manifest.tsv"))return p;
  return "";
 }
 static SDL_Texture* makeCloudEdge(SDL_Renderer* renderer,SDL_Surface* source){
  SDL_Surface* rgba=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_RGBA32,0);
  if(!rgba)return nullptr;
  SDL_Surface* edge=SDL_CreateRGBSurfaceWithFormat(0,rgba->w,rgba->h,32,SDL_PIXELFORMAT_RGBA32);
  if(!edge){SDL_FreeSurface(rgba);return nullptr;}
  SDL_FillRect(edge,nullptr,SDL_MapRGBA(edge->format,0,0,0,0));
  const unsigned bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
  for(int y=1;y+1<rgba->h;++y){
   for(int x=1;x+1<rgba->w;++x){
    const Uint8* p=(const Uint8*)rgba->pixels+(size_t)y*rgba->pitch+x*4;
    const Uint8* n=(const Uint8*)rgba->pixels+(size_t)(y-1)*rgba->pitch+x*4;
    const Uint8* d=(const Uint8*)rgba->pixels+(size_t)(y+1)*rgba->pitch+x*4;
    // Alpha edges, or high-contrast/light cloud boundaries in opaque skies.
    const int l=(int(p[0])+int(p[1])+int(p[2]))/3;
    const int lu=(int(n[0])+int(n[1])+int(n[2]))/3;
    const int ld=(int(d[0])+int(d[1])+int(d[2]))/3;
    const bool cloudEdge=(std::abs(int(p[3])-int(n[3]))>18 ||
      std::abs(int(p[3])-int(d[3]))>18 ||
      (l>100&&(std::abs(l-lu)>25||std::abs(l-ld)>25)));
    if(!cloudEdge||bayer[y&3][x&3]>=10u)continue;
    Uint8* q=(Uint8*)edge->pixels+(size_t)y*edge->pitch+x*4;
    q[0]=p[0];q[1]=p[1];q[2]=p[2];q[3]=(Uint8)std::min(85,int(p[3])/3);
   }
  }
  SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer,edge);
  SDL_FreeSurface(edge);SDL_FreeSurface(rgba);
  if(tex){SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(tex,SDL_ScaleModeNearest);}
  return tex;
 }
 bool load(SDL_Renderer* renderer){
  if(failed)return false;
  if(fighters && !layers.empty())return true;
  if(!renderer)return false;
  root=locate();
  if(root.empty()){
   std::fprintf(stderr,
      "XM87.290 GUILE_ASSETS_MISSING run python tools/extract_guile_stage.py; native stage fallback\n");
   failed=true;return false;
  }
  std::ifstream manifest((root+"manifest.tsv").c_str());
  if(!manifest){failed=true;return false;}
  std::string line;
  std::map<std::string,size_t> actorIndex;
  unsigned count[4]={};
  int formatVersion=0;
  while(std::getline(manifest,line)){
   if(line.empty()||line[0]=='#')continue;
   const std::vector<std::string> f=split(line);
   if(f.size()==3&&f[0]=="CANVAS"){
    canvasW=std::atoi(f[1].c_str());canvasH=std::atoi(f[2].c_str());
   }else if(f.size()==2&&f[0]=="FORMAT"){
    formatVersion=std::atoi(f[1].c_str());
   }else if(f.size()==5&&f[0]=="GROUND"){
    groundX=std::atoi(f[1].c_str());groundY=std::atoi(f[2].c_str());
    groundW=std::atoi(f[3].c_str());groundH=std::atoi(f[4].c_str());
   }
  }
  if(formatVersion!=3||groundW!=GROUND_W||groundH!=GROUND_H){
   std::fprintf(stderr,
    "XM87.293 GUILE_STALE_MANIFEST format=%d ground=(%d,%d %dx%d); regenerate with extractor\n",
    formatVersion,groundX,groundY,groundW,groundH);
   failed=true;return false;
  }
  // The first 9 columns define image bounds; the remaining entries retain
  // original PSD pixel bounds, layer name, full group path and visibility.
  manifest.clear();manifest.seekg(0);
  while(std::getline(manifest,line)){
   if(line.empty()||line[0]=='#')continue;
   const std::vector<std::string> f=split(line);
   if(f.size()<16||f[0]!="LAYER")continue;
   Layer l;
   l.role=f[1];l.actor=f[2];l.order=std::atoi(f[3].c_str());
   l.name=f[9];
   // Numerical frame order is independent of Photoshop stacking order.
   size_t digit=l.name.size();
   while(digit>0&&l.name[digit-1]>='0'&&l.name[digit-1]<='9')--digit;
   l.frameNumber=digit<l.name.size()?std::atoi(l.name.c_str()+digit):0;
   l.x=std::atoi(f[4].c_str());l.y=std::atoi(f[5].c_str());
   l.w=std::atoi(f[6].c_str());l.h=std::atoi(f[7].c_str());
   if(l.w<1||l.h<1||l.w>8192||l.h>8192)continue;
   const std::string path=root+f[8];
   SDL_Surface* surface=IMG_Load(path.c_str());
   if(!surface){
    std::fprintf(stderr,"XM87.290 GUILE_PNG_MISSING %s (%s)\n",
                 path.c_str(),IMG_GetError());
    close();failed=true;return false;
   }
   l.tex=SDL_CreateTextureFromSurface(renderer,surface);
   if(l.role=="sky")l.edge=makeCloudEdge(renderer,surface);
   SDL_FreeSurface(surface);
   if(!l.tex){close();failed=true;return false;}
   SDL_SetTextureBlendMode(l.tex,SDL_BLENDMODE_BLEND);
   SDL_SetTextureScaleMode(l.tex,SDL_ScaleModeNearest);
   const size_t index=layers.size();
   if(l.role=="actor"){
    auto it=actorIndex.find(l.actor);
    if(it==actorIndex.end()){
     Actor a;a.name=l.actor;
     for(char c:l.actor)a.rng=(a.rng^unsigned((unsigned char)c))*16777619u;
     if(!a.rng)a.rng=0x5EA0D123u;
     actorIndex[l.actor]=actors.size();actors.push_back(a);
     it=actorIndex.find(l.actor);
    }
    actors[it->second].frames.push_back(index);
   }else if(l.role=="sky")++count[0];
   else if(l.role=="bg")++count[1];
   else if(l.role=="ground")++count[2];
   layers.push_back(l);
  }
  if(!canvasW||!canvasH||layers.empty()||!count[2]||actors.empty()){
   std::fprintf(stderr,"XM87.290 GUILE_MANIFEST_INVALID source=%dx%d layers=%zu ground=%u\n",
                canvasW,canvasH,layers.size(),count[2]);
   close();failed=true;return false;
  }
  fighters=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
       SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
  flash=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
       SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
  if(!fighters||!flash){close();failed=true;return false;}
  SDL_SetTextureBlendMode(fighters,SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(flash,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(fighters,SDL_ScaleModeNearest);
  SDL_SetTextureScaleMode(flash,SDL_ScaleModeNearest);
  for(Actor& a:actors){
   std::stable_sort(a.frames.begin(),a.frames.end(),
       [&](size_t i,size_t j){return layers[i].frameNumber<layers[j].frameNumber;});
   a.hold=6+(int)(rand32(a.rng)%7u);
  }
  std::fprintf(stderr,
    "XM87.290 GUILE_READY root=%s canvas=%dx%d sky=%u bg=%u ground=%u actors=%zu textures=%zu\n",
    root.c_str(),canvasW,canvasH,count[0],count[1],count[2],
    actors.size(),layers.size());
  return true;
 }
 void step(unsigned long long frame){
  if(!active||frame<lastFrame){
   resetMotion();active=true;
  }
  const unsigned long long delta=(lastFrame==~0ull)?1ull:
           std::min(3600ull,frame-lastFrame);
  lastFrame=frame;
  for(Actor& actor:actors){
   if(actor.frames.size()<2u)continue;
   for(unsigned long long t=0;t<delta;++t){
    if(--actor.hold<=0){
     actor.current=(actor.current+1u)%actor.frames.size();
     actor.hold=6+(int)(rand32(actor.rng)%7u);
    }
   }
  }
 }
 // Native camera delta pans inside the 248 authored pixels outside the
 // centered 768px crop. Always START centered, regardless of the ROM camera
 // coordinate at the first rendered stage frame.
 int groundCrop(int nativeCameraX){
  if(!cameraBaseSet){cameraBaseX=nativeCameraX;cameraBaseSet=true;}
  return clamp((groundW-AUTHORED_W)/2+(nativeCameraX-cameraBaseX),
               0,groundW-AUTHORED_W);
 }
 bool drawTexture(SDL_Renderer* render,SDL_Texture* tex,
                  const Layer& l,double sourceCropX,int cameraY,
                  const SDL_Rect& view)const{
  if(!tex)return true;
  // x/y describe ORIGINAL PSD coordinates. Characters' transparent padding
  // lives in their PNGs. No layer-specific auto-centering or scale-to-fit.
  const double sourceX=double(l.x-groundX)-sourceCropX;
  const double sourceY=double(l.y-groundY)+
                       double(GROUND_CAMERA_Y-cameraY)*double(AUTHORED_H)/VIEW_H;
  SDL_Rect dst{
   view.x+(int)std::lround(sourceX*view.w/AUTHORED_W),
   view.y+(int)std::lround(sourceY*view.h/AUTHORED_H),
   std::max(1,(int)std::lround(double(l.w)*view.w/AUTHORED_W)),
   std::max(1,(int)std::lround(double(l.h)*view.h/AUTHORED_H))
  };
  return SDL_RenderCopy(render,tex,nullptr,&dst)==0;
 }
 bool drawSky(SDL_Renderer* render,const Layer& l,
              unsigned long long frame,int cameraY,const SDL_Rect& view)const{
  const int tile=std::max(1,l.w);
  // Rightward drift is 3 source pixels per second at 60 Hz, endless wrap.
  // Respect the sky layer's ORIGINAL PSD X offset relative to ground.
  const int shift=int((frame/20u)%unsigned(tile));
  const int centerCrop=(groundW-AUTHORED_W)/2;
  const int relative=l.x-groundX-centerCrop+shift;
  const int origin=((relative%tile)+tile)%tile-tile;
  const double skyY=double(l.y-groundY)+
                   double(GROUND_CAMERA_Y-cameraY)*0.20*AUTHORED_H/VIEW_H;
  const int height=std::max(1,(int)std::lround(double(l.h)*view.h/AUTHORED_H));
  const int top=view.y+(int)std::lround(skyY*view.h/AUTHORED_H);
  // The 8px bands move independently by <=1 source pixel. This gives clouds
  // a restrained, deterministic wiggle with a dithered bright-edge overlay.
  for(int x=origin;x<VIEW_W+tile;x+=tile){
   if(x+tile<0)continue;
   // Bound draw calls even for a high-resolution PSD: <=28 sky bands.
   const int band=std::max(8,(l.h+27)/28);
   for(int row=0;row<l.h;row+=band){
    const int lines=std::min(band,l.h-row);
    const double cycle=double(frame)/137.0+double(row)/29.0+double(l.order);
    const int wobble=(int)std::lround(std::sin(cycle));
    const SDL_Rect src{0,row,l.w,lines};
    const SDL_Rect dst{
     view.x+(int)std::lround(double(x+wobble)*view.w/AUTHORED_W),
     top+(int)std::lround(double(row)/l.h*height),
     std::max(1,(int)std::lround(double(tile)*view.w/AUTHORED_W)),
     std::max(1,(int)std::lround(double(lines)/l.h*height)+1)
    };
    if(SDL_RenderCopy(render,l.tex,&src,&dst)!=0)return false;
    if(l.edge){
     SDL_Rect edgeDst=dst;
     edgeDst.x+=(int)std::lround(double((int((frame/17u+unsigned(row))%3u)-1))*view.w/AUTHORED_W);
     SDL_SetTextureAlphaMod(l.edge,(Uint8)(80u+unsigned(frame/11u+row)%5u*28u));
     if(SDL_RenderCopy(render,l.edge,&src,&edgeDst)!=0)return false;
    }
   }
  }
  return true;
 }

};
static inline Scene& scene(){static Scene s;return s;}
static inline void cleanup(){scene().close();}
static inline bool paint(SDL_Renderer* render,const SDL_Rect& view){
 if(xm77StageResolved!=3||!xm8649Visual.sceneActive||xm8649Visual.stage!=5){
  if(scene().active)scene().resetMotion();
  return false;
 }
 if(!xm77SpriteValid||xm77SpriteWidth!=VIEW_W||xm77SpriteHeight!=VIEW_H||
    !xm77SpritePixels())return false;
 Scene& s=scene();
 if(!s.load(render))return false;
 if(SDL_UpdateTexture(s.fighters,nullptr,xm77SpritePixels(),VIEW_W*4)!=0){
  std::fprintf(stderr,"XM87.290 GUILE_FIGHTER_UPLOAD_FAILED %s\n",SDL_GetError());
  return false;
 }
 // Stun UI lives behind native sprites, exactly like Wily and FD.
 // Draw static/animated scenery first, then HUD, then native fighters/FX.
 const unsigned long long frame=xm8649Visual.frames;
 s.step(frame);
 const bool hadClip=SDL_RenderIsClipEnabled(render)==SDL_TRUE;
 SDL_Rect oldClip={0,0,0,0};
 if(hadClip)SDL_RenderGetClipRect(render,&oldClip);
 SDL_RenderSetClipRect(render,&view);
 const double crop=double(s.groundCrop(xm8649Visual.nativeCameraX));
 const double centerCrop=double((s.groundW-AUTHORED_W)/2);
 const double bgCrop=centerCrop+(crop-centerCrop)*0.45;
 const int cameraY=clamp(xm8649Visual.nativeCameraY,-14,256);
 bool good=true;
 for(auto it=s.layers.rbegin();it!=s.layers.rend()&&good;++it)
  if(it->role=="sky")good=s.drawSky(render,*it,frame,cameraY,view);
 for(auto it=s.layers.rbegin();it!=s.layers.rend()&&good;++it)
  if(it->role=="bg")good=s.drawTexture(render,it->tex,*it,bgCrop,cameraY,view);
 for(auto it=s.layers.rbegin();it!=s.layers.rend()&&good;++it)
  if(it->role=="ground")good=s.drawTexture(render,it->tex,*it,crop,cameraY,view);
 for(const Actor& a:s.actors){
  if(!good||a.frames.empty())continue;
  const Layer& l=s.layers[a.frames[a.current]];
  good=s.drawTexture(render,l.tex,l,crop,cameraY,view);
 }
 if(good){
  xm8733Overlay(render,view);
  good=SDL_RenderCopy(render,s.fighters,nullptr,&view)==0;
  const UINT8* nativeFlash=xm8757FlashPixels();
  if(good&&nativeFlash&&SDL_UpdateTexture(s.flash,nullptr,nativeFlash,VIEW_W*4)==0)
   good=SDL_RenderCopy(render,s.flash,nullptr,&view)==0;
 }
 SDL_RenderSetClipRect(render,hadClip?&oldClip:nullptr);
 if(!good)std::fprintf(stderr,"XM87.290 GUILE_DRAW_FAILED %s\n",SDL_GetError());
 return good;
}
} // namespace XMGuile
#endif
