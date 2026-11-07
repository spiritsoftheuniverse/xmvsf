#ifndef XMVSF_STAGE87_33_DIZZY_HUD_H
#define XMVSF_STAGE87_33_DIZZY_HUD_H
// Native CPS-2 384x224 HUD. Read-only fighter RAM. Stage87.35 chiseled gold bar with five/six 10-point cuts + per-hit stun.
// PackRGBA avoids collision with Win32 RGB(r,g,b) macro from windows.h. The maximums are the
// eighteen 16-bit ROM words beginning at USA 961023 program offset 0x8BC9A.
// 68000 byte (+B9,A6) is at RAW FBNeo CpsRamFF offset +B8 (word-swapped).
#include <cstdint>
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
static inline std::uint32_t RainbowPan(unsigned frame,int srcX,int row) {
    // P2 source-space x-row is constant along each diagonal segment;
    // P1 is mirrored at drawing, keeping the same slant against its cut.
    const std::uint32_t colors[7]={
        PackRGBA(255,64,48),PackRGBA(255,156,38),
        PackRGBA(255,225,58),PackRGBA(89,250,57),
        PackRGBA(37,218,243),PackRGBA(70,102,255),
        PackRGBA(220,61,248)};
    const int pan=int((frame/2u)%42u);
    const int stripe=(srcX-row+pan+420)/6;
    return colors[(unsigned)stripe%7u];
}
static inline void DrawOne(unsigned char* draw,int pitch,int side,
                           const Reading& s,bool noir,unsigned frame,
                           const BarArt* art=nullptr){
    if(s.total<=0)return;
    const bool newArt=art&&art->valid();
    const int y=45;  // original native stun base top row (unaltered)
    const int w=newArt?61:76,h=newArt?6:7;
    // Anchor the left outer end and right outer end to the old HUD positions.
    // This keeps P1/P2 symmetric despite the narrower 61-pixel source.
    const int x=side?(323-w):61;
    const int used=s.flash?s.total:s.used;
    const std::uint32_t cyan=noir?PackRGBA(158,192,192):PackRGBA(68,226,240);
    const std::uint32_t pink=noir?PackRGBA(198,145,186):PackRGBA(255,104,220);

    if(newArt){
        // UI order 1: raw PNG base. P2 unchanged; P1 horizontally mirrored.
        Base(draw,pitch,x,y,side!=0,*art);
        // The example's orange is exactly 44 px wide at full capacity,
        // and slants +1 pixel per row. Capacity controls its width only.
        const int fill=44*used/s.total;
        if(fill>0){
            for(int row=0;row<6;row++){
                const int left=54-fill+row;
                const int right=54+row; // exclusive; full: x10..53 at row0
                for(int src=left;src<right;src++){
                    const int dx=side?src:(60-src);
                    Pixel(draw,pitch,x+dx,y-2+row,
                          RainbowPan(frame,src,row));
                }
            }
            // UI order 3: sample-proven one-pixel shadow at the growing
            // diagonal cut, then inside the base under the orange (row6
            // of the 8px reference = base row4). Never move silhouettes.
            const std::uint32_t inset=PackRGBA(67,68,69);
            for(int row=2;row<6;row++){
                const int src=54-fill+row-1;
                const int dx=side?src:(60-src);
                Pixel(draw,pitch,x+dx,y-2+row,inset);
            }
            const int shadowLeft=56-fill; // full: x12, exactly the example
            for(int src=shadowLeft;src<=59;src++){
                const int dx=side?src:(60-src);
                Pixel(draw,pitch,x+dx,y+4,inset);
            }
        }
    }else{
        // Older buildtrees without the newly committed PNG retain the
        // former geometry and readable meter rather than a blank HUD.
        const int fill=w*used/s.total;
        const std::uint32_t outline=PackRGBA(45,47,49);
        const std::uint32_t empty=PackRGBA(100,103,105);
        for(int row=0;row<h;row++){
            const int leftCut=side?row:(h-1-row);
            const int rightCut=side?(h-1-row):row;
            for(int ix=leftCut;ix<w-rightCut;ix++){
                std::uint32_t c=row==0||row==h-1?outline:empty;
                if(row>=1&&row<h-1&&(side?ix>=w-fill:ix<fill))
                    c=RainbowPan(frame,side?ix:(w-1-ix),row);
                Pixel(draw,pitch,x+ix,y+row,c);
            }
        }
    }

    // ROM-owned stun amount and per-hit numbers are untouched.
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
