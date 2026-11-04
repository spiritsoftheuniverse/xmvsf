#ifndef XMVSF_STAGE87_57_FLASH_RULES_H
#define XMVSF_STAGE87_57_FLASH_RULES_H
// Stage87.57: pure, unit-testable classifier, independent of SDL and game RAM.
// The real native raster writes BGRA/BGRX in 32-bit FBNeo output.
#include <cstddef>
#include <cstdint>
namespace xm8757flash {
struct Coverage {unsigned nearWhite=0,white=0,rows=0;bool active=false;};
inline bool nearWhite(const unsigned char* p){return p[0]>=220 && p[1]>=220 && p[2]>=220;}
inline bool white(const unsigned char* p){return p[0]>=248 && p[1]>=248 && p[2]>=248;}
inline Coverage classify(const unsigned char* pixels,int width,int height,int pitch){
 Coverage out;
 if(!pixels || width<=0 || height<=0 || pitch<width*4)return out;
 for(int y=0;y<height;++y){
  const unsigned char* row=pixels+(std::size_t)y*pitch;
  unsigned rowNear=0;
  for(int x=0;x<width;++x){
   const unsigned char* p=row+4*x;
   if(nearWhite(p)){++out.nearWhite;++rowNear;}
   if(white(p))++out.white;
  }
  if(rowNear*4u>=unsigned(width)*3u)++out.rows;
 }
 // Strong coverage *and* scanline test. No timer, character ID or ROM patch.
 // The 87.56 capture observed 65,617/86,016 near-white and 147/224
 // white rows on the very first partial-wipe frame (2017).
 const std::uint64_t pixelsCount=(std::uint64_t)width*height;
 out.active=(std::uint64_t)out.nearWhite*10u>=pixelsCount*7u &&
             out.rows*2u>=unsigned(height);
 return out;
}
inline void compose(const unsigned char* native,int pitch,const unsigned char* sprites,
                    unsigned char* rgba,int width,int height){
 if(!native || !rgba)return;
 for(int y=0;y<height;++y){
  const unsigned char* row=native+(std::size_t)y*pitch;
  for(int x=0;x<width;++x){
   const unsigned char* p=row+4*x;
   unsigned char* d=rgba+4*((std::size_t)y*width+x);
   const std::size_t i=(std::size_t)y*width+x;
   d[0]=p[2];d[1]=p[1];d[2]=p[0]; // native BGRX -> SDL RGBA32
   // A native white/near-white flash is above the native fighters.
   // Colored objects/FX retain the native raster's original final color
   // wherever the filtered OBJ layer includes those pixels.
   d[3]=(nearWhite(p) || (sprites && sprites[4*i+3]))?255:0;
  }
 }
}
}
#endif
