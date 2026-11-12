#ifndef XMVSF_STAGE87_290_GUILE_H
#define XMVSF_STAGE87_290_GUILE_H
// Guile Stage, PSD-authored scenery. Native CPS2 stage 05 is used only for
// camera, combat, object filtering and collision. All custom art is cosmetic.
// Source: _art/guilestage/guilestage.psd. One-time extracted layers are emitted
// by tools/extract_guile_stage.py to _art/guilestage/extracted/.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace XMGuile {
// Display the native 384x224 game viewport inside the authored 768x471
// world window. Ground/actors/BG use ONE PSD pixel for ONE native CPS2
// pixel: never shrink 768 PSD pixels to 384 while fighters stay 1:1.
// The 1016x471 ground raster can extend without altering source positions.
static const int VIEW_W=384,VIEW_H=224;
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
 int bgTopY=0; // PSD source top of BG: sky's fixed-in-world bottom seam
 int cropStartX=0;
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
  canvasW=canvasH=0;groundX=groundY=groundW=groundH=bgTopY=0;
  cropStartX=0;
  failed=false;active=false;lastFrame=~0ull;
  root.clear();
 }
 void resetMotion(){
  active=false;lastFrame=~0ull;
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
  int formatVersion=0,viewW=0,viewH=0;
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
   }else if(f.size()==4&&f[0]=="VIEW"){
    viewW=std::atoi(f[1].c_str());viewH=std::atoi(f[2].c_str());
    cropStartX=std::atoi(f[3].c_str());
   }
  }
  if(formatVersion!=3||groundW<AUTHORED_W||groundH<AUTHORED_H||
     groundW>8192||groundH>8192||viewW!=AUTHORED_W||
     viewH!=AUTHORED_H||cropStartX<0||
     cropStartX>groundW-AUTHORED_W){
   std::fprintf(stderr,
    "XM87.300 GUILE_STALE_MANIFEST format=%d ground=(%d,%d %dx%d); regenerate with extractor\n",
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
  // Register sky to the original Photoshop BG top. Prefer the named
  // BG layer over auxiliary scenery also categorized as background.
  bool foundBg=false,foundNamedBg=false;
  bgTopY=groundY;
  for(const Layer& l:layers){
   if(l.role!="bg")continue;
   std::string name=l.name;
   std::transform(name.begin(),name.end(),name.begin(),
     [](unsigned char c){return (char)std::tolower(c);});
   const bool namedBg=(name=="bg"||name=="background");
   if(!foundBg||(namedBg&&!foundNamedBg)||
      (namedBg==foundNamedBg&&l.y<bgTopY)){
    bgTopY=l.y;foundBg=true;foundNamedBg=namedBg;
   }
  }
  std::fprintf(stderr,
   "XM87.300 GUILE_SKY_SEAM bg_top_source_y=%d ground_y=%d named_bg=%d camera_y_lock=ground\n",
   bgTopY,groundY,(int)foundNamedBg);
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
    "XM87.300 GUILE_READY root=%s canvas=%dx%d ground=(%d,%d %dx%d) crop=768x471 at x=%d sky=%u bg=%u ground=%u actors=%zu textures=%zu\n",
    root.c_str(),canvasW,canvasH,groundX,groundY,groundW,groundH,cropStartX,
    count[0],count[1],count[2],actors.size(),layers.size());
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
 // The Guile composition was centered on the 87.109 first-live-stage
 // camera (point X=462, opponent X=514, native camera X=296). Preserve
 // that authored registration exactly, but follow EVERY subsequent native
 // camera sample at a direct one-to-one world-pixel displacement.
 // Never latch an origin from the first pre-fight paint (the camera may not
 // have initialized yet): that produces a match-start jump and run-dependent
 // offsets. This reference is fixed, not a simulated/following camera.
 static constexpr int OPENING_NATIVE_CAMERA_X=296;
 int centerCrop()const{
  return cropStartX+(AUTHORED_W-VIEW_W)/2;
 }
 int groundCrop(int nativeCameraX)const{
  return clamp(centerCrop()+(nativeCameraX-OPENING_NATIVE_CAMERA_X),
               cropStartX,cropStartX+AUTHORED_W-VIEW_W);
 }
 bool drawTexture(SDL_Renderer* render,SDL_Texture* tex,
                  const Layer& l,double sourceCropX,int cameraY,
                  const SDL_Rect& view)const{
  if(!tex)return true;
  // World units are ORIGINAL PSD pixels, identical to native sprite pixels.
  // Pin Ground's last authored row to the native viewport bottom at Y=256.
  // Rising cameras move scenery DOWN, as on the established FD stage.
  // Ground and BG MUST share this exact vertical transform: BG only
  // parallax-scrolls in X, never in Y. 471 is WORLD height, never
  // scaled-to-fit into the 224px viewport.
  const double sourceX=double(l.x-groundX)-sourceCropX;
  const double sourceY=double(l.y-groundY)+double(VIEW_H-groundH)+
                       double(GROUND_CAMERA_Y-cameraY);
  SDL_Rect dst{
   view.x+(int)std::lround(sourceX*view.w/VIEW_W),
   view.y+(int)std::lround(sourceY*view.h/VIEW_H),
   std::max(1,(int)std::lround(double(l.w)*view.w/VIEW_W)),
   std::max(1,(int)std::lround(double(l.h)*view.h/VIEW_H))
  };
  return SDL_RenderCopy(render,tex,nullptr,&dst)==0;
 }
 bool drawSky(SDL_Renderer* render,const Layer& l,
              unsigned long long frame,int sourceCropX,int cameraY,
              const SDL_Rect& view)const{
  if(!l.tex||l.w<1||l.h<1)return false;
  // The source sky now tiles at its left/right edges. Draw each tile as
  // ONE continuous image: the old independent 8px bands moved sideways
  // by different amounts and their +1px overlaps flickered as seams.
  // Keep exactly the 87.299 world-Y seam and horizontal camera/parallax.
  const int tileW=l.w,tileH=l.h;
  const int shift=int((frame/20u)%unsigned(tileW));
  const int skyCameraX=centerCrop()+(sourceCropX-centerCrop())/5;
  const int localX=l.x-groundX-skyCameraX+shift;
  const int originX=((localX%tileW)+tileW)%tileW-tileW;
  const int skyBottomY=bgTopY-groundY+VIEW_H-groundH+
                       (GROUND_CAMERA_Y-cameraY);
  const int visibleBottom=clamp(skyBottomY,0,VIEW_H);
  if(visibleBottom<=0)return true;
  // A shared 1px drift preserves gentle sky motion while leaving adjacent
  // tiles and all their rows aligned on exactly the same pixel boundary.
  const int wobble=(int)std::lround(
      std::sin(double(frame)/137.0+double(l.order)));
  if(l.edge)SDL_SetTextureAlphaMod(l.edge,100);
  for(int x=originX;x<VIEW_W;x+=tileW){
   if(x+tileW<=0)continue;
   const int destLeft=view.x+(int)std::lround(
       double(x+wobble)*view.w/VIEW_W);
   const int destRight=view.x+(int)std::lround(
       double(x+wobble+tileW)*view.w/VIEW_W);
   if(destRight<=destLeft)continue;
   // Preserve original vertical world registration. Do not move sky,
   // BG, ground, actors, or the BG seam relative to one another.
   for(int y=skyBottomY-tileH;y+tileH>0;y-=tileH){
    if(y>=VIEW_H)continue;
    const int top=std::max(0,y);
    const int bottom=std::min(visibleBottom,y+tileH);
    if(bottom<=top)continue;
    const SDL_Rect src{0,top-y,tileW,bottom-top};
    const int destTop=view.y+(int)std::lround(
        double(top)*view.h/VIEW_H);
    const int destBottom=view.y+(int)std::lround(
        double(bottom)*view.h/VIEW_H);
    if(destBottom<=destTop)continue;
    const SDL_Rect dst{destLeft,destTop,destRight-destLeft,
                       destBottom-destTop};
    if(SDL_RenderCopy(render,l.tex,&src,&dst)!=0)return false;
    // Static checkerboard cloud edge: same footprint as the sky tile.
    // No independently wobbling strip or alpha blink at tile joins.
    if(l.edge&&SDL_RenderCopy(render,l.edge,&src,&dst)!=0)
     return false;
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
 const double centerCrop=double(s.centerCrop());
 const double bgCrop=centerCrop+(crop-centerCrop)*0.45;
 const int cameraY=clamp(xm8649Visual.nativeCameraY,-14,256);
 bool good=true;
 for(auto it=s.layers.rbegin();it!=s.layers.rend()&&good;++it)
  if(it->role=="sky")good=s.drawSky(render,*it,frame,(int)crop,cameraY,view);
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
