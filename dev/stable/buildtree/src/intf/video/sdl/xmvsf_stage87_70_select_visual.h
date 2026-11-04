#ifndef XMVSF_STAGE87_70_SELECT_VISUAL_H
#define XMVSF_STAGE87_70_SELECT_VISUAL_H
// Stage87.71 retains Stage87.70's post-Double-Feature owner indicators.
// The old fixed-96-event GPU-screen capture has been retired entirely.
#include <SDL.h>
#include <algorithm>
#include <cstdio>
namespace XM8770 {
struct Tracker {
 void Overlay(SDL_Renderer*r,const SDL_Rect&game,int owner,bool training,bool selecting){
  if(!training||!selecting||!r)return;
  Uint8 oldR=0,oldG=0,oldB=0,oldA=0;
  SDL_GetRenderDrawColor(r,&oldR,&oldG,&oldB,&oldA);
  SDL_BlendMode oldBlend;SDL_GetRenderDrawBlendMode(r,&oldBlend);
  SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
  const int width=std::max(26,game.w*52/384),height=std::max(7,game.h*13/224);
  for(int p=0;p<2;++p){
   const SDL_Rect tab={p?game.x+game.w-width-7:game.x+7,
     game.y+game.h-height-8,width,height};
   if(p==owner){
    if(p==0)SDL_SetRenderDrawColor(r,0,220,255,255);
    else SDL_SetRenderDrawColor(r,255,135,32,255);
   }else SDL_SetRenderDrawColor(r,28,38,55,255);
   SDL_RenderFillRect(r,&tab);
   SDL_SetRenderDrawColor(r,255,255,255,255);
   SDL_Rect tick={tab.x+4,tab.y+2,2,tab.h>5?tab.h-4:2};
   SDL_RenderFillRect(r,&tick);
   if(p){tick.x+=5;SDL_RenderFillRect(r,&tick);}
  }
  SDL_SetRenderDrawBlendMode(r,oldBlend);
  SDL_SetRenderDrawColor(r,oldR,oldG,oldB,oldA);
  static bool announced=false;
  if(!announced){
   std::fprintf(stderr,"XM87.70 FINAL_SELECT_COLOR post_DF=1 native_cursor_unchanged=1\n");
   announced=true;
  }
 }
};
static inline Tracker& State(){static Tracker s;return s;}
}
#endif
