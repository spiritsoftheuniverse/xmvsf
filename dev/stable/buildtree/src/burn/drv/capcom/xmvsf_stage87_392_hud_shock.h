#ifndef XMVSF_STAGE87_392_HUD_SHOCK_H
#define XMVSF_STAGE87_392_HUD_SHOCK_H
// TEST 87.392: one read-only, game-frame HUD shock timeline for native OAM
// and the separate SDL stun bar. No ROM/RAM mutations or opaque screen masks.
// 60 game frames per second: 210 hidden, 30 ordered-dither reveal.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace XM87392 {
static const unsigned DelayFrames=210u;
static const unsigned RevealFrames=30u;

struct Timeline {
    unsigned start=0u;
    unsigned lastFrame=~0u;
    int lastPhase=-1;
    bool armed=false;

    void observe(int phase,unsigned frame,bool allowed) {
        if(!allowed){
            armed=false;lastFrame=frame;lastPhase=-1;return;
        }
        if(frame==lastFrame)return; // paused/repeated paint is not elapsed game time
        if(lastFrame!=~0u && frame<lastFrame){armed=false;lastPhase=-1;}
        if(phase==6 && lastPhase!=6){
            start=frame;armed=true; // stage opens, before FIGHT
        }else if(phase==8 && !armed){
            start=frame;armed=true; // snapshot/direct-combat entry
        }else if(phase!=6 && phase!=8){
            armed=false; // KO/results/selection: never conceal the score screen
        }
        lastFrame=frame;lastPhase=phase;
    }
    unsigned step(unsigned frame)const {
        if(!armed || frame<start)return RevealFrames;
        const unsigned elapsed=frame-start;
        if(elapsed<DelayFrames)return 0u;
        if(elapsed>=DelayFrames+RevealFrames)return RevealFrames;
        return elapsed-DelayFrames+1u;
    }
};
inline Timeline& timeline(){static Timeline s;return s;}

// Strictly scoped to the upper native HUD; never blanket-filter the fighter,
// arena, introductory logo walls, or unrelated large priority-7 artwork.
inline bool object(unsigned priority,unsigned palette,unsigned tile,
                   int x,int y,unsigned wide,unsigned tall,bool portrait=false){
    if(!wide || !tall || x>=384 || x+int(wide*16u)<=0 ||
       y>=56 || y+int(tall*16u)<=0)return false;
    if(priority==0u)
        return palette==1u && wide==2u && tall==2u &&
               tile>=87808u && tile<=87826u && ((tile-87808u)&1u)==0u &&
               x>=130 && x<=226 && y>=0 && y<52; // verified time digits
    if(priority!=7u)return false;
    if(portrait)return y>=-32 && y<=46;
    if(y< -32 || wide>9u || tall>5u)return false;
    // Health-strip sources can be fairly long. Exclude other large pre-fight
    // sprites except the source-verified HP tile families.
    if(wide>=5u || tall>=5u){
        const bool hp=(palette==2u || palette==4u) &&
                      (y==20 || y==32) &&
                      ((tile>=88417u && tile<=88579u) ||
                        tile==89417u || tile==89418u);
        if(!hp)return false;
    }
    return true;
}
static const unsigned char Bayer8[64]={
     0,48,12,60, 3,51,15,63,
    32,16,44,28,35,19,47,31,
     8,56, 4,52,11,59, 7,55,
    40,24,36,20,43,27,39,23,
     2,50,14,62, 1,49,13,61,
    34,18,46,30,33,17,45,29,
    10,58, 6,54, 9,57, 5,53,
    42,26,38,22,41,25,37,21
};
inline bool visible(int x,int y,unsigned step){
    if(step>=RevealFrames)return true;
    if(!step)return false;
    return (step*64u/RevealFrames)>Bayer8[((unsigned)y&7u)*8u+((unsigned)x&7u)];
}
inline unsigned byteBlend(unsigned a,unsigned b,unsigned amount,unsigned total){
    return (a*(total-amount)+b*amount+total/2u)/total;
}
// White blast -> electric blue -> exactly the authored source colors.
// Input/output is 0x00RRGGBB from BurnHighCol (high byte preserved).
inline std::uint32_t shockRGB(std::uint32_t source,unsigned step) {
    if(step>=RevealFrames)return source;
    const unsigned r=(source>>16)&255u,g=(source>>8)&255u,b=source&255u;
    const unsigned lum=(r*77u+g*150u+b*29u+128u)>>8;
    const unsigned blueR=25u+lum/5u;
    const unsigned blueG=92u+lum/3u;
    const unsigned blueB=182u+lum/4u;
    unsigned rr,gg,bb;
    if(step<=12u){
        rr=byteBlend(255u,blueR,step,12u);
        gg=byteBlend(255u,blueG,step,12u);
        bb=byteBlend(255u,blueB,step,12u);
    }else{
        const unsigned amount=step-12u;
        rr=byteBlend(blueR,r,amount,RevealFrames-12u);
        gg=byteBlend(blueG,g,amount,RevealFrames-12u);
        bb=byteBlend(blueB,b,amount,RevealFrames-12u);
    }
    return (source&0xff000000u)|(rr<<16)|(gg<<8)|bb;
}

