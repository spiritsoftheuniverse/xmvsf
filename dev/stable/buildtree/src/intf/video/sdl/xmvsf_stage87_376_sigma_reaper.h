#ifndef XMVSF_STAGE87_376_SIGMA_REAPER_H
#define XMVSF_STAGE87_376_SIGMA_REAPER_H
// 87.376: Reaper Sigma, independent stage-only actor (not a ROM fighter).
// Animations are extracted losslessly from reapersigma.psd by the local
// build. Coordinates are in the 768x471 Sigma world, bottom anchored.
// Tick exactly once per original game frame, NOT per SDL paint call.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace XM87376Reaper {
static const int WORLD_WIDTH=768, WORLD_HEIGHT=471;
static const int LEFT_BOUND=140,RIGHT_BOUND=628;
static const int BODY_ABOVE_BOTTOM=140;
static const double PI=3.1415926535897932384626433832795;

struct Frame {
 SDL_Texture* texture=nullptr;
 int x=0,y=0,w=0,h=0; // alpha-cropped PSD original layer bounds
 void close(){
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;x=y=w=h=0;
 }
};
enum Motion {IDLE=0,SMASH=1,SLASH=2,BALL=3,BALL_HOLD=4};

struct Actor {
 std::array<Frame,3> idle{};
 std::array<Frame,4> smash{};
 std::array<Frame,3> run{};
 std::array<Frame,8> ball{};
 Frame body;
 bool loaded=false,failed=false;
 unsigned failedGeneration=~0u,stageGeneration=~0u,lastFrame=~0u;
 std::uint32_t random=0u;
 double x=384.0;
 int facing=-1; // idle sheet faces left, slash sheet faces RIGHT
 Motion motion=IDLE;
 unsigned modeFrame=0u,decisionIn=300u,hoverFrame=0u;

