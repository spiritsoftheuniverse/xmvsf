#ifndef XMVSF_STAGE87_33_DIZZY_HUD_H
#define XMVSF_STAGE87_33_DIZZY_HUD_H
// Native CPS-2 384x224 HUD. Read-only fighter RAM. Stage87.35 chiseled gold bar with five/six 10-point cuts + per-hit stun.
// PackRGBA avoids collision with Win32 RGB(r,g,b) macro from windows.h. The maximums are the
// eighteen 16-bit ROM words beginning at USA 961023 program offset 0x8BC9A.
// 68000 byte (+B9,A6) is at RAW FBNeo CpsRamFF offset +B8 (word-swapped).
#include <cstdint>
#include <algorithm> // moving source-mask bounds, self-contained C++11 header
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM8733 {
static const unsigned char capacity[18]={
    50,60,60,60,60,60,50,60,60,60,60,60,60,60,60,50,60,60
};
static inline int Capacity(unsigned char charID) {
    if ((charID&1u) || charID>34u) return 0;
    return capacity[charID>>1];
}
struct Track {
    bool valid=false,flash=false;
    unsigned char id=0,remaining=0,state=0;
    unsigned flashStart=0;
    unsigned lastHitFrame=0;
    int lastHitStun=0;
    bool hitVisible=false;
};
struct Reading {int total=0,used=0,hitStun=0;bool flash=false;};
static inline Reading Sample(Track &track,const unsigned char* rec,unsigned frame) {
    Reading result;
    if(!rec){track.valid=false;track.hitVisible=false;return result;}
    const unsigned char id=rec[0x52];
    const int max=Capacity(id);
    if(!max){track.valid=false;track.hitVisible=false;return result;}
    const unsigned char remaining=rec[0xB8], state=rec[0x134];
    // A normal hit reduces the game's REMAINING stun resistance. The per-hit
    // pink integer is the exact observed reduction, never the HP damage or
    // total accumulated stun. Duplicate video paints cannot double-count it.
    if(track.valid && track.id==id && remaining<track.remaining &&
       remaining<=max && track.remaining<=max) {
        track.lastHitStun=(int)track.remaining-(int)remaining;
        track.lastHitFrame=frame;
        track.hitVisible=true;
    }
    // On the dizzy-triggering hit the ROM restores resistance to maximum in
    // the same frame. The actual overkill amount cannot be read from the
    // end-of-frame difference; hide pink rather than fabricating a value.
    if(track.valid && track.id==id && remaining==max && state==255u &&
       track.state!=255u && track.remaining<=2u) {
        track.flash=true;track.flashStart=frame;
        track.hitVisible=false;
        std::fprintf(stderr,"XM87.33 DIZZY_HUD trigger id=%02X max=%d prior_remaining=%u frame=%u\n",
                     (unsigned)id,max,(unsigned)track.remaining,frame);
    }
    if(!track.valid || track.id!=id)track.hitVisible=false;
    if(track.hitVisible && (unsigned)(frame-track.lastHitFrame)>=45u)track.hitVisible=false;
    if(track.flash && (unsigned)(frame-track.flashStart)>=60u)track.flash=false;
    track.valid=true;track.id=id;track.remaining=remaining;track.state=state;
    result.total=max;
    result.used=max-(int)remaining;
    if(result.used<0)result.used=0;
    if(result.used>max)result.used=max;
    result.flash=track.flash;
    result.hitStun=track.hitVisible?track.lastHitStun:0;
    return result;
}
static inline std::uint32_t PackRGBA(unsigned r,unsigned g,unsigned b) {
    return (std::uint32_t)(0xFF000000u|r|(g<<8)|(b<<16)); // SDL RGBA32 byte order R/G/B/A
}
static inline void Pixel(unsigned char* draw,int pitch,int x,int y,std::uint32_t color) {
    if((unsigned)x>=384u||(unsigned)y>=224u)return;
    std::memcpy(draw+(std::size_t)y*(std::size_t)pitch+(std::size_t)x*4u,&color,4);
}
static inline void Rect(unsigned char* draw,int pitch,int x,int y,int w,int h,std::uint32_t color){
    for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)Pixel(draw,pitch,x+dx,y+dy,color);
}
static inline const unsigned char* Digit(char ch) {
    static const unsigned char bitmap[11][5]={
       {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},
       {5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},
       {7,5,7,5,7},{7,5,7,1,7},{1,1,2,4,4}
    };
    const unsigned idx=ch=='/'?10u:(ch>='0'&&ch<='9')?(unsigned)(ch-'0'):10u;
    return bitmap[idx];
}
static inline void Text(unsigned char* draw,int pitch,int x,int y,const char* s,std::uint32_t color){
    for(;*s;s++,x+=4) {const unsigned char *d=Digit(*s);
        for(int row=0;row<5;row++)for(int col=0;col<3;col++)
            if(d[row]&(1u<<(2-col)))Pixel(draw,pitch,x+col,y+row,color);
    }
}
static inline std::uint32_t Rainbow(unsigned frame,unsigned phase) {
    // Six hard RGB accents, one hue every four frames during the 60-frame dizzy flash.
    const std::uint32_t colors[6]={PackRGBA(255,40,40),PackRGBA(255,205,40),PackRGBA(30,255,65),
       PackRGBA(20,230,255),PackRGBA(50,70,255),PackRGBA(240,50,255)};
    return colors[((frame/4u)+phase)%6u];
}
// Source art _art/stunbar_base.png is 61x6, authored for P2 (right).
// _art/examples/example_stunbar.png is a 61x8 COMPOSITION REFERENCE only:
// the base occupies composition rows 2..7, while orange fills rows 0..5,
// and the one-pixel shadow covers row 6 and the fill's diagonal cut.
// Example orange is PARTIAL (44px), not a limit for the full bar.
// Preserve the original HUD base top pixel at y=45 (composition begins y=43).
struct BarArt {
    const unsigned char* rgba=nullptr;
    int width=0,height=0,pitch=0;
    bool valid()const{
        return rgba && width==61 && height==6 && pitch>=width*4;
    }
};
static inline void Base(unsigned char* draw,int pitch,int x,int y,
                        bool right,const BarArt& art) {
    for(int row=0;row<art.height;row++)
        for(int col=0;col<art.width;col++){
            const int sx=right?col:art.width-1-col;
            const unsigned char* p=art.rgba+
                (std::size_t)row*(std::size_t)art.pitch+(std::size_t)sx*4u;
            if(!p[3])continue;
            const std::uint32_t rgba=std::uint32_t(p[0])|
                    (std::uint32_t(p[1])<<8u)|
                    (std::uint32_t(p[2])<<16u)|
                    (std::uint32_t(p[3])<<24u);
            Pixel(draw,pitch,x+col,y+row,rgba);
        }
}
// Six-pixel base comes from PNG; full fill spans its entire original
// opaque width. Normal stun is warm yellow/orange. Fast diagonal rainbow
// is reserved EXCLUSIVELY for a visually full stun bar / dizzy flash.
static inline std::uint32_t RainbowPan(unsigned frame,int srcX,int row) {
    const std::uint32_t colors[7]={
        PackRGBA(255,55,49),PackRGBA(255,145,22),
        PackRGBA(255,231,39),PackRGBA(81,249,52),
        PackRGBA(33,231,245),PackRGBA(61,96,255),
        PackRGBA(210,45,255)};
    // 3 native pixels EVERY emulated frame, versus 1 per 2 frames.
    // Narrow 4px diagonals give a rapid, continuous rainbow pan.
    const int pan=int((frame*3u)%28u);
    const int band=(srcX-row+pan+280)/4;
    return colors[(unsigned)band%7u];
}
static inline std::uint32_t WarmStun(unsigned row) {
    static const std::uint32_t colors[6]={
        PackRGBA(255,225,65),PackRGBA(255,200,41),
        PackRGBA(255,168,24),PackRGBA(251,146,12),
        PackRGBA(244,121,6),PackRGBA(230,101,0)};
    return colors[row<6u?row:5u];
}
static inline bool HasBasePixel(const BarArt& art,int x,int row) {
    if(x<0 || x>=art.width || row<0 || row>=art.height)return false;
    return art.rgba[(std::size_t)row*(std::size_t)art.pitch+
                    (std::size_t)x*4u+3u]>=8u;
}
static inline void SourceSpan(const BarArt& art,int compositionRow,
                              int& left,int& right) {
    // The example's fill begins 2 rows ABOVE the base. Project the first
    // opaque base row upward for those 2 rows, then use actual source art.
    const int baseRow=compositionRow<2?0:std::min(art.height-1,compositionRow-2);
    left=art.width;right=-1;
    for(int x=0;x<art.width;x++)if(HasBasePixel(art,x,baseRow)){
        if(x<left)left=x;
        right=x;
    }
}
// The live fill and its one-pixel diagonal tip share EXACTLY the
// same moving bounds. There is no separate stationary cutout texture.
// All positions are in original P2 source pixels before P1 mirroring.
static inline bool FillBounds(const BarArt& art,int row,int used,int total,
                              int& first,int& last){
    if(used<=0 || total<=0 || row<0 || row>=6)return false;
    int left=0,right=0;
    SourceSpan(art,row,left,right);
    if(left>right)return false;
    const int span=right-left+1;
    if(used>=total){first=left;last=right;return true;}
    const int filled=(span*used+total-1)/total;
    // A 45-degree leading cut progresses WITH the bar. Limit its
    // row offset by the genuinely unfilled portion, eliminating a
    // stationary tip that lingers when the fill approaches 100%.
    const int missing=span-filled;
    first=right-filled+1+std::min(row,missing);
    last=right;
    return first<=last && first>=left;
}
static inline void DrawOne(unsigned char* draw,int pitch,int side,
                           const Reading& s,bool noir,unsigned frame,
                           const BarArt* art=nullptr){
    if(s.total<=0 || !art || !art->valid())return;
    const int y=45;  // existing native stun bar BASE top, not the floating fill
    const int w=61;
    const int x=side?(323-w):61; // P2 right-aligned, P1 mirrored at x=61
    const int used=s.flash?s.total:s.used;
    const bool atCapacity=used>=s.total; // rainbow ONLY at 100%
    const std::uint32_t cyan=noir?PackRGBA(158,192,192):PackRGBA(68,226,240);
    const std::uint32_t pink=noir?PackRGBA(198,145,186):PackRGBA(255,104,220);
    const std::uint32_t inset=PackRGBA(67,68,69);

    // (1) Original 61x6 base; no legacy 76px meter fallback exists.
    Base(draw,pitch,x,y,side!=0,*art);
    if(used>0){
        // (2) The live fill and its diagonal cut move together. Mirror
        // only at the final P1 pixel-coordinate conversion.
        for(int row=0;row<6;row++){
            int first=0,last=-1;
            if(!FillBounds(*art,row,used,s.total,first,last))continue;
            for(int src=first;src<=last;src++){
                const int dx=side?src:(60-src);
                Pixel(draw,pitch,x+dx,y-2+row,
                      atCapacity?RainbowPan(frame,src,row):
                                 WarmStun((unsigned)row));
            }
            // (3a) One-pixel inset exactly ON the live leading tip.
            if(row>=2 && HasBasePixel(*art,first,row-2)){
                const int dx=side?first:(60-first);
                Pixel(draw,pitch,x+dx,y-2+row,inset);
            }
        }
        // (3b) One bottom shadow scanline inside the base. Recalculate
        // from the SAME live moving mask; never extend outside the bar.
        int first=0,last=-1;
        if(FillBounds(*art,5,used,s.total,first,last)){
            for(int src=first;src<=last;src++){
                if(!HasBasePixel(*art,src,4))continue;
                const int dx=side?src:(60-src);
                Pixel(draw,pitch,x+dx,y+4,inset);
            }
        }
    }

    // Gameplay-owned stun accumulation, total and hit numbers unchanged.
    char totalText[16];
    std::snprintf(totalText,sizeof(totalText),"%d/%d",used,s.total);
    const int totalX=side?326:(x-5-(int)std::strlen(totalText)*4);
    Text(draw,pitch,totalX,y+1,totalText,
         s.flash?Rainbow(frame,(unsigned)side):cyan);
    if(s.hitStun>0&&!s.flash){
        char hitText[16];
        std::snprintf(hitText,sizeof(hitText),"%d",s.hitStun);
        const int hitX=side?351:(34-(int)std::strlen(hitText)*4);
        Text(draw,pitch,hitX,y+1,hitText,pink);
    }
}
static inline void Draw(const unsigned char* ram,unsigned char* framebuffer,int pitch,int bpp,
                        unsigned frame,int gameStatus,bool noir,
                        const BarArt* art=nullptr) {
    static Track tracked[2];
    static bool logged=false;
    if(!ram || !framebuffer || gameStatus!=8 || bpp!=4 || pitch<384*4){
        if(gameStatus!=8){tracked[0].valid=false;tracked[1].valid=false;tracked[0].flash=false;tracked[1].flash=false;}
        return;
    }
    const char* option=std::getenv("XM8733_DIZZY_BAR");
    if(option && option[0]=='0' && option[1]=='\0')return;
    if(!logged){std::fprintf(stderr,"XM87.33 DIZZY_HUD active 384x224 raw=B8 table=8BC9A mode=game-wide\n");logged=true;}
    // Read native active P1 +0x4000 and P2 +0x4400. All player state is READ-ONLY.
    for(int p=0;p<2;p++){
        Reading r=Sample(tracked[p],ram+0x4000+(unsigned)p*0x400u,frame);
        DrawOne(framebuffer,pitch,p,r,noir,frame,art);
    }
}
}
#endif