// Snapshot only 16x16 pixels of the *selected HUD tile*. Restoring removed
// pixels and Z positions reveals the real scenery/fighters underneath instead
// of drawing an opaque rectangular overlay across the stage.
struct Tile {
    int x=0,y=0,w=0,h=0,pitch=0,screenWidth=0;
    std::uint32_t before[256]={};
    std::uint16_t zBefore[256]={};
    bool depthValid=false;
    void save(const std::uint8_t* pixels,int rowPitch,
              const std::uint16_t* depth,int sw,int sh,int tx,int ty){
        x=tx;y=ty;pitch=rowPitch;screenWidth=sw;
        w=(tx>=0 && tx+16<=sw)?16:0;
        h=(ty>=0 && ty+16<=sh)?16:0;
        // Care path also covers partially offscreen tiles.
        if(!w || !h){w=16;h=16;}
        depthValid=depth!=nullptr;
        if(!pixels || rowPitch<sw*4){w=h=0;return;}
        for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx){
            const int px=tx+xx,py=ty+yy;
            if(px<0 || py<0 || px>=sw || py>=sh)continue;
            const unsigned index=(unsigned)yy*16u+(unsigned)xx;
            std::memcpy(&before[index],pixels+(std::size_t)py*rowPitch+px*4,4);
            if(depth)zBefore[index]=depth[(std::size_t)py*sw+px];
        }
    }
    void finish(std::uint8_t* pixels,std::uint16_t* depth,
                int sw,int sh,unsigned step)const{
        if(!pixels || !w || !h || step>=RevealFrames)return;
        for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx){
            const int px=x+xx,py=y+yy;
            if(px<0 || py<0 || px>=sw || py>=sh)continue;
            const unsigned index=(unsigned)yy*16u+(unsigned)xx;
            std::uint8_t* dest=pixels+(std::size_t)py*pitch+px*4;
            std::uint32_t after=0u;
            std::memcpy(&after,dest,4);
            if(!visible(px,py,step)){
                std::memcpy(dest,&before[index],4);
                if(depth && depthValid)depth[(std::size_t)py*sw+px]=zBefore[index];
            }else if(after!=before[index]){
                const std::uint32_t tinted=shockRGB(after,step);
                std::memcpy(dest,&tinted,4);
            }
        }
    }
};

// Native SDL stun artwork is premultiplied neither by the emulator palette
// nor the CPS2 Z buffer. Apply the SAME Bayer positions and clock to its alpha.
inline void stunOverlay(std::uint8_t* rgba,int pitch,unsigned step){
    if(!rgba || pitch<384*4 || step>=RevealFrames)return;
    for(int y=43;y<=54;++y)for(int x=0;x<384;++x){
        std::uint8_t* p=rgba+(std::size_t)y*pitch+x*4;
        if(!p[3])continue;
        if(!visible(x,y,step)){p[3]=0;continue;}
        const std::uint32_t native=(std::uint32_t(p[0])<<16)|
                                   (std::uint32_t(p[1])<<8)|p[2];
        const std::uint32_t tinted=shockRGB(native,step);
        p[0]=(unsigned char)((tinted>>16)&255u);
        p[1]=(unsigned char)((tinted>>8)&255u);
        p[2]=(unsigned char)(tinted&255u);
    }
}
}
#endif
