#ifndef XMVSF_STAGE87_73_NATIVE_SELECTOR_H
#define XMVSF_STAGE87_73_NATIVE_SELECTOR_H
// Stage87.73 TEST ONLY. Native selection outline, identified by actual ROM pens.
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
 if(prio!=7 || pal!=27 || x<35 || x>=330 || y<88 || y>=224 ||
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
static inline void Color(Role role,unsigned pen,std::uint8_t*dst){
 static const std::uint8_t p1[3][3]={{12,128,190},{0,205,245},{132,245,255}};
 static const std::uint8_t p2[3][3]={{155,30,145},{238,51,204},{255,178,244}};
 const int idx=int(pen)-(role==P1?13:10);
 if(idx<0||idx>=3)return;
 const std::uint8_t* rgb=(role==P1?p1[idx]:p2[idx]);
 dst[0]=rgb[0];dst[1]=rgb[1];dst[2]=rgb[2];dst[3]=255;
}
struct NativeLayer {
 unsigned frame,picks,objects[3],width,height;
 bool enabled;
 std::vector<std::uint8_t> rgba;
 NativeLayer():frame(~0u),picks(0),width(0),height(0),enabled(false){objects[0]=objects[1]=objects[2]=0;}
 void Begin(unsigned f,bool selection,unsigned accepted,unsigned w,unsigned h){
  if(frame==f)return;
  frame=f;picks=accepted;enabled=selection && w>=160 && w<=1024 && h>=96 && h<=768;
  width=w;height=h;objects[0]=objects[1]=objects[2]=0;
  if(enabled){
   if(rgba.size()!=(size_t)w*h*4)rgba.resize((size_t)w*h*4);
   std::memset(rgba.data(),0,rgba.size());
  }
 }
 void Stamp(Role role,int x,int y,unsigned base,unsigned wide,unsigned tall,unsigned flip,
    const std::uint8_t*gfx,size_t len,size_t mask){
  if(!enabled||!Allow(role,true,picks)||!gfx||len<128)return;
  ++objects[role];
  for(unsigned sy=0;sy<tall;++sy)for(unsigned sx=0;sx<wide;++sx){
   const unsigned tx=flip&1u?wide-sx-1:sx;
   const unsigned ty=flip&2u?tall-sy-1:sy;
   const unsigned tile=(base&~15u)+(sy<<4)+((base+sx)&15u);
   size_t addr=TileByte(tile,mask);if(addr>len-128)continue;
   const std::uint8_t* pixels=gfx+addr;
   for(unsigned py=0;py<16;++py)for(unsigned px=0;px<16;++px){
    // Flip applies to both tile placement and interior tile pixel order.
    const unsigned ix=(flip&1u)?15u-px:px,iy=(flip&2u)?15u-py:py;
    unsigned pen=Pen(pixels,ix,iy);
    if((role==P1 && (pen<13||pen>15))||(role==P2 && (pen<10||pen>12)))continue;
    const int xx=x+int(tx*16+px),yy=y+int(ty*16+py);
    if(xx<0||yy<0||xx>=int(width)||yy>=int(height))continue;
    Color(role,pen,&rgba[((size_t)yy*width+xx)*4]);
   }
  }
 }
 const std::uint8_t* Read(int*w,int*h,int*pitch)const{
  if(!enabled||rgba.empty()||objects[1]+objects[2]==0)return NULL;
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
