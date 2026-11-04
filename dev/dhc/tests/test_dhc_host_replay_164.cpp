// XMVSF 87.164 — isolated C++11 native host-aware F9 movie test.
// Verifies two game sessions, a host-menu state checkpoint, native
// save-state loads, exact emulated input bytes and end-of-session gating.
// The real SDL/CPS2 Windows app must still be verified separately.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
typedef unsigned char UINT8;
struct BurnInputInfo {
    const char* szName;
    int nType;
    UINT8* pVal;
    const char* szInfo;
};
enum { DRV_NAME=1, BIT_GROUP_ANALOG=0x100 };
static UINT8 p1=0u,p2=0u;
static UINT8 nativeMarker=0u;
static unsigned loads=0u,saves=0u;
static int BurnDrvGetInputInfo(BurnInputInfo* x,unsigned index){
    if(index>=2u)return 1;
    x->szName=index?"P2":"P1";
    x->nType=1;x->pVal=index?&p2:&p1;x->szInfo="mock";return 0;
}
static const char* BurnDrvGetTextA(int){return "xmvsfu";}
static int BurnStateSave(char* path,int){
    FILE* f=std::fopen(path,"wb");if(!f)return 1;
    const bool good=std::fwrite("FB1 ",1,4,f)==4 &&
                    std::fwrite(&nativeMarker,1,1,f)==1;
    std::fclose(f);if(good)++saves;return good?0:1;
}
static int BurnStateLoad(char* path,int,void*){
    FILE* f=std::fopen(path,"rb");if(!f)return 1;
    char magic[4]={};
    bool good=std::fread(magic,1,4,f)==4 &&
        std::memcmp(magic,"FB1 ",4)==0 &&
        std::fread(&nativeMarker,1,1,f)==1;
    std::fclose(f);if(good)++loads;return good?0:1;
}
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_164_host_replay.h"
int main(){
    char name[]="/tmp/xmvsf_v2_XXXXXX";
    const char* root=mkdtemp(name);assert(root);
    assert(setenv("XM8764_REPLAY_ROOT",root,1)==0);
    const std::string publicStatus=std::string(root)+"/F9_status.log";
    assert(setenv("XM8764_REPLAY_STATUS_FILE",publicStatus.c_str(),1)==0);
    XM8764::initialize();assert(XM8764::context().mode==XM8764::OFF);
    assert(XM8764::toggleF9());
    assert(XM8764::isRecording());
    assert(XM8764::recordingFrames()==1); // visible 000000 before game
    assert(XM8764::visualStatus()==1);
    const std::string folder=XM8764::context().folder;
    // F9 from the host menu creates the on-disk manifest IMMEDIATELY,
    // before any ROM/Training selection was made.
    assert(XM8764::exists(folder+"/manifest.tsv"));
    assert(XM8764::exists(publicStatus));
    XM8764::Host training;training.training=1;training.owner=0;
    XM8764::gameBegin(1);
    nativeMarker=3;p1=0x12;p2=0x01;
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    p1=0x35;p2=0x05;
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    // A host-only Training owner switch is not part of CPS2 input
    // bytes; it must split the movie and save a new checkpoint.
    training.owner=1;
    XM8764::hostEvent("TRAINING_OWNER_CHANGED",1);
    nativeMarker=7;p1=0x21;p2=0x09;
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    XM8764::gameEnd();
    XM8764::Host arcade;arcade.training=0;
    XM8764::gameBegin(0);
    nativeMarker=11;p1=0x40;p2=0x02;
    assert(XM8764::beforeFrame(arcade)==XM8764::FRAME_READY);
    XM8764::gameEnd();
    assert(XM8764::toggleF9());
    assert(XM8764::recordingFrames()==0);
    assert(XM8764::visualStatus()==2); // saved indicator
    assert(XM8764::exists(std::string(root)+"/latest.txt"));
    assert(saves==3u);
    assert(XM8764::exists(folder+"/manifest.tsv"));
    // Simulate a new process opening the same finished v2 session.
    XM8764::Context& c=XM8764::context();
    c.initialized=false;c.mode=XM8764::OFF;c.root.clear();
    c.stream=nullptr;c.journal=nullptr;
    c.segments.clear();c.playIndex=0;c.allFrames=0;
    c.insideGame=false;c.exitSession=false;c.failed=false;
    assert(setenv("XM8764_REPLAY_SESSION",folder.c_str(),1)==0);
    assert(XM8764::isPlayback());
    assert(c.segments.size()==3u);
    assert(XM8764::nextMode()==1);
    XM8764::gameBegin(1);
    nativeMarker=0;p1=p2=0;
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    assert(nativeMarker==3 && p1==0x12 && p2==0x01);
    assert(c.hostChanged && c.restored.owner==0);c.hostChanged=false;
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    assert(p1==0x35 && p2==0x05);
    assert(XM8764::beforeFrame(training)==XM8764::FRAME_READY);
    assert(nativeMarker==7 && p1==0x21 && p2==0x09);
    assert(c.hostChanged && c.restored.owner==1);c.hostChanged=false;
    assert(XM8764::beforeFrame(training)==XM8764::END_GAME);
    assert(c.exitSession && c.playIndex==2u);
    XM8764::gameEnd();
    assert(XM8764::nextMode()==0);
    XM8764::gameBegin(0);
    assert(XM8764::beforeFrame(arcade)==XM8764::FRAME_READY);
    assert(nativeMarker==11 && p1==0x40 && p2==0x02);
    assert(XM8764::beforeFrame(arcade)==XM8764::END_GAME);
    assert(c.mode==XM8764::COMPLETE && c.allFrames==4u);
    XM8764::gameEnd();
    assert(loads==3u);
    assert(XM8764::exists(folder+"/replay_v2_status.log"));
    // Failed disk/path setups produce a visible ERROR indicator and a
    // persistent root-level status log, not fake RECORDING success.
    unsetenv("XM8764_REPLAY_SESSION");
    c.initialized=false;c.mode=XM8764::OFF;c.folder.clear();
    c.journal=nullptr;c.stream=nullptr;
    assert(setenv("XM8764_REPLAY_ROOT",
                  "/dev/null/xmvsf_f9_unwritable",1)==0);
    assert(!XM8764::toggleF9());
    assert(c.mode==XM8764::FAILED);
    assert(XM8764::visualStatus()==-1);
    assert(XM8764::exists(publicStatus));
    return 0;
}
