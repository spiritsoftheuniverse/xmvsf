// XMVSF 87.61 — opt-in, SDL game-input movie recorder/replayer.
// The canonical emulated inputs are sampled AFTER InputMake(true) and BEFORE BurnDrvFrame().
// This is not a video file. It combines full BurnStateSave/Load RAM with per-frame input bytes.
// No Classic/Tournament/DF dependency. No use outside XM8761_REPLAY_MODE=record|play.
#ifndef XMVSF_STAGE87_61_REPLAY_H
#define XMVSF_STAGE87_61_REPLAY_H
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

namespace XM8761 {
struct Slot { UINT8* address; UINT8 type; UINT8 width; };
struct Context {
    int mode=0; // 0=off, 1=record, 2=play, 3=refused, 4=replay finished
    bool hotkeyRequest=false;
    unsigned recordingSerial=0;
    bool prepared=false;
    bool errorReported=false;
    FILE* stream=nullptr;
    std::vector<Slot> slots;
    std::string dataPath,statePath;
    uint32_t index=0;
    uint32_t recordedCount=0;
    ~Context(){if(stream)std::fclose(stream);}
};
static Context& ctx(){static Context c;return c;}
static bool write32(FILE* f,uint32_t v){
    UINT8 b[4]={(UINT8)v,(UINT8)(v>>8),(UINT8)(v>>16),(UINT8)(v>>24)};
    return std::fwrite(b,1,4,f)==4;
}
static bool read32(FILE* f,uint32_t& v){
    UINT8 b[4];if(std::fread(b,1,4,f)!=4)return false;
    v=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;
}
static void report(const char* message){
    std::fprintf(stderr,"XM87.61 REPLAY %s\n",message);std::fflush(stderr);
}
static bool collect(std::vector<Slot>& slots) {
    slots.clear();
    for(unsigned i=0;i<1024;i++){
        BurnInputInfo info={};
        if(BurnDrvGetInputInfo(&info,i))break;
        if(!info.pVal)continue;
        unsigned width=(info.nType & BIT_GROUP_ANALOG)?2u:1u;
        slots.push_back({info.pVal,info.nType,(UINT8)width});
    }
    return !slots.empty() && slots.size()<=1024;
}
static bool init(){
    Context& c=ctx();c.prepared=true;
    const char* mode=std::getenv("XM8761_REPLAY_MODE");
    if(c.hotkeyRequest)mode="record";
    if(!mode || !*mode || !std::strcmp(mode,"off")){c.mode=0;return true;}
    c.mode=!std::strcmp(mode,"record")?1:(!std::strcmp(mode,"play")?2:3);
    if(c.mode==3){report("REFUSED: invalid mode");return false;}
    const char* directory=std::getenv("XM8761_REPLAY_DIR");
    if(!directory || !*directory){c.mode=3;report("REFUSED: replay directory unset");return false;}
    if(!collect(c.slots)){c.mode=3;report("REFUSED: no game inputs found");return false;}
    c.dataPath=std::string(directory)+"/xm8761_inputs.bin";
    c.statePath=std::string(directory)+"/xm8761_start.fs";
    if(c.hotkeyRequest){
        // Each F9 recording is an independent input+state pair; never overwrite a previous movie.
        bool available=false;
        for(unsigned attempt=0;attempt<10000;attempt++){
            char stem[64];std::snprintf(stem,sizeof(stem),"xm8761_f9_%04u",++c.recordingSerial);
            c.dataPath=std::string(directory)+"/"+stem+"_inputs.bin";
            c.statePath=std::string(directory)+"/"+stem+"_start.fs";
            FILE* prev=std::fopen(c.dataPath.c_str(),"rb");
            FILE* state=std::fopen(c.statePath.c_str(),"rb");
            available=!prev && !state;
            if(prev)std::fclose(prev);
            if(state)std::fclose(state);
            if(available)break;
        }
        if(!available){c.mode=3;report("REFUSED: no free recording filename");return false;}
    }
    // Playback may point at a specific F9 recording by setting both override paths.
    if(c.mode==2){
        const char* inputs=std::getenv("XM8761_REPLAY_INPUT_FILE");
        const char* state=std::getenv("XM8761_REPLAY_STATE_FILE");
        if(inputs && *inputs)c.dataPath=inputs;
        if(state && *state)c.statePath=state;
    }
    const char* rom=BurnDrvGetTextA(DRV_NAME);
    char romName[32]={};if(rom)std::strncpy(romName,rom,sizeof(romName)-1);
    if(c.mode==1){
        // Capture the exact full emulator state before emulating the first saved input frame.
        if(BurnStateSave(const_cast<char*>(c.statePath.c_str()),1)!=0){c.mode=3;report("REFUSED: full state save failed");return false;}
        c.stream=std::fopen(c.dataPath.c_str(),"wb");
        if(!c.stream){c.mode=3;report("REFUSED: input recording file could not be opened");return false;}
        if(std::fwrite("XM8761R1",1,8,c.stream)!=8 ||
            !write32(c.stream,(uint32_t)c.slots.size()) ||
            std::fwrite(romName,1,32,c.stream)!=32){c.mode=3;report("REFUSED: header write failed");return false;}
        for(const Slot& s:c.slots){
            const UINT8 meta[2]={s.type,s.width};
            if(std::fwrite(meta,1,2,c.stream)!=2){c.mode=3;report("REFUSED: slot header write failed");return false;}
        }
        std::fprintf(stderr,"XM87.132 RECORD_START state=%s inputs=%s\n",c.statePath.c_str(),c.dataPath.c_str());
        report("RECORD_READY input stream plus full state anchor");
    } else {
        c.stream=std::fopen(c.dataPath.c_str(),"rb");
        if(!c.stream){c.mode=3;report("REFUSED: replay input file missing");return false;}
        char signature[8],fileRom[32];uint32_t count=0;
        if(std::fread(signature,1,8,c.stream)!=8 ||
           std::memcmp(signature,"XM8761R1",8) || !read32(c.stream,count) ||
           count!=c.slots.size() || std::fread(fileRom,1,32,c.stream)!=32 ||
           std::memcmp(romName,fileRom,32)){
            c.mode=3;report("REFUSED: replay header / ROM / input count mismatch");return false;
        }
        for(const Slot& s:c.slots){
            UINT8 meta[2];
            if(std::fread(meta,1,2,c.stream)!=2 || meta[0]!=s.type || meta[1]!=s.width){
                c.mode=3;report("REFUSED: input layout changed between builds");return false;
            }
        }
        if(BurnStateLoad(const_cast<char*>(c.statePath.c_str()),1,NULL)!=0){
            c.mode=3;report("REFUSED: full state load failed (incompatible build or missing state)");return false;
        }
        report("PLAY_READY: full state loaded; emulated inputs will be injected");
    }
    return true;
}
static void clearInputs(){
    Context& c=ctx();
    for(const Slot& s:c.slots){
        if(s.width==1)*s.address=0;
        else {s.address[0]=0;s.address[1]=0;}
    }
}
// Hook exactly once per real emulated frame, AFTER InputMake(true), prior to BurnDrvFrame().
static void afterInput(){
    Context& c=ctx();
    if(!c.prepared) if(!init())return;
    if(c.mode==0 || c.mode==3)return;
    if(c.mode==4){clearInputs();return;}
    if(c.mode==1){
        if(!write32(c.stream,c.index)){c.mode=3;report("STOP: input stream write failed");return;}
        for(const Slot& s:c.slots){
            if(std::fwrite(s.address,1,s.width,c.stream)!=s.width){
                c.mode=3;report("STOP: input stream write failed");return;
            }
        }
        ++c.index;
        if((c.index%60)==0)std::fflush(c.stream);
        if(c.index>=36000){std::fflush(c.stream);c.mode=3;report("STOP: maximum 36000 recorded frames");}
    } else if(c.mode==2){
        uint32_t frame;
        if(!read32(c.stream,frame)){
            clearInputs();c.mode=4;report("PLAY_END: input stream exhausted; all emulated inputs released");return;
        }
        if(frame!=c.index){clearInputs();c.mode=3;report("REFUSED: input frame sequence mismatch");return;}
        // Read whole frame before injecting. Never mix a partial replay frame with live inputs.
        std::vector<UINT8> values;
        for(const Slot& s:c.slots){UINT8 value[2];
            if(std::fread(value,1,s.width,c.stream)!=s.width){
                clearInputs();c.mode=3;report("REFUSED: truncated frame");return;
            }
            values.push_back(value[0]);if(s.width==2)values.push_back(value[1]);
        }
        size_t offset=0;for(const Slot& s:c.slots){
            for(unsigned j=0;j<s.width;j++)s.address[j]=values[offset++];
        }
        ++c.index;
        if(c.index==1)report("PLAY_FRAME_1 injected");
    }
}
// F9 toggles recording from a live game. The state anchor is captured
// on the next *real* input frame, never in a speculative/runahead frame.
static bool toggleF9(bool activeGame){
    Context& c=ctx();
    if(c.mode==2){report("F9_REFUSED: playback active");return false;}
    if(c.mode==1){
        if(c.stream){std::fflush(c.stream);std::fclose(c.stream);c.stream=nullptr;}
        std::fprintf(stderr,"XM87.132 RECORD_STOP frames=%u state=%s inputs=%s\n",
            c.index,c.statePath.c_str(),c.dataPath.c_str());
        std::fflush(stderr);
        c.hotkeyRequest=false;c.mode=0;c.prepared=true;c.slots.clear();
        return true;
    }
    if(!activeGame){report("F9_REFUSED: no active game");return false;}
    if(c.stream){std::fclose(c.stream);c.stream=nullptr;}
    c.hotkeyRequest=true;c.mode=0;c.prepared=false;c.index=0;c.slots.clear();
    report("F9_RECORD_ARMED: next emulated frame will save state and begin inputs");
    return true;
}
static int recordingFrames(){
    const Context& c=ctx();
    return (c.hotkeyRequest && (c.mode==1 || !c.prepared)) ? int(c.index)+1 : 0;
}
} // namespace XM8761
#endif
