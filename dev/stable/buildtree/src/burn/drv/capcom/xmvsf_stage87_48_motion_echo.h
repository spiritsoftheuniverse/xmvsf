#ifndef XMVSF_STAGE87_48_MOTION_ECHO_H
#define XMVSF_STAGE87_48_MOTION_ECHO_H
// Native seven-silhouette motion echoes. No gameplay writes, no projectile copies.
// Source: buffered CPS-2 object provenance, same owner gate as 87.28 fighter mask.
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
namespace XM8748 {
constexpr unsigned kHistory=96u, kEchoes=7u, kDashFrames=90u, kCollapseFrames=12u;
struct Image {
    unsigned frame=~0u;
    int left=0,top=0,right=0,bottom=0;
    unsigned count=0;
    std::vector<std::uint8_t> value;
    void Allocate(int w,int h){if(value.size()!=std::size_t(w)*std::size_t(h))value.assign(std::size_t(w)*h,0);else std::memset(value.data(),0,value.size());
      left=w;top=h;right=0;bottom=0;count=0;}
};
struct Actor {
    std::array<Image,kHistory> history;
    XM8747::Fighter previous={0,0,0,0};
    bool seen=false;
    unsigned mode=0; // 0 none; 1 dash; 2 superjump
    unsigned started=0,ending=~0u;
    unsigned long long triggers=0;
};
struct Effect {
    std::array<Actor,2> actors;
    unsigned frame=~0u;
    int width=0,height=0;
    bool active=false;
    unsigned lastReport=0,lastDrawn=~0u;
    unsigned long long acceptedTiles=0,acceptedPixels=0,drawnPasses=0;
    unsigned lastDiagnostic=0;
    static inline std::uint8_t Ink(const std::uint8_t* tile,unsigned sx,unsigned sy){
        std::uint32_t v=0;
        std::memcpy(&v,tile+sy*8u+(sx/8u)*4u,4);
        return (std::uint8_t)((v>>(28u-4u*(sx&7u)))&15u);
    }
    void Reset(){for(auto& a:actors){a.mode=0;a.seen=false;a.ending=~0u;for(auto& h:a.history)h.frame=~0u;} }
    void Begin(unsigned f,int w,int h,const unsigned char* ram,int phase){
        if(frame==f)return;
        const bool valid=ram && phase==8 && w==384 && h==224;
        if(!valid){if(active)Reset();active=false;frame=f;return;}
        active=true;width=w;height=h;frame=f;
        for(unsigned p=0;p<2u;++p){
            Actor& a=actors[p];
            const unsigned base=0x4000u+p*0x400u;
            if(!ram[base]){a.seen=false;a.mode=0;continue;}
            const XM8747::Fighter now=XM8747::Read(ram,base);
            const XM8747::Event e=XM8747::Identify(a.previous,now,a.seen);
            if(e==XM8747::GroundDash || e==XM8747::AirDash || e==XM8747::SuperJump){
                a.mode=(e==XM8747::SuperJump)?2u:1u;
                a.started=f;a.ending=~0u;++a.triggers;
                if(a.triggers<12u || a.triggers%64u==0u)
                    std::fprintf(stderr,"XM87.48 ECHO_TRIGGER frame=%u p=%u event=%s mode=%u\n",f,p+1,XM8747::Name(e),a.mode);
            }
            // The airborne superjump state can change to attack; +AA remains set.
            if(e==XM8747::SuperJumpLand && a.mode==2u){a.ending=f;}
            if(a.mode==1u && (f-a.started)>=(kDashFrames-kCollapseFrames) && a.ending==~0u)a.ending=f;
            if(a.mode && a.ending!=~0u && f-a.ending>=kCollapseFrames)a.mode=0;
            a.previous=now;a.seen=true;
            Image& img=a.history[f%kHistory];img.Allocate(w,h);img.frame=f;
        }
    }
    void Stamp(unsigned owner,const std::uint8_t* tile,int x,int y,unsigned flip,const std::uint32_t* pal){
        if(!active || owner<1u || owner>2u || !tile || !pal)return;
        Actor& a=actors[owner-1u];if(!a.seen)return;
        Image& im=a.history[frame%kHistory];
        ++acceptedTiles;
        if(im.frame!=frame || x>=width || y>=height || x<=-16 || y<=-16)return;
        for(int py=0;py<16;++py){const int yy=y+py;if(yy<0 || yy>=height)continue;
          const unsigned sy=(flip&2u)?unsigned(15-py):unsigned(py);
          for(int px=0;px<16;++px){const int xx=x+px;if(xx<0 || xx>=width)continue;
            const unsigned sx=(flip&1u)?unsigned(15-px):unsigned(px);
            const unsigned pen=Ink(tile,sx,sy);if(!pen)continue;
            const unsigned c=pal[pen];
            const unsigned r=(c>>16)&255u,g=(c>>8)&255u,b=c&255u;
            // Keep the original artwork's pen luminance: not a flat rectangular blob.
            const unsigned l=(r*77u+g*150u+b*29u)>>8;
            const unsigned idx=unsigned(yy)*unsigned(width)+unsigned(xx);
            im.value[idx]=(std::uint8_t)(64u+(l*191u/255u));
            im.left=im.left<xx?im.left:xx;im.top=im.top<yy?im.top:yy;
            im.right=im.right>xx+1?im.right:xx+1;im.bottom=im.bottom>yy+1?im.bottom:yy+1;
            ++im.count;++acceptedPixels;
        }}
    }
    static inline void Hue(unsigned phase,unsigned& r,unsigned& g,unsigned& b){
        // 95% saturation, full value: strong rotating hues with rapid stepped flicker.
        const unsigned low=13u, high=255u,sector=(phase/256u)%6u;
        const unsigned t=phase&255u,up=low+(high-low)*t/255u,down=high-(high-low)*t/255u;
        switch(sector){case 0:r=high;g=up;b=low;break;case 1:r=down;g=high;b=low;break;
          case 2:r=low;g=high;b=up;break;case 3:r=low;g=down;b=high;break;
          case 4:r=up;g=low;b=high;break;default:r=high;g=low;b=down;break;}
    }
    static inline unsigned Opacity(unsigned slot){
        // 50%, 43%, 36%, 30%, 24%, 17%, 10% (0..255).
        static const unsigned a[7]={128,110,92,77,61,43,26};return a[slot%7u];
    }
    static inline unsigned Lag(unsigned slot){static const unsigned lag[7]={3,5,8,12,17,24,34};return lag[slot%7u];}
    void Draw(unsigned f,std::uint8_t* dst,int pitch,int bpp){
        // Draw the previous *completed* fighter snapshot. The current frame
        // is stamped inside the native body draw loop AFTER this pass.
        // Do not use a once-per-frame guard: Wily can draw more than one
        // native-sprite target during a single emulated frame.
        if(!active || !dst || bpp!=4 || pitch<width*4 || !f)return;
        lastDrawn=f;++drawnPasses;
        unsigned rendered=0;
        // In descending age order, newer echoes appear above the older echoes.
        for(unsigned p=0;p<2u;++p){Actor& a=actors[p];if(!a.mode)continue;
          const Image& cur=a.history[(f-1u)%kHistory];
          if(cur.frame!=f-1u || !cur.count)continue;
          for(int slot=6;slot>=0;--slot){
            const unsigned delay=Lag(unsigned(slot));if(f<=delay)continue;
            const unsigned oldFrame=f-1u-delay;
            const Image& old=a.history[oldFrame%kHistory];
            if(old.frame!=oldFrame || old.count==0)continue;
            const float age=a.ending==~0u?0.f:float(f-a.ending)/float(kCollapseFrames);
            const float collapse=age<=0.f?0.f:(age>=1.f?1.f:std::log1p(9.f*age)/std::log(10.f));
            //  Logarithmic easing: remain in the wake, then converge on expiry.
            const float follow=0.12f+0.88f*collapse;
            const int curCx=(cur.left+cur.right)/2,curCy=(cur.top+cur.bottom)/2;
            const int oldCx=(old.left+old.right)/2,oldCy=(old.top+old.bottom)/2;
            const int shiftX=int((curCx-oldCx)*follow),shiftY=int((curCy-oldCy)*follow);
            unsigned hueR,hueG,hueB;
            Hue((f/2u*193u+unsigned(slot)*211u+p*617u)%1536u,hueR,hueG,hueB);
            unsigned alpha=Opacity(unsigned(slot));
            if(a.ending!=~0u)alpha=unsigned(float(alpha)*(1.f-age));
            if(!alpha)continue;
            for(int y=old.top;y<old.bottom;++y){int yy=y+shiftY;if(yy<0 || yy>=height)continue;
              for(int x=old.left;x<old.right;++x){int xx=x+shiftX;if(xx<0 || xx>=width)continue;
                unsigned ink=old.value[unsigned(y)*unsigned(width)+unsigned(x)];if(!ink)continue;
                // Relative luminance preserves internal shapes and native sprite shading.
                const unsigned scale=ink;
                const unsigned srcR=hueR*scale/255u,srcG=hueG*scale/255u,srcB=hueB*scale/255u;
                const unsigned aPix=(alpha*ink+127u)/255u;
                std::uint8_t* at=dst+std::size_t(yy)*pitch+unsigned(xx)*4u;
                std::uint32_t prev=0;std::memcpy(&prev,at,4);
                // BurnHighCol is the engine's authoritative byte order.
                const std::uint32_t source=BurnHighCol(srcR,srcG,srcB,0);
                std::uint32_t out=prev;
                for(unsigned byte=0;byte<3u;++byte){
                    const unsigned shift=byte*8u;
                    const unsigned from=(prev>>shift)&255u;
                    const unsigned into=(source>>shift)&255u;
                    const unsigned mixed=(from*(255u-aPix)+into*aPix+127u)/255u;
                    out=(out&~(255u<<shift))|(mixed<<shift);
                }
                std::memcpy(at,&out,4);++rendered;
            }}
          }
        }
        if(f>60u && (lastDiagnostic==0u || f-lastDiagnostic>=180u)){
            std::fprintf(stderr,
                "XM87.49 ECHO_PATH frame=%u tiles=%llu pixels=%llu "
                "triggers=%llu/%llu modes=%u/%u passes=%llu blended=%u\n",
                f,acceptedTiles,acceptedPixels,actors[0].triggers,
                actors[1].triggers,actors[0].mode,actors[1].mode,
                drawnPasses,rendered);
            lastDiagnostic=f;
        }
        if(rendered && (lastReport==0u || f-lastReport>=240u)){
            std::fprintf(stderr,"XM87.49 ECHO_DRAW frame=%u blended_pixels=%u echoes=7 max_alpha=128/255\n",f,rendered);lastReport=f;
        }
    }
};
inline Effect& State(){static Effect e;return e;}
}
#endif
