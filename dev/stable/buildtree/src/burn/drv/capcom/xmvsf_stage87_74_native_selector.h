#ifndef XMVSF_STAGE87_73_NATIVE_SELECTOR_H
#define XMVSF_STAGE87_73_NATIVE_SELECTOR_H
// Stage87.74 TEST ONLY. Native selection outline, identified by actual ROM pens.
// No guessed tile IDs, no rectangular fake graphics, no gameplay-state writes.
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstdio>
#include <cstring>
namespace XM8773 {
enum Role { None=0, P1=1, P2=2 };
static inline Role ByPens(unsigned mask) {
 const unsigned one=(1u<<13)|(1u<<14)|(1u<<15);
 const unsigned two=(1u<<10)|(1u<<11)|(1u<<12);
 return mask && !(mask&~one)?P1:mask && !(mask&~two)?P2:None;
}
static inline unsigned TilePens(const std::uint8_t* src) {
 unsigned m=0; if(!src)return 0;
 for(unsigned k=0;k<128;++k){unsigned v=src[k];
  if(v&15)m|=1u<<(v&15);
  if(v>>4)m|=1u<<(v>>4);
 }return m;
}
static inline size_t TileByte(unsigned t,size_t mask){return (size_t(t)<<7)&mask;}
static inline Role ObjectRole(unsigned prio,unsigned pal,int x,int y,unsigned tile,
 unsigned wide,unsigned tall,const std::uint8_t*gfx,size_t length,size_t gfxMask){
 if(prio!=7 || pal!=27 || x>=360 || y>=224 || x+int(wide*16)<=24 || y+int(tall*16)<=16 ||
    !gfx || wide==0 || tall==0 || wide>16 || tall>16 || length<128)return None;
 unsigned used=0;
 for(unsigned yy=0;yy<tall;++yy)for(unsigned xx=0;xx<wide;++xx){
  unsigned t=(tile&~15u)+(yy<<4)+((tile+xx)&15u);
  size_t addr=TileByte(t,gfxMask);
  if(addr>length-128)return None;
  used|=TilePens(gfx+addr);
  if(used && ByPens(used)==None)return None;
 }
 return ByPens(used);
}
static inline bool Allow(Role r,bool trainingSelect,unsigned p1Picks){
 return trainingSelect && (r==P1 || (r==P2 && p1Picks>=2));
}
// Packed CPS2 tile: two pixels per byte, display order reads high/low nibble
// from each group of four bytes in descending byte order (ctv_do.h).
static inline unsigned Pen(const std::uint8_t*tile,unsigned x,unsigned y){
 const unsigned pair=x>>1;
 const unsigned index=y*8+(pair&~3u)+(3u-(pair&3u));
 const unsigned b=tile[index];return (x&1u)?(b&15u):(b>>4);
}
// Preserve the ROM's own frame-by-frame pen colors and flicker. No fixed
// palette, blinking timer, or artificial animation is applied.
static inline void ColorFromNative(unsigned raw,unsigned bpp,std::uint8_t*dst){
 if(bpp==2){ // RGB565 native framebuffer
  const unsigned r=(raw>>11)&31u,g=(raw>>5)&63u,b=raw&31u;
  dst[0]=(std::uint8_t)((r*255u+15u)/31u);
  dst[1]=(std::uint8_t)((g*255u+31u)/63u);
  dst[2]=(std::uint8_t)((b*255u+15u)/31u);
 }else{ // native SDL_PIXELFORMAT_RGB888 (0x00RRGGBB)
  dst[0]=(std::uint8_t)(raw>>16);dst[1]=(std::uint8_t)(raw>>8);
  dst[2]=(std::uint8_t)raw;
 }
 dst[3]=255;
}
// Stage87.75: the selector pen identities are verified by ROM tile decode.
// Protect their role-specific native hue when the cached effective CPS palette
// temporarily resolves to an unrelated color (reported dark-green P2).
// No independent flash/blink timer: live palette changes drive the intensity,
// and valid native colors pass through unchanged.
// Stage87.76 selector: the original palette 27's P2 pens render as bright
// magenta/pink in the verified Stage87.72 native screen, not burgundy.
// P1 remains the original Stage87.75 implementation unchanged.
// Keep the live ROM pen luma as the intensity signal; NEVER run a new blink timer.
static inline void CorrectSelectorColor(Role role,unsigned pen,unsigned raw,unsigned bpp,std::uint8_t*dst){
 ColorFromNative(raw,bpp,dst);
 if(role==P2 && pen>=10u && pen<=12u){
  // Direct reference values sampled from native P2 border pixels, rather
  // than dark medians erroneously used by Stage87.75.
  static const std::uint8_t magenta[3][3] = {
   {202,15,204}, {236,13,236}, {245,11,245}
  };
  const unsigned r=dst[0],g=dst[1],b=dst[2];
  const unsigned sourceLuma=(54u*r+183u*g+19u*b+128u)>>8;
  // Brighter pink without flattening the three original selector pens:
  // changes to the ROM palette continue modulating this exact paint.
  const unsigned gain=230u+(sourceLuma*26u)/255u;
  const std::uint8_t* ref=magenta[pen-10u];
  for(unsigned c=0;c<3u;++c){
   const unsigned v=(unsigned(ref[c])*gain+128u)>>8;
   dst[c]=(std::uint8_t)(v>255u?255u:v);
  }
  dst[3]=255u;
  static bool reported=false;
  if(!reported){std::fprintf(stderr,"XM87.76 P2_MAGENTA_CALIBRATED original_pens=10_11_12\n");reported=true;}
  return;
 }
 const unsigned r=dst[0],g=dst[1],b=dst[2];
 const bool valid=((b>g+8u && b>r+8u) || (g>r+24u && b>r+24u));
 if(valid)return;
 static const std::uint8_t p1[3][3]={{63,47,112},{76,131,192},{109,219,225}};
 if(role!=P1 || pen<13u || pen>15u)return;
 const std::uint8_t*base=p1[pen-13u];
 const unsigned lum=(54u*r+183u*g+19u*b+128u)>>8;
 const unsigned factor=224u+(lum*64u)/255u;
 for(unsigned c=0;c<3u;++c){unsigned v=(base[c]*factor+128u)>>8;
   dst[c]=(std::uint8_t)(v>255u?255u:v);}
 dst[3]=255;
}
// The supplemental pixels must be present in the *actual* native framebuffer.
// Without this visibility test a topmost overlay resurrects masked/occluded
// outlines, and can appear offset from their real cursor position.
struct NativeLayer {
 unsigned frame,picks,objects[3],width,height;
 bool enabled;
 std::vector<std::uint8_t> rgba;
 std::vector<std::uint32_t> nativeExpected;
 NativeLayer():frame(~0u),picks(0),width(0),height(0),enabled(false){objects[0]=objects[1]=objects[2]=0;}
 void Begin(unsigned f,bool selection,unsigned accepted,unsigned w,unsigned h){
  if(frame==f)return;
  frame=f;picks=accepted;enabled=selection && w>=160 && w<=1024 && h>=96 && h<=768;
  width=w;height=h;objects[0]=objects[1]=objects[2]=0;
  if(enabled){
   const size_t count=(size_t)w*h;
   if(rgba.size()!=count*4)rgba.resize(count*4);
   if(nativeExpected.size()!=count)nativeExpected.resize(count);
   std::memset(rgba.data(),0,rgba.size());
   std::memset(nativeExpected.data(),0,count*sizeof(std::uint32_t));
  }
 }
 // Stamp the *same native tile position and flip* used by CpstOneObjDoX,
 // not an independently reassembled composite with guessed orientation.
 void StampTile(Role role,int x,int y,const std::uint8_t*tile,
                unsigned flip,const std::uint32_t*palette,
                const std::uint32_t*originalPalette,unsigned nativeBpp){
  if(!enabled||!Allow(role,true,picks)||!tile||!palette||!originalPalette||
     (nativeBpp!=2 && nativeBpp!=4))return;
  ++objects[role];
  for(unsigned py=0;py<16;++py)for(unsigned px=0;px<16;++px){
   const unsigned ix=(flip&1u)?15u-px:px;
   const unsigned iy=(flip&2u)?15u-py:py;
   const unsigned pen=Pen(tile,ix,iy);
   if((role==P1 && (pen<13||pen>15))||(role==P2 && (pen<10||pen>12)))continue;
   const int xx=x+(int)px,yy=y+(int)py;
   if(xx<0||yy<0||xx>=int(width)||yy>=int(height))continue;
   const size_t pos=(size_t)yy*width+(unsigned)xx;
   CorrectSelectorColor(role,pen,originalPalette[pen],nativeBpp,&rgba[pos*4]);
   nativeExpected[pos]=palette[pen];
  }
 }
 // A post-grade recolor is allowed only where the original renderer still
 // contains the exact selector pen. Occluded pixels are not reintroduced.
 const std::uint8_t* ReadChecked(int*w,int*h,int*pitch,
   const std::uint8_t* native,int nativePitch,int nativeBpp){
  if(!enabled||rgba.empty()||objects[1]+objects[2]==0 || !native ||
     (nativeBpp!=2 && nativeBpp!=4) || nativePitch<int(width)*nativeBpp)return NULL;
  unsigned seen=0,retained=0;
  for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
   const size_t pos=(size_t)y*width+x;
   std::uint8_t*pixel=&rgba[pos*4];
   if(pixel[3]==0)continue;
   ++seen;
   std::uint32_t actual=0;
   std::memcpy(&actual,native+(size_t)y*nativePitch+(size_t)x*nativeBpp,(size_t)nativeBpp);
   const std::uint32_t expected=nativeExpected[pos];
   const std::uint32_t mask=nativeBpp==2?0xffffu:0x00ffffffu;
   if((actual&mask)!=(expected&mask))pixel[3]=0;
   else ++retained;
  }
  static unsigned reports=0;
  if(reports==0)std::fprintf(stderr,"XM87.75 SELECTOR_PEN_HUE_GUARD enabled=1 native_palette_or_reference=1\n");
  if(seen && (reports++<12u || reports%120u==0u))
   std::fprintf(stderr,"XM87.74 SELECT_PIXEL_MATCH frame=%u tested=%u retained=%u\n",frame,seen,retained);
  if(!retained)return NULL;
  *w=(int)width;*h=(int)height;*pitch=(int)width*4;return rgba.data();
 }
};
// Shared selector buffer for the CPS object draw translation unit. The video
// presentation path accesses this same instance via xm8773GetNativeSelectorOverlay.
// Explicitly provide the accessor used by the production integration; the
// Stage 87.73 original package omitted it and failed to compile.
static inline NativeLayer& State() {
 static NativeLayer layer;
 return layer;
}
} // namespace XM8773
#endif
