#ifndef XMVSF_STAGE87_120_CYCLOPS_CANCEL_H
#define XMVSF_STAGE87_120_CYCLOPS_CANCEL_H
// XM87.120 CYCLOPS_AIR_THROW_SUPER_JUMP
// Opt-in experimental native-state transition derived from 87.119 RAM snapshots.
// Only Cyclops (+0x40 bank 0021), only 200C -> 200E, only after other fighter grounded.
// Does not alter victim record, damage, collision, ROM, Stable, or unrelated fighters.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87120 {
static inline unsigned b(const std::uint8_t* r,unsigned addr){return r[(addr&0xFFFFu)^1u];}
static inline unsigned w(const std::uint8_t* r,unsigned addr){return (b(r,addr)<<8)|b(r,addr+1);}
static inline std::uint32_t d(const std::uint8_t* r,unsigned addr){return (std::uint32_t(w(r,addr))<<16)|w(r,addr+2);}
static inline void sb(std::uint8_t* r,unsigned a,unsigned v){r[(a&0xFFFFu)^1u]=std::uint8_t(v);}
static inline void sw(std::uint8_t* r,unsigned a,unsigned v){sb(r,a,v>>8);sb(r,a+1u,v);}
static inline void sd(std::uint8_t* r,unsigned a,std::uint32_t v){sw(r,a,v>>16);sw(r,a+2u,v);}
struct Tracker{
    bool initialized,enabled,valid[2],pending[2];
    unsigned lastFrame,previous[2],watchUntil[2];
    std::FILE* log;
    Tracker():initialized(false),enabled(false),lastFrame(~0u),log(NULL){
        for(unsigned i=0;i<2;++i){valid[i]=pending[i]=false;previous[i]=0;watchUntil[i]=0;}
    }
    ~Tracker(){if(log)std::fclose(log);}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* on=std::getenv("XM87120_CYCLOPS_CANCEL");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM87120_CYCLOPS_LOG_FILE");
        if(path && *path){log=std::fopen(path,"wb");if(log)
          std::fputs("frame\tplayer\tphase\taction\tmode\tsuper\tstep06\tstep08\tx\ty\tvy\tother_action\tother_y\tdecision\n",log);}
        std::fprintf(stderr,"XM87.120 CYCLOPS_AIR_THROW_SUPER_JUMP enabled=%d\n",int(enabled));
    }
    void reset(){for(unsigned i=0;i<2;++i){valid[i]=pending[i]=false;watchUntil[i]=0;}}
    void emit(unsigned frame,unsigned n,const char* phase,const std::uint8_t* r,unsigned p,
              unsigned other,const char* decision){
        if(!log)return;
        std::fprintf(log,"%u\t%u\t%s\t%04X\t%02X\t%u\t%04X\t%04X\t%u\t%u\t%08X\t%04X\t%u\t%s\n",
          frame,n,phase,w(r,p+0x60u),w(r,p+0xA0u)>>8,w(r,p+0xAAu)>>8,
          w(r,p+0x06u),w(r,p+0x08u),w(r,p+0x0Cu),w(r,p+0x10u),d(r,p+0x1Cu),
          w(r,other+0x60u),w(r,other+0x10u),decision);
        std::fflush(log);
    }
    // Native references: 87.119 P1 frames 1780/4301/5596 (020C, negative A2)
    // and 3048/3550/4681 (020E, non-negative A2). On transition both use
    // 1A28, +AA high byte 01, vertical FFF00000, gravity 00006400,
    // anim 8826, and pointer-pair +34/+38 matched to 020C/020E respectively.
    void nativeSuperJump(std::uint8_t* r,unsigned p){
        const bool variantC=(w(r,p+0xA2u)&0xFF00u)==0xFF00u;
        const std::uint32_t script=variantC?0x000A5F4Au:0x000A602Eu;
        sw(r,p+0x06u,0u);sw(r,p+0x08u,0u);
        sd(r,p+0x14u,0u);sd(r,p+0x18u,0u);
        sd(r,p+0x1Cu,0xFFF00000u);sd(r,p+0x20u,0x00006400u);
        sd(r,p+0x34u,script);sd(r,p+0x38u,script+0x10u);
        sw(r,p+0x3Cu,0x0100u);sw(r,p+0x3Eu,0x0003u);
        sw(r,p+0x42u,0x8826u);sw(r,p+0x44u,0x0080u);
        sw(r,p+0x4Cu,0x0002u);
        sw(r,p+0x60u,variantC?0x020Cu:0x020Eu);
        sw(r,p+0x64u,0u);sw(r,p+0x66u,0u);
        sw(r,p+0x82u,0x8080u);
        sw(r,p+0xA0u,0x1A28u);sw(r,p+0xA4u,0u);
        sw(r,p+0xAAu,0x0104u);
        sw(r,p+0xD2u,0x0080u);sw(r,p+0xD4u,0u);
        // Keep world X/Y, facing, fighter identity, HP, meter and opponent intact.
    }
    void frame(unsigned frame,std::uint8_t* r){
        init();if(!r||lastFrame==frame)return;
        lastFrame=frame;
        if(b(r,0x5000u)!=8u){reset();return;}
        for(unsigned i=0;i<2u;++i){
            const unsigned p=i?0x4400u:0x4000u,other=i?0x4000u:0x4400u;
            const unsigned action=w(r,p+0x60u);
            const bool cyclops=w(r,p+0x40u)==0x0021u && w(r,p)==0x0101u;
            const bool hitGround=w(r,other+0x10u)>=472u &&
              w(r,other+0x60u)==0x1C34u;
            const bool firstFlip=valid[i] && previous[i]==0x200Cu &&
              action==0x200Eu && w(r,p+0x06u)==4u && w(r,p+0x08u)==6u &&
              w(r,p+0x10u)<472u && (w(r,p+0xAAu)>>8)==0u;
            if(enabled && cyclops && firstFlip){
                emit(frame,i+1u,"BEFORE",r,p,other,hitGround?"GROUND_IMPACT":"NO_IMPACT");
                if(hitGround){
                    nativeSuperJump(r,p);
                    emit(frame,i+1u,"AFTER",r,p,other,"NATIVE_JUMP_TEMPLATE");
                    pending[i]=true;watchUntil[i]=frame+16u;
                }
            }else if(pending[i]){
                emit(frame,i+1u,"FOLLOW",r,p,other,
                    (w(r,p+0xAAu)>>8)?"SUPER_MODE_LIVE":"SUPER_MODE_LOST");
                if(frame>=watchUntil[i] || (w(r,p+0xAAu)>>8)==0u)pending[i]=false;
            }
            previous[i]=w(r,p+0x60u);valid[i]=true;
        }
    }
};
static inline Tracker& state(){static Tracker t;return t;}
}
#endif
