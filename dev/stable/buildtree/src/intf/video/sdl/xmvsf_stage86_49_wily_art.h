#ifndef XMVSF_STAGE86_49_WILY_ART_H
#define XMVSF_STAGE86_49_WILY_ART_H
// XMVSF Stage 75 — independent SDL2 presentation proof, NOT the combat executable.
// Consumes lossless capture-derived RGBA textures and an explicit stage manifest.

#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace xm76stage {
static std::string kRoot="assets/xmvsf/wily75";
constexpr int VIEW_W=384, VIEW_H=224;
int rnd(double f) { return static_cast<int>(std::floor(f+0.5)); } // JavaScript Math.round
int clamp(int v,int a,int b){return std::max(a,std::min(v,b));}
std::vector<std::string> fields(const std::string& s, char sep='\t') {
 std::vector<std::string> r;std::istringstream in(s);std::string v;
 while(std::getline(in,v,sep))r.push_back(v);
 return r;
}
int number(const std::string& s){return std::stoi(s);}
std::vector<int> ints(const std::string& s,char sep=','){
 std::vector<int> a;if(s.empty())return a;
 for(const auto& v:fields(s,sep)) a.push_back(number(v));
 return a;
}
struct Cell{
 int x=0,y=0,w=16,h=16;
 std::vector<int> states; // base: direct 104 frame indices
 std::vector<std::pair<int,int>> keys; // sampled extension phases
};
struct Atlas{
 int originX=0,originY=0,columns=1;
 std::string baseTex,sheetTex;
 std::vector<Cell> cells;
};
struct Layer{
 double xs=1,ys=1,xoff=0,yoff=0;
 Atlas upper,left,base;
 bool visible=true;
};
struct Frame{int emu=0,x=0,y=0;};
struct Actor{std::string image;int x=0,y=0;};
struct World{
 std::string chimneyTexture,chimneyMode;
 std::vector<int> chimneyPoses;
 int chimneyX=0,chimneyY=0,chimneyW=0,chimneyH=0,chimneyN=0;
 int xmin=-64,xmax=317,ymin=-14,ymax=256;
 Layer layers[4];
 std::vector<Frame> frames;
 std::map<int,Actor> actors;
 std::map<int,Actor> actors62;
 std::vector<int> leftTimes;
 std::set<std::pair<int,int>> verifiedUpperCells;
 std::map<std::pair<int,int>,std::vector<int>> upperFactoryCycle;
 std::map<std::pair<int,int>,std::vector<int>> foregroundVaporCycle;
 std::string foregroundPackedTexture;
 std::vector<int> foregroundPoseByPhase;
 int foregroundX=0,foregroundY=0,foregroundW=0,foregroundH=0,foregroundPoseCount=0;
 int tailStart=4081,tailEnd=4097;
 int factoryWrapY=240; // observed layer-1 emission reset, preview only
 int factoryXStop=-1; // user-directed point at which factory Layer1 stops scrolling horizontally
 int naturalX=0,naturalY=0;
 std::string naturalPatch; // opaque pixels sampled at a real Y=0 game-camera position
 std::string skyFillTexture; // Stage75.3: donor-color-only sky extension under the original upper source
 int skyFillX=0,skyFillY=0,skyFillW=0,skyFillH=0;
};
Atlas& atlas(Layer& l,const std::string& kind){
 if(kind=="upper")return l.upper;
 if(kind=="left")return l.left;
 if(kind=="base")return l.base;
 throw std::runtime_error("Bad atlas kind: "+kind);
}
World loadManifest(const std::string& filename){
 std::ifstream in(filename);if(!in)throw std::runtime_error("Cannot open "+filename);
 World world;std::string line;int ln=0;
 while(std::getline(in,line)){
  ++ln;if(line.empty()||line[0]=='#')continue;
  const auto f=fields(line);
  try{
   if(f[0]=="SIZE"&&f.size()==3){if(number(f[1])!=VIEW_W||number(f[2])!=VIEW_H)throw std::runtime_error("Unexpected viewport");}
   else if(f[0]=="LIMITS"&&f.size()==5){world.xmin=number(f[1]);world.xmax=number(f[2]);world.ymin=number(f[3]);world.ymax=number(f[4]);}
   else if(f[0]=="PARALLAX"&&f.size()==6){auto& l=world.layers[number(f[1])];l.xs=std::stod(f[2]);l.ys=std::stod(f[3]);l.xoff=std::stod(f[4]);l.yoff=std::stod(f[5]);}
   else if(f[0]=="FACTORY_X_STOP"&&f.size()==2){world.factoryXStop=number(f[1]);}
   else if(f[0]=="SKY_BACKFILL"&&f.size()==6){world.skyFillTexture=f[1];world.skyFillX=number(f[2]);world.skyFillY=number(f[3]);world.skyFillW=number(f[4]);world.skyFillH=number(f[5]);}
   else if(f[0]=="ATLAS"&&f.size()==8){auto& a=atlas(world.layers[number(f[1])],f[2]);a.originX=number(f[3]);a.originY=number(f[4]);a.baseTex=f[5];a.sheetTex=f[6];a.columns=number(f[7]);if(a.columns<=0)throw std::runtime_error("Zero sheet columns");}
   else if(f[0]=="CELL_BASE"&&f.size()==7){Cell c;c.x=number(f[2]);c.y=number(f[3]);c.w=number(f[4]);c.h=number(f[5]);c.states=ints(f[6]);world.layers[number(f[1])].base.cells.push_back(std::move(c));}
   else if(f[0]=="CELL_KEYS"&&f.size()==6){Cell c;c.x=number(f[3]);c.y=number(f[4]);for(const auto& pair:fields(f[5],',')){const auto v=fields(pair,':');if(v.size()!=2)throw std::runtime_error("Bad keyframe");c.keys.emplace_back(number(v[0]),number(v[1]));}atlas(world.layers[number(f[1])],f[2]).cells.push_back(std::move(c));}
   else if(f[0]=="FRAME"&&f.size()==4){world.frames.push_back({number(f[1]),number(f[2]),number(f[3])});}
   else if(f[0]=="ACTOR"&&f.size()==5){world.actors[number(f[1])]={f[2],number(f[3]),number(f[4])};}
   else if(f[0]=="FACTORY_WRAP_Y"&&f.size()==2){world.factoryWrapY=number(f[1]);}
   else if(f[0]=="NATURAL_PATCH"&&f.size()==4){world.naturalX=number(f[1]);world.naturalY=number(f[2]);world.naturalPatch=f[3];}
   else if(f[0]=="ACTOR62"&&f.size()==5){world.actors62[number(f[1])]={f[2],number(f[3]),number(f[4])};}
   else if(f[0]=="LEFT_TIMES"&&f.size()==2){world.leftTimes=ints(f[1]);}
   else if(f[0]=="WILY_TAIL"&&f.size()==3){world.tailStart=number(f[1]);world.tailEnd=number(f[2]);}
   else throw std::runtime_error("Unknown record or field count: "+f[0]);
  }catch(const std::exception& e){throw std::runtime_error("manifest line "+std::to_string(ln)+": "+e.what());}
 }
 if(world.frames.empty())throw std::runtime_error("No frame timeline");
 if(world.factoryXStop!=222)throw std::runtime_error("Stage75.6 factory parallax must stop at camera X=222");
 if(world.skyFillTexture!="assets/factory_sky_rightfill.xrga" || world.skyFillX!=640 || world.skyFillY!=72 || world.skyFillW!=1024 || world.skyFillH!=168)
  throw std::runtime_error("Missing or invalid Stage75.3 sky backing record");
 if(std::abs(world.layers[1].xs-1.25)>0.0001 || std::abs(world.layers[1].ys-0.75)>0.0001 || std::abs(world.layers[1].xoff-241.75)>0.0001)
  throw std::runtime_error("Unexpected Stage75.5 -112px background alignment");
 for(int i=1;i<=3;++i)if(world.layers[i].base.baseTex.empty()||world.layers[i].upper.baseTex.empty()||world.layers[i].left.baseTex.empty())throw std::runtime_error("Missing atlas");
 // Stage65.2: the whitelist is copied from the exact frame-89 donor overlap.
 // A row from the normal base manifest must exist at each coordinate.
 std::ifstream verified(std::string(kRoot)+"/verified_upper_cells.tsv");
 if(!verified)throw std::runtime_error("Missing verified_upper_cells.tsv");
 std::string coord;int records=0;
 while(std::getline(verified,coord)){
  if(coord.empty()||coord[0]=='#')continue;
  const auto vals=fields(coord);
  if(vals.size()!=2)throw std::runtime_error("Malformed verified upper coordinate");
  const auto pos=std::make_pair(number(vals[0]),number(vals[1]));
  if(!world.verifiedUpperCells.insert(pos).second)throw std::runtime_error("Duplicate verified upper coordinate");
  ++records;
 }
 if(records!=146)throw std::runtime_error("Expected precisely 146 donor-verified upper cells");
 // Stage 67: each upper cell gets the SAME 28-frame global clock.
 // Stage53's 104 per-cell values contain held approximations when that cell
 // was outside the original capture. Use the observed period instead.
 std::ifstream phases(std::string(kRoot)+"/upper_factory_cycle.tsv");
 if(!phases)throw std::runtime_error("Missing upper_factory_cycle.tsv");
 std::string cycleLine;
 while(std::getline(phases,cycleLine)){
  if(cycleLine.empty()||cycleLine[0]=='#')continue;
  const auto vals=fields(cycleLine);
  if(vals.size()!=3)throw std::runtime_error("Invalid upper factory 28-phase row");
  const auto pos=std::make_pair(number(vals[0]),number(vals[1]));
  if(!world.verifiedUpperCells.count(pos))throw std::runtime_error("Unverified upper cycle coordinate");
  const auto states=ints(vals[2]);
  if(states.size()!=28)throw std::runtime_error("Upper factory requires precisely 28 states per cell");
  if(!world.upperFactoryCycle.emplace(pos,states).second)throw std::runtime_error("Duplicate upper cycle row");
 }
 if(world.upperFactoryCycle.size()!=146)throw std::runtime_error("Missing synchronized upper factory cycle rows");
 // Stage 67 source: the moving pale-gray foreground vapor is a separate Layer 3
 // animation, NOT part of the 28-frame factory parallax. All 20 cells were
 // directly observed across sample frames 25..69 and match frames +45.
 std::ifstream vapor(std::string(kRoot)+"/foreground_vapor_cycle.tsv");
 if(!vapor)throw std::runtime_error("Missing foreground_vapor_cycle.tsv");
 std::string vaporLine;
 while(std::getline(vapor,vaporLine)){
  if(vaporLine.empty() || vaporLine[0]=='#')continue;
  auto vals=fields(vaporLine);
  if(vals.size()!=3)throw std::runtime_error("Malformed foreground vapor record");
  auto xy=std::make_pair(number(vals[0]),number(vals[1]));
  auto states=ints(vals[2]);
  if(states.size()!=45)throw std::runtime_error("Foreground vapor must contain 45 recorded states");
  if(!world.foregroundVaporCycle.emplace(xy,states).second)throw std::runtime_error("Duplicate vapor cell");
 }
 if(world.foregroundVaporCycle.size()!=20)throw std::runtime_error("Expected 20 foreground vapor cells");
 // Stage72: reduce the twenty source-aligned 16x16 smoke tiles to five
 // observed 160x48 global poses. The 45-frame phase list is checked
 // against both independent donor captures by VERIFY_STAGE72_SMOKE.py.
 std::ifstream poses(std::string(kRoot)+"/foreground_pose_cycle.tsv");
 if(!poses)throw std::runtime_error("Missing foreground_pose_cycle.tsv");
 bool gotAtlas=false,gotCycle=false;
 std::string poseLine;
 while(std::getline(poses,poseLine)){
  if(poseLine.empty()||poseLine[0]=='#')continue;
  const auto v=fields(poseLine);
  if(v[0]=="POSES"&&v.size()==8&&!gotAtlas){
   world.foregroundPackedTexture=v[1];
   world.foregroundX=number(v[2]);world.foregroundY=number(v[3]);
   world.foregroundW=number(v[4]);world.foregroundH=number(v[5]);
   world.foregroundPoseCount=number(v[6]);
   // Last field is source-cell count: guard against silent omissions.
   if(number(v[7])!=20)throw std::runtime_error("Wrong foreground source cell count");
   gotAtlas=true;
  }else if(v[0]=="CYCLE"&&v.size()==2&&!gotCycle){
   world.foregroundPoseByPhase=ints(v[1]);gotCycle=true;
  }else throw std::runtime_error("Malformed or duplicate packed foreground record");
 }
 if(!gotAtlas||!gotCycle||world.foregroundX!=342||world.foregroundY!=387
    ||world.foregroundW!=160||world.foregroundH!=48||world.foregroundPoseCount!=5
    ||world.foregroundPoseByPhase.size()!=45)
   throw std::runtime_error("Stage72 foreground metadata mismatch");
 for(const auto p:world.foregroundPoseByPhase)
  if(p<0||p>=world.foregroundPoseCount)throw std::runtime_error("Stage72 foreground phase outside pose sheet");

 // Stage75: upper-right Scroll2 smoke uses captured donor artwork and
 // the fully observed 28-frame clock (four authentic poses).
 std::ifstream chimney(std::string(kRoot)+"/chimney_cycle.tsv");
 if(!chimney)throw std::runtime_error("Missing chimney_cycle.tsv");
 bool gotChimney=false,gotChimneyCycle=false,gotMode=false;
 std::string chimneyLine;
 while(std::getline(chimney,chimneyLine)){
  if(chimneyLine.empty()||chimneyLine[0]=='#')continue;
  const auto f=fields(chimneyLine);
  if(f[0]=="CHIMNEY"&&f.size()==8&&!gotChimney){
   world.chimneyTexture=f[1];world.chimneyX=number(f[2]);world.chimneyY=number(f[3]);
   world.chimneyW=number(f[4]);world.chimneyH=number(f[5]);world.chimneyN=number(f[6]);
   // The final field is reserved for source coverage, not a synthetic pose.
   if(f[7]!="SCROLL2")throw std::runtime_error("Chimney provenance must be SCROLL2");
   gotChimney=true;
  }else if(f[0]=="MODE"&&f.size()==2&&!gotMode){world.chimneyMode=f[1];gotMode=true;}
  else if(f[0]=="CYCLE"&&f.size()==2&&!gotChimneyCycle){world.chimneyPoses=ints(f[1]);gotChimneyCycle=true;}
  else throw std::runtime_error("Malformed chimney metadata record");
 }
 if(!gotChimney||!gotChimneyCycle||!gotMode||world.chimneyX!=440||world.chimneyY!=-20||world.chimneyW!=275||world.chimneyH!=80||world.chimneyN<1||world.chimneyN>180||world.chimneyPoses.empty()||world.chimneyPoses.size()>360)
  throw std::runtime_error("Invalid Stage75 chimney metadata");
 for(int p:world.chimneyPoses)if(p<0||p>=world.chimneyN)throw std::runtime_error("Chimney pose index invalid");
 world.layers[2].upper.baseTex="assets/upper__layer2_without_chimney.xrga";
 for(auto& v:world.foregroundVaporCycle){
  bool found=false;
  for(auto& c:world.layers[3].base.cells)
   if(c.x==v.first.first && c.y==v.first.second && c.w==16 && c.h==16 && c.states.size()==104){found=true;break;}
  if(!found)throw std::runtime_error("Foreground vapor cell has no base source");
 }

 for(const auto& pos:world.verifiedUpperCells){
  bool found=false;
  for(const auto& cell:world.layers[1].base.cells)
   if(cell.x==pos.first && cell.y==pos.second && cell.states.size()==104 && cell.w==16 && cell.h==16){found=true;break;}
  if(!found)throw std::runtime_error("Upper cell has no matching source animation");
 }
 return world;
}
struct Texture{SDL_Texture* tex=nullptr;int w=0,h=0;std::array<Uint8,3> lastMod{{255,255,255}};bool actor=false;};
struct Textures{
 SDL_Renderer* renderer; std::map<std::string,Texture> known;
 // XM86.64 recovered Wily scenery is 30%-bright and 70%-saturated with a
 // 35s hue cycle; actor stays untinted grayscale at original intensity.
 bool sceneTint=true;
 bool noirMode=true;
 // 87.62: hue always advances continuously; brightness is selected on each draw.
 // Scene luma is kept at full 8-bit precision on load. Modulation provides
 // 30% normally and 90% during the verified portrait-exit super pulse. Actor textures are exempt.
 bool superPulse=false;
 Uint32 lastHueTick=0;
 double huePhase=0.0;
 bool hueStarted=false;
 std::array<Uint8,3> currentSpectral{{255,255,255}};
 explicit Textures(SDL_Renderer* r):renderer(r){
   const char* df=std::getenv("XM8649_WILY_RENDER");
   noirMode=df && df[0]=='1' && df[1]=='\0';
   sceneTint=noirMode;
 }
 static std::array<Uint8,3> spectral(double phase,bool full){
   // Smooth HSV, 70% saturation, no stepped 85ms quantization.
   const double h=phase*6.0;
   const double c=255.0*(full?1.0:0.70);
   const double x=c*(1.0-std::fabs(std::fmod(h,2.0)-1.0));
   const double m=255.0-c;
   double r=0,g=0,b=0;
   if(h<1.0){r=c;g=x;}
   else if(h<2.0){r=x;g=c;}
   else if(h<3.0){g=c;b=x;}
   else if(h<4.0){g=x;b=c;}
   else if(h<5.0){r=x;b=c;}
   else {r=c;b=x;}
   auto ch=[&](double v)->Uint8{return static_cast<Uint8>(std::max(0.0,std::min(255.0,std::round(v+m))));};
   return {{ch(r),ch(g),ch(b)}};
 }
 void stepHue(){
   if(!sceneTint)return;
   const Uint32 now=SDL_GetTicks();
   if(!hueStarted){
     hueStarted=true;
     lastHueTick=now;
     huePhase=double(now%35000u)/35000.0;
     std::fprintf(stderr,"XM87.64 DF_HUE_RATE normal_period_ms=35000 super_period_ms=875 brightness_pct=90 saturation_pct=100 phase_continuous=1\n");
   }else{
     const Uint32 elapsed=now-lastHueTick; // wraps safely
     lastHueTick=now;
     if(elapsed<1500u){
       // Normal 35s turn; super 0.875s turn (4x Stage87.63 speed). No reset at either edge.
       huePhase+=double(elapsed)/(superPulse?875.0:35000.0);
       huePhase-=std::floor(huePhase);
     }
   }
   currentSpectral=spectral(huePhase,superPulse);
 }
 SDL_Texture* select(Texture& t){
   // Painter updates hue ONCE per presented frame, not once per tile draw.
   const std::array<Uint8,3> white{{255,255,255}};
   const auto& spectralColor=(sceneTint && !t.actor)?currentSpectral:white;
   const unsigned brightness=(sceneTint && !t.actor)?(superPulse?90u:30u):100u;
   const std::array<Uint8,3> m={{
      static_cast<Uint8>((unsigned(spectralColor[0])*brightness+50u)/100u),
      static_cast<Uint8>((unsigned(spectralColor[1])*brightness+50u)/100u),
      static_cast<Uint8>((unsigned(spectralColor[2])*brightness+50u)/100u)
   }};
   if(m!=t.lastMod){
     if(SDL_SetTextureColorMod(t.tex,m[0],m[1],m[2])!=0)
       throw std::runtime_error("Wily hue mod: "+std::string(SDL_GetError()));
     t.lastMod=m;
   }
   return t.tex;
 }
 ~Textures(){for(auto& item:known)SDL_DestroyTexture(item.second.tex);}
 Texture& get(const std::string& path){
  auto it=known.find(path);if(it!=known.end())return it->second;
  std::ifstream in(std::string(kRoot)+"/"+path,std::ios::binary);if(!in)throw std::runtime_error("Missing RGBA asset: "+path);
  unsigned char header[12]={};in.read(reinterpret_cast<char*>(header),12);
  if(in.gcount()!=12||header[0]!='X'||header[1]!='R'||header[2]!='G'||header[3]!='A')throw std::runtime_error("Invalid texture header: "+path);
  auto u32=[&](int off)->std::uint32_t{return std::uint32_t(header[off])|(std::uint32_t(header[off+1])<<8)|(std::uint32_t(header[off+2])<<16)|(std::uint32_t(header[off+3])<<24);};
  const int w=static_cast<int>(u32(4)),h=static_cast<int>(u32(8));
  if(w<=0||h<=0||w>16384||h>16384||std::uint64_t(w)*h>20000000)throw std::runtime_error("Texture size invalid: "+path);
  std::vector<std::uint8_t> rgba(std::size_t(w)*h*4);in.read(reinterpret_cast<char*>(rgba.data()),static_cast<std::streamsize>(rgba.size()));
  if(static_cast<std::size_t>(in.gcount())!=rgba.size())throw std::runtime_error("Truncated RGBA asset: "+path);
  // All recovered Wily tiles are sprite-derived artwork. Convert once on load,
  // preserving alpha and avoiding GPU readback/frame-order inconsistencies.
  for(std::size_t px=0;px<rgba.size();px+=4){
    const unsigned y=(77u*rgba[px]+150u*rgba[px+1]+29u*rgba[px+2]+128u)>>8;
    // XM87.62: scene luma stays full range; color modulation applies
    // 30%/80% at render time. No additive pass or screen-wide overlay.
    const bool isWilyActor=path.compare(0,11,"assets/wily")==0;
    static bool darkReported=false;
    if(noirMode && !isWilyActor && !darkReported){
      std::fprintf(stderr,"XM86.64 WILY_DARK_SCENE scene_brightness=30pct hue_saturation=70pct actor_exempt=1 additive=0\n");
      darkReported=true;
    }
    if(noirMode){
      rgba[px]=rgba[px+1]=rgba[px+2]=(std::uint8_t)y;
    } // Classic color: keep original captured RGB, including actor color.
  }
  SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormatFrom(rgba.data(),w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);
  if(!surf)throw std::runtime_error("SDL surface error: "+std::string(SDL_GetError()));
  SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer,surf);SDL_FreeSurface(surf);
  if(!tex)throw std::runtime_error("SDL texture error: "+std::string(SDL_GetError()));
  SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
  Texture t{tex,w,h};t.actor=(path.compare(0,11,"assets/wily")==0);
  return known.emplace(path,t).first->second;
 }
};
struct Options{
 bool animate=true,actor=true,upper=true,left=true,quarantine=true,bounded=true,play=true,follow=false,actorPlay=true;
 bool factoryWrapGuard=false; // legacy global donor cutoff, OFF
 bool factoryMirrorAnimation=true; // phase-clean upper backing, including observed 72px footer copy
 bool upperWrapQuarantine=true; // Stage65 reconstructed-preview cropping, keep available
 bool donorFactoryReference=false; // D: exact captured single-phase donor factory layer for comparison
 bool naturalUnderlay=true; // J: corrected to wrap-normalized world Y=0
 bool foregroundVaporSync=true; // Stage67: independent Layer3 45-frame clock
 bool unifiedUpperClock=true; // Y: compare synchronized 28-frame cycle with legacy held-cell sequence
 bool foregroundVapor=true; // foreground 45-frame vapor enabled
 bool chimneySmoke=true,chimneyPlay=true; // Scroll2: separate upper-right chimney plume
 bool verifiedUpperAnimation=true; // I: verified cells only, after original upper donor image
 bool verifiedUpperPriority=true; // K: donor-verified upper capture wins over unsynchronized overlays
 bool nativeStableAtlas=false; // Stage76.6: freeze unverified donor timeline cells in live gameplay only
 bool nativeSmoothUpper=false; // Stage76.5: live game fades between overlapping donor phases (preview unchanged)
};
struct Preview{
 World& w;Textures& textures;SDL_Renderer* render;SDL_Window* window;
 Options opt;
 // XM86.50: numbered visual audit groups. The nine slots are kept
 // distinct from the CPS stage register/scroll layer order.
 bool auditPass[10] = {false,true,true,true,true,true,true,true,true,true};
 int cx=317,cy=-14,fi=23;std::int64_t factoryTick=23;std::uint64_t actorTick=0;std::uint64_t chimneyTick=0;int actorMode=0;bool useNew=true;std::vector<int> actorFrames,actorFrames62;int tailBegin=0;bool drag=false;int dragX=0,dragY=0,initialX=0,initialY=0;
 // Temporary, user-visible diagnostic controls only. Source-space registration is untouched.
 // XM86.53: near-static factory with adjustable registration.
 // +64 px source scroll moves the factory two 32px tile columns LEFT,
 // covering the left-side void observed at the right-hand camera limit.
 int factoryRightShiftPx=64;
 double factoryDrift=0.03; // 0.03px per camera px; ~8.6 px end-to-end
 int factoryVerticalNudge=0;
 bool factoryMatchForegroundY=false;
 // Freeze only Layer 1 horizontally once the camera passes the catwalk end.
 // Layer2 (including chimney), Layer3, actor, and vertical scrolling retain the real camera.
 int layerCameraX(const Layer& layer)const{
  return (&layer==&w.layers[1])?std::min(cx,w.factoryXStop):cx;
 }
 // XM86.53: right-edge-referenced layer-1 registration. Earlier 0.18x
 // moved 51px across this camera envelope and exposed two 32px black columns
 // at the RIGHT end. Keep a small 0.03x drift while positioning the visible
 // scenery two 32px columns further LEFT at camera X=222 (factory stop).
 // '[',']' adjust the source scroll in 16px increments; '-'/'=' adjust drift.
 // Do not modify donor manifest or scroll of native camera/other layers.
 double layerScrollX(const Layer& layer)const{
  if(&layer==&w.layers[1]) {
   const double priorRight=layer.xoff+layer.xs*128.0+0.18*(double(w.factoryXStop)-128.0);
   return priorRight+double(factoryRightShiftPx)+factoryDrift*(double(layerCameraX(layer))-double(w.factoryXStop));
  }
  return layer.xs*layerCameraX(layer)+layer.xoff;
 }
 double factoryYRate()const{return factoryMatchForegroundY?1.0:w.layers[1].ys;}
 double layerYRate(const Layer& l)const{return (&l==&w.layers[1])?factoryYRate():l.ys;}
 int layerYOffset(const Layer& l)const{return (&l==&w.layers[1])?factoryVerticalNudge:0;}
 Preview(World& world,Textures& t,SDL_Renderer* r,SDL_Window* win):w(world),textures(t),render(r),window(win){
  for(const auto& p:w.actors)actorFrames.push_back(p.first);
  for(const auto& p:w.actors62)actorFrames62.push_back(p.first);
  if(actorFrames62.size()!=62 || actorFrames62.front()!=4132 || actorFrames62.back()!=4193)throw std::runtime_error("Stage62.1 captured sequence missing or malformed");
  if(actorFrames.empty())throw std::runtime_error("No Wily actor records");
  const auto tail=std::lower_bound(actorFrames.begin(),actorFrames.end(),w.tailStart);
  tailBegin=static_cast<int>(tail-actorFrames.begin());
  if(tailBegin>=static_cast<int>(actorFrames.size())||actorFrames.back()!=w.tailEnd)
   throw std::runtime_error("Unverified Wily echo tail in manifest");
 }
 void limitCamera(){if(opt.bounded){cx=clamp(cx,w.xmin,w.xmax);cy=clamp(cy,w.ymin,w.ymax);}}
 // Scenery and the selected actor capture advance on independent clocks.
 // Stage62 has exactly 62 consecutive REAL frames 4132..4193. The capture
 // ended without a completion marker; the original animation's period is unknown.
 // Mode 0: observed frames forward, then backward (preview construction, not donor timing).
 // Mode 1: observed frames once, then hold last frame (strict observation).
 // Mode 2: repeat 62 observed frames (hard seam; clearly labeled).
 int actorIndex()const{
  const auto& vec=useNew?actorFrames62:actorFrames;
  const int n=static_cast<int>(vec.size());
  if(actorTick<static_cast<std::uint64_t>(n))return static_cast<int>(actorTick);
  if(actorMode==1)return n-1;
  if(actorMode==2)return static_cast<int>(actorTick%n);
  if(useNew){
   // Every displayed pose in this ping-pong mode is a genuine recorded pose,
   // but reversing the order is only a preview convenience.
   const std::uint64_t period=static_cast<std::uint64_t>(2*(n-1));
   const int phase=static_cast<int>(actorTick%period);
   return phase<n?phase:2*(n-1)-phase;
  }
  const int span=n-1-tailBegin;
  if(span<=0)return n-1;
  const std::uint64_t t=(actorTick-static_cast<std::uint64_t>(n-1))%(2ULL*static_cast<unsigned>(span));
  return t<=static_cast<unsigned>(span)?n-1-static_cast<int>(t):tailBegin+static_cast<int>(t)-span;
 }
 int actorEmu()const{
  const auto& vec=useNew?actorFrames62:actorFrames;
  return vec[actorIndex()];
 }
 const Actor* currentActor()const{
  const auto& map=useNew?w.actors62:w.actors;
  const auto it=map.find(actorEmu());
  return it==map.end()?nullptr:&it->second;
 }
 const char* actorModeLabel()const{return actorMode==0?"PREVIEW PING-PONG":actorMode==1?"HOLD REAL FINAL FRAME":"CAPTURE REPEAT";}
 void sceneStep(int n){const int count=static_cast<int>(w.frames.size());fi=(fi+n%count+count)%count;factoryTick+=n;
  if(opt.chimneyPlay){if(n>=0)chimneyTick+=static_cast<unsigned>(n);else chimneyTick=chimneyTick>=static_cast<unsigned>(-n)?chimneyTick-static_cast<unsigned>(-n):0;}
 }
 int factoryPhase()const{const auto r=factoryTick%28;return static_cast<int>(r<0?r+28:r);}
 void actorStep(int n){if(n>=0)actorTick+=static_cast<unsigned>(n);else actorTick=actorTick>=static_cast<unsigned>(-n)?actorTick-static_cast<unsigned>(-n):0;}
 void reset(){cx=317;cy=-14;fi=23;factoryTick=23;actorTick=0;chimneyTick=0;opt.chimneySmoke=true;opt.chimneyPlay=true;actorMode=0;useNew=true;opt.actorPlay=true;factoryVerticalNudge=0;factoryMatchForegroundY=false;opt.factoryWrapGuard=false;opt.factoryMirrorAnimation=true;opt.upperWrapQuarantine=true;opt.donorFactoryReference=false;opt.verifiedUpperPriority=true;opt.verifiedUpperAnimation=true;opt.unifiedUpperClock=true;opt.foregroundVapor=true;opt.foregroundVaporSync=true;limitCamera();}
 int extPhase()const{
  if(opt.nativeStableAtlas)return 0; // source sweep not a verified looping animation
  if(w.leftTimes.empty())return 0;
  const int base=w.leftTimes[0],step=w.leftTimes.size()>1?std::max(1,w.leftTimes[1]-base):12;
  const int rel=w.frames[fi].emu-base;
  // floor division for negative offsets, matching JS Math.floor.
  const int q=static_cast<int>(std::floor(double(rel)/step));
  const int n=static_cast<int>(w.leftTimes.size());return (q%n+n)%n;
 }
 void blit(const std::string& path,const SDL_Rect* src,const SDL_Rect& dst){
  auto& t=textures.get(path);if(SDL_RenderCopy(render,textures.select(t),src,&dst)!=0)throw std::runtime_error("SDL_RenderCopy: "+std::string(SDL_GetError()));
 }
 // A Stage57 upper snapshot is frozen at a different time than Stage53.
 // All 146 spatially overlapping base cells agree exactly with Stage57's
 // snapshot at Stage53 frame index 89. That known source phase was used to
 // cut those specific cells out of the upper backing. Refill them from the
 // authentic Stage53 animation sheet each frame, never paint over frozen
 // pixels of another phase. The Stage56 left sweep has no globally
 // synchronized upper-frame phase and is deliberately NOT mirrored here.
 // Stage75.3 user-directed horizontal parallax: 1.25x; vertical donor reference stays 0.75x.
 void drawFactoryUpperMirrors(const Layer& layer){
  if(!opt.factoryMirrorAnimation || !opt.upper)return;
  const Atlas& top=layer.upper;
  constexpr int period=256;
  // A 256px repetition starts at source atlas row 72, not row 0.
  // The complete cloud->factory composition ends at row 328. Crop the
  // exact captured display region; preserve the repeated 72px footer
  // that Stage62.4 incorrectly cut away.
  constexpr int visibleRows=328;
  const auto& upperTexture=textures.get(top.baseTex);
  const int xMin=top.originX, xMax=xMin+upperTexture.w;
  const int yMin=top.originY, yMax=yMin+visibleRows;
  const Atlas& source=layer.base;
  for(const Cell& c:source.cells){
   const int wx=source.originX+c.x;
   const int sourceY=source.originY+c.y;
   const int firstY=sourceY-period;
   if(wx<xMin || wx+c.w>xMax || firstY<yMin || firstY+c.h>top.originY+period)continue;
   if(c.states.empty())continue;
   // Ground stays on its original clock. Reconstruct the first upper band
   // from verified matching source tiles; repeat *only* those tiles within
   // the second 72px portion, with a precisely truncated source rectangle.
   const int state=c.states[(opt.nativeStableAtlas || !opt.animate)?
        std::min(89,static_cast<int>(c.states.size())-1):std::min(fi,static_cast<int>(c.states.size())-1)];
   if(state<0)continue;
   for(int copy=0;copy<2;++copy){
    const int wy=firstY+copy*period;
    const int clipTop=std::max(wy,yMin),clipEnd=std::min(wy+c.h,yMax);
    if(clipTop>=clipEnd)continue;
    const int px=wx-rnd(layerScrollX(layer));
    const int py=clipTop-rnd(layerYRate(layer)*cy+layer.yoff)+layerYOffset(layer);
    const int h=clipEnd-clipTop;
    if(px>=VIEW_W || py>=VIEW_H || px+c.w<=0 || py+h<=0)continue;
    const SDL_Rect srcRect{(state%source.columns)*16,(state/source.columns)*16+(clipTop-wy),c.w,h};
    blit(source.sheetTex,&srcRect,SDL_Rect{px,py,c.w,h});
   }
  }
 }
 void drawAtlas(const Layer& layer,const Atlas& a,const std::string& kind){
  if(a.baseTex.empty())return;
  // Match CPS-2 integer scroll registers: ROUND scroll FIRST, then subtract.
  // In particular, round(-88 - 0.75*138) = -191 is WRONG; -88-round(103.5) = -192.
  const int posX=a.originX-rnd(layerScrollX(layer));
  const int posY=a.originY-rnd(layerYRate(layer)*cy+layer.yoff)+layerYOffset(layer);
  const bool factoryUpper=(kind=="upper" && &layer==&w.layers[1]);
  // Preserve the exact raw screenshot for comparison (M off). With M on,
  // the backing has blank slots ONLY where source phase 89 matched each
  // animated tile exactly. These are refilled by drawFactoryUpperMirrors.
  const std::string texPath=(factoryUpper && opt.factoryMirrorAnimation)
      ?"assets/upper__layer1_phasecomplete.xrga":a.baseTex;
  auto& tex=textures.get(texPath);
  if(factoryUpper && opt.upperWrapQuarantine){
   // Stage62.4 incorrectly assumed atlas row 0 began a complete vertical
   // period. Original donor rows [0,72) == [256,328) pixel-for-pixel.
   // The clouds start at row 72; their factory footer continues to 328.
   // Show that complete captured sequence, without the next cloud repeat.
   const int cap=std::min(tex.h,328);
   const SDL_Rect srcRect{0,0,tex.w,cap};
   blit(texPath,&srcRect,SDL_Rect{posX,posY,tex.w,cap});
  }else blit(texPath,nullptr,SDL_Rect{posX,posY,tex.w,tex.h});
  if(a.cells.empty())return;
  const int phase=extPhase();
  for(const auto& c:a.cells){
   // Stage67: defer original Layer3 vapor cells until the true foreground pass.
   if(kind=="base" && &layer==&w.layers[3]
      && w.foregroundVaporCycle.count(std::make_pair(c.x,c.y)))continue;
   const int x=posX+(kind=="base"?c.x:c.x-a.originX),y=posY+(kind=="base"?c.y:c.y-a.originY);
   if(x>=VIEW_W||y>=VIEW_H||x+c.w<=0||y+c.h<=0)continue;
   int state=-1;
   if(kind=="base"){
    if(c.states.empty())continue;
    const int index=opt.nativeStableAtlas?std::min(89,static_cast<int>(c.states.size())-1):
      (opt.animate?std::min(fi,static_cast<int>(c.states.size())-1):0);
    state=c.states[index];
   }else{
    if(c.keys.empty())continue;
    state=c.keys.front().second;
    if(opt.animate){for(const auto& key:c.keys){if(key.first>phase)break;state=key.second;}}
   }
   if(state<0)continue;
   const SDL_Rect src{(state%a.columns)*16,(state/a.columns)*16,c.w,c.h};
   blit(a.sheetTex,&src,SDL_Rect{x,y,c.w,c.h});
  }
 }
 // Stage65.2: restore only artwork that has a PROVEN relationship
 // to the donor upper still. All 146 cells were an exact full-RGBA match
 // to that still at frame index 89; never draw a whole lower atlas over it.
 // Upper cell coordinates are source base positions minus the observed 256px
 // tilemap period. This is separate from the 256-unit scroll register wrap.
 // Every matched cell is fully opaque, so replacing it after the frozen
 // upper source cannot reveal or discard an underlying transparent pixel.
 // In native play upper artwork fades in across a full camera travel region.
 // The Stage75 proof keeps its original discrete policy unless explicitly enabled.
 Uint8 upperPriorityAlpha()const{
  if(!opt.nativeSmoothUpper)return cy<=180?255:0;
  // 0 at ground approach Y=224; fully opaque by upper camera Y=112.
  return static_cast<Uint8>(clamp(rnd((224.0-cy)*255.0/112.0),0,255));
 }
 void blitAlpha(const std::string& path,const SDL_Rect* src,const SDL_Rect& dst,Uint8 alpha){
  if(!alpha)return;
  auto& tex=textures.get(path);
  Uint8 saved=255;
  SDL_Texture* selected=textures.select(tex);
  SDL_GetTextureAlphaMod(selected,&saved);
  if(SDL_SetTextureAlphaMod(selected,alpha)!=0)throw std::runtime_error("SDL_SetTextureAlphaMod failed");
  const int rc=SDL_RenderCopy(render,selected,src,&dst);
  SDL_SetTextureAlphaMod(selected,saved);
  if(rc!=0)throw std::runtime_error("SDL_RenderCopy(alpha) failed");
 }
 void drawVerifiedUpperAnimation(const Layer& layer){
  if(!opt.verifiedUpperAnimation || !opt.animate || !opt.upper || !opt.verifiedUpperPriority || opt.donorFactoryReference || !upperPriorityAlpha())return;
  const Atlas& source=layer.base;
  const int sx0=rnd(layerScrollX(layer));
  const int sy0=rnd(layerYRate(layer)*cy+layer.yoff);
  for(const Cell& c:source.cells){
   if(!w.verifiedUpperCells.count(std::make_pair(c.x,c.y)))continue;
   const int legacyState=c.states[std::min(fi,static_cast<int>(c.states.size())-1)];
   const auto cycleIt=w.upperFactoryCycle.find(std::make_pair(c.x,c.y));
   if(cycleIt==w.upperFactoryCycle.end())throw std::runtime_error("Missing upper cycle for verified cell");
   const int state=opt.unifiedUpperClock?cycleIt->second[factoryPhase()]:legacyState;
   if(state<0)continue;
   const int x=source.originX+c.x-sx0;
   const int y=source.originY+c.y-256-sy0+layerYOffset(layer);
   if(x>=VIEW_W||y>=VIEW_H||x+c.w<=0||y+c.h<=0)continue;
   const SDL_Rect srcRect{(state%source.columns)*16,(state/source.columns)*16,c.w,c.h};
   if(opt.nativeSmoothUpper)blitAlpha(source.sheetTex,&srcRect,SDL_Rect{x,y,c.w,c.h},upperPriorityAlpha());
   else blit(source.sheetTex,&srcRect,SDL_Rect{x,y,c.w,c.h});
  }
 }
 // Real Layer3 smoke lies in front of the factory, not at the .75 parallax.
 // Source cells are rendered in a separate pass to enforce independent phase
 // and source-to-screen registration; this does NOT add imaginary smoke.
 void drawChimneySmoke(){
  if(!opt.chimneySmoke || !opt.upper || !w.layers[2].visible)return;
  const Layer& l=w.layers[2];
  const int phase=static_cast<int>(chimneyTick%w.chimneyPoses.size());
  const int pose=(!opt.animate)?w.chimneyPoses[0]:w.chimneyPoses[phase];
  const SDL_Rect src{0,pose*w.chimneyH,w.chimneyW,w.chimneyH};
  const SDL_Rect dst{w.chimneyX-rnd(layerScrollX(l)),w.chimneyY-rnd(l.ys*cy+l.yoff),w.chimneyW,w.chimneyH};
  blit(w.chimneyTexture,&src,dst);
 }
 void drawForegroundVapor(){
  if(!opt.foregroundVapor || !w.layers[3].visible)return;
  const Layer& l=w.layers[3];const Atlas& a=l.base;
  const int scrollX=rnd(layerScrollX(l)),scrollY=rnd(l.ys*cy+l.yoff);
  const int phase=static_cast<int>(((factoryTick-25)%45+45)%45);
  if(opt.animate&&opt.foregroundVaporSync){
   // One authentic pose on the Layer3 1:1 scroll, AFTER all scenery.
   // This draw exactly replaces the 20 verified source cell draws.
   const int pose=w.foregroundPoseByPhase[phase];
   const SDL_Rect src{0,pose*w.foregroundH,w.foregroundW,w.foregroundH};
   const SDL_Rect dst{w.foregroundX-scrollX,w.foregroundY-scrollY,w.foregroundW,w.foregroundH};
   if(dst.x<VIEW_W&&dst.y<VIEW_H&&dst.x+dst.w>0&&dst.y+dst.h>0)
    blit(w.foregroundPackedTexture,&src,dst);
   return;
  }
  // Debug: source 104-frame per-cell playback (Z) or frozen source (A).
  for(const Cell& c:a.cells){
   auto it=w.foregroundVaporCycle.find(std::make_pair(c.x,c.y));
   if(it==w.foregroundVaporCycle.end())continue;
   const int sourceIndex=opt.animate?std::min(fi,103):0;
   const int state=c.states[sourceIndex];
   if(state<0)continue;
   const int px=a.originX+c.x-scrollX,py=a.originY+c.y-scrollY;
   if(px>=VIEW_W||py>=VIEW_H||px+c.w<=0||py+c.h<=0)continue;
   const SDL_Rect src{(state%a.columns)*16,(state/a.columns)*16,c.w,c.h};
   blit(a.sheetTex,&src,SDL_Rect{px,py,c.w,c.h});
  }
 }

