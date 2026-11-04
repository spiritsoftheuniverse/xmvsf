#ifndef XMVSF_STAGE87_73_SELECT_PRESENT_H
#define XMVSF_STAGE87_73_SELECT_PRESENT_H
// Test-only exact-pixel original selector overlay AFTER DF selection grayscale.
// No fake tabs/boxes. The source mask is built from the decoded CPS2 OAM pens.
#include <SDL.h>
#include <cstdio>
#include <cstdint>
extern "C" const unsigned char* xm8773GetNativeSelectorOverlay(int*,int*,int*);
namespace XM8773Present {
struct Renderer {
 SDL_Texture*texture;SDL_Renderer*owner;int width,height;
 Renderer():texture(NULL),owner(NULL),width(0),height(0){}
 void Close(){if(texture)SDL_DestroyTexture(texture);texture=NULL;owner=NULL;width=height=0;}
 void Draw(SDL_Renderer*r,const SDL_Rect&rect){
  if(!r)return;
  int w=0,h=0,pitch=0;
  const unsigned char* pixels=xm8773GetNativeSelectorOverlay(&w,&h,&pitch);
  if(!pixels||w<1||h<1||pitch<w*4)return;
  if(!texture||owner!=r||w!=width||h!=height){
   Close();owner=r;width=w;height=h;
   texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,w,h);
   if(!texture){std::fprintf(stderr,"XM87.73 SELECT_TEXTURE_FAILED %s\n",SDL_GetError());return;}
   SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
   std::fprintf(stderr,"XM87.73 NATIVE_SELECTORS_COLORED post_df=1 native_pens=1 no_test_graphics=1\n");
  }
  if(SDL_UpdateTexture(texture,NULL,pixels,pitch)==0)
   SDL_RenderCopy(r,texture,NULL,&rect);
 }
};
static inline Renderer& State(){static Renderer r;return r;}
} // namespace XM8773Present
#endif
