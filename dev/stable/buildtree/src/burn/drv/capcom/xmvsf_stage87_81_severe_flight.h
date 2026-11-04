#ifndef XMVSF_STAGE87_81_SEVERE_FLIGHT_H
#define XMVSF_STAGE87_81_SEVERE_FLIGHT_H
// Experimental: observed severe-vs-soft flying-screen RAM profiles.
// Do not treat a paired RAM profile as proof of the full ROM knockdown state.
// No ROM, animation pointer, damage, command, camera, or Stable modifications.
// Source: 2026-11-02 captured 8,566 combat frames (34 87.80 launch overrides).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8781Flight {
static inline unsigned byteAt(const std::uint8_t* r,unsigned p){return r[(p&0xffffu)^1u];}
static inline unsigned wordAt(const std::uint8_t* r,unsigned p){return (byteAt(r,p)<<8)|byteAt(r,p+1);}
static inline void setByte(std::uint8_t* r,unsigned p,unsigned v){r[(p&0xffffu)^1u]=std::uint8_t(v);}
static inline void setWord(std::uint8_t* r,unsigned p,unsigned v){setByte(r,p,v>>8);setByte(r,p+1,v);}
static inline std::uint32_t rawFixed(const std::uint8_t* r,unsigned p){return (std::uint32_t(wordAt(r,p))<<16)|wordAt(r,p+2);}
static inline std::int64_t signedFixed(std::uint32_t x){return (x&0x80000000u)?std::int64_t(x)-0x100000000LL:std::int64_t(x);}
static inline std::int64_t magnitude(std::int64_t v){return v<0?-v:v;}
static inline void setFixed(std::uint8_t* r,unsigned p,std::uint32_t v){setWord(r,p,v>>16);setWord(r,p+2,v);}
static const std::uint32_t TARGET_RIGHT=0x0010AA00u;
static const std::uint32_t TARGET_LEFT=0xFFEF5600u;
static const std::uint32_t TARGET_Y=0x0000B000u;
static const std::uint16_t SOFT_ACTION=0x1C16u;
static const std::uint16_t SEVERE_ACTION=0x1C18u;
static const std::uint16_t SOFT_PROFILE=0x0220u;
static const std::uint16_t SEVERE_PROFILE=0x0230u;
// Wolverine (observed character bank 0021), two Drill Claw candidate
// animation/action signatures. Both are protected until separately resolved.
// Do NOT exempt all 0x2Cxx actions or velocity classes.
static inline bool drillClawCandidate(unsigned actorBank,unsigned move){
    return actorBank==0x0021u && (move==0x2C1Eu || move==0x2C46u);
}
struct Decision {
    bool candidate,launch,exclude,velocityWritten,stateWritten;
    unsigned attackerBank,attackerAction,victimActionBefore,victimActionAfter;
    unsigned victimProfileBefore,victimProfileAfter,substate,reaction,y;
    std::uint32_t beforeX,beforeY,afterX,afterY;
    const char* outcome;
    Decision():candidate(false),launch(false),exclude(false),velocityWritten(false),stateWritten(false),
       attackerBank(0),attackerAction(0),victimActionBefore(0),victimActionAfter(0),
       victimProfileBefore(0),victimProfileAfter(0),substate(0),reaction(0),y(0),
       beforeX(0),beforeY(0),afterX(0),afterY(0),outcome("NO_LAUNCH"){}
};
struct Actor {
    bool observed,priorFlight;
    std::uint32_t previousX;
    Actor():observed(false),priorFlight(false),previousX(0){}
    Decision evaluate(std::uint8_t* r,unsigned victimBase,unsigned attackerBase,bool enabled,bool combat){
        Decision d;
        if(!r)return d;
        d.reaction=wordAt(r,victimBase+6u);
        d.substate=wordAt(r,victimBase+8u);
        d.y=wordAt(r,victimBase+0x10u);
        d.beforeX=rawFixed(r,victimBase+0x14u);
        d.beforeY=rawFixed(r,victimBase+0x1Cu);
        d.attackerBank=wordAt(r,attackerBase+0x40u);
        d.attackerAction=wordAt(r,attackerBase+0x60u);
        d.victimActionBefore=wordAt(r,victimBase+0x60u);
        d.victimProfileBefore=wordAt(r,victimBase+0xE0u);
        std::int64_t speed=signedFixed(d.beforeX);
        d.candidate=combat && d.reaction==0x000Cu && d.substate==2u &&
            d.y<472u && magnitude(speed)>=0x000C0000LL && magnitude(speed)<=0x00300000LL;
        const bool continuation=combat && d.reaction==0x000Cu && d.y<472u &&
            magnitude(speed)>=0x000C0000LL && magnitude(speed)<=0x00300000LL;
        std::int64_t last=signedFixed(previousX);
        bool changed=!observed || !priorFlight || ((speed<0)!=(last<0)) ||
                     magnitude(speed-last)>=0x00020000LL;
        d.launch=enabled && d.candidate && changed;
        d.exclude=d.launch && drillClawCandidate(d.attackerBank,d.attackerAction);
        d.afterX=d.beforeX;d.afterY=d.beforeY;
        if(d.launch){
            if(d.exclude){
                d.outcome="DRILL_CLAW_PROTECT";
            }else{
                d.afterX=(speed<0)?TARGET_LEFT:TARGET_RIGHT;
                d.afterY=TARGET_Y;
                setFixed(r,victimBase+0x14u,d.afterX);
                setFixed(r,victimBase+0x1Cu,d.afterY);
                d.velocityWritten=true;
                if(d.victimActionBefore==SOFT_ACTION && d.victimProfileBefore==SOFT_PROFILE){
                    // Paired state experiment: preserve native animation pointers;
                    // the resulting severe behavior requires live visual validation.
                    setWord(r,victimBase+0x60u,SEVERE_ACTION);
                    setWord(r,victimBase+0xE0u,SEVERE_PROFILE);
                    d.stateWritten=true;
                    d.outcome="SOFT_TO_SEVERE_PAIR";
                }else if(d.victimActionBefore==SEVERE_ACTION &&
                         d.victimProfileBefore==SEVERE_PROFILE){
                    d.outcome="NATIVE_SEVERE_PAIR";
                }else{
                    d.outcome="UNKNOWN_NATIVE_PAIR_UNTOUCHED";
                }
            }
        }
        d.victimActionAfter=wordAt(r,victimBase+0x60u);
        d.victimProfileAfter=wordAt(r,victimBase+0xE0u);
        // Preserve the wall substate excursion latch from 87.80. Crucially,
        // use ORIGINAL speed for subsequent comparisons on excluded moves.
        previousX=d.velocityWritten?d.afterX:d.beforeX;
        priorFlight=continuation;
        observed=true;
        if(!combat)priorFlight=false;
        return d;
    }
};
struct Controller {
    Actor actor[2];
    bool initialized,enabled;
    std::FILE* log;
    Controller():initialized(false),enabled(false),log(NULL){}
    ~Controller(){if(log)std::fclose(log);}
    void init(){
        initialized=true;
        const char* on=std::getenv("XM8781_SEVERE_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8781_SEVERE_LOG_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\tvelocity_written\tstate_pair_written\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.81 SEVERE_PAIR_READY enabled=%d experimental=1 drill_claw_guard=0021/2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r){
        if(!initialized)init();
        if(!r)return;
        const bool combat=byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(r,i?0x4400u:0x4000u,i?0x4000u:0x4400u,enabled,combat);
            if(!d.launch)continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%s\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.velocityWritten),unsigned(d.stateWritten),d.outcome);
                std::fflush(log);
            }
            std::fprintf(stderr,"XM87.81 FLIGHT frame=%u victim=P%u attacker_bank=%04X action=%04X result=%s\n",
              tick,i+1,d.attackerBank,d.attackerAction,d.outcome);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
} // namespace XM8781Flight
#endif
