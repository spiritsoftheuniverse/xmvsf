#ifndef XMVSF_STAGE87_159_PROJECTILE_CUTOFF_H
#define XMVSF_STAGE87_159_PROJECTILE_CUTOFF_H
// Stage 87.159: DHC-only OFFENSIVE collision cutoff for the outgoing
// Ryu Shinkuu Hadoken's independently linked P1 projectile objects.
//
// Native CPS2 linked projectile list: FF[E3D8 - slot*4], slots 1..12.
// These are the same original ROM object links already decoded by the
// established XM23 Training hitbox renderer. They are NOT fighter slots.
// For each projectile, +70/+72 are offensive box IDs, +82 attack gate.
// Disable ONLY these three fields for objects that existed before Ken's
// point/reserve swap, not the P1 owner list, sprites, motion, hurtboxes,
// Ken's later new projectiles, any P2 object, or a normal unqueued super.
//
// The game's ROM may renew the attack IDs each frame: apply before the
// 68000 CPU and again after its frame. Check native collision in gameplay.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87159Projectile {
static inline unsigned b(const std::uint8_t* ram,unsigned offset) {
    return ram[(offset & 65535u)^1u];
}
static inline unsigned w(const std::uint8_t* ram,unsigned offset) {
    return (b(ram,offset)<<8u)|b(ram,offset+1u);
}
static inline unsigned l(const std::uint8_t* ram,unsigned offset) {
    return (w(ram,offset)<<16u)|w(ram,offset+2u);
}
static inline void setB(std::uint8_t* ram,unsigned offset,unsigned value) {
    ram[(offset & 65535u)^1u]=std::uint8_t(value);
}
static inline bool valid(unsigned base) {
    return base>=0xFF5000u && base<=0xFFFF00u;
}
struct State {
    bool armed=false,cutoff=false,freeze=false;
    unsigned captured[12]={},count=0,acquired=0,suppressed=0;
    unsigned lastReportedFrame=~0u;
    std::FILE* log=nullptr;
    bool triedLog=false;
    ~State(){if(log)std::fclose(log);}
    void openLog(){
        if(triedLog)return;
        triedLog=true;
        const char* path=std::getenv("XM87159_PROJECTILE_LOG");
        if(path&&*path){
            log=std::fopen(path,"wb");
            if(log)std::fputs(
                "frame\tevent\ttracked\tbase\tattack70_before\tattack72_before\tgate82_before\n",log);
        }
    }
    void note(unsigned now,const char* what,unsigned base,
              unsigned a=0,unsigned d=0,unsigned gate=0){
        openLog();
        if(log){
            std::fprintf(log,"%u\t%s\t%u\t%08X\t%04X\t%04X\t%02X\n",
                         now,what,count,base,a,d,gate);
            std::fflush(log);
        }
    }
    void reset(){
        armed=cutoff=freeze=false;count=acquired=suppressed=0;
        for(unsigned i=0;i<12u;++i)captured[i]=0;
    }
    // P1 projectile identity is a separate linked native object, not
    // the owner fighter's active action state or script timer.
    static unsigned collect(const std::uint8_t* ram,unsigned* out) {
        if(!ram||b(ram,0x5000u)!=8u)return 0u;
        unsigned count=0;
        for(unsigned slot=1;slot<=12u;++slot){
            const unsigned link=0xFFE3D8u-slot*4u;
            const unsigned base=l(ram,link);
            if(!valid(base))break;
            bool duplicate=false;
            for(unsigned i=0;i<count;++i)duplicate|=out[i]==base;
            if(!duplicate)out[count++]=base;
        }
        return count;
    }
    bool contains(unsigned base)const{
        for(unsigned i=0;i<count;++i)if(captured[i]==base)return true;
        return false;
    }
    void scan(std::uint8_t* ram,unsigned now,bool acquireNew){
        if(!armed||!ram)return;
        unsigned live[12]={};
        const unsigned liveN=collect(ram,live);
        // Drop objects immediately when their ROM list entry disappears.
        // A new Ken projectile reusing an address after removal must not
        // inherit Ryu's old-offense cutoff.
        for(unsigned i=0;i<count;){
            bool stillPresent=false;
            for(unsigned j=0;j<liveN;++j)stillPresent|=live[j]==captured[i];
            if(!stillPresent){
                note(now,"OLD_PROJECTILE_LINK_GONE",captured[i]);
                for(unsigned k=i+1;k<count;++k)captured[k-1]=captured[k];
                captured[--count]=0u;
            }else ++i;
        }
        if(acquireNew && !freeze){
            for(unsigned i=0;i<liveN && count<12u;++i){
                if(contains(live[i]))continue;
                captured[count++]=live[i];
                ++acquired;
                note(now,"OUTGOING_RYU_PROJECTILE_CAPTURED",live[i],
                     w(ram,live[i]+0x70u),w(ram,live[i]+0x72u),
                     b(ram,live[i]+0x82u));
            }
        }
    }
    void arm(std::uint8_t* ram,unsigned now){
        reset();armed=ram && b(ram,0x5000u)==8u;
        if(!armed)return;
        note(now,"DHC_OUTGOING_RYU_LINK_SNAPSHOT",0u);
        scan(ram,now,true);
    }
    void activate(std::uint8_t* ram,unsigned now){
        if(!armed)return;
        scan(ram,now,true);
        cutoff=true;
        note(now,"DHC_P1_OLD_PROJECTILE_HITBOX_DISABLED",0u);
        suppress(ram,now);
    }
    void swapped(unsigned now){
        if(!armed)return;
        freeze=true;
        note(now,"KEN_POINT_SWAP_FREEZE_RYU_LINK_SET",0u);
    }
    void abort(unsigned now,bool kenPoint){
        if(!armed)return;
        if(kenPoint){
            freeze=true;
            note(now,"KEEP_CUTOFF_AFTER_KEN_SWAP",0u);
        }else{
            note(now,"DISCARD_CUTOFF_FAILED_TAG",0u);
            reset();
        }
    }
    // This call happens before CPU dispatch, so stale Ryu projectile
    // attack descriptors cannot contribute to the next ROM collision pass.
    void beforeCPU(std::uint8_t* ram,unsigned now){
        if(!armed||!cutoff||!ram)return;
        if(b(ram,0x5000u)!=8u){reset();return;}
        scan(ram,now,false);
        suppress(ram,now);
    }
    // Re-gather during the native Ryu handoff only. Once Ken is point,
    // freeze this list; his new projectiles keep their full attack boxes.
    void afterCPU(std::uint8_t* ram,unsigned now,bool ryuStillPoint){
        if(!armed||!ram)return;
        if(b(ram,0x5000u)!=8u){reset();return;}
        scan(ram,now,ryuStillPoint && !freeze);
        if(cutoff)suppress(ram,now);
        if(freeze && count==0u){
            note(now,"ALL_OLD_RYU_PROJECTILES_EXPIRED",0u);
            reset();
        }
    }
    void suppress(std::uint8_t* ram,unsigned now){
        if(!cutoff||!ram)return;
        for(unsigned i=0;i<count;++i){
            const unsigned base=captured[i];
            if(!valid(base))continue;
            const unsigned attackA=w(ram,base+0x70u);
            const unsigned attackB=w(ram,base+0x72u);
            const unsigned gate=b(ram,base+0x82u);
            if(!attackA&&!attackB&&!gate)continue;
            setB(ram,base+0x70u,0u);setB(ram,base+0x71u,0u);
            setB(ram,base+0x72u,0u);setB(ram,base+0x73u,0u);
            setB(ram,base+0x82u,0u);
            ++suppressed;
            // Log first suppression and any later native box reassertion.
            note(now,"OLD_RYU_OFFENSE_CLEARED",base,attackA,attackB,gate);
        }
    }
};
} // namespace XM87159Projectile
#endif
