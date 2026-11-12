#ifndef XMVSF_STAGE87_278_PINK_LIGHT_H
#define XMVSF_STAGE87_278_PINK_LIGHT_H
// Final Destination: source-color-derived neon mask and world-locked
// trapezoidal sweeps. The original PNG is read, never rewritten.
// The original platform keeps its exact artist colors; only light within
// the source-pink mask is additively brightened by a maximum of 30%.
// All coordinates and textures are in the original PNG's pixel space.
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace XMFDLight {
struct Lane {
 float x0,y0,x1,y1;
 unsigned repeat,framesPerPixel,phase;
};
struct Pixel {
 std::uint32_t index;
 short along,normal;
 unsigned char lane,alpha,red,green,blue;
};
struct Mask {
 SDL_Texture* light=nullptr;
 int width=0,height=0;
 std::vector<Pixel> keyed;
 std::vector<Uint8> frameRGBA;
 unsigned long long previousFrame=~0ull;

 void close(){
  if(light)SDL_DestroyTexture(light);
  light=nullptr;width=height=0;
  previousFrame=~0ull;
  keyed.clear();frameRGBA.clear();
 }
 // Lane directions are traced against the six prominent sloped/straight
 // neon rails in the 600x368 reference art; normalized coordinates permit
 // the user to replace the source PNG at another resolution.
 static const Lane* lanes(){
  static const Lane s[]={
   {  42.f,316.f,228.f,283.f,  99u,5u, 0u}, // top-left rising rail
   { 372.f,283.f,558.f,316.f, 101u,5u,31u}, // top-right falling rail
   {  56.f,316.f,544.f,316.f, 119u,6u,53u}, // front horizontal upper strip
   {  10.f,356.f,178.f,341.f,  91u,5u,17u}, // front left perspective edge
   { 422.f,341.f,590.f,356.f,  89u,5u,69u}, // front right perspective edge
   { 178.f,293.f,421.f,293.f, 103u,6u,41u}, // distant top inner rail
   {  98.f,358.f,502.f,358.f, 113u,6u,81u}  // lower horizontal neon
  };
  return s;
 }
 static int laneCount(){return 7;}
 static int clamp255(int n){return n<0?0:(n>255?255:n);}
 // The prepared surface retains ALL original pixel colors and alpha.
 // Pink source pixels are recorded in a separate mask for additive light.
 SDL_Surface* prepare(SDL_Renderer* renderer,SDL_Surface* original){
  if(!renderer || !original || original->w<=0 || original->h<=0)return nullptr;
  close();
  SDL_Surface* base=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
  if(!base)return nullptr;
  width=base->w;height=base->h;
  const Lane* refs=lanes();
  for(int y=0;y<height;++y){
   Uint8* row=(Uint8*)base->pixels+(size_t)y*(size_t)base->pitch;
   for(int x=0;x<width;++x){
    Uint8* p=row+(size_t)x*4u;
    const int r=p[0],g=p[1],b=p[2],a=p[3];
    const int magenta=std::min(r-g,b-g);
    if(a==0 || r<95 || b<95 || magenta<40)continue;
    // R/B significantly above G marks the original luminous pink,
    // excluding purple metal shading. Partial edge alpha is retained.
    const int strength=clamp255((magenta-32)*5);
    const int extracted=a*strength/255;
    if(extracted<=0)continue;
    const float fx=(float)x*600.f/(float)width;
    const float fy=(float)y*368.f/(float)height;
    float best=1.0e30f;
    int which=0,along=0,normal=0;
    for(int lane=0;lane<laneCount();++lane){
     const Lane& l=refs[lane];
     const float dx=l.x1-l.x0,dy=l.y1-l.y0;
     const float len=std::sqrt(dx*dx+dy*dy);
     const float ax=fx-l.x0,ay=fy-l.y0;
     const float path=(ax*dx+ay*dy)/len;
     const float side=(ay*dx-ax*dy)/len;
     const float bounded=std::max(0.f,std::min(len,path));
     const float outside=path-bounded;
     const float distance=side*side+outside*outside*4.f;
     if(distance<best){
      best=distance;which=lane;
      along=(int)std::floor(path+0.5f);
      normal=(int)std::floor(side+0.5f);
     }
    }
    Pixel q;
    q.index=(std::uint32_t)((size_t)y*(size_t)width+(size_t)x);
    q.along=(short)std::max(-32767,std::min(32767,along));
    q.normal=(short)std::max(-32767,std::min(32767,normal));
    q.lane=(unsigned char)which;
    q.alpha=(unsigned char)extracted;
    q.red=(Uint8)r;q.green=(Uint8)g;q.blue=(Uint8)b;
    keyed.push_back(q);
    // No holes, recoloring, or alpha changes to the original platform.
    // Only the separate additively blended streak light varies.
   }
  }
  frameRGBA.resize((size_t)width*(size_t)height*4u,0);
  light=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_STREAMING,width,height);
  if(!light){
   std::fprintf(stderr,"XM87.279 FD_LIGHT_TEXTURE_FAILED %s\n",SDL_GetError());
   SDL_FreeSurface(base);close();return nullptr;
  }
  SDL_SetTextureBlendMode(light,SDL_BLENDMODE_ADD);
  SDL_SetTextureScaleMode(light,SDL_ScaleModeNearest);
  std::fprintf(stderr,
   "XM87.279 FD_PINK_MASK image=%dx%d pixels=%zu lanes=%d effect=world_trapezoids filter=nearest\n",
   width,height,keyed.size(),laneCount());
  return base;
 }
 bool update(unsigned long long frame){
  if(!light)return false;
  if(frame==previousFrame)return true;
  std::memset(frameRGBA.data(),0,frameRGBA.size());
  const Lane* refs=lanes();
  for(const Pixel& p:keyed){
   const Lane& lane=refs[p.lane];
   // One source pixel per 5-6 emulated frames; previously 2-3 pixels
   // advanced PER frame, so this is substantially slower.
   const unsigned advance=(unsigned)((frame/lane.framesPerPixel+lane.phase)%lane.repeat);
   // The sweep moves along the rail tangent; lateral skew creates
   // trapezoidal, angled leading/trailing boundaries, not vertical bars.
   const int cross=std::max(-20,std::min(20,(int)p.normal));
   const int displaced=(int)p.along-(int)advance-cross/2;
   const int pos=((displaced%(int)lane.repeat)+(int)lane.repeat)%(int)lane.repeat;
   const int front=std::max(0,5+cross/3);
   const int back=std::max(front+1,37+cross/2);
   int glow=0;
   if(pos>=front && pos<back){
    const int fade=std::min(pos-front,back-1-pos);
    glow=clamp255(45+fade*38);
   }
   // Preserve the original hue, plus at most 30% of each original
   // RGB channel. Outside a passing trapezoid there is zero addition.
   // The base platform remains unchanged in the composited result.
   Uint8* out=frameRGBA.data()+(size_t)p.index*4u;
   out[0]=(Uint8)((unsigned)p.red*30u/100u);
   out[1]=(Uint8)((unsigned)p.green*30u/100u);
   out[2]=(Uint8)((unsigned)p.blue*30u/100u);
   out[3]=(Uint8)((unsigned)p.alpha*(unsigned)glow/255u);
  }
  if(SDL_UpdateTexture(light,nullptr,frameRGBA.data(),width*4)!=0){
   std::fprintf(stderr,"XM87.279 FD_LIGHT_UPLOAD_FAILED %s\n",SDL_GetError());
   return false;
  }
  previousFrame=frame;
  return true;
 }
};
} // namespace XMFDLight
#endif
