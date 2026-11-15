#ifndef XMVSF_STAGE87_376_SIGMA_REAPER_H
#define XMVSF_STAGE87_376_SIGMA_REAPER_H
// 87.382: persist Reaper stage diagnostics through RUN_TEST.bat,
// validate exact screen foot and particle draw pass, amplify FX visibility.
// subtle breathing and colored curl-noise energy/cloth particles.
// Manifest integers accept CRLF/whitespace on Windows; junk suffixes fail.
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
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace XM87376Reaper {
static const int WORLD_WIDTH=768, WORLD_HEIGHT=471;
static const int LEFT_BOUND=140,RIGHT_BOUND=628;
static const int ORIGINAL_ABOVE_BOTTOM=140; // prior world foot
// New foot is lowered by actual extracted ball1body height minus 20px.
static const double PI=3.1415926535897932384626433832795;
// This is intentionally a game-session file, not the append-only build log.
// RUN_TEST.bat supplies an absolute path beneath test/<version>/observer.
static inline void diagnostic(const char* fmt,...){
 char line[700]={0};
 va_list a;
 va_start(a,fmt);
 std::vsnprintf(line,sizeof(line),fmt,a);
 va_end(a);
 std::fputs(line,stderr);
 std::fflush(stderr);
 const char* path=std::getenv("XM87382_REAPER_LOG");
 if(path && *path){
  FILE* f=std::fopen(path,"ab");
  if(f){
   std::fputs(line,f);
   std::fclose(f);
  }
 }
}

struct EmissionPixel {
 std::uint16_t x=0,y=0;
 Uint8 r=0,g=0,b=0;
};
struct Frame {
 SDL_Texture* texture=nullptr;
 int x=0,y=0,w=0,h=0; // alpha-cropped PSD original layer bounds
 // Sample actual opaque sprite colors once at load. The scythe spawns
 // from GREEN pixels; the robe hem spawns from dark red/gray pixels.
 std::vector<EmissionPixel> green,hem;
 void collect(SDL_Surface* original){
  SDL_Surface* rgba=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
  if(!rgba)return;
  if(SDL_MUSTLOCK(rgba) && SDL_LockSurface(rgba)!=0){
   SDL_FreeSurface(rgba);return;
  }
  for(int yy=0;yy<rgba->h;yy+=2)for(int xx=0;xx<rgba->w;xx+=2){
   const Uint8* p=(const Uint8*)rgba->pixels+(size_t)yy*rgba->pitch+4u*xx;
   const int r=p[0],g=p[1],b=p[2],a=p[3];
   if(a<96)continue;
   const bool energy=g>=95 && g>r+20 && g>b+10;
   const bool red=r>=32 && r<=170 && r>g+10 && r>b+7 && g<115;
   const bool gray=r>=22 && r<=145 && std::abs(r-g)<22 &&
                   std::abs(g-b)<28;
   // Sparse subsampling is enough and avoids large per-frame vectors.
   if(((xx+yy*3)&3)!=0)continue;
   if(energy){
    EmissionPixel q;
    q.x=std::uint16_t(xx);q.y=std::uint16_t(yy);
    q.r=Uint8(r);q.g=Uint8(g);q.b=Uint8(b);
    if(green.size()<128u)green.push_back(q);
    else green[(unsigned(xx)*7u+unsigned(yy)*13u)%128u]=q;
   }
   if(yy>=rgba->h*55/100 && (red||gray)){
    EmissionPixel q;
    q.x=std::uint16_t(xx);q.y=std::uint16_t(yy);
    q.r=Uint8(r);q.g=Uint8(g);q.b=Uint8(b);
    if(hem.size()<128u)hem.push_back(q);
    else hem[(unsigned(xx)*17u+unsigned(yy)*11u)%128u]=q;
   }
  }
  if(SDL_MUSTLOCK(rgba))SDL_UnlockSurface(rgba);
  SDL_FreeSurface(rgba);
 }
 void close(){
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;x=y=w=h=0;
  green.clear();hem.clear();
 }
};
struct Particle {
 float x=0.0f,y=0.0f,vx=0.0f,vy=0.0f;
 unsigned life=0,initial=1;
 Uint8 r=0,g=0,b=0,opacity=0,size=1;
 bool cloak=false;
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
 unsigned slashesSinceBall=0u;
 std::array<Particle,320> particles{};
 unsigned nextParticle=0u;
 unsigned lastDebugFrame=~0u;
 unsigned emitted=0u;
 unsigned rendered=0u;
 int silhouetteH=0;

 void close(){
  for(auto& f:idle)f.close();
  for(auto& f:smash)f.close();
  for(auto& f:run)f.close();
  for(auto& f:ball)f.close();
  body.close();
  loaded=failed=false;
  failedGeneration=stageGeneration=lastFrame=~0u;
  random=0u;x=384.0;facing=-1;motion=IDLE;
  modeFrame=hoverFrame=0u;decisionIn=300u;slashesSinceBall=0u;
  particles={};nextParticle=0u;
  lastDebugFrame=~0u;emitted=rendered=0u;silhouetteH=0;
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
 int dropPixels()const{
  // The BALL body is not necessarily the complete figure; the three
  // original IDLE sheets are the authoritative full-body silhouette.
  // Keep the largest visible idle frame as the fixed stage-wide drop.
  int h=0;
  for(const Frame& f:idle)h=std::max(h,f.h);
  // Distinct idle poses can have different visible dimensions; the
  // stage actor uses a single stable offset for every animation frame.
  return std::max(0,h-20);
 }
 double footBase()const{
  return double(WORLD_HEIGHT-ORIGINAL_ABOVE_BOTTOM+dropPixels());
 }
 double hoverOffset()const{
  return (motion==SMASH||motion==SLASH)?0.0:
         12.0*std::sin(double(hoverFrame)*2.0*PI/210.0);
 }
 double breath()const{
  // Two slow, low-amplitude waves keep the cloak alive without wobble.
  return std::sin(double(hoverFrame)*2.0*PI/186.0)+
         0.17*std::sin(double(hoverFrame)*2.0*PI/113.0);
 }
 // Smooth scalar noise and its perpendicular gradient make a
 // divergence-free curling flow (no per-frame allocations or textures).
 static float noiseHash(int x,int y){
  std::uint32_t v=std::uint32_t(x)*0x9E3779B9u ^
                  std::uint32_t(y)*0x85EBCA6Bu ^ 0xB5297A4Du;
  v^=v>>16;v*=0x7FEB352Du;
  v^=v>>15;v*=0x846CA68Bu;
  v^=v>>16;
  return float(v&65535u)/65535.0f;
 }
 static float noise(float x,float y){
  const int ix=int(std::floor(x)),iy=int(std::floor(y));
  const float u=x-float(ix),v=y-float(iy);
  const float a=u*u*(3.0f-2.0f*u),b=v*v*(3.0f-2.0f*v);
  const float t=noiseHash(ix,iy)*(1.0f-a)+noiseHash(ix+1,iy)*a;
  const float q=noiseHash(ix,iy+1)*(1.0f-a)+noiseHash(ix+1,iy+1)*a;
  return t*(1.0f-b)+q*b;
 }
 static void curl(float x,float y,float time,float& dx,float& dy){
  const float sx=x*0.065f+time*0.012f;
  const float sy=y*0.065f-time*0.009f;
  const float d=0.06f;
  dx=(noise(sx,sy+d)-noise(sx,sy-d))/(2.0f*d);
  dy=-(noise(sx+d,sy)-noise(sx-d,sy))/(2.0f*d);
 }
 void spawn(const Frame& f,bool flip,bool cloak){
  const std::vector<EmissionPixel>& anchors=cloak?f.hem:f.green;
  float localX=0.0f,localY=0.0f;
  Uint8 r=0,g=0,b=0;
  if(!anchors.empty()){
   const EmissionPixel& q=anchors[range(0u,unsigned(anchors.size()-1u))];
   localX=float(q.x);localY=float(q.y);
   r=q.r;g=q.g;b=q.b;
  }else{
   // Only when a frame has no qualifying green/hem palette pixels:
   // use its blade/hem region, never edit the PSD or infer collisions.
   localX=float(f.w)*(cloak?0.45f:0.79f);
   localY=float(f.h)*(cloak?0.88f:0.24f);
   r=cloak?73u:77u;g=cloak?52u:218u;b=cloak?54u:117u;
  }
  if(flip)localX=float(f.w)-1.0f-localX;
  Particle& p=particles[nextParticle++%particles.size()];
  p.x=float(x)-float(f.w)*0.5f+localX;
  p.y=float(footBase()+hoverOffset())-float(f.h)+localY;
  const float rx=float(int(range(0u,100u))-50)/100.0f;
  const float ry=float(int(range(0u,100u))-50)/100.0f;
  p.cloak=cloak;
  p.vx=rx*(cloak?0.22f:0.42f);
  p.vy=(cloak?0.35f:-0.16f)+ry*0.18f;
  p.life=p.initial=clampParticleLife(cloak);
  if(cloak){
   // Keep source-based dark reds/grays; no neon/white cloak pixels.
   p.r=Uint8(std::min(150,int(r)));
   p.g=Uint8(std::min(110,int(g)));
   p.b=Uint8(std::min(115,int(b)));
   p.opacity=195u;
  }else{
   p.r=Uint8(std::min(150,int(r)));
   p.g=Uint8(std::min(255,int(g)+20));
   p.b=Uint8(std::min(180,int(b)+10));
   p.opacity=235u;
  }
  // The old emission was below a visible particle density in idle.
  // Keep pixel shapes (not a blurred glow), but use 2-3 world pixels.
  p.size=Uint8((motion==SMASH||motion==SLASH)?
     range(2u,3u):range(2u,3u));
  ++emitted;
 }
 unsigned clampParticleLife(bool cloak){
  return cloak?range(16u,29u):range(12u,23u);
 }
 void updateParticles(){
  const float t=float(hoverFrame);
  for(auto& p:particles){
   if(!p.life)continue;
   float cx=0.0f,cy=0.0f;
   curl(p.x,p.y,t,cx,cy);
   p.vx=p.vx*0.967f+cx*(p.cloak?0.038f:0.060f);
   p.vy=p.vy*0.968f+cy*(p.cloak?0.036f:0.060f)+
        (p.cloak?0.028f:-0.009f);
   p.x+=p.vx;p.y+=p.vy;
   --p.life;
  }
 }
 void emitCurrent(){
  const Frame* f=nullptr;
  bool flip=false;
  if(motion==IDLE){
   static const unsigned seq[]={0u,1u,2u,1u,0u};
   f=&idle[seq[(modeFrame/11u)%5u]];
   flip=facing>0;
  }else if(motion==SMASH){
   f=&smash[std::min(3u,modeFrame/6u)];flip=facing<0;
  }else if(motion==SLASH){
   static const unsigned seq[]={0u,1u,2u,1u,0u};
   f=&run[seq[(modeFrame/6u)%5u]];flip=facing<0;
  }else{
   f=&body;flip=facing>0;
  }
  if(!f->texture)return;
  const bool slash=(motion==SLASH||motion==SMASH);
  // Enough distinct particles to read as emission even at 384x224
  // internal resolution. Heavy curl trail follows every slash frame.
  const unsigned greenCount=slash?(motion==SLASH?12u:7u):3u;
  const unsigned hemCount=slash?6u:2u;
  for(unsigned i=0u;i<greenCount;++i)spawn(*f,flip,false);
  for(unsigned i=0u;i<hemCount;++i)spawn(*f,flip,true);
 }
 unsigned visibleParticles()const{
  unsigned n=0u;
  for(const auto& p:particles)if(p.life)++n;
  return n;
 }
 void drawParticles(SDL_Renderer* renderer,const SDL_Rect& view,
                    int cameraX,int cameraY){
  SDL_BlendMode oldBlend=SDL_BLENDMODE_NONE;
  Uint8 oldR=0,oldG=0,oldB=0,oldA=0;
  SDL_GetRenderDrawBlendMode(renderer,&oldBlend);
  SDL_GetRenderDrawColor(renderer,&oldR,&oldG,&oldB,&oldA);
  SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
  const double sx=double(view.w)/384.0,sy=double(view.h)/224.0;
  unsigned points=0u;
  for(const auto& p:particles){
   if(!p.life)continue;
   const unsigned a=unsigned(p.opacity)*p.life/p.initial;
   if(!a)continue;
   const int dx=view.x+int(std::lround((double(p.x)-cameraX)*sx));
   const int dy=view.y+int(std::lround((9.0+double(p.y)-cameraY)*sy));
   const int size=std::max(2,int(std::lround(double(p.size)*sx)));
   const SDL_Rect dot={dx,dy,size,size};
   SDL_SetRenderDrawColor(renderer,p.r,p.g,p.b,Uint8(a));
   if(SDL_RenderFillRect(renderer,&dot)==0)++points;
  }
  rendered=points;
  SDL_SetRenderDrawColor(renderer,oldR,oldG,oldB,oldA);
  SDL_SetRenderDrawBlendMode(renderer,oldBlend);
 }
 void enterStage(unsigned generation,unsigned frame){
  stageGeneration=generation;
  lastFrame=frame;
  random=0xA3B1C2D5u^(generation*747796405u)^(frame*2891336453u);
  if(!random)random=0x91E10DA5u;
  x=384.0+double(int(range(0u,160u))-80);
  x=std::max(double(LEFT_BOUND),std::min(double(RIGHT_BOUND),x));
  facing=-1;motion=IDLE;
  modeFrame=hoverFrame=0u;slashesSinceBall=0u;
  particles={};nextParticle=0u;
  emitted=rendered=0u;lastDebugFrame=~0u;
  decisionIn=range(240u,420u); // 4-7 game seconds at 60 fps
  diagnostic(
    "XM87.382 SIGMA_REAPER_SPAWN gen=%u worldX=%.1f "
    "bounds=140..628 bodyH=%d idleH=%d loweredBy=%d foot=%.1f nextDecision=%u\n",
    generation,x,body.h,silhouetteH,dropPixels(),footBase(),decisionIn);
 }
 void goIdle(){
  motion=IDLE;modeFrame=0u;
  decisionIn=range(240u,420u);
 }
 void choose(){
  // One ball for approximately every two completed slashes:
  // 18% early after the first, otherwise guaranteed after the second.
  // Turns do not reset the attack counter or postpone a due ball.
  const bool ballDue=slashesSinceBall>=2u ||
        (slashesSinceBall==1u && range(0u,99u)<18u);
  if(ballDue){
   motion=BALL;modeFrame=0u;slashesSinceBall=0u;
   diagnostic("XM87.382 SIGMA_REAPER_BALL x=%.1f afterSlash=1-2\n",x);
   return;
  }
  if(range(0u,99u)<15u){
   facing=-facing;decisionIn=range(240u,420u);return;
  }
  // Slash always heads to the side with more space, not current facing.
  const double left=x-LEFT_BOUND,right=RIGHT_BOUND-x;
  facing=(right>=left)?1:-1;
  motion=SMASH;modeFrame=0u;
  ++slashesSinceBall;
  diagnostic(
    "XM87.382 SIGMA_REAPER_SLASH x=%.1f toward=%s slashCount=%u\n",
    x,facing>0?"RIGHT":"LEFT",slashesSinceBall);
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
  // Game-clock-based simulation only; repeated SDL paints never
  // spawn extra particles or change the original native frame clock.
  updateParticles();
  emitCurrent();
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
    if(std::sscanf(px.c_str(),"%d %c",&x,&tail)!=1 ||
       std::sscanf(py.c_str(),"%d %c",&y,&tail)!=1 ||
       std::sscanf(pw.c_str(),"%d %c",&w,&tail)!=1 ||
       std::sscanf(ph.c_str(),"%d %c",&h,&tail)!=1){
      success=false;break;
    }
    if(w<=0||h<=0||w>2048||h>2048){success=false;break;}
    SDL_Surface* s=IMG_Load((root+file).c_str());
    if(!s || s->w!=w || s->h!=h){
     if(s)SDL_FreeSurface(s);
     success=false;break;
    }
    dst->texture=SDL_CreateTextureFromSurface(renderer,s);
    if(dst->texture)dst->collect(s);
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
    silhouetteH=0;
    for(const Frame& pose:idle)silhouetteH=std::max(silhouetteH,pose.h);
    diagnostic(
      "XM87.382 SIGMA_REAPER_READY frames=%u root=%s idleH=%d bodyH=%d "
      "drop=%d greenSamples=%u/%u/%u hemSamples=%u/%u/%u\n",
      total,root.c_str(),silhouetteH,body.h,dropPixels(),
      unsigned(idle[0].green.size()),unsigned(idle[1].green.size()),
      unsigned(idle[2].green.size()),unsigned(idle[0].hem.size()),
      unsigned(idle[1].hem.size()),unsigned(idle[2].hem.size()));
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
  diagnostic(
    "XM87.382 SIGMA_REAPER_MISSING: run BUILD_TEST.bat to extract "
    "_art/sigmastage/reapersigma.psd into extracted/manifest.tsv "
    "and 19 named PNGs; stage background remains usable\n");
  return false;
 }
 // World coordinates are aligned to Sigma's exact bottom-anchored
 // camera: backdrop top = 9 - nativeCameraY for the 768x471 level.
 // Breathe scales ~1% around the bottom center, not from the top-left.
 bool drawFrame(SDL_Renderer* renderer,const SDL_Rect& viewport,
                const Frame& frame,double xLeft,double bottom,
                int cameraX,int cameraY,bool flip,double breathing=0.0)const {
  if(!frame.texture)return false;
  const double sx=double(viewport.w)/384.0;
  const double sy=double(viewport.h)/224.0;
  const int bx=viewport.x+int(std::lround((xLeft-cameraX)*sx));
  const int by=viewport.y+int(std::lround((
       9.0-double(cameraY)+bottom-double(frame.h))*sy));
  const int bw=std::max(1,int(std::lround(double(frame.w)*sx)));
  const int bh=std::max(1,int(std::lround(double(frame.h)*sy)));
  const int ww=std::max(1,int(std::lround(double(bw)*(1.0+0.008*breathing))));
  const int hh=std::max(1,int(std::lround(double(bh)*(1.0+0.014*breathing))));
  const SDL_Rect dst={bx-(ww-bw)/2,by+bh-hh,ww,hh};
  return SDL_RenderCopyEx(renderer,frame.texture,nullptr,&dst,
                          0.0,nullptr,flip?SDL_FLIP_HORIZONTAL:SDL_FLIP_NONE)==0;
 }
 bool paint(SDL_Renderer* renderer,const SDL_Rect& view,
            unsigned generation,unsigned gameFrame,int cameraX,int cameraY){
  if(!load(renderer,generation))return false;
  update(generation,gameFrame);
  if(lastDebugFrame==~0u || gameFrame<lastDebugFrame ||
     gameFrame-lastDebugFrame>=180u){
    lastDebugFrame=gameFrame;
    diagnostic(
      "XM87.382 SIGMA_REAPER_LIVE frame=%u gen=%u mode=%u "
      "idleH=%d bodyH=%d drop=%d foot=%.1f cameraY=%d "
      "particles=%u totalEmitted=%u screenFoot=%d drawn=%u "
      "gameClockDelta=%u\n",
      gameFrame,generation,unsigned(motion),silhouetteH,body.h,
      dropPixels(),footBase()+hoverOffset(),cameraY,
      visibleParticles(),emitted,
      view.y+int(std::lround((9.0-double(cameraY)+
                       footBase()+hoverOffset())*double(view.h)/224.0)),
      rendered,gameFrame>=lastFrame?gameFrame-lastFrame:0u);
  }
  const double foot=footBase()+hoverOffset();
  const double breathing=breath();
  if(motion==BALL || motion==BALL_HOLD){
   // ball1body is ALWAYS visible, while frames 2..9 pass in front.
   const double bodyLeft=x-double(body.w)*0.5;
   const bool flip=facing>0; // ball body inherits idle-facing sheet
   bool ok=drawFrame(renderer,view,body,bodyLeft,foot,
                     cameraX,cameraY,flip,breathing);
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
   drawParticles(renderer,view,cameraX,cameraY);
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
  const bool ok=drawFrame(renderer,view,*f,x-double(f->w)*0.5,foot,
                          cameraX,cameraY,flip,breathing);
  drawParticles(renderer,view,cameraX,cameraY);
  return ok;
 }
};
inline Actor& actor(){static Actor a;return a;}
inline void cleanup(){actor().close();}
} // namespace XM87376Reaper
#endif
