#ifndef XMVSF_STAGE87_52_UNDERLAY_GHOST_H
#define XMVSF_STAGE87_52_UNDERLAY_GHOST_H
// Seven solid-color silhouettes rendered BEHIND the accepted Stage87.25/28 fighter outline.
// Seven optional fighter-body passes, no whole-frame sprite capture or owner-only FX guess.
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
namespace XM8752 {
constexpr unsigned kEchoes=7,kHistory=96,kDash=90,kCollapse=12;
struct Coord {unsigned frame=~0u; int x=0,y=0;};
struct Actor {
    std::array<Coord,kHistory> history;
    XM8747::Fighter previous={0,0,0,0};
    bool ready=false; unsigned mode=0,started=0,ending=~0u;
    unsigned triggers=0;
};
struct Offset {int x=0,y=0; unsigned alpha=0;};
struct Effect {
    Actor actors[2]; unsigned frame=~0u; bool active=false;
    int width=0,height=0; unsigned long long tiles=0,echoPasses=0;
    unsigned lastReport=0;
    void Reset(){for(Actor& a:actors){a.mode=0;a.ready=false;a.ending=~0u;
      for(Coord& h:a.history)h.frame=~0u;}}
    void Begin(unsigned f,int w,int h,const unsigned char* ram,int phase){
        if(frame==f)return;
        frame=f;active=(ram && phase==8 && w==384 && h==224);
        width=w;height=h;
        if(!active){Reset();return;}
        for(unsigned p=0;p<2;++p){
            Actor& a=actors[p];const unsigned base=0x4000u+0x400u*p;
            const XM8747::Fighter now=XM8747::Read(ram,base);
            // Action byte / coordinates are read-only game-owned state.
            const XM8747::Event e=XM8747::Identify(a.previous,now,a.ready);
            if(e==XM8747::GroundDash || e==XM8747::AirDash || e==XM8747::SuperJump){
                a.mode=e==XM8747::SuperJump?2u:1u;
                a.started=f;a.ending=~0u;++a.triggers;
                if(a.triggers<=12u || a.triggers%64u==0u)
                    std::fprintf(stderr,"XM87.52 UNDERLAY_TRIGGER frame=%u p=%u event=%s count=%u\n",
                                 f,p+1u,XM8747::Name(e),a.triggers);
            }
            if(e==XM8747::SuperJumpLand && a.mode==2u)a.ending=f;
            if(a.mode==1u && a.ending==~0u && f-a.started>=kDash-kCollapse)a.ending=f;
            if(a.mode && a.ending!=~0u && f-a.ending>=kCollapse)a.mode=0;
            Coord& point=a.history[f%kHistory];point.frame=f;point.x=int(now.x);point.y=int(now.y);
            a.previous=now;a.ready=true;
        }
    }
    bool Enabled(unsigned owner)const {return active && owner>=1u && owner<=2u && actors[owner-1u].mode!=0;}
    Offset Position(unsigned owner,unsigned index)const {
        Offset o;if(!Enabled(owner) || index>=kEchoes)return o;
        const Actor& a=actors[owner-1u];
        static const unsigned lag[7]={3,5,8,12,17,24,34};
        static const unsigned opacity[7]={204,173,143,115,89,64,41}; // visible 80%-16% per-copy
        if(frame<=lag[index])return o;
        const Coord& old=a.history[(frame-lag[index])%kHistory];
        const Coord& now=a.history[frame%kHistory];
        if(old.frame!=frame-lag[index] || now.frame!=frame)return o;
        const unsigned delta= a.ending==~0u?0u:frame-a.ending;
        if(delta>=kCollapse)return o;
        const float t=std::fmin(1.f,2.f*float(delta)/float(kCollapse));
        const float ease=delta?std::log1p(9.f*t)/std::log(10.f):0.f;
        const float remaining=1.f-ease;
        o.x=int(2.f*float(old.x-now.x)*remaining);
        o.y=int(2.f*float(old.y-now.y)*remaining);
        // Big coordinate jumps may be screen transitions or teleports.
        if(o.x>240 || o.x< -240 || o.y>200 || o.y< -200)return Offset();
        o.alpha=unsigned(float(opacity[index])*remaining);
        return o;
    }
    static void Hue(unsigned phase,unsigned& r,unsigned& g,unsigned& b){
        const unsigned low=6,high=255,seg=(phase/256u)%6u,t=phase&255u;
        const unsigned up=low+(high-low)*t/255u,down=high-(high-low)*t/255u;
        switch(seg){case 0:r=high;g=up;b=low;break;case 1:r=down;g=high;b=low;break;
          case 2:r=low;g=high;b=up;break;case 3:r=low;g=down;b=high;break;
          case 4:r=up;g=low;b=high;break;default:r=high;g=low;b=down;break;}
    }
    void Palette(const std::uint32_t* normal,std::uint32_t out[16],unsigned owner,unsigned slot)const {
        unsigned r,g,b;Hue((frame/2u*193u+slot*211u+owner*617u)%1536u,r,g,b);
        // The original tile art only decides opaque versus transparent.
        // EVERY non-transparent ink in an echo uses exactly the same color,
        // regardless of the source sprite's ink value or tile boundary.
        out[0]=normal[0];
        const std::uint32_t solid=BurnHighCol(r,g,b,0);
        for(unsigned pen=1;pen<16;++pen)out[pen]=solid;
    }
    struct TileBlend {
        int x=0,y=0,pitch=0,w=0,h=0;
        unsigned char* pixels=nullptr;
        std::uint32_t before[256];
        void Save(unsigned char* dst,int stride,int px,int py,int width,int height){
            pixels=dst;pitch=stride;x=px;y=py;w=width;h=height;
            if(!dst || stride<width*4)return;
            for(int sy=0;sy<16;++sy)for(int sx=0;sx<16;++sx){
                const int xx=px+sx,yy=py+sy;std::uint32_t old=0;
                if(xx>=0 && xx<width && yy>=0 && yy<height)
                    std::memcpy(&old,dst+std::size_t(yy)*stride+std::size_t(xx)*4u,4);
                before[sy*16+sx]=old;
            }
        }
        static std::uint8_t Ink(const unsigned char* tile,unsigned sx,unsigned sy){
            std::uint32_t v=0;std::memcpy(&v,tile+sy*8u+(sx/8u)*4u,4);
            return (std::uint8_t)((v>>(28u-4u*(sx&7u)))&15u);
        }
        void Composite(const unsigned char* tile,unsigned flip,unsigned alpha,
                       int /* reserved */=0){
            if(!pixels || !tile || !alpha)return;
            for(int sy=0;sy<16;++sy)for(int sx=0;sx<16;++sx){
                const int xx=x+sx,yy=y+sy;
                if(xx<0 || xx>=w || yy<0 || yy>=h)continue;
                // Ghosts render before the native fighter; body occlusion is
                // provided by the subsequent normal draw, not by a pixel mask.
                const unsigned tx=(flip&1u)?15u-unsigned(sx):unsigned(sx);
                const unsigned ty=(flip&2u)?15u-unsigned(sy):unsigned(sy);
                if(!Ink(tile,tx,ty))continue;
                const std::uint32_t original=before[sy*16+sx];
                std::uint32_t current=0,out=original;
                unsigned char* pixel=pixels+std::size_t(yy)*pitch+std::size_t(xx)*4u;
                std::memcpy(&current,pixel,4);
                for(unsigned shift=0;shift<24;shift+=8){
                    const unsigned old=(original>>shift)&255u,newer=(current>>shift)&255u;
                    const unsigned mix=(old*(255u-alpha)+newer*alpha+127u)/255u;
                    out=(out&~(255u<<shift))|(mix<<shift);
                }
                std::memcpy(pixel,&out,4);
            }
        }
    };
    void Report(unsigned rendered){
        ++tiles;echoPasses+=rendered;
        if(rendered && (lastReport==0u || frame-lastReport>=180u)){
            lastReport=frame;
            std::fprintf(stderr,"XM87.52 MONO_UNDERLAY_ECHO frame=%u passes=%llu tiles=%llu mode=%u/%u opacity<=204\n",
                frame,echoPasses,tiles,actors[0].mode,actors[1].mode);
        }
    }
};
inline Effect& State(){static Effect fx;return fx;}
}
#endif
