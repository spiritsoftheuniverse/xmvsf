#ifndef XMVSF_STAGE87_82_CYCLOPS_SLAM_H
#define XMVSF_STAGE87_82_CYCLOPS_SLAM_H
// Isolated Tournament experiment, NOT an Oracle/ROM state replacement.
// Native source: captured Cyclops aerial down+MP sequence, frame 2536:
// P2 FF4414=00066200 (+6.3828125), FF441C=00132A00 (+19.1640625 down),
// FF4460=1C18, FF44E0=0230, FF4406=000C, FF4408=0002.
// This applies the recorded impulse ONCE to observed flying-screen pairs,
// while preserving Drill Claw candidate actions and the native reference.
// No damage, animation pointer, frame scheduling, wall collision, or Stable edit.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8782Flight {
static inline unsigned byteAt(const std::uint8_t* r,unsigned p){return r[(p&0xffffu)^1u];}
static inline unsigned wordAt(const std::uint8_t* r,unsigned p){return (byteAt(r,p)<<8)|byteAt(r,p+1);}
static inline void setByte(std::uint8_t* r,unsigned p,unsigned v){r[(p&0xffffu)^1u]=std::uint8_t(v);}
static inline void setWord(std::uint8_t* r,unsigned p,unsigned v){setByte(r,p,v>>8);setByte(r,p+1,v);}
static inline std::uint32_t rawFixed(const std::uint8_t* r,unsigned p){return (std::uint32_t(wordAt(r,p))<<16)|wordAt(r,p+2);}
static inline std::int64_t signedFixed(std::uint32_t x){return (x&0x80000000u)?std::int64_t(x)-0x100000000LL:std::int64_t(x);}
static inline std::int64_t magnitude(std::int64_t v){return v<0?-v:v;}
static inline void setFixed(std::uint8_t* r,unsigned p,std::uint32_t v){setWord(r,p,v>>16);setWord(r,p+2,v);}
static const std::uint32_t TARGET_RIGHT=0x00066200u;  // +6.3828125
static const std::uint32_t TARGET_LEFT =0xFFF99E00u;  // -6.3828125
static const std::uint32_t TARGET_DOWN =0x00132A00u;  // +19.1640625
static const std::uint16_t SOFT_ACTION=0x1C16u;
static const std::uint16_t SEVERE_ACTION=0x1C18u;
static const std::uint16_t SOFT_PROFILE=0x0220u;
static const std::uint16_t SEVERE_PROFILE=0x0230u;
static inline bool isDrillClawCandidate(unsigned action){
    // Observed Wolverine special-action candidates from the prior capture.
    // Do not depend on previously uncertain attacker-bank identification.
    return action==0x2C1Eu || action==0x2C46u;
}
struct Decision {
    bool candidate,launch,excluded,velocityWritten,stateWritten;
    unsigned attackerBank,attackerAction,victimActionBefore,victimActionAfter;
    unsigned victimProfileBefore,victimProfileAfter,substate,reaction,y;
    std::uint32_t beforeX,beforeY,afterX,afterY;
    const char* outcome;
    Decision():candidate(false),launch(false),excluded(false),velocityWritten(false),stateWritten(false),
       attackerBank(0),attackerAction(0),victimActionBefore(0),victimActionAfter(0),
       victimProfileBefore(0),victimProfileAfter(0),substate(0),reaction(0),y(0),
       beforeX(0),beforeY(0),afterX(0),afterY(0),outcome("NO_LAUNCH"){}
};
struct Actor {
    bool observed,priorFlight;
    std::uint32_t previousX;
    Actor():observed(false),priorFlight(false),previousX(0){}
    Decision evaluate(std::uint8_t* r,unsigned victim,unsigned attacker,bool enabled,bool combat){
        Decision d;
        if(!r)return d;
        d.reaction=wordAt(r,victim+6u);
        d.substate=wordAt(r,victim+8u);
        d.y=wordAt(r,victim+0x10u);
        d.beforeX=rawFixed(r,victim+0x14u);
        d.beforeY=rawFixed(r,victim+0x1Cu);
        d.attackerBank=wordAt(r,attacker+0x40u);
        d.attackerAction=wordAt(r,attacker+0x60u);
        d.victimActionBefore=wordAt(r,victim+0x60u);
        d.victimProfileBefore=wordAt(r,victim+0xE0u);
        d.afterX=d.beforeX;d.afterY=d.beforeY;
        const std::int64_t speed=signedFixed(d.beforeX);
        const std::int64_t absSpeed=magnitude(speed);
        const bool soft=d.victimActionBefore==SOFT_ACTION && d.victimProfileBefore==SOFT_PROFILE;
        const bool severe=d.victimActionBefore==SEVERE_ACTION && d.victimProfileBefore==SEVERE_PROFILE;
        const bool paired=soft || severe;
        // Do not turn ordinary no-horizontal-speed mid-air hits into a slam.
        // Unknown reaction profiles are diagnostic-only, never rewritten.
        const bool flightLike=combat && d.reaction==0x000Cu && d.y<472u &&
            absSpeed>=0x00060000LL && absSpeed<=0x00300000LL;
        d.candidate=flightLike && d.substate==2u;
        const std::int64_t priorSpeed=signedFixed(previousX);
        const bool speedChange=magnitude(speed-priorSpeed)>=0x00020000LL;
        const bool changed=!observed || !priorFlight || speedChange;
        d.launch=enabled && d.candidate && changed;
        if(d.launch){
            if(isDrillClawCandidate(d.attackerAction)){
                d.excluded=true;
                d.outcome="DRILL_CLAW_PROTECT";
            }else if(!paired){
                d.outcome="UNKNOWN_PROFILE_SKIPPED";
            }else if(severe && d.attackerAction==0x1806u &&
                     magnitude(signedFixed(d.beforeY))>=0x00100000LL &&
                     signedFixed(d.beforeY)>0 && absSpeed<=0x000A0000LL){
                // Preserve Cyclops down+MP's own native severe aerial slam.
                d.excluded=true;
                d.outcome="CYCLOPS_NATIVE_REFERENCE";
            }else if(severe && signedFixed(d.beforeY)>=0x00100000LL &&
                     absSpeed<=0x000A0000LL){
                // Native near-vertical severe slams already supply this behavior.
                d.excluded=true;
                d.outcome="NATIVE_STEEP_SLAM_PRESERVED";
            }else{
                d.afterX=speed<0?TARGET_LEFT:TARGET_RIGHT;
                d.afterY=TARGET_DOWN;
                setFixed(r,victim+0x14u,d.afterX);
                setFixed(r,victim+0x1Cu,d.afterY);
                d.velocityWritten=true;
                if(soft){
                    setWord(r,victim+0x60u,SEVERE_ACTION);
                    setWord(r,victim+0xE0u,SEVERE_PROFILE);
                    d.stateWritten=true;
                    d.outcome="SOFT_TO_CYCLOPS_SLAM";
                }else{
                    d.outcome="SEVERE_TO_CYCLOPS_SLAM";
                }
            }
        }
        d.victimActionAfter=wordAt(r,victim+0x60u);
        d.victimProfileAfter=wordAt(r,victim+0xE0u);
        // Retain original velocity on exclusions and after unknown profiles.
        // Prior-flight latch survives non-candidate wall substates; the native
        // ROM still handles rebounds, gravity and ground collision.
        previousX=d.velocityWritten?d.afterX:d.beforeX;
        priorFlight=flightLike;
        observed=true;
        if(!combat || d.y>=472u || d.reaction!=0x000Cu)priorFlight=false;
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
        const char* on=std::getenv("XM8782_SLAM_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8782_SLAM_LOG_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\tvelocity_written\tstate_pair_written\texcluded\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.82 CYCLOPS_SLAM_READY enabled=%d target_vx=00066200 target_vy=00132A00 drill_guard=2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r){
        if(!initialized)init();
        if(!r)return;
        const bool combat=byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(r,i?0x4400u:0x4000u,i?0x4000u:0x4400u,enabled,combat);
            if(!d.launch)continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%u\t%s\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.velocityWritten),unsigned(d.stateWritten),
                  unsigned(d.excluded),d.outcome);
                std::fflush(log);
            }
            std::fprintf(stderr,"XM87.82 FLIGHT frame=%u victim=P%u attacker_bank=%04X action=%04X result=%s\n",
              tick,i+1,d.attackerBank,d.attackerAction,d.outcome);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
} // namespace XM8782Flight
#endif
