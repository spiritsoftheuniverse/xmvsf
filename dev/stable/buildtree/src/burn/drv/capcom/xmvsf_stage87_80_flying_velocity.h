#ifndef XMVSF_STAGE87_80_FLYING_VELOCITY_H
#define XMVSF_STAGE87_80_FLYING_VELOCITY_H
// Tournament experimental flying-screen launch normalization.
// Source: native FF44xx actor observations from Storm Air HK -> Air HP.
// The exact ROM launch writer/flight-state flag is not yet proven. This is a
// velocity + reaction candidate detector, not a claimed universal state flag.
// Never change animation, damage, input restrictions or stage/camera registers.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8780Fly {
static const std::int64_t REFERENCE_SPEED = 0x0010AA00LL; // +/- 16.6640625
static const std::uint32_t REFERENCE_VERTICAL = 0x0000B000u; // +0.6875 downward
static const std::int64_t THRESHOLD = 0x000C0000LL; // 12.0 => no first-hit Air HK
static const std::int64_t MAX_CANDIDATE = 0x00300000LL; // +/-48.0, reject corrupt/unrelated speeds
static inline unsigned byteAt(const std::uint8_t* r,unsigned pos) {
    return (unsigned)r[(pos&0xffffu)^1u];
}
static inline unsigned wordAt(const std::uint8_t* r,unsigned pos) {
    return (byteAt(r,pos)<<8) | byteAt(r,pos+1u);
}
static inline void setByte(std::uint8_t* r,unsigned pos,unsigned value) {
    r[(pos&0xffffu)^1u]=(std::uint8_t)(value&0xffu);
}
static inline void setWord(std::uint8_t* r,unsigned pos,unsigned value) {
    setByte(r,pos,value>>8);setByte(r,pos+1u,value);
}
static inline std::uint32_t fixedRaw(const std::uint8_t* r,unsigned pos) {
    return (std::uint32_t(wordAt(r,pos))<<16) | wordAt(r,pos+2u);
}
static inline std::int64_t signedFixed(std::uint32_t raw) {
    return (raw&0x80000000u)?(std::int64_t(raw)-0x100000000LL):std::int64_t(raw);
}
static inline std::int64_t mag(std::int64_t x){return x<0?-x:x;}
static inline void setFixed(std::uint8_t* r,unsigned pos,std::uint32_t raw) {
    setWord(r,pos,raw>>16);setWord(r,pos+2u,raw);
}
struct Decision {
    bool candidate;
    bool fire;
    std::uint32_t originalX,originalY,targetX,targetY;
    unsigned reaction,substate,y;
    Decision():candidate(false),fire(false),originalX(0),originalY(0),
         targetX(0),targetY(0),reaction(0),substate(0),y(0){}
};
struct Actor {
    bool observed, priorFlight;
    std::uint32_t previousX;
    unsigned previousSubstate;
    Actor():observed(false),priorFlight(false),previousX(0),previousSubstate(0){}
    Decision evaluate(std::uint8_t* ram,unsigned base,bool enabled,bool combat){
        Decision d;
        if(!ram)return d;
        d.reaction=wordAt(ram,base+6u);
        d.substate=wordAt(ram,base+8u);
        d.y=wordAt(ram,base+0x10u);
        d.originalX=fixedRaw(ram,base+0x14u);
        d.originalY=fixedRaw(ram,base+0x1cu);
        const std::int64_t speed=signedFixed(d.originalX);
        // Native world ground (472 in the observed stage); require airborne
        // reaction substate 2 in addition to speed threshold. No FF52BE gate.
        d.candidate=combat && d.reaction==0x000Cu && d.substate==2u &&
               d.y<472u && mag(speed)>=THRESHOLD && mag(speed)<=MAX_CANDIDATE;
        // The wall reaction temporarily switches substate 2 -> 0 -> 2 while
        // preserving large native VX (frames 1930..1935). Keep the flight
        // latch through that substate excursion; never reset wall bounce.
        const bool flightContinuing=combat && d.reaction==0x000Cu && d.y<472u &&
               mag(speed)>=THRESHOLD && mag(speed)<=MAX_CANDIDATE;
        const std::int64_t prevSpeed=signedFixed(previousX);
        const bool sharpTransition=!observed || !priorFlight ||
             ((speed<0)!=(prevSpeed<0)) ||
             mag(speed-prevSpeed)>=0x00020000LL; // >= 2.0 speed discontinuity
        d.fire=enabled && d.candidate && sharpTransition;
        d.targetX=(speed<0)?0xFFEF5600u:0x0010AA00u;
        d.targetY=REFERENCE_VERTICAL;
        if(d.fire){
            setFixed(ram,base+0x14u,d.targetX);
            setFixed(ram,base+0x1cu,d.targetY);
        }
        previousX=d.fire?d.targetX:d.originalX;
        previousSubstate=d.substate;
        priorFlight=flightContinuing;
        observed=true;
        if(!combat){priorFlight=false;previousSubstate=0u;}
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
        const char* on=std::getenv("XM8780_TOURNAMENT_OVERRIDE");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8780_FLY_OVERRIDE_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log) std::fputs("frame\tvictim\treaction\tsubstate\ty\toriginal_vx\toriginal_vy\ttarget_vx\ttarget_vy\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.80 FLY_OVERRIDE_READY enabled=%d threshold=12.0 target=16.6640625 scope=experimental\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* ram){
        if(!initialized)init();
        if(!ram)return;
        const bool combat=byteAt(ram,0x5000u)==8u || byteAt(ram,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(ram,i?0x4400u:0x4000u,enabled,combat);
            if(!d.fire)continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%u\t%08X\t%08X\t%08X\t%08X\tNORMALIZE\n",
                    tick,i+1u,d.reaction,d.substate,d.y,d.originalX,d.originalY,d.targetX,d.targetY);
                std::fflush(log);
            }
            std::fprintf(stderr,"XM87.80 FLY_NORMALIZE frame=%u victim=P%u from=%08X/%08X to=%08X/%08X\n",
                tick,i+1u,d.originalX,d.originalY,d.targetX,d.targetY);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
}
#endif
