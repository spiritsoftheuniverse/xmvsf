#ifndef XMVSF_STAGE87_40_HUD_COLOR_H
#define XMVSF_STAGE87_40_HUD_COLOR_H
// Stage 87.40: no character-name recoloring. Only connected portrait backdrop
// blue and actual health-bar green become red and blue, respectively.
// Source pixels: Wily compositor RGBA32; standard FBNeo RGB888 little endian BGRx.
// The destination is a transparent SDL_PIXELFORMAT_RGBA32 overlay.
#include <cstdint>
#include <cstring>
namespace XM8740 {
struct RGB { unsigned r,g,b; };
static inline RGB Read(const unsigned char* p,bool bgra) {
    return bgra?RGB{p[2],p[1],p[0]}:RGB{p[0],p[1],p[2]};
}
static inline bool BluePortrait(const RGB& c) {
    // Game uses two dominant backdrop colors (17,102,255), (102,170,255).
    // Also accepts the same desaturated blue under the noir compositor.
    return c.b>=170u && c.b>=c.g+28u && c.g>=c.r+18u &&
           c.r<=135u && c.g>=70u;
}
static inline bool HealthGreen(const RGB& c) {
    // Includes full-bright (0,255,0), which older code accidentally rejected.
    return c.g>=128u && c.g>=c.r+55u && c.g>=c.b+55u && c.r<=120u && c.b<=120u;
}
static inline unsigned char Cap(unsigned v){return (unsigned char)(v>255u?255u:v);}
static inline RGB Red(const RGB& c) {
    // Map original blue luminance into red, retaining two native shade bands.
    return RGB{Cap(c.b),Cap(c.g*33u/100u),Cap(c.r*20u/100u)};
}
static inline RGB HpBlue(const RGB& c){
    // Native green amplitude maps into the portrait-blue family.
    return RGB{Cap(c.g*7u/100u),Cap(c.g*40u/100u),Cap(c.g)};
}
static inline void Put(unsigned char* dst,int pitch,unsigned x,unsigned y,const RGB& c){
    unsigned char* p=dst+(unsigned)pitch*y+x*4u;
    p[0]=(unsigned char)c.r;p[1]=(unsigned char)c.g;p[2]=(unsigned char)c.b;p[3]=255u;
}
static inline bool InPortrait(unsigned x,unsigned y){
    return y>=9u && y<=44u && (x<=31u || x>=352u);
}
static inline bool InHp(unsigned x,unsigned y){
    // Upper/lower life bars, excluding the round timer and character portraits.
    return y>=19u && y<=44u && ((x>=30u&&x<=164u) || (x>=220u&&x<=353u));
}
static inline unsigned Recolor(const unsigned char* src,int sourcePitch,bool bgra,
                               unsigned char* dst,int destPitch){
    if(!src || !dst || sourcePitch<384*4 || destPitch<384*4)return 0;
    unsigned changed=0;
    // Blue life bars. Strict per-pixel native green condition, only bar region.
    for(unsigned y=19;y<=44;++y)for(unsigned x=30;x<=353;++x){
        if(!InHp(x,y))continue;
        const unsigned char* p=src+(unsigned)sourcePitch*y+x*4u;
        if(!bgra && p[3]==0u)continue; // transparent RGBA compositor pixel
        RGB c=Read(p,bgra);
        if(HealthGreen(c)){Put(dst,destPitch,x,y,HpBlue(c));++changed;}
    }
    // Portrait backdrops are the blue regions connected to OUTER screen edges.
    // Flood only inside the 32px portrait panels, never through name glyphs.
    struct Point{unsigned short x,y;};
    Point queue[4096];
    unsigned char visited[384*45];
    std::memset(visited,0,sizeof visited);
    unsigned head=0,tail=0;
    for(unsigned y=9;y<=44;++y)for(unsigned side=0;side<2;++side){
        const unsigned x=side?383u:0u;
        const unsigned char* p=src+(unsigned)sourcePitch*y+x*4u;
        if((!bgra && !p[3]) || !BluePortrait(Read(p,bgra)))continue;
        unsigned at=y*384u+x;
        if(!visited[at]){visited[at]=1;queue[tail++]={(unsigned short)x,(unsigned short)y};}
    }
    while(head<tail){
        const Point q=queue[head++];
        const unsigned char* p=src+(unsigned)sourcePitch*q.y+q.x*4u;
        Put(dst,destPitch,q.x,q.y,Red(Read(p,bgra)));++changed;
        const int offsets[4][2]={{-1,0},{1,0},{0,-1},{0,1}};
        for(unsigned k=0;k<4;++k){
            const int xx=(int)q.x+offsets[k][0], yy=(int)q.y+offsets[k][1];
            if(xx<0||yy<0||!InPortrait((unsigned)xx,(unsigned)yy))continue;
            const unsigned at=(unsigned)yy*384u+(unsigned)xx;
            if(visited[at])continue;
            const unsigned char* np=src+(unsigned)sourcePitch*(unsigned)yy+(unsigned)xx*4u;
            if((!bgra && !np[3]) || !BluePortrait(Read(np,bgra)))continue;
            visited[at]=1;
            if(tail<4096u)queue[tail++]={(unsigned short)xx,(unsigned short)yy};
        }
    }
    return changed;
}
}
#endif
