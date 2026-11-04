#ifndef XMVSF_STAGE87_26_SEAMLESS_BLOOM_H
#define XMVSF_STAGE87_26_SEAMLESS_BLOOM_H
// XM87.26 - seamless, luminous external edge for XM87.25's proven fighter owner gate.
// The CPS2 4-bpp opaque pixel union excludes ALL fighter interior borders.
// Core: HSV V=100%, S=80%. Bloom: soft outer two-pixel halo.
// No FX, scenery, HUD or other source-classification changes.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace XM8726 {
// Stage87.27: deliberately retire the post-frame silhouette path.
// The already-working CPS2 object draws are the sole outline rendering method.
inline bool Seamless(){ return false; }
inline bool Bloom(){
    static const bool yes=[](){const char* e=std::getenv("XM8726_BLOOM");
        return !(e && e[0]=='0' && !e[1]);}();
    return yes;
}
struct RGB {unsigned r,g,b;};
inline RGB Color(unsigned hue){
    const unsigned hi=255u,lo=51u; // 100% value, 80% saturation
    hue%=1536u;
    const unsigned section=hue/256u,t=hue%256u;
    const unsigned up=lo+(hi-lo)*t/255u;
    const unsigned down=hi-(hi-lo)*t/255u;
    switch(section){
    case 0:return {hi,up,lo};
    case 1:return {down,hi,lo};
    case 2:return {lo,hi,up};
    case 3:return {lo,down,hi};
    case 4:return {up,lo,hi};
    default:return {hi,lo,down};
    }
}
inline std::uint32_t Mix(std::uint32_t current,std::uint32_t ink,unsigned opacity){
    if(opacity>255u)opacity=255u;
    const unsigned inverse=255u-opacity;
    std::uint32_t result=current&0xff000000u;
    for(unsigned shift=0;shift<=16u;shift+=8u){
        const unsigned src=(ink>>shift)&255u;
        const unsigned dst=(current>>shift)&255u;
        result|=((src*opacity+dst*inverse+127u)/255u)<<shift;
    }
    return result;
}
struct Painter {
    int width=0,height=0;
    bool armed=false;
    std::vector<std::uint8_t> opaque;
    unsigned long long tiles=0,pixels=0;
    void Begin(bool combat,int w,int h,int bpp){
        armed=Seamless() && combat && bpp==4 && w>0 && h>0 && w<=2048 && h<=2048;
        width=armed?w:0;height=armed?h:0;
        tiles=0;pixels=0;
        if(armed)opaque.assign(std::size_t(width)*height,0);
        else opaque.clear();
    }
    // CPS-2 CTV nibble order; transparent pen=0. Match native x/y and flips.
    static unsigned Pen(const std::uint8_t* tile,unsigned x,unsigned y){
        std::uint32_t v=0;
        std::memcpy(&v,tile+y*8u+(x/8u)*4u,4u);
        return (v>>(28u-4u*(x%8u)))&15u;
    }
    void Stamp(const std::uint8_t* tile,int x,int y,unsigned flip){
        if(!armed || !tile || x>=width || y>=height || x<=-16 || y<=-16)return;
        ++tiles;
        for(int yy=0;yy<16;++yy){
            const int destY=y+yy;
            if(destY<0 || destY>=height)continue;
            const unsigned srcY=(flip&2u)?unsigned(15-yy):unsigned(yy);
            for(int xx=0;xx<16;++xx){
                const int destX=x+xx;
                if(destX<0 || destX>=width)continue;
                const unsigned srcX=(flip&1u)?unsigned(15-xx):unsigned(xx);
                if(Pen(tile,srcX,srcY)){
                    const std::size_t index=std::size_t(destY)*width+destX;
                    if(!opaque[index]){opaque[index]=1; ++pixels;}
                }
            }
        }
    }
    bool Has(int x,int y) const {
        return x>=0 && x<width && y>=0 && y<height &&
               opaque[std::size_t(y)*width+x]!=0;
    }
    static unsigned Phase(unsigned frame,unsigned dir){
        return ((frame%360u)*1536u/360u+dir*384u)%1536u;
    }
    void Composite(std::uint8_t* dest,int bpp,int pitch,unsigned frame){
        static unsigned logged=0;
        if(!armed || !dest || bpp!=4 || pitch<width*4 ||
           opaque.size()!=std::size_t(width)*height)return;
        unsigned long long edges=0,halo=0;
        if(tiles){
            std::uint32_t ink[4];
            for(unsigned d=0;d<4u;++d){
                const RGB c=Color(Phase(frame,d));
                ink[d]=BurnHighCol(c.r,c.g,c.b,0);
            }
            for(int y=0;y<height;++y){
                for(int x=0;x<width;++x){
                    if(Has(x,y))continue; // Never reveal a 16x16 tile/object seam.
                    const bool touch[4]={Has(x+1,y),Has(x-1,y),Has(x,y+1),Has(x,y-1)};
                    bool near=false;
                    for(unsigned d=0;d<4u;++d)if(touch[d])near=true;
                    std::uint8_t* loc=dest+std::size_t(y)*pitch+std::size_t(x)*4u;
                    std::uint32_t pixel=0;
                    if(!near && Bloom()){
                        // Exactly two pixels outside the union; low-opacity
                        // halo with diagonal falloff, never on character ink.
                        static const int offsets[12][2]={
                            {2,0},{-2,0},{0,2},{0,-2},
                            {2,1},{2,-1},{-2,1},{-2,-1},
                            {1,2},{-1,2},{1,-2},{-1,-2}};
                        int dir=-1;
                        unsigned strength=0;
                        for(int i=0;i<12;++i)if(Has(x+offsets[i][0],y+offsets[i][1])){
                            dir=(i<4)?i:(i<8?(i<6?0:1):(i<10?2:3));
                            strength=(i<4)?75u:43u;break;
                        }
                        if(dir<0){
                            const int diag[4][2]={{1,1},{-1,1},{1,-1},{-1,-1}};
                            for(int i=0;i<4;++i)if(Has(x+diag[i][0],y+diag[i][1])){
                                dir=(i<2)?2:3;strength=32u;break;
                            }
                        }
                        if(dir>=0){
                            std::memcpy(&pixel,loc,4);
                            pixel=Mix(pixel,ink[dir],strength);
                            std::memcpy(loc,&pixel,4);++halo;
                        }
                    }
                    if(near){
                        std::memcpy(&pixel,loc,4);
                        for(unsigned d=0;d<4u;++d)
                            if(touch[d])pixel=Mix(pixel,ink[d],225u);
                        std::memcpy(loc,&pixel,4);++edges;
                    }
                }
            }
        }
        if((++logged)<=4u || logged%120u==0u){
            std::fprintf(stderr,"XM87.26 SEAMLESS_BLOOM frame=%u tiles=%llu body_pixels=%llu edge_pixels=%llu halo_pixels=%llu brightV=100 saturation=80 bpp=%d\n",
                frame,tiles,pixels,edges,halo,bpp);
        }
    }
};
inline Painter& State(){static Painter painter;return painter;}
}
#endif