 // XM86.56: actor can be composited AFTER the scenery lighting grade.
 void drawActorOverlay(){
  if(opt.actor && auditPass[8]){
   if(const Actor* a=currentActor()){
    auto& tex=textures.get(a->image);
    blit(a->image,nullptr,SDL_Rect{rnd(a->x-cx),rnd(a->y-cy),tex.w,tex.h});
   }
  }
 }
 void draw(){
  if(opt.follow){cx=w.frames[fi].x;cy=w.frames[fi].y;limitCamera();}
  SDL_SetRenderDrawColor(render,9,17,27,255);SDL_RenderClear(render);
  for(int i=1;i<=3;++i){
   const Layer& l=w.layers[i];if(!l.visible || !auditPass[i])continue;
   // The quarantine follows the camera envelope, not the 256px donor VRAM
   // wrap seam. The old factory cutoff remains opt-in as a diagnostic.
   int left=0,top=0,right=VIEW_W,bottom=VIEW_H;
   if(opt.quarantine){
    left=std::max(left,rnd(l.xs*(w.xmin-layerCameraX(l))));
    top=std::max(top,rnd(layerYRate(l)*(w.ymin-cy)+layerYOffset(l)));
    right=std::min(right,rnd(l.xs*(w.xmax+VIEW_W-layerCameraX(l))));
    bottom=std::min(bottom,rnd(layerYRate(l)*(w.ymax+VIEW_H-cy)+layerYOffset(l)));
   }
   if(i==1 && opt.factoryWrapGuard){
    // Historical Stage62.1 debug cutoff; deliberately OFF by default.
    // It discarded genuine opaque CPS-2 donor pixels over much of the viewport.
    // Never infer real hardware camera bounds from a forced-register sweep.
    bottom=std::min(bottom,rnd(w.factoryWrapY-layerYRate(l)*cy-l.yoff+layerYOffset(l)));
   }
   if(opt.quarantine || (i==1 && opt.factoryWrapGuard)){
    const SDL_Rect clip{left,top,std::max(0,right-left),std::max(0,bottom-top)};
    SDL_RenderSetClipRect(render,&clip);
   }
   // Stage75.3: restore the missing light-blue sky UNDER the source-scrolling
   // factory and chimney. The original upper capture has a 128px transparent
   // x=861..988 gap, visible under the upper-right plume. This underlay
   // extends 640..1663 atlas X with the authentic last-column palette colors;
   // never paints over recovered machinery/cloud/smoke pixels. The source
   // upper atlases are drawn later in their original priority order.
   if(i==1 && auditPass[4] && opt.upper && !opt.donorFactoryReference){
    const auto& fill=textures.get(w.skyFillTexture);
    const auto& upper=w.layers[1].upper;
    const int px=upper.originX+w.skyFillX-rnd(layerScrollX(l));
    const int py=upper.originY+w.skyFillY-rnd(layerYRate(l)*cy+l.yoff)+layerYOffset(l);
    blit(w.skyFillTexture,nullptr,SDL_Rect{px,py,fill.w,fill.h});
   }
   // Natural camera at frame 11657 recorded 49,396 pixels absent from
   // all approved Layer-1 atlas backing at their visible world coordinates.
   // This patch draws first, underneath the known scenery, so the Stage62.6
   // ground-level and upper artwork retains exact precedence. Single frozen
   // capture phase; J toggles for visual audit. No virtual scroll or invented tiles.
   if(i==1 && auditPass[5] && opt.naturalUnderlay && !opt.donorFactoryReference && !w.naturalPatch.empty()){
    const auto& patch=textures.get(w.naturalPatch);
    const int px=w.naturalX-rnd(layerScrollX(l));
    const int py=w.naturalY-rnd(layerYRate(l)*cy+l.yoff)+layerYOffset(l);
    blit(w.naturalPatch,nullptr,SDL_Rect{px,py,patch.w,patch.h});
   }
   if(i==1 && opt.donorFactoryReference && opt.upper){
    // Raw 84-view donor replay comparison. Recovered original upper atlas
    // reproduces every opaque pixel at the recorded camera positions once
    // register rounding is corrected. Bypass other factory atlases, state
    // overlays and preview quarantines. This is intentionally frozen at
    // captured frame 4071, not a substitute for authentic long animation.
    SDL_RenderSetClipRect(render,nullptr);
    const Atlas& top=l.upper;
    const auto& raw=textures.get(top.baseTex);
    const int x=top.originX-rnd(layerScrollX(l));
    const int y=top.originY-rnd(layerYRate(l)*cy+l.yoff)+layerYOffset(l);
    blit(top.baseTex,nullptr,SDL_Rect{x,y,raw.w,raw.h});
   }else{
   if(opt.upper){
    drawAtlas(l,l.upper,"upper");
    if(i==1 && auditPass[6])drawFactoryUpperMirrors(l);
   }
   if(opt.left)drawAtlas(l,l.left,"left");
   drawAtlas(l,l.base,"base");
   // STAGE 65.1: Source precedence, not an invented parallax/row adjustment.
   // Stage57's original upper Layer-1 atlas is pixel-exact at all 84
   // recorded donor camera positions; Stage64.1 natural camera (121,0)
   // reproduces its complete 70,560 opaque pixels. Stage65 previously
   // painted asynchronous lower-scene cells over this correct image.
   // Repaint the original upper capture LAST when it represents upper
   // camera territory. Preserve the old ground-level composition at
   // cy=256, where its animated factory/cells were user-confirmed.
   if(i==1 && auditPass[6] && opt.verifiedUpperPriority && opt.upper){
    const Atlas& top=l.upper;
    const auto& raw=textures.get(top.baseTex);
    // Stage76.5 live game: interpolate the upper donor over the overlap
    // instead of hard-cutting at cy=180. Both captures retain their original
    // source coordinates. Preview behavior is unchanged unless opted in.
    if(upperPriorityAlpha()){
     const int maxWorldY=328;
     const int cap=std::max(0,std::min(raw.h,maxWorldY-top.originY));
     if(cap>0){
      const SDL_Rect rect{0,0,raw.w,cap};
      const int px=top.originX-rnd(layerScrollX(l));
      const int py=top.originY-rnd(layerYRate(l)*cy+l.yoff)+layerYOffset(l);
      if(opt.nativeSmoothUpper)blitAlpha(top.baseTex,&rect,SDL_Rect{px,py,raw.w,cap},upperPriorityAlpha());
      else blit(top.baseTex,&rect,SDL_Rect{px,py,raw.w,cap});
      // Paint the donor-verified animation AFTER the still, not beneath it.
      // The ground-level cell ordering remains unchanged at cy>180.
      drawVerifiedUpperAnimation(l);
     }
    }
   }
   }
   SDL_RenderSetClipRect(render,nullptr);
   // Chimney is Scroll Layer 2, so composite BEFORE foreground Layer 3.
   if(i==2 && auditPass[7])drawChimneySmoke();
  }
  // Foreground vapor is drawn after all scrolling layers, before Wily actor.
  if(auditPass[7])drawForegroundVapor();
  drawActorOverlay();
  // No SDL_RenderPresent: paint into the host video target instead.
 }
};

} // namespace xm76stage

#endif