 void close(){
  for(auto& f:idle)f.close();
  for(auto& f:smash)f.close();
  for(auto& f:run)f.close();
  for(auto& f:ball)f.close();
  body.close();
  loaded=failed=false;
  failedGeneration=stageGeneration=lastFrame=~0u;
  random=0u;x=384.0;facing=-1;motion=IDLE;
  modeFrame=hoverFrame=0u;decisionIn=300u;
 }
 std::uint32_t random32(){
  // Deterministic local animation RNG; does not touch game state/ROM RNG.
  random^=random<<13;
  random^=random>>17;
  random^=random<<5;
  return random;
 }
 unsigned range(unsigned a,unsigned b){
  return a+(random32()%(b-a+1u));
 }
 void enterStage(unsigned generation,unsigned frame){
  stageGeneration=generation;
  lastFrame=frame;
  random=0xA3B1C2D5u^(generation*747796405u)^(frame*2891336453u);
  if(!random)random=0x91E10DA5u;
  x=384.0+double(int(range(0u,160u))-80);
  x=std::max(double(LEFT_BOUND),std::min(double(RIGHT_BOUND),x));
  facing=-1;motion=IDLE;
  modeFrame=hoverFrame=0u;
  decisionIn=range(240u,420u); // 4-7 game seconds at 60 fps
  std::fprintf(stderr,
    "XM87.376 SIGMA_REAPER_SPAWN gen=%u worldX=%.1f "
    "bounds=140..628 yAboveBottom=140 nextDecision=%u\n",
    generation,x,decisionIn);
 }
 void goIdle(){
  motion=IDLE;modeFrame=0u;
  decisionIn=range(240u,420u);
 }
 void choose(){
  const unsigned outcome=range(0u,99u);
  if(outcome<5u){
   // Rare: ball body remains; 2..9 animate above it.
   motion=BALL;modeFrame=0u;
   std::fprintf(stderr,"XM87.376 SIGMA_REAPER_BALL x=%.1f\n",x);
  }else if(outcome<30u){
   // Slash always toward the farther remaining wall, irrespective
   // of the direction he was facing when the decision was made.
   const double left=x-LEFT_BOUND,right=RIGHT_BOUND-x;
   facing=(right>=left)?1:-1;
   motion=SMASH;modeFrame=0u;
   std::fprintf(stderr,
     "XM87.376 SIGMA_REAPER_SLASH x=%.1f toward=%s left=%.1f right=%.1f\n",
     x,facing>0?"RIGHT":"LEFT",left,right);
  }else{
   facing=-facing;
   decisionIn=range(240u,420u);
  }
 }
 void advance(){
  ++hoverFrame;
  ++modeFrame;
  if(motion==IDLE){
   x+=double(facing)*0.55; // native world pixels / game frame
   if(x<=LEFT_BOUND){
    x=LEFT_BOUND;facing=1;
    decisionIn=range(240u,420u);
   }else if(x>=RIGHT_BOUND){
    x=RIGHT_BOUND;facing=-1;
    decisionIn=range(240u,420u);
   }
   if(decisionIn)--decisionIn;
   if(!decisionIn)choose();
  }else if(motion==SMASH){
   // All four authored startup frames are displayed exactly once.
   // Idle hovering stops at startup, not when the movement begins.
   if(modeFrame>=4u*6u){
    motion=SLASH;modeFrame=0u;
   }
  }else if(motion==SLASH){
   x+=double(facing)*3.8;
   if(x<=LEFT_BOUND || x>=RIGHT_BOUND){
    x=(facing<0)?LEFT_BOUND:RIGHT_BOUND;
    // On arrival, face inward for the next normal idle patrol.
    facing=-facing;
    goIdle();
   }
  }else if(motion==BALL){
   if(modeFrame>=8u*7u){
    motion=BALL_HOLD;modeFrame=0u;
   }
  }else if(motion==BALL_HOLD){
   if(modeFrame>=12u)goIdle();
  }
 }
 void update(unsigned generation,unsigned frame){
  if(stageGeneration!=generation || lastFrame==~0u || frame<lastFrame){
   enterStage(generation,frame);return;
  }
  if(frame==lastFrame)return;
  const unsigned elapsed=frame-lastFrame;
  // Advance in genuine game-frame steps, never multiple times because
  // the renderer was invoked twice in one frame. Large skips are capped
  // to avoid a long catch-up if a replay jumps elsewhere.
  for(unsigned i=0u;i<std::min(elapsed,600u);++i)advance();
  lastFrame=frame;
 }
 Frame* role(const std::string& name){
  if(name=="ball1body")return &body;
  for(int i=1;i<=3;++i)
   if(name=="idle"+std::to_string(i))return &idle[i-1];
  for(int i=1;i<=4;++i)
   if(name=="smash"+std::to_string(i))return &smash[i-1];
  for(int i=5;i<=7;++i)
   if(name=="slashrun"+std::to_string(i))return &run[i-5];
  for(int i=2;i<=9;++i)
   if(name=="ball"+std::to_string(i))return &ball[i-2];
  return nullptr;
 }
 static std::vector<std::string> roots(){
  std::vector<std::string> names={
   "_art/sigmastage/extracted/",
   "../_art/sigmastage/extracted/",
   "../../_art/sigmastage/extracted/"
  };
  char* b=SDL_GetBasePath();
  if(b){
   const std::string base(b);SDL_free(b);
   names.push_back(base+"../../_art/sigmastage/extracted/");
   names.push_back(base+"../_art/sigmastage/extracted/");
   names.push_back(base+"_art/sigmastage/extracted/");
  }
  return names;
 }
 bool load(SDL_Renderer* renderer,unsigned generation){
  if(loaded)return true;
  if(failed && failedGeneration==generation)return false;
  failed=false;
  if(!renderer)return false;
  for(const auto& root:roots()){
   std::ifstream input((root+"manifest.tsv").c_str());
   if(!input)continue;
   std::vector<std::string> rows;
   std::string line;
   while(std::getline(input,line))rows.push_back(line);
   bool success=true;
   unsigned total=0u;
   for(const auto& row:rows){
    if(row.compare(0,6,"FRAME\t")!=0)continue;
    std::istringstream stream(row);
    std::string marker,name,file,px,py,pw,ph;
    if(!std::getline(stream,marker,'\t') ||
       !std::getline(stream,name,'\t') ||
       !std::getline(stream,file,'\t') ||
       !std::getline(stream,px,'\t') ||
       !std::getline(stream,py,'\t') ||
       !std::getline(stream,pw,'\t') ||
       !std::getline(stream,ph,'\t')){
      success=false;break;
    }
    Frame* dst=role(name);
    if(!dst||dst->texture){success=false;break;}
    if(file.find('/')!=std::string::npos ||
       file.find('\\')!=std::string::npos ||
       file.find("..")!=std::string::npos){
      success=false;break;
    }
    // No exceptions (FBNeo supports -fno-exceptions build variants).
    // A malformed/stale manifest simply disables this stage actor.
    int x=0,y=0,w=0,h=0;
    char tail=0;
    if(std::sscanf(px.c_str(),"%d%c",&x,&tail)!=1 ||
       std::sscanf(py.c_str(),"%d%c",&y,&tail)!=1 ||
       std::sscanf(pw.c_str(),"%d%c",&w,&tail)!=1 ||
       std::sscanf(ph.c_str(),"%d%c",&h,&tail)!=1){
      success=false;break;
    }
    if(w<=0||h<=0||w>2048||h>2048){success=false;break;}
    SDL_Surface* s=IMG_Load((root+file).c_str());
    if(!s || s->w!=w || s->h!=h){
     if(s)SDL_FreeSurface(s);
     success=false;break;
    }
    dst->texture=SDL_CreateTextureFromSurface(renderer,s);
    SDL_FreeSurface(s);
    if(!dst->texture){success=false;break;}
    dst->x=x;dst->y=y;dst->w=w;dst->h=h;
    SDL_SetTextureBlendMode(dst->texture,SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(dst->texture,SDL_ScaleModeNearest);
    ++total;
   }
   for(auto& f:idle)success=success && f.texture;
   for(auto& f:smash)success=success && f.texture;
   for(auto& f:run)success=success && f.texture;
   for(auto& f:ball)success=success && f.texture;
   success=success && body.texture && total==19u;
   if(success){
    loaded=true;
    std::fprintf(stderr,
      "XM87.376 SIGMA_REAPER_READY frames=%u root=%s bounds=140..628\n",
      total,root.c_str());
    return true;
   }
   // A stale/incomplete manifest cannot leave half of the textures
   // from one root when probing another installation directory.
   for(auto& f:idle)f.close();
   for(auto& f:smash)f.close();
   for(auto& f:run)f.close();
   for(auto& f:ball)f.close();
   body.close();
  }
  failed=true;failedGeneration=generation;
  std::fprintf(stderr,
    "XM87.376 SIGMA_REAPER_MISSING: run BUILD_TEST.bat to extract "
    "_art/sigmastage/reapersigma.psd into extracted/manifest.tsv "
    "and 19 named PNGs; stage background remains usable\n");
  return false;
 }
 // Stage coordinates include 768px of horizontal art. Sprite Y stays
 // 140px above the artwork's BOTTOM, regardless of image transparency.
 bool drawFrame(SDL_Renderer* renderer,const SDL_Rect& viewport,
                const Frame& frame,double xLeft,double bottom,
                int cameraX,int cameraY,bool flip)const {
  if(!frame.texture)return false;
  const double sx=double(viewport.w)/384.0;
  const double sy=double(viewport.h)/224.0;
  const int xx=viewport.x+int(std::lround((xLeft-cameraX)*sx));
  const int yy=viewport.y+int(std::lround((
       224.0+256.0-double(cameraY)-double(BODY_ABOVE_BOTTOM)
       +(bottom-double(WORLD_HEIGHT-BODY_ABOVE_BOTTOM))
       -double(frame.h))*sy));
  const int ww=std::max(1,int(std::lround(double(frame.w)*sx)));
  const int hh=std::max(1,int(std::lround(double(frame.h)*sy)));
  const SDL_Rect dst={xx,yy,ww,hh};
  return SDL_RenderCopyEx(renderer,frame.texture,nullptr,&dst,
                          0.0,nullptr,flip?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE)==0;
 }
 bool paint(SDL_Renderer* renderer,const SDL_Rect& view,
            unsigned generation,unsigned gameFrame,int cameraX,int cameraY){
  if(!load(renderer,generation))return false;
  update(generation,gameFrame);
  const bool slash=motion==SMASH||motion==SLASH;
  const double hover=slash?0.0:
     12.0*std::sin(double(hoverFrame)*2.0*PI/210.0);
  const double foot=double(WORLD_HEIGHT-BODY_ABOVE_BOTTOM)+hover;
  if(motion==BALL || motion==BALL_HOLD){
   // ball1body is ALWAYS visible, while frames 2..9 pass in front.
   const double bodyLeft=x-double(body.w)*0.5;
   const bool flip=facing>0; // ball body inherits idle-facing sheet
   bool ok=drawFrame(renderer,view,body,bodyLeft,foot,
                     cameraX,cameraY,flip);
   if(motion==BALL){
    const Frame& orb=ball[std::min(7u,modeFrame/7u)];
    const double dx=double(orb.x-body.x);
    const double ox=flip?
        bodyLeft+double(body.w)-(dx+orb.w):bodyLeft+dx;
    const double orbBottom=foot-double(body.h)
       +double(orb.y-body.y)+double(orb.h);
    ok=drawFrame(renderer,view,orb,ox,orbBottom,
                 cameraX,cameraY,flip) && ok;
   }
   return ok;
  }
  const Frame* f=nullptr;
  bool flip=false;
  if(motion==IDLE){
   static const unsigned seq[]={0u,1u,2u,1u,0u};
   f=&idle[seq[(modeFrame/11u)%5u]];
   flip=facing>0; // PSD idle originally faces LEFT
  }else if(motion==SMASH){
   f=&smash[std::min(3u,modeFrame/6u)];
   flip=facing<0; // PSD slash originally travels RIGHT
  }else{
   static const unsigned seq[]={0u,1u,2u,1u,0u};
   f=&run[seq[(modeFrame/6u)%5u]];
   flip=facing<0;
  }
  return drawFrame(renderer,view,*f,x-double(f->w)*0.5,foot,
                   cameraX,cameraY,flip);
 }
};
inline Actor& actor(){static Actor a;return a;}
inline void cleanup(){actor().close();}
} // namespace XM87376Reaper
#endif
