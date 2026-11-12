#ifndef XMVSF_STAGE87_102_TRAINING_LAB_H
#define XMVSF_STAGE87_102_TRAINING_LAB_H
// XM87.102: first-live-frame save state + read-only KO visual/audio provenance.
// Native KO sound command and yellow/orange KO artwork are NOT identified yet.
// Raw OAM + Z80 sound RAM + PCM samples permit source-based correlation.
#include "xmvsf_stage14_mode.h"
#include "burnint.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
extern int xm87102DamageMode; // 0 Regenerate; 1 KO Enabled
extern void XM44WriteBufferedSprites(FILE*,int); // original CPS-2 buffered OAM
namespace XM87102Lab {
static const char* SNAPSHOT="config/games/xmvsfu_training_match_87102.fs";
inline unsigned read8(cpu_core_config* c,unsigned a){return c->read(a)&255u;}
inline unsigned read16(cpu_core_config* c,unsigned a){return read8(c,a)<<8|read8(c,a+1u);}
inline unsigned read32(cpu_core_config* c,unsigned a){return (read16(c,a)<<16)|read16(c,a+2u);}
struct Capture {
    bool live=false,saved=false,tried=false,traceEnabled=false,started=false;
    unsigned frames=0,wasHp[2]={0,0},koFrame=0,postCombatFrames=0;
    FILE* positionLog=nullptr;
    FILE* cpuLog=nullptr;
    FILE* oamLog=nullptr;
    FILE* soundLog=nullptr;
    FILE* pcmLog=nullptr;
    std::string dir;
    ~Capture(){close();}
    void close(){
        if(positionLog){std::fclose(positionLog);positionLog=nullptr;}
        if(cpuLog){std::fclose(cpuLog);cpuLog=nullptr;}
        if(oamLog){std::fclose(oamLog);oamLog=nullptr;}
        if(soundLog){std::fclose(soundLog);soundLog=nullptr;}
        if(pcmLog){std::fclose(pcmLog);pcmLog=nullptr;}
        started=false;
    }
    static FILE* open(const std::string& name,const char* heading){
        FILE* f=std::fopen(name.c_str(),"ab");
        if(f){std::fseek(f,0,SEEK_END);if(std::ftell(f)==0)std::fputs(heading,f);std::fflush(f);}
        else std::fprintf(stderr,"XM87.102 KO_TRACE_OPEN_FAILED %s\n",name.c_str());
        return f;
    }
    void beginTrace(){
        if(started)return;
        started=true;
        const char* p=std::getenv("XM87102_KO_TRACE_DIR");
        if(!p||!*p)return;
        dir=p;
        traceEnabled=true;
        const char slash=(dir.find('\\')!=std::string::npos)?'\\':'/';
        if(dir.back()!='\\'&&dir.back()!='/')dir+=slash;
        positionLog=open(dir+"xm87109_training_positions.tsv",
            "frame\tphase\tteam\tslot\tid_raw\tx_u16\ty_raw_u16\tvx_raw_u32\tvy_raw_u32\taction\n");
        cpuLog=open(dir+"xm87102_ko_fighter.tsv",
            "frame\tdamage_mode\tphase\tp1_hp\tp2_hp\tp1_action\tp2_action\tp1_reaction\tp2_reaction\tko_onset\n");
        oamLog=open(dir+"xm87102_ko_oam.tsv",
            "frame\tobject_slot\tx\ty\ttile\tattribute\tpriority\tpalette\twidth\theight\n");
        soundLog=open(dir+"xm87102_ko_z80.tsv","frame\tf000_f03f_hex\n");
        pcmLog=open(dir+"xm87102_ko_pcm.tsv","frame\tpeak\tmean_abs\n");
    }
    void reset(){
        live=false;saved=false;tried=false;frames=0;wasHp[0]=wasHp[1]=0;koFrame=0;postCombatFrames=0;
        close();traceEnabled=false;dir.clear();
    }
    void pcm(const INT16* data,unsigned samples,unsigned frame){
        if(xm87102DamageMode!=1||!pcmLog||!data||samples==0)return;
        unsigned peak=0;unsigned long long sum=0;
        for(unsigned i=0;i<samples;++i){
            const int n=(int)data[i];const unsigned v=(unsigned)(n<0?-n:n);
            peak=std::max(peak,v);sum+=v;
        }
        std::fprintf(pcmLog,"%u\t%u\t%llu\n",frame,peak,sum/samples);
        if((frame%60u)==0u)std::fflush(pcmLog);
    }
    void record(unsigned frame,unsigned phase,unsigned hp0,unsigned hp1,
                unsigned a0,unsigned a1,unsigned r0,unsigned r1,bool ko){
        // Original CPS-2 buffered objects and QSound RAM, read-only.
        if(traceEnabled && xm87102DamageMode==1){
            if(cpuLog){
                std::fprintf(cpuLog,"%u\t%d\t%u\t%u\t%u\t%04X\t%04X\t%04X\t%04X\t%u\n",
                    frame,xm87102DamageMode,phase,hp0,hp1,a0,a1,r0,r1,unsigned(ko));
                if((frames%60u)==0u)std::fflush(cpuLog);
            }
            if(oamLog){XM44WriteBufferedSprites(oamLog,(int)frame);if((frames%60u)==0u)std::fflush(oamLog);}
            if(soundLog){
                cpu_core_config* sound=GetCpuCoreConfig(1);
                if(sound&&sound->read&&sound->open&&sound->close){
                    sound->open(0);
                    std::fprintf(soundLog,"%u\t",frame);
                    for(unsigned a=0xF000u;a<=0xF03Fu;++a)
                        std::fprintf(soundLog,"%02X",read8(sound,a));
                    std::fputc('\n',soundLog);sound->close();
                    if((frames%60u)==0u)std::fflush(soundLog);
                }
            }
        }
    }
    void frame(bool ready,unsigned frame){
        const char* driver=BurnDrvGetTextA(DRV_NAME);
        if(!xm14TrainingMode||!ready||bBurnRunAheadFrame||!driver||std::strcmp(driver,"xmvsfu")!=0){
            if(!xm14TrainingMode)reset();
            return;
        }
        cpu_core_config* c=GetCpuCoreConfig(0);
        if(!c||!c->open||!c->close||!c->read)return;
        c->open(0);
        const unsigned phase=read8(c,0xFF5000u);
        const unsigned ptr=read32(c,0xFFA014u);
        const bool now=phase==8u&&ptr==0x00FF4000u;
        const unsigned hp0=read16(c,0xFF4210u),hp1=read16(c,0xFF4610u);
        const unsigned bases109[4]={0xFF4000u,0xFF4800u,0xFF4400u,0xFF4C00u};
        unsigned raw109[4][6]={};
        for(unsigned i=0;i<4u;++i){
            const unsigned a=bases109[i];
            raw109[i][0]=read16(c,a);
            raw109[i][1]=read16(c,a+0x0Cu); // confirmed native X
            raw109[i][2]=read16(c,a+0x10u); // raw Y candidate
            raw109[i][3]=read32(c,a+0x18u); // raw motion field
            raw109[i][4]=read32(c,a+0x1Cu); // raw vertical impulse
            raw109[i][5]=read16(c,a+0x60u); // action
        }
        const unsigned a0=read16(c,0xFF4060u),a1=read16(c,0xFF4460u);
        const unsigned r0=read16(c,0xFF4006u),r1=read16(c,0xFF4406u);
        c->close();
        if(!now){
            if(live){
                ++postCombatFrames;
                // The KO typography and QSound effects can outlive phase 08.
                // Continue tracking both after combat exits for up to ten seconds.
                if(postCombatFrames<=600u){
                    record(frame,phase,hp0,hp1,a0,a1,r0,r1,false);
                }else{
                    std::fprintf(stderr,"XM87.102 KO_POSTCOMBAT_TRACE_END frame=%u\n",frame);
                    reset();
                }
            }
            return;
        }
        if(live && postCombatFrames>=10u){
            // A distinct Training round has started. Snapshot its real first frame.
            reset();
        }
        postCombatFrames=0;
        if(!live){
            live=true;frames=0;wasHp[0]=hp0;wasHp[1]=hp1;
            beginTrace();
            std::fprintf(stderr,"XM87.109 TRAINING_POSITION_CAPTURE frame=%u enabled=%u\n",
                frame,unsigned(positionLog!=nullptr));
            // Stage16's selection snapshot remains a separate file.
            tried=true;
            const int rc=BurnStateSave((TCHAR*)SNAPSHOT,1);
            saved=(rc==0);
            std::fprintf(stderr,"XM87.102 TRAINING_MATCH_START_STATE frame=%u save=%s path=%s\n",
                frame,saved?"OK":"FAILED",SNAPSHOT);
        }
        ++frames;
        if(positionLog){
            for(unsigned i=0;i<4u;++i)
                std::fprintf(positionLog,"%u\t%u\t%u\t%u\t%04X\t%u\t%u\t%08X\t%08X\t%04X\n",
                    frame,phase,(i<2u)?1u:2u,(i&1u)+1u,raw109[i][0],raw109[i][1],
                    raw109[i][2],raw109[i][3],raw109[i][4],raw109[i][5]);
            if((frames%60u)==0u)std::fflush(positionLog);
        }
        const bool ko=(xm87102DamageMode==1 &&
              ((wasHp[0]>0 && hp0==0)||(wasHp[1]>0&&hp1==0)));
        if(ko&&!koFrame){koFrame=frame;std::fprintf(stderr,"XM87.102 KO_CANDIDATE frame=%u P1=%u P2=%u\n",frame,hp0,hp1);}
        wasHp[0]=hp0;wasHp[1]=hp1;
        if(frames<=12000u || (koFrame && frame-koFrame<=240u))
            record(frame,phase,hp0,hp1,a0,a1,r0,r1,ko);
    }
};
inline Capture& capture(){static Capture c;return c;}
} // namespace XM87102Lab
#endif
