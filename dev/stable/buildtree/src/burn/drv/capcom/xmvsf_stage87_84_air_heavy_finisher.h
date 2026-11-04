#ifndef XMVSF_STAGE87_84_AIR_HEAVY_FINISHER_H
#define XMVSF_STAGE87_84_AIR_HEAVY_FINISHER_H
// Stage87.84: capture-confirmed ROM bank/action/animation-pointer gate.
// No claim that these addresses are a universal CPS-2 flying-screen flag.
// The positive signatures below were observed at native hit-state transitions
// AND subsequently reached |VX| >= 6 within 20 recorded frames.
// Negative examples: 21/1802/AD7E and 21/1808/2CDE entered soft 1C16/0220,
// but never reached true flying speed in the corresponding 20-frame windows.
// An initial aerial hit immediately after native upward launch 1C14/0210 is
// conservatively protected, independently of its button strength; the ROM
// recording does not yet expose a reliable cross-character heavy-button flag.
// Stage87.84 adds a second route: an airborne heavy normal as the SECOND OR
// LATER air attack can become severe even if its impact pointer is not one of
// Stage87.83's known signatures and the native horizontal speed is below six.
// The first air follow-up after a launcher stays protected. Air-attack index
// counts distinct airborne attack starts, not frames or inherited victim VX.
// The heavy action family (1804/180A/1A04/1A0A) is a conservative inference
// from the observed even-numbered action slots. Input buttons were all zero
// in both stored recordings; universal heavy mapping is not ROM-proven.
// Protected Stage87.64, Drill Claw, and native Cyclops down+MP are unchanged.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8784Flight {
static inline unsigned byteAt(const std::uint8_t* r,unsigned p){return r[(p&0xffffu)^1u];}
static inline unsigned wordAt(const std::uint8_t* r,unsigned p){return (byteAt(r,p)<<8)|byteAt(r,p+1);}
static inline void setByte(std::uint8_t* r,unsigned p,unsigned v){r[(p&0xffffu)^1u]=std::uint8_t(v);}
static inline void setWord(std::uint8_t* r,unsigned p,unsigned v){setByte(r,p,v>>8);setByte(r,p+1,v);}
static inline std::uint32_t rawFixed(const std::uint8_t* r,unsigned p){return (std::uint32_t(wordAt(r,p))<<16)|wordAt(r,p+2);}
static inline std::int64_t signedFixed(std::uint32_t x){return (x&0x80000000u)?std::int64_t(x)-0x100000000LL:std::int64_t(x);}
static inline std::int64_t magnitude(std::int64_t v){return v<0?-v:v;}
static inline void setFixed(std::uint8_t* r,unsigned p,std::uint32_t v){setWord(r,p,v>>16);setWord(r,p+2,v);}
static const std::uint32_t TARGET_RIGHT=0x00066200u;
static const std::uint32_t TARGET_LEFT =0xFFF99E00u;
static const std::uint32_t TARGET_DOWN =0x00132A00u;
static const unsigned SOFT_ACTION=0x1C16u,SEVERE_ACTION=0x1C18u;
static const unsigned SOFT_PROFILE=0x0220u,SEVERE_PROFILE=0x0230u;
static inline bool isDrillCandidate(unsigned action){return action==0x2C1Eu || action==0x2C46u;}
static inline bool normalAirAttack(unsigned action){
    return ((action&0xFFF0u)==0x1800u || (action&0xFFF0u)==0x1A00u) &&
           (action&1u)==0 && (action&0x000Eu)<=0x000Au;
}
static inline bool heavyAirAttack(unsigned action){
    return action==0x1804u || action==0x180Au || action==0x1A04u || action==0x1A0Au;
}
struct Signature {unsigned bank,action,pointer,victimAction,victimProfile;bool preserveNative;};
// Bank/action/pointer are captured from attacker FFxx40/FFxx60/FFxx42
// on the FIRST victim reaction transition, NOT from a later animation frame.
static const Signature CONFIRMED[] = {
    {0x22,0x1A04,0x5DAA,0x1C18,0x0230,false}, // storm recording, VX 16.664
    {0x22,0x1A02,0x5C84,0x1C16,0x0220,false}, // follow-up flying-screen motion
    {0x21,0x1A04,0x31D2,0x1C16,0x0220,false}, // later airborne hit, VX 9.238
    {0x27,0x1A04,0xD7A6,0x1C18,0x0230,false}, // Zangief recording, VX 16.664
    {0x21,0x1806,0xCB34,0x1C18,0x0230,true},  // native Cyclops down+MP
};
static inline const Signature* match(unsigned bank,unsigned action,unsigned pointer,unsigned va,unsigned vp){
    for(const Signature& s:CONFIRMED)
        if(s.bank==bank && s.action==action && s.pointer==pointer && s.victimAction==va && s.victimProfile==vp)return &s;
    return NULL;
}
struct Decision {
    bool onset,candidate,launch,excluded,velocityWritten,stateWritten,signatureKnown,openingFollowup,heavyFollowup,airAttackStart;
    unsigned attackerBank,attackerAction,attackerPointer,airAttackIndex,victimActionBefore,victimActionAfter;
    unsigned victimProfileBefore,victimProfileAfter,substate,reaction,y;
    std::uint32_t beforeX,beforeY,afterX,afterY;
    const char* outcome;
    Decision():onset(false),candidate(false),launch(false),excluded(false),velocityWritten(false),stateWritten(false),
       signatureKnown(false),openingFollowup(false),heavyFollowup(false),airAttackStart(false),attackerBank(0),attackerAction(0),attackerPointer(0),airAttackIndex(0),
       victimActionBefore(0),victimActionAfter(0),victimProfileBefore(0),victimProfileAfter(0),
       substate(0),reaction(0),y(0),beforeX(0),beforeY(0),afterX(0),afterY(0),outcome("NO_LAUNCH"){}
};
struct Actor {
    bool observed,priorFlight,knownHit,nativeHit,openingHit,heavyHit,hitProcessed,prevAttackerAir;
    unsigned prevAction,prevProfile,prevSubstate,prevY,hitAge,impactBank,impactAction,prevAttackerAction,airAttackIndex,latchedAirIndex,latchedAttackSerial,airAttackSerial;
    std::uint32_t previousX;
    Actor():observed(false),priorFlight(false),knownHit(false),nativeHit(false),openingHit(false),heavyHit(false),hitProcessed(false),prevAttackerAir(false),prevAction(0),prevProfile(0),prevSubstate(0),prevY(0),hitAge(1000),impactBank(0),impactAction(0),prevAttackerAction(0),airAttackIndex(0),latchedAirIndex(0),latchedAttackSerial(0),airAttackSerial(0),previousX(0){}
    Decision evaluate(std::uint8_t* r,unsigned victim,unsigned attacker,bool enabled,bool combat){
        Decision d;
        if(!r)return d;
        d.reaction=wordAt(r,victim+6u);d.substate=wordAt(r,victim+8u);
        d.y=wordAt(r,victim+0x10u);
        d.beforeX=rawFixed(r,victim+0x14u);d.beforeY=rawFixed(r,victim+0x1Cu);
        d.attackerBank=wordAt(r,attacker+0x40u);
        d.attackerAction=wordAt(r,attacker+0x60u);
        d.attackerPointer=wordAt(r,attacker+0x42u);
        const unsigned attackerY=wordAt(r,attacker+0x10u);
        const bool attackerAir=attackerY<472u && attackerY>0u;
        const bool attackNow=attackerAir && normalAirAttack(d.attackerAction);
        d.airAttackStart=attackNow && (!prevAttackerAir || d.attackerAction!=prevAttackerAction);
        if(!combat || !attackerAir){airAttackIndex=0;airAttackSerial=0;}
        if(d.airAttackStart){
            if(airAttackIndex<255u)++airAttackIndex;
            ++airAttackSerial;
        }
        d.airAttackIndex=airAttackIndex;
        d.victimActionBefore=wordAt(r,victim+0x60u);
        d.victimProfileBefore=wordAt(r,victim+0xE0u);
        d.afterX=d.beforeX;d.afterY=d.beforeY;
        const std::int64_t speed=signedFixed(d.beforeX);
        const std::int64_t absSpeed=magnitude(speed);
        const bool soft=d.victimActionBefore==SOFT_ACTION && d.victimProfileBefore==SOFT_PROFILE;
        const bool severe=d.victimActionBefore==SEVERE_ACTION && d.victimProfileBefore==SEVERE_PROFILE;
        const bool paired=soft || severe;
        const bool flightLike=combat && d.reaction==0x000Cu && d.y<472u &&
            absSpeed>=0x00060000LL && absSpeed<=0x00300000LL;
        // Bind to the exact hit transition. Merely inheriting a soft reaction
        // or a high preexisting VX is not proof that a medium caused flight.
        const bool newlyPaired=(prevAction!=d.victimActionBefore || prevProfile!=d.victimProfileBefore);
        // A second air hit can reuse the same 1C16/0220 reaction. Identify
        // a fresh attack start as well as the native 2 -> 0 hit substate reset.
        // A wall rebound cannot qualify without a distinct airborne attack.
        const bool newAirHit=attackNow && airAttackSerial!=latchedAttackSerial &&
            prevSubstate==2u && d.substate==0u;
        d.onset=combat && d.reaction==0x000Cu && d.y<472u && d.substate==0u && paired &&
            observed && (newlyPaired || newAirHit);
        if(d.onset){
            const Signature* s=match(d.attackerBank,d.attackerAction,d.attackerPointer,
                                     d.victimActionBefore,d.victimProfileBefore);
            knownHit=s!=NULL;nativeHit=s && s->preserveNative;
            // Opening aerial heavy is not a finisher. Any earlier airborne
            // normal attack can establish the sequence, including a medium.
            // Keep exact native Cyclops special separate from the heavy route.
            heavyHit=attackNow && heavyAirAttack(d.attackerAction) && airAttackIndex>=2u;
            latchedAirIndex=airAttackIndex;
            latchedAttackSerial=airAttackSerial;
            // 1C14/0210 is the observed upward-launch reaction preceding
            // first airborne hits in both recordings. Do NOT reclassify that
            // first follow-up as a finishing flying-screen hit.
            openingHit=(airAttackIndex<=1u) && prevAction==0x1C14u && prevProfile==0x0210u &&
                prevSubstate==2u && prevY<472u;
            hitAge=0;hitProcessed=false;impactBank=d.attackerBank;impactAction=d.attackerAction;
            d.outcome=openingHit?"OPENING_AIR_FOLLOWUP_LATCHED":
                (knownHit?"QUALIFIED_HIT_LATCHED":"UNREGISTERED_HIT_LATCHED");
        }else if(hitAge<1000u){++hitAge;}
        d.signatureKnown=knownHit && hitAge<=24u && impactBank==d.attackerBank && impactAction==d.attackerAction;
        d.openingFollowup=openingHit && hitAge<=24u;
        d.heavyFollowup=heavyHit && hitAge<=24u && impactBank==d.attackerBank &&
            impactAction==d.attackerAction && attackNow;
        // The combo-position heavy route deliberately does not require the
        // native |VX|>=6 threshold: that was losing soft second-hit finishers.
        d.candidate=(flightLike || (d.heavyFollowup && combat && paired &&
            d.reaction==0x000Cu && d.y<472u)) && d.substate==2u;
        const bool speedChange=magnitude(speed-signedFixed(previousX))>=0x00020000LL;
        const bool changed=!observed || !priorFlight || speedChange || d.heavyFollowup;
        d.launch=enabled && d.candidate && changed && hitAge<=24u && !hitProcessed;
        if(d.launch){
            hitProcessed=true; // one hit, one decision: never override again after gravity or bounce
            if(isDrillCandidate(d.attackerAction)){
                d.excluded=true;d.outcome="DRILL_CLAW_PROTECTED";
            }else if(!paired){
                d.outcome="UNKNOWN_VICTIM_PROFILE_PRESERVED";
            }else if(d.openingFollowup){
                d.excluded=true;d.outcome="FIRST_AERIAL_FOLLOWUP_PRESERVED";
            }else if(!d.signatureKnown && !d.heavyFollowup){
                d.excluded=true;d.outcome="UNREGISTERED_ATTACK_PRESERVED";
            }else if(nativeHit){
                d.excluded=true;d.outcome="CYCLOPS_NATIVE_DOWN_MP_PRESERVED";
            }else if(severe && signedFixed(d.beforeY)>=0x00100000LL && absSpeed<=0x000A0000LL){
                d.excluded=true;d.outcome="NATIVE_STEEP_SLAM_PRESERVED";
            }else{
                d.afterX=speed<0?TARGET_LEFT:TARGET_RIGHT;
                d.afterY=TARGET_DOWN;
                setFixed(r,victim+0x14u,d.afterX);
                setFixed(r,victim+0x1Cu,d.afterY);
                d.velocityWritten=true;
                if(soft){
                    setWord(r,victim+0x60u,SEVERE_ACTION);
                    setWord(r,victim+0xE0u,SEVERE_PROFILE);
                    d.stateWritten=true;d.outcome=d.heavyFollowup?"AIR_HEAVY_2PLUS_SOFT_TO_SEVERE":"ROM_QUALIFIED_SOFT_TO_SEVERE";
                }else d.outcome=d.heavyFollowup?"AIR_HEAVY_2PLUS_SEVERE_SLAM":"ROM_QUALIFIED_SEVERE_SLAM";
            }
        }
        d.victimActionAfter=wordAt(r,victim+0x60u);
        d.victimProfileAfter=wordAt(r,victim+0xE0u);
        previousX=d.velocityWritten?d.afterX:d.beforeX;
        priorFlight=flightLike;
        observed=true;
        prevAction=d.victimActionAfter;prevProfile=d.victimProfileAfter;
        prevSubstate=d.substate;prevY=d.y;
        prevAttackerAir=attackNow;
        prevAttackerAction=d.attackerAction;
        // A reaction reset or ground landing ends the last aerial-hit identity.
        if(!combat || d.y>=472u || d.reaction!=0x000Cu){
            priorFlight=false;knownHit=false;nativeHit=false;openingHit=false;heavyHit=false;hitProcessed=false;hitAge=1000;
            if(!combat || d.y>=472u){airAttackIndex=0;airAttackSerial=0;}
        }
        return d;
    }
};
struct Controller {
    Actor actor[2];bool initialized,enabled;std::FILE* log;
    Controller():initialized(false),enabled(false),log(NULL){}
    ~Controller(){if(log)std::fclose(log);}
    void init(){
        initialized=true;
        const char* on=std::getenv("XM8784_AIR_HEAVY_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8784_AIR_HEAVY_LOG_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\tattacker_pointer\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\trom_signature_matched\tfirst_air_followup\tair_attack_index\theavy_followup\tnew_air_attack\tvelocity_written\tstate_pair_written\texcluded\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.84 AIR_HEAVY_FINISHER_READY enabled=%d target=00066200/00132A00 air_heavy_second=1 protected_drill=2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r){
        if(!initialized)init();
        if(!r)return;
        const bool combat=byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(r,i?0x4400u:0x4000u,i?0x4000u:0x4400u,enabled,combat);
            if(!d.launch && !d.onset)continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.signatureKnown),unsigned(d.openingFollowup),d.airAttackIndex,
                  unsigned(d.heavyFollowup),unsigned(d.airAttackStart),unsigned(d.velocityWritten),unsigned(d.stateWritten),unsigned(d.excluded),d.outcome);
                std::fflush(log);
            }
            if(d.launch)std::fprintf(stderr,"XM87.84 FLIGHT frame=%u victim=P%u bank=%04X action=%04X anim=%04X decision=%s\n",
                tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.outcome);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
} // namespace XM8784Flight
#endif
