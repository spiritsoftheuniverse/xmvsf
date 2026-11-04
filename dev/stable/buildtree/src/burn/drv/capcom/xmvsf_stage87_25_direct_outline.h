#ifndef XMVSF_STAGE87_25_DIRECT_OUTLINE_H
#define XMVSF_STAGE87_25_DIRECT_OUTLINE_H
// Stage87.25: draw source-owned fighter objects four times at cardinal 1px
// offsets behind the normal grayscale object. No silhouette buffers/compositor.
// Stage87.27: default V100/S80 bright direct outline, no buffer/bloom. Hue cycles 360 frames.
#include <cstdlib>
#include <cstdio>
#include <cstdint>
namespace XM8725 {
inline bool Enabled() {
    static const bool on=[](){const char* p=std::getenv("XM8725_DIRECT_OUTLINE");
        return !(p && p[0]=='0' && p[1]=='\0');}();
    return on;
}
inline bool Bright() {
    static const bool yes=[](){const char* p=std::getenv("XM8727_BRIGHT_LINE");
        return !(p && p[0]=='0' && p[1]=='\0');}();
    return yes;
}
inline unsigned Hue(unsigned frame,unsigned dir) {
    return ((frame%360u)*1536u/360u+dir*384u)%1536u;
}
struct Rgb {unsigned r,g,b;};
inline Rgb Color(unsigned phase) {
    const unsigned hi=Bright()?255u:153u; // 100% value (optional previous 60%)
    const unsigned lo=Bright()?51u:31u;  // 20% of value: 80% saturation
    const unsigned seg=(phase%1536u)/256u;
    const unsigned pos=phase%256u;
    const unsigned up=lo+(hi-lo)*pos/255u;
    const unsigned down=hi-(hi-lo)*pos/255u;
    switch(seg){
      case 0:return {hi,up,lo};
      case 1:return {down,hi,lo};
      case 2:return {lo,hi,up};
      case 3:return {lo,down,hi};
      case 4:return {up,lo,hi};
      default:return {hi,lo,down};
    }
}
inline int OffsetX(int pass){return pass==0?-1:pass==1?1:0;}
inline int OffsetY(int pass){return pass==2?-1:pass==3?1:0;}
// BurnHighCol is the engine's actual framebuffer format conversion.
inline void Palettes(UINT32 (&pal)[4][16],unsigned frame){
    for(unsigned d=0;d<4u;++d){
        const Rgb c=Color(Hue(frame,d));
        const UINT32 ink=BurnHighCol(c.r,c.g,c.b,0);
        for(unsigned pen=0;pen<16u;++pen)pal[d][pen]=ink;
    }
}
inline void Observe(unsigned frame,bool ink,bool owner,bool eligible){
    static unsigned long long seen=0,accepted=0;
    ++seen; if(eligible)++accepted;
    if(seen<=4ull || seen%8192ull==0ull)
        std::fprintf(stderr,"XM87.25 OUTLINE_GATE frame=%u p6=%llu ink=%d owner=%d eligible=%d accepted=%llu\n",
            frame,seen,(int)ink,(int)owner,(int)eligible,accepted);
}
inline void ObjectDrawn(unsigned frame,unsigned tiles){
    static unsigned long long objects=0,sourceTiles=0;
    ++objects;sourceTiles+=tiles;
    if(objects<=8ull || objects%4096ull==0ull)
        std::fprintf(stderr,"XM87.25 DIRECT_SPRITE_OUTLINE frame=%u objects=%llu tiles=%llu copies_per_object=4 value=%u saturation=80\n",
            frame,objects,sourceTiles,Bright()?100u:60u);
    if(objects<=8ull || objects%4096ull==0ull)
        std::fprintf(stderr,"XM87.27 BRIGHT_LINE frame=%u objects=%llu direct_copies=4 value=%u saturation=80 bloom=0\n",
            frame,objects,Bright()?100u:60u);
}
} // namespace XM8725
#endif
