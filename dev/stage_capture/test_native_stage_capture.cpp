#include "xmvsf_stage87_203_stage_capture.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
int main(){
    using namespace XM87203Stage;
    assert(le16((const unsigned char*)"\x01\x02",0)==0x0201u);
    // Real MVC stage_capture.zip register snapshot from frame 100.
    // Maps: 0x908000, 0x90C000 and 0x910000, all inside graphics RAM.
    const unsigned char mvcReg[0x100]={
        0x80,0x90,0x80,0x90,0xc0,0x90,0x00,0x91,
        0x60,0x91,0x40,0x91
    };
    assert(gfxMapBank(mvcReg,2u)==0x908000u);
    assert(gfxMapBank(mvcReg,4u)==0x90C000u);
    assert(gfxMapBank(mvcReg,6u)==0x910000u);
    assert(rowBank(mvcReg,8u)==0x916000u);
    assert(isMarvelVsCapcom("mvscur1"));
    assert(isMarvelVsCapcom("mvsc"));
    assert(isMarvelVsCapcom("mvscj"));
    assert(!isMarvelVsCapcom("xmvsfu"));
    Frame f; f.frame=100;f.stage=5;f.phase=8;f.sliceCount=1;
    f.driver="mvscur1";f.captureActive=true;
    unsigned char r[256]={},frg[16]={},m[3][MAP_BYTES]={},row[ROW_BYTES]={},
                  pal[PALETTE_BYTES]={},oam[OAM_BYTES]={};
    f.reg[0]=r;f.frg[0]=frg;f.rasterStart[0]=0u;
    std::memcpy(r,mvcReg,12u);
    for(unsigned x=0;x<3u;++x){
        f.map[0][x]=m[x];
        f.mapBank[0][x]=gfxMapBank(r,2u+2u*x);
    }
    f.row[0]=row;f.rowBank[0]=0x8000u;f.palette=pal;f.oam=oam;
    unsigned char decoded[2048]={};
    decoded[5]=0xA0;f.gfx=decoded;f.gfxSize=sizeof(decoded);
    f.gfxMask=0x7FF;f.gfxScrollOffset[1]=0x100;
    const char* root=std::getenv("XM87203_STAGE_CAPTURE_DIR");
    assert(root && *root);
    Recorder s;
    Frame wrong=f;wrong.driver="xmvsfu";
    s.observe(wrong);assert(!s.initialized);
    wrong.driver="mvscur1";wrong.captureActive=false;
    s.observe(wrong);assert(!s.initialized);
    s.observe(f);assert(s.enabled&&s.gfxReady&&s.frames==1u&&s.mapWrites==3u&&
                        s.rowWrites==1u&&s.paletteWrites==1u);
    s.observe(f);assert(s.frames==1u);
    f.frame=101;s.observe(f);assert(s.frames==2u&&s.mapWrites==3u);
    m[0][0]=1;row[0]=2;f.frame=102;s.observe(f);
    assert(s.mapWrites==4u&&s.rowWrites==2u);
    f.stage=6;f.frame=103;s.observe(f);
    assert(s.mapWrites==7u&&s.rowWrites==3u&&s.paletteWrites==2u);
    s.close();
    std::puts("PASS 87.203 read-only stage telemetry, changed maps/rows, duplicate-frame guard, stage reset");
}
