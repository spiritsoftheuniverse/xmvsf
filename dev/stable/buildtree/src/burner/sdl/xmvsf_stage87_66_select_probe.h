#ifndef XMVSF_STAGE87_66_SELECT_PROBE_H
#define XMVSF_STAGE87_66_SELECT_PROBE_H
// Stage87.67 verified selection timer fix; 87.66 trace formats unchanged.
// Training select directional/button input evidence + raw RAM timer survey.
// Only Training selection may write verified address FF515E; opt-in through
// XM8766_SELECT_HOLD=1. No combat or normal Stable changes.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
namespace XM8766 {
struct CounterTrack {
    unsigned char prev, step;
    unsigned short frames;
    CounterTrack():prev(0xff),step(0),frames(0){}
    // Identify a slow, decimally coded select timer; no writes here.
    bool Next(unsigned char v){
        if(v==prev){if(frames<65000u)++frames;return false;}
        const unsigned old=prev, duration=frames;
        prev=v;frames=1;
        if(v==9u){step=1;return false;}
        if(step==1 && old==9u && v==8u && duration>=20u && duration<=240u){step=2;return false;}
        if(step==2 && old==8u && v==7u && duration>=20u && duration<=240u){step=3;return true;}
        step=0;return false;
    }
};
static inline void Put32(unsigned char*p,unsigned n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(8u*i));}
struct InputTrace {
    FILE*file;bool initialized;unsigned char previous[2][256];
    InputTrace():file(NULL),initialized(false){std::memset(previous,255,sizeof(previous));}
    ~InputTrace(){if(file)std::fclose(file);}
    void Init(){if(initialized)return;initialized=true;
        const char *dir=std::getenv("XM8766_SELECTION_TRACE");if(!dir||!*dir)return;
        char path[1200];std::snprintf(path,sizeof(path),"%s/xm8766_select_input.tsv",dir);
        file=std::fopen(path,"w");if(!file){std::fprintf(stderr,"XM87.66 SELECT_INPUT_OPEN_FAILED path=%s\n",path);return;}
        std::fprintf(file,"host_frame\tgame_slot\tinput_code\tphysical_source\tvalue\tselect_phase\towner\n");
        std::fprintf(stderr,"XM87.66 SELECT_INPUT_READY path=%s\n",path);
    }
    void Record(int slot,int code,int src,int v,unsigned f,unsigned phase,int owner){
        if(slot<0||slot>1||code<0||code>255||
           !(code<=3||(code>=0x80&&code<=0x87))||phase==0||phase==4)return;
        Init();if(!file)return;
        const unsigned char value=(unsigned char)(v!=0);
        if(previous[slot][code]==value)return;
        previous[slot][code]=value;
        std::fprintf(file,"%u\t%d\t%02X\t%d\t%d\t%u\t%d\n",f,slot,code,src,(int)value,phase,owner);
        std::fflush(file);
    }
};
static inline InputTrace& InputState(){static InputTrace s;return s;}
static inline void RecordInput(int slot,int code,int source,int delivered,unsigned frame,unsigned phase,int owner){
    InputState().Record(slot,code,source,delivered,frame,phase,owner);
}
struct TimerProbe {
    FILE*file;bool initialized;bool holdRequested;unsigned lockedOffset;
    unsigned foundOffset,foundFrame,foundCount;CounterTrack watches[4096];
    TimerProbe():file(NULL),initialized(false),holdRequested(false),lockedOffset(0xffffffffu),
        foundOffset(0xffffffffu),foundFrame(0),foundCount(0){}
    ~TimerProbe(){if(file)std::fclose(file);}
    void Reset(){ // Called after snapshot restoration; don't repeatedly reopen logs.
        for(unsigned i=0;i<4096;++i)watches[i]=CounterTrack();
        lockedOffset=foundOffset=0xffffffffu;foundFrame=foundCount=0;
    }
    void Init(){if(initialized)return;initialized=true;
        const char*dir=std::getenv("XM8766_SELECTION_TRACE");if(!dir||!*dir)return;
        holdRequested=std::getenv("XM8766_SELECT_HOLD")&&std::strcmp(std::getenv("XM8766_SELECT_HOLD"),"1")==0;
        char path[1200];std::snprintf(path,sizeof(path),"%s/xm8766_select_ram.bin",dir);
        file=std::fopen(path,"wb");if(!file){std::fprintf(stderr,"XM87.66 SELECT_RAM_OPEN_FAILED path=%s\n",path);return;}
        std::fwrite("XM8766R1",1,8,file);
        std::fprintf(stderr,"XM87.66 SELECT_RAM_READY hold=%d scope=FF5000-FF5FFF path=%s\n",holdRequested?1:0,path);
    }
    void Record(unsigned frame,unsigned phase,unsigned char *bytes){
        Init();if(!file)return;
        if(frame<=12000u){unsigned char header[12];Put32(header,GetCurrentFrame());Put32(header+4,phase);Put32(header+8,frame);
            std::fwrite(header,1,12,file);std::fwrite(bytes,1,4096,file);
            if(frame%120u==0)std::fflush(file);
        }
        // Stage87.67: Source capture has positively identified FF515E as the
        // 50-frame per tick selection countdown (20 -> 0), reset on picks.
        // The 65-frame FF5462 address that the speculative 09/08/07 survey
        // chose is NOT the selection timeout. Never nominate it for writes.

    }
};
static inline TimerProbe& TimerState(){static TimerProbe s;return s;}
static inline void SampleTimer(unsigned frame,unsigned phase){
    const char *dir=std::getenv("XM8766_SELECTION_TRACE");if(!dir||!*dir||phase==0u||phase==4u)return;
    cpu_core_config*cpu=GetCpuCoreConfig(0);
    if(!cpu||!cpu->open||!cpu->close||!cpu->read)return;
    cpu->open(0);
    const unsigned gamePhase=cpu->read(0xFF5000u)&255u;
    if(gamePhase>=8u){cpu->close();return;}
    unsigned char bytes[4096];
    for(unsigned i=0;i<4096;++i)bytes[i]=(unsigned char)(cpu->read(0xFF5000u+i)&255u);
    TimerProbe&s=TimerState();if(frame==1u)s.Reset();s.Record(frame,phase,bytes);
    // Stage87.67 Training SELECT ONLY. Captured P2 auto-pick occurs after
    // FF515E reaches 0 (50 host frames per decrement), even while FF5462
    // is held at nine. Freeze the actual ROM counter at 0x14 (20) BEFORE
    // its visible nine-count phase begins. Record() above logs pre-write RAM.
    const unsigned kSelectTimerOffset=0x15Eu;
    const unsigned kSelectTimerAddr=0xFF515Eu;
    if(s.holdRequested && cpu->write){
        const unsigned v=bytes[kSelectTimerOffset];
        if(v <= 0x14u){
            if(s.lockedOffset==0xffffffffu){
                s.lockedOffset=kSelectTimerOffset;
                std::fprintf(stderr,"XM87.67 SELECT_TIMER_HOLD_ACTIVE address=FF515E initial=%u target=20 selection_only=1\n",v);
            }
            // Do not write during combat, Versus, menus or other game modes.
            // SampleTimer is only invoked from Training's active selection
            // phases and has already verified that FF5000 < 8.
            cpu->write(kSelectTimerAddr,0x14u);
        } else {
            std::fprintf(stderr,"XM87.67 SELECT_TIMER_HOLD_ABORT address=FF515E unexpected=%u frame=%u\n",v,frame);
            s.holdRequested=false;
        }
    }
    cpu->close();
}
} // XM8766
#endif
