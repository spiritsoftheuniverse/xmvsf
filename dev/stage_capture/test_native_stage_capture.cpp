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
    s.observe(f);
    // Regression: the first capture must create an ABSOLUTE root/session_NNNN
    // directory, not a relative session_NNNN under the current working dir.
    assert(s.sessionDir.size()>s.root.size()+1u);
    assert(s.sessionDir.compare(0,s.root.size()+1u,s.root+"/")==0);
    assert(s.sessionDir.find('/',s.root.size()+1u)==std::string::npos);
    {
        std::FILE* latest=std::fopen((s.root+"/latest.txt").c_str(),"rb");
        assert(latest);
        char line[64]={};
        assert(std::fgets(line,sizeof(line),latest)!=NULL);
        std::fclose(latest);
        line[std::strcspn(line,"\r\n")]=0;
        assert(std::strlen(line)==12u);
        assert(s.sessionDir==s.root+"/"+std::string(line));
    }
    assert(s.enabled&&s.gfxReady&&s.frames==1u&&s.mapWrites==3u&&
                        s.rowWrites==1u&&s.paletteWrites==1u);
    s.observe(f);assert(s.frames==1u);
    f.frame=101;s.observe(f);assert(s.frames==2u&&s.mapWrites==3u);
    m[0][0]=1;row[0]=2;f.frame=102;s.observe(f);
    assert(s.mapWrites==4u&&s.rowWrites==2u);
    f.stage=6;f.frame=103;s.observe(f);
    assert(s.mapWrites==7u&&s.rowWrites==3u&&s.paletteWrites==2u);
    s.close();
    // Another recorder in the same process must not reuse or overwrite the
    // previous session's raw data.
    const std::string original=s.sessionDir;
    Recorder second;
    Frame f2=f;f2.frame=104u;
    second.observe(f2);
    assert(second.enabled&&second.sessionDir!=original);
    assert(second.sessionDir.compare(0,second.root.size()+1u,second.root+"/")==0);
    // Regression against the historical 36,000 frame silent hard-stop.
    // Boundaries rotate into a new session *without losing the boundary
    // frame*, and are tested at the minimum 300-frame batch.
    // Set XM87203_BATCH_FRAMES=300 for the smoke test from BUILD_TEST.sh.
    const unsigned limit=batchFrameLimit();
    assert(limit>=300u && limit<=36000u);
    const std::string batch0=second.sessionDir;
    for(unsigned i=1;i<=limit;++i){
        f2.frame=104u+i;
        second.observe(f2);
        assert(second.enabled&&!second.failed);
    }
    assert(second.completedBatches==1u);
    assert(second.totalFrames==static_cast<unsigned long long>(limit)+1ull);
    assert(second.frames==1u);
    assert(second.sessionDir!=batch0);
    assert(second.lastFrame==104u+limit);
    assert(second.mapWrites==3u);
    second.close();
    std::puts("PASS 87.203 native telemetry + continuous frame-batch rollover, no lost boundary frame or old-session overwrite");
}
