#ifndef XMVSF_STAGE87_130_PLACEMENT_PROBE_H
#define XMVSF_STAGE87_130_PLACEMENT_PROBE_H
// 87.130 read-only placement audit. No changes to ROM, render targets, transforms, or pixels.
#include <SDL.h>
extern SDL_Window* sdlWindow;
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
namespace XM87130 {
static inline bool enabled(){
  const char* s=std::getenv("XM87130_VIDEO_DIR");return s && *s;
}
static inline const char* dir(){const char* s=std::getenv("XM87130_VIDEO_DIR");return s?s:"";}
static inline void geometry(const char* where,SDL_Window* win,SDL_Renderer* r,
                            int phase,unsigned frame,const SDL_Rect* dest=nullptr,
                            SDL_Texture* texture=nullptr){
 if(!enabled()||!win||!r)return;
 int wx=0,wy=0,ww=0,wh=0,ow=0,oh=0,lw=0,lh=0,display=-1;
 SDL_Rect vp={0,0,0,0},clip={0,0,0,0};float sx=0,sy=0;
 SDL_GetWindowPosition(win,&wx,&wy);SDL_GetWindowSize(win,&ww,&wh);
 SDL_GetRendererOutputSize(r,&ow,&oh);SDL_RenderGetLogicalSize(r,&lw,&lh);
 SDL_RenderGetViewport(r,&vp);SDL_RenderGetScale(r,&sx,&sy);
 bool clipped=SDL_RenderIsClipEnabled(r)==SDL_TRUE;
 if(clipped)SDL_RenderGetClipRect(r,&clip);
 display=SDL_GetWindowDisplayIndex(win);
 SDL_Rect desktop={0,0,0,0};float dpi=0,hdpi=0,vdpi=0;
 if(display>=0){SDL_GetDisplayBounds(display,&desktop);SDL_GetDisplayDPI(display,&dpi,&hdpi,&vdpi);}
 int top=-1,left=-1,bottom=-1,right=-1;
 SDL_GetWindowBordersSize(win,&top,&left,&bottom,&right);
 SDL_RendererInfo rendererInfo={};SDL_GetRendererInfo(r,&rendererInfo);
 SDL_Texture* target=SDL_GetRenderTarget(r);
 int tw=0,th=0,access=0;Uint32 fmt=0;
 if(texture)SDL_QueryTexture(texture,&fmt,&access,&tw,&th);
 
 std::fprintf(stderr,
  "XM87.130 GEOM point=%s phase=%02X frame=%u window=%d,%d,%d,%d display=%d flags=%08X "
  "display_bounds=%d,%d,%d,%d dpi=%.2f,%.2f,%.2f borders=%d,%d,%d,%d driver=%s "
  "output=%dx%d logical=%dx%d scale=%.6f,%.6f viewport=%d,%d,%d,%d "
  "clipEnabled=%d clip=%d,%d,%d,%d target=%p "
  "dest=%d,%d,%d,%d texture=%dx%d texformat=%08X texaccess=%d\n",
  where,phase&255,frame,wx,wy,ww,wh,display,SDL_GetWindowFlags(win),
  desktop.x,desktop.y,desktop.w,desktop.h,double(dpi),double(hdpi),double(vdpi),
  top,left,bottom,right,rendererInfo.name?rendererInfo.name:"?",ow,oh,
  lw,lh,double(sx),double(sy),vp.x,vp.y,vp.w,vp.h,int(clipped),
  clip.x,clip.y,clip.w,clip.h,(void*)target,
  dest?dest->x:0,dest?dest->y:0,dest?dest->w:0,dest?dest->h:0,
  tw,th,(unsigned)fmt,access);
}
struct BandSummary {int x0=-1,y0=-1,x1=-1,y1=-1;unsigned bright[4]={0,0,0,0};uint64_t sampleHash=1469598103934665603ULL;};
static inline BandSummary analyze(const uint8_t* data,int w,int h,int pitch,int bpp){
 BandSummary s;
 if(!data||w<1||h<1||pitch<w*bpp||(bpp!=2&&bpp!=4))return s;
 for(int y=0;y<h;y+=3){for(int x=0;x<w;x+=3){
  const uint8_t* p=data+(size_t)y*pitch+(size_t)x*bpp;
  unsigned r=0,g=0,b=0;
  if(bpp==4){uint32_t pixel=0;std::memcpy(&pixel,p,4);
     r=(pixel>>16)&255u;g=(pixel>>8)&255u;b=pixel&255u;}
  else {uint16_t pixel=0;std::memcpy(&pixel,p,2);
     r=((pixel>>11)&31u)*255u/31u;g=((pixel>>5)&63u)*255u/63u;b=(pixel&31u)*255u/31u;}
  const unsigned l=(77u*r+150u*g+29u*b+128u)>>8;
  s.sampleHash^=uint64_t((r<<16)|(g<<8)|b);s.sampleHash*=1099511628211ULL;
  if(l>48){if(s.x0<0||x<s.x0)s.x0=x;if(s.y0<0||y<s.y0)s.y0=y;
     if(x>s.x1){s.x1=x;}
     if(y>s.y1){s.y1=y;}
     ++s.bright[(4*y)/h];}
 }}return s;
}
static inline void report(const char* point,int phase,unsigned frame,
                          const uint8_t* data,int w,int h,int pitch,int bpp){
 if(!enabled())return;
 const BandSummary s=analyze(data,w,h,pitch,bpp);
 std::fprintf(stderr,
   "XM87.130 PIXELS point=%s phase=%02X frame=%u size=%dx%d pitch=%d bpp=%d "
   "bbox_luma_gt48=%d,%d,%d,%d yquartiles=%u,%u,%u,%u samplehash=%016llX\n",
   point,phase&255,frame,w,h,pitch,bpp,s.x0,s.y0,s.x1,s.y1,
   s.bright[0],s.bright[1],s.bright[2],s.bright[3],
   (unsigned long long)s.sampleHash);
}
static inline void captureNative(const uint8_t* data,int w,int h,int pitch,int bpp,int phase,unsigned frame){
 if(!enabled()||!data||w<1||h<1||pitch<w*bpp||(bpp!=2&&bpp!=4))return;
 char path[1024];
 const int n=std::snprintf(path,sizeof path,"%s/xm87130_%02X_%06u_native.bmp",dir(),phase&255,frame);
 if(n<0||(size_t)n>=sizeof path)return;
 const Uint32 format=bpp==4?SDL_PIXELFORMAT_RGB888:SDL_PIXELFORMAT_RGB565;
 SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormatFrom((void*)data,w,h,bpp*8,pitch,format);
 if(!surf){std::fprintf(stderr,"XM87.130 NATIVE_SURFACE_ERROR %s\n",SDL_GetError());return;}
 const int rc=SDL_SaveBMP(surf,path);SDL_FreeSurface(surf);
 std::fprintf(stderr,"XM87.130 NATIVE_CAPTURE phase=%02X frame=%u path=%s result=%d%s%s\n",
   phase&255,frame,path,rc,rc?" error=":"",rc?SDL_GetError():"");
}
static inline void capture(SDL_Renderer* r,const char* point,int phase,unsigned frame){
 if(!enabled()||!r||SDL_GetRenderTarget(r)!=nullptr)return;
 int w=0,h=0;
 if(SDL_GetRendererOutputSize(r,&w,&h)!=0||w<1||h<1||w>4096||h>4096)return;
 std::vector<Uint32> px((size_t)w*h);
 const int rc=SDL_RenderReadPixels(r,nullptr,SDL_PIXELFORMAT_ARGB8888,px.data(),w*4);
 if(rc){std::fprintf(stderr,"XM87.130 READBACK_FAILED point=%s code=%s\n",point,SDL_GetError());return;}
 report(point,phase,frame,(const uint8_t*)px.data(),w,h,w*4,4);
 // One diagnostic BMP per checkpoint. No textures or render coordinates changed.
 char filename[1024];
 const int n=std::snprintf(filename,sizeof filename,"%s/xm87130_%02X_%06u_%s.bmp",dir(),phase&255,frame,point);
 if(n<0||(size_t)n>=sizeof filename)return;
 SDL_Surface* surf=SDL_CreateRGBSurfaceWithFormatFrom(px.data(),w,h,32,w*4,SDL_PIXELFORMAT_ARGB8888);
 if(!surf){std::fprintf(stderr,"XM87.130 BMP_SURFACE_ERROR %s\n",SDL_GetError());return;}
 const int save=SDL_SaveBMP(surf,filename);SDL_FreeSurface(surf);
 std::fprintf(stderr,"XM87.130 CAPTURE point=%s phase=%02X frame=%u path=%s result=%d%s%s\n",
    point,phase&255,frame,filename,save,save?" error=":"",save?SDL_GetError():"");
}
} // namespace XM87130
#endif
