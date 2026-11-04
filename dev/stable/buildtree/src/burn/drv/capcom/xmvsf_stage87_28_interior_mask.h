#ifndef XMVSF_STAGE87_28_INTERIOR_MASK_H
#define XMVSF_STAGE87_28_INTERIOR_MASK_H
// Stage 87.28: clip 87.27's four direct, bright, 1px sprite copies against
// the union of P1/P2 body opacity. NOT a post-frame compositor or bloom.
// Each object is drawn normally: we restore only destination pixels which
// were occupied by ANY fighter body before its offset draw.
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
namespace XM8728 {
inline bool Enabled() {
    static const bool on=[](){const char* s=std::getenv("XM8728_MASK_INTERIOR");
        return !(s && s[0]=='0' && s[1]=='\0');}();
    return on;
}
struct Mask {
    unsigned frame=0;
    bool started=false, ready=false;
    int width=0,height=0;
    std::vector<std::uint8_t> body;
    unsigned long long occupied=0, tiles=0, clipped=0, drawn=0;
    static unsigned Pen(const std::uint8_t* p,unsigned x,unsigned y){
        std::uint32_t v=0;
        std::memcpy(&v,p+y*8u+(x/8u)*4u,4u);
        return (v>>(28u-4u*(x&7u)))&15u;
    }
    void Begin(unsigned current,int w,int h,int bpp){
        if(started && frame==current)return;
        started=true;frame=current; ready=false;
        width=0;height=0;occupied=tiles=clipped=drawn=0;
        if(Enabled() && bpp==4 && w>0 && w<=2048 && h>0 && h<=2048){
            width=w;height=h;
            body.assign(std::size_t(width)*std::size_t(height),0);
        }else body.clear();
    }
    void Stamp(const std::uint8_t* pixels,int x,int y,unsigned flip){
        if(!width || !pixels || x>=width || y>=height || x<=-16 || y<=-16)return;
        ++tiles;
        for(int py=0;py<16;++py){
            const int dy=y+py;
            if(dy<0 || dy>=height)continue;
            const unsigned sy=(flip&2u)?unsigned(15-py):unsigned(py);
            for(int px=0;px<16;++px){
                const int dx=x+px;
                if(dx<0 || dx>=width)continue;
                const unsigned sx=(flip&1u)?unsigned(15-px):unsigned(px);
                if(Pen(pixels,sx,sy)){
                    const std::size_t i=std::size_t(dy)*std::size_t(width)+unsigned(dx);
                    if(!body[i]){body[i]=1;++occupied;}
                }
            }
        }
    }
    struct Backup {
        std::uint32_t previous[256];
        std::uint8_t keep[256];
        unsigned count=0;
        Backup():previous(),keep(){}
    };
    void Save(Backup& backup,const std::uint8_t* dest,int pitch,int x,int y){
        if(!ready || !dest || pitch<width*4 || !width || x>=width || y>=height ||
           x<=-16 || y<=-16)return;
        ++drawn;
        for(int py=0;py<16;++py){
            const int dy=y+py;
            if(dy<0 || dy>=height)continue;
            for(int px=0;px<16;++px){
                const int dx=x+px;
                if(dx<0 || dx>=width)continue;
                if(!body[std::size_t(dy)*width+dx])continue;
                const int idx=py*16+px;
                backup.keep[idx]=1;
                std::memcpy(&backup.previous[idx],dest+std::size_t(dy)*pitch+std::size_t(dx)*4u,4);
                ++backup.count;
            }
        }
    }
    void Restore(const Backup& backup,std::uint8_t* dest,int pitch,int x,int y){
        if(!dest || !backup.count)return;
        for(int py=0;py<16;++py){
            const int dy=y+py;
            if(dy<0 || dy>=height)continue;
            for(int px=0;px<16;++px){
                const int dx=x+px;
                if(dx<0 || dx>=width)continue;
                const int idx=py*16+px;
                if(backup.keep[idx]){
                    std::memcpy(dest+std::size_t(dy)*pitch+std::size_t(dx)*4u,&backup.previous[idx],4);
                }
            }
        }
        clipped+=backup.count;
    }
    void Report(){
        static unsigned last=~0u;
        if(frame==last || (!drawn && !tiles))return;
        if(frame<3u || frame%120u==0u){
            std::fprintf(stderr,
                "XM87.28 INTERIOR_MASK frame=%u tiles=%llu body_pixels=%llu masked_positions=%llu offset_tiles=%llu ready=%d brightness=100 saturation=80 bloom=0\n",
                frame,tiles,occupied,clipped,drawn,(int)ready);
        }
        last=frame;
    }
};
inline Mask& State(){static Mask mask;return mask;}
}
#endif
