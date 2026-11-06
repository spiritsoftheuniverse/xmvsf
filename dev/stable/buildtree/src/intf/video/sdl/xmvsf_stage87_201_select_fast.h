#ifndef XMVSF_STAGE87_201_SELECT_FAST_H
#define XMVSF_STAGE87_201_SELECT_FAST_H
// DF character select: grade the NATIVE CPU framebuffer before SDL upload.
// Replaces the old GPU -> full-output readback -> CPU grayscale ->
// full-output upload -> screen copy on EVERY selection frame.
// Neither the emulated framebuffer nor emulation timing is changed.
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace XM87201Select {
struct FastGrade {
    std::vector<Uint8> scratch;
    bool announced=false;
    bool warned=false;
    bool Upload(SDL_Texture* texture,const void* original,
                int width,int height,int pitch,int bpp){
        if(!texture || !original || width<1 || height<1 ||
           width>2048 || height>2048 || (bpp!=4 && bpp!=2) ||
           pitch<width*bpp || pitch>16384)return false;
        const size_t bytes=size_t(height)*size_t(pitch);
        scratch.resize(bytes);
        std::memcpy(scratch.data(),original,bytes);
        for(int y=0;y<height;++y){
            Uint8* row=scratch.data()+size_t(y)*pitch;
            for(int x=0;x<width;++x){
                if(bpp==4){
                    // SDL_PIXELFORMAT_RGB888 / native BurnHighCol channel order.
                    // Same Rec.601 coefficients as xm8656Grayscale.
                    Uint32 pixel;
                    std::memcpy(&pixel,row+x*4,sizeof(pixel));
                    const unsigned red=(pixel>>16)&255u;
                    const unsigned green=(pixel>>8)&255u;
                    const unsigned blue=pixel&255u;
                    const Uint32 luma=(77u*red+150u*green+29u*blue+128u)>>8;
                    pixel=(pixel&0xff000000u)|(luma<<16)|(luma<<8)|luma;
                    std::memcpy(row+x*4,&pixel,sizeof(pixel));
                }else{
                    // FBNeo's 16-bit-only fallback uses RGB565.
                    Uint16 pixel;
                    std::memcpy(&pixel,row+x*2,sizeof(pixel));
                    const unsigned red=(((pixel>>11)&31u)*255u+15u)/31u;
                    const unsigned green=(((pixel>>5)&63u)*255u+31u)/63u;
                    const unsigned blue=((pixel&31u)*255u+15u)/31u;
                    const unsigned lum=(77u*red+150u*green+29u*blue+128u)>>8;
                    const Uint16 gray=Uint16((((lum*31u+127u)/255u)<<11)|
                        (((lum*63u+127u)/255u)<<5)|((lum*31u+127u)/255u));
                    std::memcpy(row+x*2,&gray,sizeof(gray));
                }
            }
        }
        if(SDL_UpdateTexture(texture,nullptr,scratch.data(),pitch)!=0){
            if(!warned){
                std::fprintf(stderr,"XM87.201 DF_SELECT_FAST_UPLOAD_FAILED %s\n",SDL_GetError());
                warned=true;
            }
            return false;
        }
        if(!announced){
            std::fprintf(stderr,
                "XM87.201 DF_SELECT_FAST native=%dx%d bpp=%d gpu_readbacks=0 output_uploads=0 mode=Rec601\n",
                width,height,bpp);
            announced=true;
        }
        return true;
    }
};
inline FastGrade& state(){static FastGrade value;return value;}
}
#endif
