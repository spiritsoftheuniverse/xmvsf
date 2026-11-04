// XMVSF 87.163: exercise the ORIGINAL 87.61 F9 state+input movie
// writer and reader with a lightweight CPS2 input/mock state harness.
// Isolated C++11 test only; the live Windows native driver still needs
// normal BUILD_TEST.bat and RUN_REPLAY.bat verification.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
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
static UINT8 player1=0u,player2=0u;
static bool stateLoaded=false;
static int BurnDrvGetInputInfo(BurnInputInfo* x,unsigned index){
    if(index>=2u)return 1;
    x->szName=index?"P2":"P1";
    x->nType=1;
    x->pVal=index?&player2:&player1;
    x->szInfo="mock input";
    return 0;
}
static const char* BurnDrvGetTextA(int){return "xmvsfu";}
static int BurnStateSave(char* path,int){
    FILE* f=std::fopen(path,"wb");
    if(!f)return 1;
    const bool good=std::fwrite("FB1 ",1,4,f)==4;
    std::fclose(f);
    return good?0:1;
}
static int BurnStateLoad(char* path,int,void*){
    FILE* f=std::fopen(path,"rb");
    if(!f)return 1;
    char magic[4]={};
    const bool good=std::fread(magic,1,4,f)==4 &&
                    std::memcmp(magic,"FB1 ",4)==0;
    std::fclose(f);
    stateLoaded=good;
    return good?0:1;
}
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_61_replay.h"
int main(){
    char dirTemplate[]="/tmp/xmvsf163_f9_XXXXXX";
    const char* dir=mkdtemp(dirTemplate);
    assert(dir);
    const std::string status=std::string(dir)+"/playback_status.log";
    assert(setenv("XM8761_REPLAY_MODE","off",1)==0);
    assert(setenv("XM8761_REPLAY_DIR",dir,1)==0);
    assert(setenv("XM8761_REPLAY_STATUS_LOG",status.c_str(),1)==0);
    assert(XM8761::toggleF9(true));
    player1=0x21u;player2=0x13u;
    XM8761::afterInput();
    assert(XM8761::ctx().mode==1);
    assert(XM8761::ctx().index==1);
    player1=0x10u;player2=0x04u;
    XM8761::afterInput();
    assert(XM8761::ctx().index==2);
    assert(XM8761::toggleF9(true));
    assert(XM8761::ctx().mode==0);
    const std::string movie=XM8761::ctx().dataPath;
    const std::string state=XM8761::ctx().statePath;
    FILE* f=std::fopen(movie.c_str(),"rb");
    assert(f);
    std::fseek(f,0,SEEK_END);
    assert(std::ftell(f)==8+4+32+2*2+2*(4+2));
    std::fclose(f);
    // A new process would begin with prepared=false, mode=0.
    XM8761::Context& c=XM8761::ctx();
    c.mode=0;c.index=0;c.prepared=false;c.hotkeyRequest=false;
    c.slots.clear();c.recordedCount=0;c.errorReported=false;
    stateLoaded=false;
    assert(setenv("XM8761_REPLAY_MODE","play",1)==0);
    assert(setenv("XM8761_REPLAY_INPUT_FILE",movie.c_str(),1)==0);
    assert(setenv("XM8761_REPLAY_STATE_FILE",state.c_str(),1)==0);
    player1=player2=0;
    XM8761::afterInput();
    assert(stateLoaded);
    assert(c.mode==2 && c.index==1 && c.recordedCount==2);
    assert(player1==0x21u && player2==0x13u);
    XM8761::afterInput();
    assert(c.mode==2 && c.index==2);
    assert(player1==0x10u && player2==0x04u);
    XM8761::afterInput();
    assert(c.mode==4 && c.index==2);
    assert(player1==0 && player2==0);
    f=std::fopen(status.c_str(),"rb");
    assert(f);
    char log[1024]={};
    const size_t n=std::fread(log,1,sizeof(log)-1,f);
    std::fclose(f);
    assert(n>0);
    assert(std::strstr(log,"PLAY_READY: native state loaded, 2 movie frames"));
    assert(std::strstr(log,"PLAY_FRAME_1 injected"));
    assert(std::strstr(log,"PLAY_END: 2/2 frames consumed"));
    std::remove(movie.c_str());
    std::remove(state.c_str());
    std::remove(status.c_str());
    rmdir(dir);
    return 0;
}
