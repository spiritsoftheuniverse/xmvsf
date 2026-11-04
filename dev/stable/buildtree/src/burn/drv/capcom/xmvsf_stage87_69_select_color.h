#ifndef XMVSF_STAGE87_69_SELECT_COLOR_H
#define XMVSF_STAGE87_69_SELECT_COLOR_H
// Test ONLY. Native Training character-select owner markers; never recolor
// guessed CPS2 cursor/portrait tile IDs. The live ROM-owned cursor sprites are
// still unidentified. Pixel drawing occurs only outside Wily capture prepasses.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM8769 {
struct SelectData { unsigned frame,phase,picks,index,owner; };
static inline bool Valid(const SelectData& s,bool training,bool nativePass,int width,int height,int bpp) {
 return training && nativePass && s.phase<8 && s.picks<=2 && s.owner<=1 &&
        width>=160 && height>=96 && (bpp==2 || bpp==3 || bpp==4);
}
static inline unsigned char Index(const unsigned char* ff, unsigned addr) {
 // CpsRamFF is host-word-swapped: logical CPU byte at FFxxxx appears at xxxx ^ 1.
 return ff ? ff[(addr&0xffffu)^1u] : 0xffu;
}
static inline void Pixel(unsigned char* data,int pitch,int bpp,int width,int height,
                          int x,int y,unsigned int color) {
 if(!data||x<0||y<0||x>=width||y>=height)return;
 unsigned char*p=data+y*pitch+x*bpp;
 if(bpp==2){p[0]=(unsigned char)color;p[1]=(unsigned char)(color>>8);}
 else if(bpp==3){p[0]=(unsigned char)color;p[1]=(unsigned char)(color>>8);p[2]=(unsigned char)(color>>16);}
 else {std::memcpy(p,&color,4);}
}
static inline void Rect(unsigned char* dst,int pitch,int bpp,int w,int h,
                        int x,int y,int rw,int rh,unsigned int c) {
 for(int yy=y;yy<y+rh;++yy)for(int xx=x;xx<x+rw;++xx)Pixel(dst,pitch,bpp,w,h,xx,yy,c);
}
// A 3x5 pixel P1/P2 glyph ensures the colored tab is identifiable even when
// the actual native sprite is obscured by portraits or multiple overlay layers.
static inline void Letters(unsigned char*dst,int pitch,int bpp,int w,int h,int x,int y,
                           unsigned int fg) {
 static const char* P[5]={"110","101","110","100","100"};
 static const char* One[5]={"010","110","010","010","111"};
 static const char* Two[5]={"110","001","010","100","111"};
 const char** number=x<w/2?One:Two;
 for(int row=0;row<5;++row)for(int col=0;col<3;++col){
  if(P[row][col]=='1')Rect(dst,pitch,bpp,w,h,x+col*2,y+row*2,2,2,fg);
  if(number[row][col]=='1')Rect(dst,pitch,bpp,w,h,x+10+col*2,y+row*2,2,2,fg);
 }
}
static inline void Draw(unsigned char* dst,int pitch,int bpp,int width,int height,
                         const SelectData& s, bool training,bool nativePass,
                         unsigned int p1Color,unsigned int p2Color,unsigned int dim) {
 if(!Valid(s,training,nativePass,width,height,bpp)||!dst)return;
 const int left=8,right=width-58,top=height-22;
 // Small solid-color ownership tabs, not a mask over any native selector art.
 Rect(dst,pitch,bpp,width,height,left,top,50,15,s.owner==0?p1Color:dim);
 Rect(dst,pitch,bpp,width,height,right,top,50,15,s.owner==1?p2Color:dim);
 Letters(dst,pitch,bpp,width,height,left+7,top+3,s.owner==0?dim:p1Color);
 Letters(dst,pitch,bpp,width,height,right+7,top+3,s.owner==1?dim:p2Color);
 // Active accent under selected owner: no delays or selection-state changes.
 const int xx=s.owner==0?left:right;
 Rect(dst,pitch,bpp,width,height,xx,top+15,50,2,s.owner==0?p1Color:p2Color);
}
struct Log {
 FILE* f;unsigned lastFrame;
 Log():f(NULL),lastFrame(~0u){}
 ~Log(){if(f)std::fclose(f);}
 void Observe(const SelectData&s) {
  if(lastFrame==s.frame)return;
  lastFrame=s.frame;
  if(!f){const char*p=std::getenv("XM8769_SELECTOR_TRACE");if(!p||!*p)return;
    f=std::fopen(p,"wb");if(!f)return;
    std::fprintf(f,"frame\tphase\taccepted_p1_picks\trom_cursor_index\tactive_player\n");
    std::fprintf(stderr,"XM87.69 SELECT_COLOR_READY file=%s\n",p);
  }
  std::fprintf(f,"%u\t%u\t%u\t%u\t%u\n",s.frame,s.phase,s.picks,s.index,s.owner+1);
  if((s.frame%60u)==0u)std::fflush(f);
 }
};
static inline Log& Recorder(){static Log o;return o;}
} // namespace XM8769
#endif
