#ifndef XMVSF_STAGE87_392_HUD_SHOCK_H
#define XMVSF_STAGE87_392_HUD_SHOCK_H
// TEST 87.395: game-frame HUD reveal from initial selection/VS handoff
// and the separate SDL stun bar. No ROM/RAM mutations or opaque screen masks.
// 60 game frames per second: 210 hidden, 30 ordered-dither reveal.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace XM87392 {
static const unsigned DelayFrames=210u;
static const unsigned RevealFrames=30u;

// Native original xmvsfu is not CpsXmvSixEnabled. Guard by actual driver
// identity so normal Arcade/Training/Versus and the 3v3 derivative all work.
inline bool eligible(const char* driver,bool version4){
    return version4 && driver && std::strncmp(driver,"xmvsf",5)==0;
}

struct Timeline {
    unsigned start=0u;
    unsigned lastFrame=~0u;
    int lastPhase=-1;
    bool armed=false;

    // ROM phase 02 is the FIRST scene handoff frame, well before phase 06
    // introduction and phase 08 FIGHT. Do not restart in either later phase.
    static bool scenePhase(int phase){
        return phase==2 || phase==4 || phase==6 || phase==8;
    }
    void observe(int phase,unsigned frame,bool allowed){
        if(!allowed){
            armed=false;lastFrame=frame;lastPhase=-1;return;
        }
        if(frame==lastFrame)return; // paused or multi-pass render
        if(lastFrame!=~0u && frame<lastFrame){
            armed=false;lastPhase=-1; // native replay/snapshot clock rewind
        }
        if(!scenePhase(phase)){
            armed=false;
        }else if(!armed){
            // Normal handoff enters at 02; phase 04/06/08 fallback covers
            // direct snapshot launch without a pre-combat transition.
            armed=true;start=frame;
            std::fprintf(stderr,
                "XM87.395 HUD_SHOCK_HANDOFF frame=%u phase=%02X "
                "hold=%u reveal=%u\\n",
                frame,unsigned(phase),DelayFrames,RevealFrames);
        }
        lastFrame=frame;lastPhase=phase;
    }
    unsigned step(unsigned frame)const{
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
                   int x,int y,unsigned wide,unsigned tall,bool portrait=false,
                   bool scenery=false){
    if(scenery || !wide || !tall || x>=384 || x+int(wide*16u)<=0 ||
       y>=56 || y+int(tall*16u)<=0)return false;
    if(priority==0u)
        return palette==1u && wide==2u && tall==2u &&
               tile>=87808u && tile<=87826u && ((tile-87808u)&1u)==0u &&
               y>=-16 && y<56; // verified ROM timer-digit atlas
    if(priority!=7u)return false;
    if(portrait)return y>=-64 && y<56;
    if(y< -48)return false;
    // Small priority-7 objects spanning the upper HUD are all part of its
    // native object stream. The earlier narrow tile/size list leaked the
    // frames, labels and portrait decorations.
    if(wide<=9u && tall<=5u)return true;
    // Long bars are independently verified ROM HP strip sheets.
    return (palette==2u || palette==4u) && (y==20 || y==32) &&
           ((tile>=88417u && tile<=88579u) ||
             tile==89417u || tile==89418u);
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

// Separate specular shock front, not just the visibility dither. Two diagonal
// white-hot sweeps travel from the portrait edges toward the central timer.
// A broad blue corona follows each narrow white core, then all light ends on
// frame 30, restoring the exact unmodified source pixel.
inline int shineDistance(int x,int y,unsigned step){
    const int travel=int(step*216u/RevealFrames);
    const int slant=(y-28)/3;
    const int left=-18+travel+slant;
    const int right=402-travel-slant;
    const int a=x>=left?x-left:left-x;
    const int b=x>=right?x-right:right-x;
    return a<b?a:b;
}
inline unsigned clamp255(unsigned value){return value>255u?255u:value;}
inline std::uint32_t shineRGB(std::uint32_t source,int x,int y,unsigned step){
    if(step==0u || step>=RevealFrames)return source;
    const std::uint32_t base=shockRGB(source,step);
    const int distance=shineDistance(x,y,step);
    if(distance>30)return base;
    const unsigned r=(base>>16)&255u,g=(base>>8)&255u,b=base&255u;
    unsigned rr=r,gg=g,bb=b;
    if(distance<=3){
        // Hard, glossy white filament at the moving leading edge.
        const unsigned w=distance==0?255u:distance==1?245u:
                         distance==2?225u:200u;
        rr=byteBlend(r,255u,w,255u);
        gg=byteBlend(g,255u,w,255u);
        bb=byteBlend(b,255u,w,255u);
    }else if(distance<=12){
        // Icy electric-blue bloom immediately behind the white crest.
        const unsigned power=(unsigned)(13-distance)*18u;
        rr=clamp255(r+power/4u);
        gg=clamp255(g+power*3u/4u);
        bb=clamp255(b+power);
    }else{
        // The wider soft corona is source-pixel scoped and cannot tint
        // the arena underneath a suppressed dither pixel.
        const unsigned power=(unsigned)(31-distance)*4u;
        rr=clamp255(r+power/6u);
        gg=clamp255(g+power/2u);
        bb=clamp255(b+power);
    }
    return (source&0xff000000u)|(rr<<16)|(gg<<8)|bb;
}

// Snapshot only 16x16 pixels of the *selected HUD tile*. Restoring removed
// pixels and Z positions reveals the real scenery/fighters underneath instead
// of drawing an opaque rectangular overlay across the stage.
struct Tile {
    int x=0,y=0,w=0,h=0,pitch=0,screenWidth=0;
    std::uint32_t before[256];
    std::uint16_t zBefore[256];
    bool depthValid=false;
    void save(const std::uint8_t* pixels,int rowPitch,
              const std::uint16_t* depth,int sw,int sh,int tx,int ty){
        x=tx;y=ty;pitch=rowPitch;screenWidth=sw;
        w=h=16; // clip each pixel below; care tiles may cross screen edges
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
                const std::uint32_t tinted=shineRGB(after,px,py,step);
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
        const std::uint32_t tinted=shineRGB(native,x,y,step);
        p[0]=(unsigned char)((tinted>>16)&255u);
        p[1]=(unsigned char)((tinted>>8)&255u);
        p[2]=(unsigned char)(tinted&255u);
    }
}
}
#endif
