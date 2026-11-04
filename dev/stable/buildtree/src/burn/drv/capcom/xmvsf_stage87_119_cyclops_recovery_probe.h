#ifndef XMVSF_STAGE87_119_CYCLOPS_RECOVERY_PROBE_H
#define XMVSF_STAGE87_119_CYCLOPS_RECOVERY_PROBE_H
// XM87.119 CYCLOPS_RECOVERY_PROBE
// READ ONLY: observe the ROM's native superjump initialization and Cyclops air throw.
// Source RAM is word-swapped CPS2 CpsRamFF. This does not write RAM or ROM.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace XM87119 {
static inline unsigned b(const std::uint8_t* r,unsigned addr) { return r[(addr & 0xFFFFu)^1u]; }
static inline unsigned w(const std::uint8_t* r,unsigned addr) { return (b(r,addr)<<8)|b(r,addr+1u); }
static inline std::uint32_t d(const std::uint8_t* r,unsigned addr) {
    return (std::uint32_t(w(r,addr))<<16)|w(r,addr+2u);
}
struct Fighter {
    unsigned id,bank,action,mode,superMode,step06,step08,x,y,anim42,profileE0;
    std::uint32_t vx14,motion18,vy1c;
    Fighter():id(0),bank(0),action(0),mode(0),superMode(0),step06(0),step08(0),
       x(0),y(0),anim42(0),profileE0(0),vx14(0),motion18(0),vy1c(0){}
};
static inline Fighter read(const std::uint8_t* r,unsigned p){
    Fighter x;
    x.id=w(r,p+0x00u);x.step06=w(r,p+0x06u);x.step08=w(r,p+0x08u);
    x.x=w(r,p+0x0Cu);x.y=w(r,p+0x10u);
    x.vx14=d(r,p+0x14u);x.motion18=d(r,p+0x18u);x.vy1c=d(r,p+0x1Cu);
    x.bank=w(r,p+0x40u);x.anim42=w(r,p+0x42u);
    x.action=w(r,p+0x60u);
    // Same ROM fields as established 87.47 movement cues, in CPU-order words.
    x.mode=w(r,p+0xA0u)>>8;x.superMode=w(r,p+0xAAu)>>8;
    x.profileE0=w(r,p+0xE0u);
    return x;
}
static inline const char* event(const Fighter& prev,const Fighter& now,bool valid,
                                const Fighter& other){
    if(!valid)return "FIRST_SAMPLE";
    if(now.action==0x0600u && prev.action!=now.action)return "SJ_PREP_0600";
    if(now.action==0x0604u && prev.action!=now.action)return "SJ_PREP_0604";
    if(!prev.superMode && now.superMode &&
      (now.mode==0x1Au||now.mode==0x1Cu))return "NATIVE_SUPERJUMP_ENTRY";
    if(now.action==0x200Cu && prev.action!=now.action)return "AIR_THROW_CAPTURE";
    if(prev.action==0x200Cu && now.action==0x200Eu && other.y>=472u)
       return "POST_THROW_FLIP_AFTER_IMPACT";
    if(prev.action==0x200Eu && now.action!=prev.action)return "POST_THROW_RECOVERY_EXIT";
    if(prev.superMode && !now.superMode)return "SUPERJUMP_END";
    if(prev.action!=now.action)return "ACTION_CHANGE";
    return "FRAME";
}
struct Tracker {
    bool initialized,valid[2]; unsigned lastFrame; Fighter previous[2];
    std::FILE *log,*snap;
    Tracker():initialized(false),lastFrame(~0u),log(NULL),snap(NULL){valid[0]=valid[1]=false;}
    ~Tracker(){if(log)std::fclose(log);if(snap)std::fclose(snap);}
    void openOnce(){
        if(initialized)return;
        initialized=true;
        const char *file=std::getenv("XM87119_PROBE_FILE");
        const char *full=std::getenv("XM87119_PROBE_SNAPSHOT_FILE");
        if(!file || !*file)return; // explicitly enabled by 87.119 runner
        log=std::fopen(file,"wb");
        if(full && *full)snap=std::fopen(full,"wb");
        if(log){
            std::fputs("frame\tplayer\tclassification\tid\tbank40\taction60\tmodeA0\tsuperAA\tstep06\tstep08\tx0C\ty10\tvelocity14\tmotion18\tvelocity1C\tanim42\tprofileE0\tother_action\tother_y\n",log);
            std::fprintf(stderr,"XM87.119 CYCLOPS_RECOVERY_PROBE enabled read-only log=%s\n",file);
        }else std::fprintf(stderr,"XM87.119 CYCLOPS_RECOVERY_PROBE could not open log=%s\n",file);
        if(snap)std::fputs("frame\tplayer\tevent\tcpu_order_0x000_to_0x0ff\n",snap);
    }
    void reset(){valid[0]=valid[1]=false;}
    void frame(unsigned frame,const std::uint8_t* raw){
        openOnce();
        if(!raw||!log||lastFrame==frame)return;
        lastFrame=frame;
        const unsigned phase=b(raw,0x5000u);
        if(phase!=8u){reset();return;}
        const Fighter now[2]={read(raw,0x4000u),read(raw,0x4400u)};
        for(unsigned p=0;p<2u;++p){
            if(!now[p].id){valid[p]=false;continue;}
            const char* ev=event(previous[p],now[p],valid[p],now[1u-p]);
            const Fighter& s=now[p],&other=now[1u-p];
            std::fprintf(log,"%u\t%u\t%s\t%04X\t%04X\t%04X\t%02X\t%u\t%04X\t%04X\t%u\t%u\t%08X\t%08X\t%08X\t%04X\t%04X\t%04X\t%u\n",
                frame,p+1u,ev,s.id,s.bank,s.action,s.mode,s.superMode,s.step06,s.step08,
                s.x,s.y,s.vx14,s.motion18,s.vy1c,s.anim42,s.profileE0,other.action,other.y);
            if(snap && ev[0]!='F' && std::strcmp(ev,"ACTION_CHANGE")!=0 && std::strcmp(ev,"SUPERJUMP_END")!=0){
                std::fprintf(snap,"%u\t%u\t%s\t",frame,p+1u,ev);
                for(unsigned offset=0;offset<0x100u;++offset)
                  std::fprintf(snap,"%02X",b(raw,(p?0x4400u:0x4000u)+offset));
                std::fputc('\n',snap);
            }
            previous[p]=s;valid[p]=true;
        }
        if((frame%30u)==0u){std::fflush(log);if(snap)std::fflush(snap);}
    }
};
static inline Tracker& state(){static Tracker t;return t;}
} // XM87119
#endif
