#ifndef XMVSF_STAGE87_421_M68K_OVERLAY_H
#define XMVSF_STAGE87_421_M68K_OVERLAY_H
// F7 renders F10's exact phase-writer PPC into the native 384x224 view,
// before F6's existing composited PNG readback. Read-only, no game timing edits.
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../../burner/sdl/xmvsf_stage87_139_probe.h"
namespace XM87421Overlay {
static constexpr int W=384,H=32;
inline unsigned char glyph(char c,int y){
 static const unsigned char num[10][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,30,1,1,17,14},
 {6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,2,12}};
 static const unsigned char alpha[26][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
 {14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
 {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},
 {14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
 {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
 {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
 {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
 {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
 {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
 if(y<0||y>=7)return 0;
 if(c>='0'&&c<='9')return num[c-'0'][y];
 if(c>='A'&&c<='Z')return alpha[c-'A'][y];
 if(c==':')return y==2||y==4?4:0;
 if(c=='-')return y==3?31:0;
 if(c=='>')return y==2||y==4?8:y==3?4:0;
 return 0;
}
inline void put(unsigned char* p,int x,int y,unsigned char r,
                unsigned char g,unsigned char b,unsigned char a){
 if(x<0||x>=W||y<0||y>=H)return;
 unsigned char* q=p+(y*W+x)*4;
 q[0]=r;q[1]=g;q[2]=b;q[3]=a;
}
inline void print(unsigned char* p,int x,int y,const char* label,
                  unsigned char r,unsigned char g,unsigned char b){
 if(!label)return;
 for(int i=0;label[i]&&x+6*i+4<W;++i)
  for(int dy=0;dy<7;++dy){
   unsigned char bits=glyph(label[i],dy);
   for(int dx=0;dx<5;++dx)
    if(bits&(1u<<(4-dx)))put(p,x+i*6+dx,y+dy,r,g,b,255);
  }
}
struct State {
 bool enabled=false,first=true;
 SDL_Texture* texture=nullptr;
 SDL_Renderer* owner=nullptr;
 std::vector<unsigned char> bytes;
 void cleanup(){
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;owner=nullptr;bytes.clear();first=true;
 }
 void toggle(){
  enabled=!enabled;
  std::fprintf(stderr,"XM87.421 M68K_OVERLAY_TOGGLE enabled=%d F10=writer_PC F6=PNG\n",int(enabled));
 }
 void paint(SDL_Renderer* r,const SDL_Rect& view,unsigned frame,int phase,
            const XM87139Probe::State& probe){
  if(!enabled||!r||view.w<=0||view.h<=0)return;
  if(texture&&owner!=r)cleanup();
  if(!texture){
   texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,W,H);
   if(!texture){
    std::fprintf(stderr,"XM87.421 M68K_OVERLAY_CREATE_FAILED %s\n",SDL_GetError());return;
   }
   owner=r;SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
   SDL_SetTextureScaleMode(texture,SDL_ScaleModeNearest);
#endif
   bytes.resize(W*H*4u);
  }
  std::memset(bytes.data(),0,bytes.size());
  for(int y=0;y<H;++y)
   for(int x=0;x<254;++x)put(bytes.data(),x,y,6,14,28,220);
  char a[100],b[100],c[100];
  if(probe.active&&probe.latestValid){
   const auto& s=probe.latest;
   std::snprintf(a,sizeof(a),"F:%08u ST:%02X STEP:%04X SG:%02X",
                 frame,s.phase,s.step,s.stage);
  }else std::snprintf(a,sizeof(a),"F:%08u ST:%02X F10 OFF",
                     frame,unsigned(phase)&255u);
  if(probe.active&&probe.writerValid){
   std::snprintf(b,sizeof(b),"WRPC:%06X NEXT:%06X",probe.writerPpc,probe.writerNext);
   std::snprintf(c,sizeof(c),"EDGE:%02X>%02X AT:%08u",
                 probe.writerBefore,probe.writerAfter,probe.writerFrame);
  }else{
   std::snprintf(b,sizeof(b),"WRPC:------ F10 TO TRACE");
   std::snprintf(c,sizeof(c),"F7 HIDE F6 SAVE");
  }
  print(bytes.data(),4,3,a,161,225,255);
  print(bytes.data(),4,12,b,235,242,252);
  print(bytes.data(),4,21,c,254,211,120);
  if(SDL_UpdateTexture(texture,nullptr,bytes.data(),W*4)!=0)return;
  const int start=view.h*(224-H)/224;
  const SDL_Rect dst={view.x,view.y+start,view.w,view.h-start};
  SDL_RenderCopy(r,texture,nullptr,&dst);
  if(first){
   first=false;
   std::fprintf(stderr,
      "XM87.421 M68K_OVERLAY_PRESENT native=384x224 F7=toggle F10=writer F6=PNG\n");
  }
 }
};
inline State& state(){static State s;return s;}
}
#endif
