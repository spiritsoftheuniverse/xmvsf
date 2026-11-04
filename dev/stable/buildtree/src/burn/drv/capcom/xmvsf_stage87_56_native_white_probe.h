#ifndef XMVSF_STAGE87_56_NATIVE_WHITE_PROBE_H
#define XMVSF_STAGE87_56_NATIVE_WHITE_PROBE_H
// XM87.56: observational native CPS-2 framebuffer sampling before the
// Wily compositor. No shader, palette, game RAM or execution modifications.
// Optional TSV: XM8756_WHITE_AUDIT_FILE=<absolute writable path>.
// CpsDraw/Redraw call only after DoDraw, before sprite-only extraction.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
static void xm8756ProbeNativeWhite(){
    const char* path=std::getenv("XM8756_WHITE_AUDIT_FILE");
    if(!path || !*path || !xm8649Visual.sceneActive || Cps!=2 ||
       !pBurnDraw || nBurnBpp!=4 || nBurnPitch<384*4 ||
       nCpsScreenWidth!=384 || nCpsScreenHeight!=224)return;
    static FILE* output=nullptr;
    static bool attempted=false,wasBright=false;
    static unsigned rows=0;
    if(!attempted){
        attempted=true;output=std::fopen(path,"wb");
        if(!output){std::fprintf(stderr,"XM87.56 WHITE_AUDIT_OPEN_FAILED path=%s\n",path);return;}
        std::fprintf(output,"frame\tstage\tstatus\twhite248\tnearwhite220\tmean_luma\twhite_rows75\tcenter_rgb\tcorner_rgb\n");
        std::fprintf(stderr,"XM87.56 WHITE_AUDIT_READY path=%s native_pre_wily=1\n",path);
    }
    if(!output)return;
    unsigned white=0,near=0,whiteRows=0;
    std::uint64_t lumaSum=0;
    unsigned center=0,corner=0;
    for(int y=0;y<224;++y){
        const unsigned char* row=pBurnDraw+(size_t)y*nBurnPitch;
        unsigned rowWhites=0;
        for(int x=0;x<384;++x){
            // CPS2 32-bit native buffer order is B,G,R,X, as used in
            // xm77CaptureSprites to upload the SDL_RGBA32 texture.
            const unsigned char* q=row+4*x;
            const unsigned r=q[2],g=q[1],b=q[0];
            const bool isWhite=r>=248u && g>=248u && b>=248u;
            white+=isWhite;rowWhites+=isWhite;
            near+=(r>=220u && g>=220u && b>=220u);
            lumaSum+=(77u*r+150u*g+29u*b+128u)>>8;
            if(x==192 && y==112)center=(r<<16)|(g<<8)|b;
            if(x==0 && y==0)corner=(r<<16)|(g<<8)|b;
        }
        whiteRows+=(rowWhites>=288u);
    }
    const unsigned frame=(unsigned)GetCurrentFrame();
    const unsigned mean=(unsigned)(lumaSum/(384u*224u));
    std::fprintf(output,"%u\t%02X\t%02X\t%u\t%u\t%u\t%u\t%06X\t%06X\n",
       frame,xm8649Visual.stage,xm8649Visual.status,white,near,mean,whiteRows,center,corner);
    const bool bright=near>=(384u*224u/2u);
    if(bright!=wasBright){
       std::fprintf(stderr,"XM87.56 NATIVE_WHITE_TRANSITION frame=%u active=%d nearwhite=%u white=%u mean=%u rows=%u\n",
           frame,int(bright),near,white,mean,whiteRows);
       std::fflush(output);
       wasBright=bright;
    }
    if((++rows%120u)==0u)std::fflush(output);
}
#endif
