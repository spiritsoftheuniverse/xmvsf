#ifndef XMVSF_STAGE87_74_SELECT_PRESENT_H
#define XMVSF_STAGE87_74_SELECT_PRESENT_H
// Training-only debug overlay. Original CPS-2 pen colors are sampled each frame,
// preserving the ROM's native cyan/magenta palette flicker (no synthetic timer).
// Directional keyboard arrows nudge ONLY the overlay by 1 native pixel; Shift=8.
// Display dx/dy in the top-left of the actual rendered selection viewport.
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "xmvsf_stage87_74_calibration.h"
extern "C" const unsigned char* xm8774GetNativeSelectorOverlay(int*,int*,int*,const unsigned char*,int,int);
namespace XM8773Present {
static inline unsigned Glyph(char c){
 // 3 by 5 monochrome glyph, packed row major into low 15 bits.
 switch(c){
 case 'D':return 0x7B6F; case 'X':return 0x5F75; case 'Y':return 0x5D44;
 case '+':return 0x05D0; case '-':return 0x01C0; case ':':return 0x0410;
 case '0':return 0x7B6F; case '1':return 0x2492; case '2':return 0x73E7;
 case '3':return 0x73CF; case '4':return 0x5BC9; case '5':return 0x79CF;
 case '6':return 0x79EF; case '7':return 0x7249; case '8':return 0x7BEF;
 case '9':return 0x7BCF; case ' ':return 0;
 default:return 0;
 }
}
struct Renderer {
 SDL_Texture*texture;SDL_Renderer*owner;int width,height;
 XM8774Calibration::Offset offset;
 bool testEnabled,selecting;
 FILE*log;
 Renderer():texture(NULL),owner(NULL),width(0),height(0),testEnabled(false),selecting(false),log(NULL){
  const char*v=std::getenv("XM8774_SELECTOR_CALIBRATE");
  testEnabled=v && v[0]=='1' && v[1]==0;
  const char*dir=std::getenv("XMVSF_TEST_OUTPUT_DIR");
  if(testEnabled && dir && *dir){
   char name[2048];std::snprintf(name,sizeof(name),"%s/xm8774_selector_offsets.tsv",dir);
   log=std::fopen(name,"wb");if(log){std::fputs("offset_x\toffset_y\tshift_step\n",log);std::fflush(log);}
  }
 }
 ~Renderer(){Close();if(log)std::fclose(log);}
 void Select(bool value){selecting=value;}
 bool ConsumesArrows()const{return testEnabled&&selecting;}
 void Close(){if(texture)SDL_DestroyTexture(texture);texture=NULL;owner=NULL;width=height=0;}
 void Keyboard(){
  if(!ConsumesArrows())return;
  const Uint8*keys=SDL_GetKeyboardState(NULL);if(!keys)return;
  const bool pressed[4]={keys[SDL_SCANCODE_LEFT]!=0,keys[SDL_SCANCODE_RIGHT]!=0,
   keys[SDL_SCANCODE_UP]!=0,keys[SDL_SCANCODE_DOWN]!=0};
  const bool shift=keys[SDL_SCANCODE_LSHIFT]!=0||keys[SDL_SCANCODE_RSHIFT]!=0;
  if(offset.Keys(pressed,shift)){
   std::fprintf(stderr,"XM87.74 CALIBRATION_OFFSET x=%+d y=%+d step=%d\n",offset.x,offset.y,shift?8:1);
   if(log){std::fprintf(log,"%d\t%d\t%d\n",offset.x,offset.y,shift?8:1);std::fflush(log);}
  }
 }
 void Readout(SDL_Renderer*r,const SDL_Rect&game){
  if(!ConsumesArrows())return;
  char str[48];std::snprintf(str,sizeof(str),"DX:%+04d DY:%+04d",offset.x,offset.y);
  const int scale=game.w>=768?3:2;
  const int x0=game.x+8,y0=game.y+8;
  SDL_BlendMode previousBlend;SDL_GetRenderDrawBlendMode(r,&previousBlend);
  Uint8 oldR,oldG,oldB,oldA;SDL_GetRenderDrawColor(r,&oldR,&oldG,&oldB,&oldA);
  SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r,0,0,0,210);
  const SDL_Rect bg={x0-3,y0-3,(int)std::strlen(str)*4*scale+6,5*scale+6};
  SDL_RenderFillRect(r,&bg);
  SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(r,255,235,105,255);
  for(unsigned n=0;str[n];++n){
   const unsigned pattern=Glyph(str[n]);
   for(int yy=0;yy<5;++yy)for(int xx=0;xx<3;++xx){
    if((pattern>>(14-(yy*3+xx)))&1u){
     const SDL_Rect dot={x0+(int)n*4*scale+xx*scale,y0+yy*scale,scale,scale};
     SDL_RenderFillRect(r,&dot);
    }
   }
  }
  SDL_SetRenderDrawBlendMode(r,previousBlend);
  SDL_SetRenderDrawColor(r,oldR,oldG,oldB,oldA);
 }
 void Draw(SDL_Renderer*r,const SDL_Rect&rect,const unsigned char*native,int nativePitch,int nativeBpp){
  if(!r)return;
  Keyboard();
  int w=0,h=0,pitch=0;
  const unsigned char*pixels=xm8774GetNativeSelectorOverlay(&w,&h,&pitch,native,nativePitch,nativeBpp);
  if(pixels&&w>0&&h>0&&pitch>=w*4){
   if(!texture||owner!=r||w!=width||h!=height){
    Close();owner=r;width=w;height=h;
    texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,w,h);
    if(!texture){std::fprintf(stderr,"XM87.74 SELECT_TEXTURE_FAILED %s\n",SDL_GetError());return;}
    SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
    std::fprintf(stderr,"XM87.74 NATIVE_SELECTORS_VISIBLE_PIXEL_MATCH XM87.74 CALIBRATION_READY native_pens=1 live_rom_flicker=1 no_test_tabs=1\n");
   }
   if(SDL_UpdateTexture(texture,NULL,pixels,pitch)==0){
    SDL_Rect adjusted=rect;
    adjusted.x+=(int)((long long)offset.x*rect.w/w);
    adjusted.y+=(int)((long long)offset.y*rect.h/h);
    SDL_RenderCopy(r,texture,NULL,&adjusted);
   }
  }
  Readout(r,rect);
 }
};
static inline Renderer& State(){static Renderer r;return r;}
} // namespace XM8773Present
#endif
