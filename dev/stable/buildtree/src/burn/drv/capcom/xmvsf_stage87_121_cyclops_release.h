#ifndef XMVSF_STAGE87_121_CYCLOPS_RELEASE_H
#define XMVSF_STAGE87_121_CYCLOPS_RELEASE_H
// XM87.121 CYCLOPS_THROW_RECOVERY_RELEASE
// Resume Cyclops's ACTUAL preceding airborne controller after air throw impact.
// No fresh super-jump launch, no arbitrary animation/script/ROM address.
// Never modify the opponent, world X/Y, HP, meter or character identity.
// Only allowed on 200C -> 200E, while victim has landed in 1C34.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87121 {
static inline unsigned b(const std::uint8_t* r,unsigned addr){return r[(addr&0xFFFFu)^1u];}
static inline unsigned w(const std::uint8_t* r,unsigned addr){return (b(r,addr)<<8)|b(r,addr+1);}
static inline std::uint32_t d(const std::uint8_t* r,unsigned a){return (std::uint32_t(w(r,a))<<16)|w(r,a+2);}
static inline void sb(std::uint8_t* r,unsigned a,unsigned v){r[(a&0xFFFFu)^1u]=std::uint8_t(v);}
static inline void sw(std::uint8_t* r,unsigned a,unsigned v){sb(r,a,v>>8);sb(r,a+1u,v);}
static inline void sd(std::uint8_t* r,unsigned a,std::uint32_t v){sw(r,a,v>>16);sw(r,a+2u,v);}
struct AirState {
    unsigned action,frame;
    // Original airborne action's live animation/physics/control values.
    std::uint32_t velocityX,motionX,velocityY,gravity,script,scriptNext;
    unsigned step06,step08,ctl3C,ctl3E,anim42,ctl44,ctl4C;
    unsigned step64,step66,ctl82,modeA0,ctlA4,flagAA,ctlD2,ctlD4;
    bool valid;
    AirState():action(0),frame(0),velocityX(0),motionX(0),velocityY(0),gravity(0),
      script(0),scriptNext(0),step06(0),step08(0),ctl3C(0),ctl3E(0),anim42(0),
      ctl44(0),ctl4C(0),step64(0),step66(0),ctl82(0),modeA0(0),ctlA4(0),
      flagAA(0),ctlD2(0),ctlD4(0),valid(false){}
};
static inline AirState observeAir(const std::uint8_t* r,unsigned p,unsigned f){
    AirState s;
    s.action=w(r,p+0x60);s.frame=f;
    s.velocityX=d(r,p+0x14);s.motionX=d(r,p+0x18);
    s.velocityY=d(r,p+0x1C);s.gravity=d(r,p+0x20);
    s.script=d(r,p+0x34);s.scriptNext=d(r,p+0x38);
    s.step06=w(r,p+0x06);s.step08=w(r,p+0x08);
    s.ctl3C=w(r,p+0x3C);s.ctl3E=w(r,p+0x3E);
    s.anim42=w(r,p+0x42);s.ctl44=w(r,p+0x44);s.ctl4C=w(r,p+0x4C);
    s.step64=w(r,p+0x64);s.step66=w(r,p+0x66);s.ctl82=w(r,p+0x82);
    s.modeA0=w(r,p+0xA0);s.ctlA4=w(r,p+0xA4);s.flagAA=w(r,p+0xAA);
    s.ctlD2=w(r,p+0xD2);s.ctlD4=w(r,p+0xD4);
    s.valid=true;return s;
}
static inline void restoreAir(std::uint8_t* r,unsigned p,const AirState& s){
    // Restore the EXACT preceding airborne controller; do not start a jump.
    sw(r,p+0x06,s.step06);sw(r,p+0x08,s.step08);
    sd(r,p+0x14,s.velocityX);sd(r,p+0x18,s.motionX);
    sd(r,p+0x1C,s.velocityY);sd(r,p+0x20,s.gravity);
    sd(r,p+0x34,s.script);sd(r,p+0x38,s.scriptNext);
    sw(r,p+0x3C,s.ctl3C);sw(r,p+0x3E,s.ctl3E);
    sw(r,p+0x42,s.anim42);sw(r,p+0x44,s.ctl44);sw(r,p+0x4C,s.ctl4C);
    sw(r,p+0x60,s.action);sw(r,p+0x64,s.step64);sw(r,p+0x66,s.step66);
    sw(r,p+0x82,s.ctl82);sw(r,p+0xA0,s.modeA0);sw(r,p+0xA4,s.ctlA4);
    sw(r,p+0xAA,s.flagAA);sw(r,p+0xD2,s.ctlD2);sw(r,p+0xD4,s.ctlD4);
    // World X/Y (+0C/+10), fighter ID, health, super meter, enemy untouched.
}
struct Tracker{
    bool initialized,enabled,valid[2],armed[2],pending[2];
    unsigned lastFrame,previous[2],until[2];
    AirState lastAir[2],selected[2];
    std::FILE* log;
    Tracker():initialized(false),enabled(false),lastFrame(~0u),log(NULL){
        for(unsigned i=0;i<2;++i){valid[i]=armed[i]=pending[i]=false;previous[i]=until[i]=0;}
    }
    ~Tracker(){if(log)std::fclose(log);}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* on=std::getenv("XM87121_CYCLOPS_RELEASE");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM87121_CYCLOPS_LOG_FILE");
        if(path && *path){log=std::fopen(path,"wb");if(log)
          std::fputs("frame\tplayer\tphase\taction\tmode\tairflag\tstep06\tstep08\tx\ty\tvy\topponent_action\topponent_y\tdecision\n",log);}
        std::fprintf(stderr,"XM87.121 CYCLOPS_THROW_RECOVERY_RELEASE enabled=%d\n",int(enabled));
    }
    void reset(){for(unsigned i=0;i<2;++i){valid[i]=armed[i]=pending[i]=false;lastAir[i].valid=false;until[i]=0;}}
    void emit(unsigned f,unsigned i,const char* phase,const std::uint8_t* r,
              unsigned p,unsigned other,const char* decision){
        if(!log)return;
        std::fprintf(log,"%u\t%u\t%s\t%04X\t%02X\t%u\t%04X\t%04X\t%u\t%u\t%08X\t%04X\t%u\t%s\n",
          f,i+1u,phase,w(r,p+0x60u),w(r,p+0xA0u)>>8,w(r,p+0xAAu)>>8,
          w(r,p+0x06u),w(r,p+0x08u),w(r,p+0x0Cu),w(r,p+0x10u),d(r,p+0x1Cu),
          w(r,other+0x60u),w(r,other+0x10u),decision);
        std::fflush(log);
    }
    void frame(unsigned f,std::uint8_t* r){
        init();if(!r||lastFrame==f)return;lastFrame=f;
        if(b(r,0x5000u)!=8u){reset();return;}
        for(unsigned i=0;i<2u;++i){
            unsigned p=i?0x4400u:0x4000u,other=i?0x4000u:0x4400u;
            unsigned action=w(r,p+0x60u);
            bool cyclops=w(r,p+0x40u)==0x0021u && w(r,p)==0x0101u;
            if(!cyclops){valid[i]=armed[i]=pending[i]=false;lastAir[i].valid=false;continue;}
            // ROM trace: 0200 / 0202, mode 1C, airborne flag 1 (rising) / 2 (descending).
            bool inJump=(action==0x0200u || action==0x0202u) &&
                (w(r,p+0xA0u)>>8)==0x1Cu &&
                ((w(r,p+0xAAu)>>8)==1u || (w(r,p+0xAAu)>>8)==2u) &&
                w(r,p+0x10u)<472u;
            if(inJump)lastAir[i]=observeAir(r,p,f);
            if(valid[i] && action==0x200Cu && previous[i]!=0x200Cu){
                armed[i]=lastAir[i].valid && f>=lastAir[i].frame && f-lastAir[i].frame<=2u;
                if(armed[i])selected[i]=lastAir[i];
                emit(f,i,"THROW_CAPTURE",r,p,other,armed[i]?"AIRBORNE_SAVED":"NO_AIRBORNE_BASELINE");
            }
            bool firstFlip=valid[i] && previous[i]==0x200Cu && action==0x200Eu &&
                w(r,p+0x06u)==4u && w(r,p+0x08u)==6u &&
                (w(r,p+0xA0u)>>8)==0x32u &&
                (w(r,p+0xAAu)>>8)==2u && w(r,p+0x10u)<472u;
            if(firstFlip){
                bool grounded=w(r,other+0x10u)>=472u && w(r,other+0x60u)==0x1C34u;
                emit(f,i,"FLIP_BEFORE",r,p,other,!grounded?"VICTIM_NOT_GROUNDED":(armed[i]?"RELEASE_READY":"NO_AIRBORNE_BASELINE"));
                if(enabled && grounded && armed[i]){
                    restoreAir(r,p,selected[i]);
                    emit(f,i,"RELEASED",r,p,other,"RESTORED_PRE_THROW_AIRBORNE");
                    pending[i]=true;until[i]=f+16u;
                }
                armed[i]=false;
            } else if(pending[i]){
                emit(f,i,"FOLLOW",r,p,other,
                   (w(r,p+0xA0u)>>8)==0x1Cu?"AIRBORNE_CONTROL":"OTHER_CONTROL");
                if(f>=until[i] || w(r,p+0x10u)>=472u)pending[i]=false;
            }
            if(action!=0x200Cu && action!=0x200Eu && !inJump && !pending[i])armed[i]=false;
            previous[i]=w(r,p+0x60u);valid[i]=true;
        }
    }
};
static inline Tracker& state(){static Tracker x;return x;}
}
#endif
