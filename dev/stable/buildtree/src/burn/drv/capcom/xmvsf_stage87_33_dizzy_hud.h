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
// Compact gold HUD design. These are the screenshot's YELLOW meter positions,
// not Stage87.33's oversized gray bars at y=53 or labels at y=61.
// Native viewport 384x224; gold strips: P1 x=61, P2 x=247, y=45, w=76.
// Stage87.36 corrected mirror relative to existing HP bars; 45-degree chiseled, mirrored 10-point divisions. Geometry changes
// only; the live stun numerator continues to come from the real ROM counter.
// The exposed fill is 100% of the available mask at s.used==s.total.
static inline void DrawOne(unsigned char* draw,int pitch,int side,const Reading& s,
                           bool noir,unsigned frame) {
    if(s.total==0)return;
    const int x=side?247:61, y=45, w=76, h=7;
    (void)noir; // saturated stun ink even in Double Feature
    const std::uint32_t shadow=PackRGBA(45,47,49);
    const std::uint32_t top=PackRGBA(255,212,69); // bright yellow bevel
    const std::uint32_t gold=PackRGBA(248,143,10); // Spanish orange/yellow
    const std::uint32_t low=PackRGBA(230,97,0);
    const std::uint32_t empty=PackRGBA(100,103,105); // energy bar base gray
    const std::uint32_t cyan=noir?PackRGBA(158,192,192):PackRGBA(68,226,240);
    const std::uint32_t pink=noir?PackRGBA(198,145,186):PackRGBA(255,104,220);
    const int segments=s.total/10; // 50->5; 60->6, one division per 10 stun
    const int used=s.flash?s.total:s.used;
    const int fill=w*used/s.total;
    for(int ry=0;ry<h;ry++) {
        // 45-degree chisel on both ends; mirror the tilt for P2.
        // A 1px change in y yields a 1px shift in its slanted edge.
        const int leftCut=side?ry:(h-1-ry);
        const int rightCut=side?(h-1-ry):ry;
        for(int ix=leftCut;ix<w-rightCut;ix++) {
            const bool filled=side?(ix>=w-fill):(ix<fill);
            std::uint32_t color=shadow;
            if(ry>=1 && ry<h-1) {
                color=empty;
                if(filled) {
                    if(s.flash)color=Rainbow(frame,(unsigned)(ix/4)+(unsigned)side*2u);
                    else if(ry==1)color=top;
                    else if(ry==h-2)color=low;
                    else color=gold;
                }
                // The dash is an etched 1px slit at every 10 stun points.
                // Do not change fill arithmetic; zero and full still map
                // precisely to 0% and 100% of the available chiseled area.
                bool dash=false;
                for(int d=1;d<segments;d++) {
                    const int split=(w*d)/segments;
                    if(ix==split){dash=true;break;}
                }
                if(dash)color=PackRGBA(67,68,69);
            }
            Pixel(draw,pitch,x+ix,y+ry,color);
        }
    }
    char totalText[16];std::snprintf(totalText,sizeof(totalText),"%d/%d",used,s.total);
    const int totalX=side?326:(x-5-(int)std::strlen(totalText)*4);
    Text(draw,pitch,totalX,y+1,totalText,s.flash?Rainbow(frame,(unsigned)side):cyan);
    if(s.hitStun>0&&!s.flash) {
        char hitText[16];std::snprintf(hitText,sizeof(hitText),"%d",s.hitStun);
        const int hitX=side?351:(34-(int)std::strlen(hitText)*4);
        Text(draw,pitch,hitX,y+1,hitText,pink);
    }
}
static inline void Draw(const unsigned char* ram,unsigned char* framebuffer,int pitch,int bpp,
                        unsigned frame,int gameStatus,bool noir) {
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
        DrawOne(framebuffer,pitch,p,r,noir,frame);
    }
}
}
#endif
