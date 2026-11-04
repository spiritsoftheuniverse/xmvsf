#ifndef XMVSF_STAGE87_85_CONNECTED_AIR_HEAVY_H
#define XMVSF_STAGE87_85_CONNECTED_AIR_HEAVY_H
// Stage87.85: connected-air-hit qualifier layered on ROM bank/action telemetry.
// No claim that these addresses are a universal CPS-2 flying-screen flag.
// The positive signatures below were observed at native hit-state transitions
// AND subsequently reached |VX| >= 6 within 20 recorded frames.
// Negative examples: 21/1802/AD7E and 21/1808/2CDE entered soft 1C16/0220,
// but never reached true flying speed in the corresponding 20-frame windows.
// An initial aerial hit immediately after native upward launch 1C14/0210 is
// conservatively protected, independently of its button strength; the ROM
// recording does not yet expose a reliable cross-character heavy-button flag.
// Stage87.85: only a connected heavy following an earlier connected airborne
// hit can receive severe velocity; whiffs don't advance the connected counter.
// This closes the 87.84 shortcut where a Storm soft-reaction medium could be
// converted solely through an exact animation pointer and victim velocity.
// Normal heavy action codes are provisional until ROM/input verification.
// Original Cyclops down+MP, Wolverine Drill Claw and protected Stable unchanged.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8785Flight {
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
    bool onset,candidate,launch,excluded,velocityWritten,stateWritten,signatureKnown;
    bool openingFollowup,heavyFollowup,airAttackStart,hitConfirmed,hadConnectedAirHit,comboContinuous,provisionalHeavy;
    unsigned attackerBank,attackerAction,attackerPointer,airAttackIndex,victimActionBefore,victimActionAfter;
    unsigned victimProfileBefore,victimProfileAfter,substate,reaction,y,connectedBefore,connectedAfter;
    std::uint32_t beforeX,beforeY,afterX,afterY;
    const char* outcome;
    Decision():onset(false),candidate(false),launch(false),excluded(false),velocityWritten(false),stateWritten(false),
       signatureKnown(false),openingFollowup(false),heavyFollowup(false),airAttackStart(false),
       hitConfirmed(false),hadConnectedAirHit(false),comboContinuous(false),provisionalHeavy(false),
       attackerBank(0),attackerAction(0),attackerPointer(0),airAttackIndex(0),
       victimActionBefore(0),victimActionAfter(0),victimProfileBefore(0),victimProfileAfter(0),
       substate(0),reaction(0),y(0),connectedBefore(0),connectedAfter(0),
       beforeX(0),beforeY(0),afterX(0),afterY(0),outcome("NO_HIT"){}
};
struct Actor {
    // Per-victim, per-combo state; only a fresh connected airborne hit counts.
    bool observed,prevAttackNow,pendingSevere;
    unsigned prevAction,prevProfile,prevSubstate,prevReaction,prevVictimY;
    unsigned prevAttackerAction,attackSerial,lastConfirmedSerial,connectedAirHits,attackAge;
    unsigned pendingAge,pendingAction,pendingBank;
    Actor():observed(false),prevAttackNow(false),pendingSevere(false),prevAction(0),prevProfile(0),prevSubstate(0),
      prevReaction(0),prevVictimY(0),prevAttackerAction(0),attackSerial(0),
      lastConfirmedSerial(0),connectedAirHits(0),attackAge(1000),pendingAge(1000),pendingAction(0),pendingBank(0){}
    void clearChain(){connectedAirHits=0;attackSerial=0;lastConfirmedSerial=0;attackAge=1000;prevAttackNow=false;pendingSevere=false;pendingAge=1000;}
    Decision evaluate(std::uint8_t* r,unsigned victim,unsigned attacker,bool enabled,bool combat){
        Decision d;
        if(!r)return d;
        d.reaction=wordAt(r,victim+6u);d.substate=wordAt(r,victim+8u);
        d.y=wordAt(r,victim+0x10u);
        d.beforeX=rawFixed(r,victim+0x14u);d.beforeY=rawFixed(r,victim+0x1Cu);
        d.attackerBank=wordAt(r,attacker+0x40u);
        d.attackerAction=wordAt(r,attacker+0x60u);
        d.attackerPointer=wordAt(r,attacker+0x42u);
        d.victimActionBefore=wordAt(r,victim+0x60u);
        d.victimProfileBefore=wordAt(r,victim+0xE0u);
        d.afterX=d.beforeX;d.afterY=d.beforeY;
        const unsigned attackerY=wordAt(r,attacker+0x10u);
        const bool attackerAir=attackerY>0u && attackerY<472u;
        const bool attackNow=attackerAir && (normalAirAttack(d.attackerAction)||isDrillCandidate(d.attackerAction));
        // A gap in native reaction, victim landing or attacker landing ends the
        // linked air combo; one later heavy cannot inherit an old hit counter.
        const bool continuity=observed && combat && attackerAir && d.y<472u &&
          d.reaction==0x000Cu && prevReaction==0x000Cu && prevVictimY<472u;
        d.comboContinuous=continuity;
        if(!combat || !attackerAir || d.y>=472u || (observed && d.reaction!=0x000Cu))clearChain();
        d.airAttackStart=attackNow && (!prevAttackNow || d.attackerAction!=prevAttackerAction);
        if(d.airAttackStart){++attackSerial;attackAge=0;}else if(attackAge<1000u)++attackAge;
        d.airAttackIndex=attackSerial;
        d.connectedBefore=connectedAirHits;
        const bool soft=d.victimActionBefore==SOFT_ACTION && d.victimProfileBefore==SOFT_PROFILE;
        const bool severe=d.victimActionBefore==SEVERE_ACTION && d.victimProfileBefore==SEVERE_PROFILE;
        const bool paired=soft||severe;
        const bool stateChanged=!observed || prevAction!=d.victimActionBefore || prevProfile!=d.victimProfileBefore;
        // Fresh victim hit onset: game transition from native hit substate 2 to 0,
        // or a new hit reaction. It must coincide with a NEW airborne move.
        const bool freshReaction=observed && d.substate==0u &&
          (prevSubstate==2u || prevReaction!=d.reaction) &&
          (stateChanged || prevSubstate==2u);
        d.hitConfirmed=combat && attackerAir && attackNow && d.reaction==0x000Cu &&
          d.y<472u && freshReaction && attackAge<=24u && attackSerial!=lastConfirmedSerial;
        if(d.hitConfirmed){
            d.onset=true;
            d.hadConnectedAirHit=continuity && connectedAirHits>0u;
            d.openingFollowup=!d.hadConnectedAirHit;
            d.provisionalHeavy=heavyAirAttack(d.attackerAction);
            // The 87.84 soft-profile medium shortcut is gone. Even exact ROM
            // signatures are descriptive only, never an independent write gate.
            const bool medium=d.attackerAction==0x1802u || d.attackerAction==0x1808u ||
                              d.attackerAction==0x1A02u || d.attackerAction==0x1A08u;
            const Signature* known=match(d.attackerBank,d.attackerAction,d.attackerPointer,
                                         d.victimActionBefore,d.victimProfileBefore);
            d.signatureKnown=known!=NULL;
            const bool preserveCyclops=known && known->preserveNative;
            // Bank 22's 1A04 attack is still input-ambiguous: its existing
            // severe vector must remain native rather than be rewritten. This
            // deliberately sacrifices automatic Storm conversion until a live
            // button-labelled recording distinguishes MK from the heavy move.
            const bool stormAmbiguous=d.attackerBank==0x0022u && d.attackerAction==0x1A04u;
            // Update the count AFTER evaluating candidate status: a first
            // connected heavy is still the FIRST hit, never a finisher.
            d.heavyFollowup=d.hadConnectedAirHit && d.provisionalHeavy && !medium &&
                             paired && !isDrillCandidate(d.attackerAction);
            d.candidate=d.heavyFollowup;
            ++connectedAirHits;
            lastConfirmedSerial=attackSerial;
            d.connectedAfter=connectedAirHits;
            if(isDrillCandidate(d.attackerAction)){
                d.excluded=true;d.outcome="DRILL_CLAW_PRESERVED";
            }else if(stormAmbiguous){
                d.excluded=true;d.outcome="STORM_AMBIGUOUS_ACTION_PRESERVED";
            }else if(preserveCyclops){
                d.excluded=true;d.outcome="CYCLOPS_NATIVE_SLAM_PRESERVED";
            }else if(medium){
                d.excluded=true;d.outcome="MEDIUM_NATIVE_PRESERVED";
            }else if(!d.hadConnectedAirHit){
                d.excluded=true;d.outcome="FIRST_CONNECTED_AIR_HIT_PRESERVED";
            }else if(!d.provisionalHeavy){
                d.excluded=true;d.outcome="NOT_HEAVY_NATIVE_PRESERVED";
            }else if(!paired){
                d.excluded=true;d.outcome="UNKNOWN_REACTION_PRESERVED";
            }else if(!enabled){
                d.outcome="TEST_DISABLED_PRESERVED";
            }else{
                // Confirm the combo NOW but wait for the game's native launch
                // state before writing. At substate 0, the original game may
                // still overwrite velocity on the following frames.
                pendingSevere=enabled;
                pendingAge=0;
                pendingAction=d.attackerAction;
                pendingBank=d.attackerBank;
                d.outcome=enabled?"CONNECTED_HEAVY_HIT_LATCHED":"TEST_DISABLED_PRESERVED";
            }
        }else{
            d.connectedAfter=connectedAirHits;
            if(d.airAttackStart)d.outcome="AIR_ATTACK_ATTEMPT_UNCONFIRMED";
        }
        if(pendingSevere && !d.hitConfirmed){
            ++pendingAge;
            const bool stale=pendingAge>24u || !combat || !attackerAir ||
              d.reaction!=0x000Cu || d.y>=472u ||
              d.attackerAction!=pendingAction || d.attackerBank!=pendingBank;
            if(stale){
                pendingSevere=false;
                d.outcome="LATCH_CANCELLED_BEFORE_NATIVE_LAUNCH";
            }else if(d.substate==2u && paired){
                pendingSevere=false;
                d.heavyFollowup=true;
                d.candidate=true;
                const std::int64_t speed=signedFixed(d.beforeX);
                const bool nativeSteep=severe && signedFixed(d.beforeY)>=0x00100000LL &&
                  magnitude(speed)<=0x000A0000LL;
                if(nativeSteep){
                    d.excluded=true;
                    d.outcome="NATIVE_STEEP_SLAM_PRESERVED";
                }else{
                    d.launch=true;
                    d.afterX=speed<0?TARGET_LEFT:TARGET_RIGHT;
                    d.afterY=TARGET_DOWN;
                    setFixed(r,victim+0x14u,d.afterX);
                    setFixed(r,victim+0x1Cu,d.afterY);
                    d.velocityWritten=true;
                    if(soft){
                        setWord(r,victim+0x60u,SEVERE_ACTION);
                        setWord(r,victim+0xE0u,SEVERE_PROFILE);
                        d.stateWritten=true;
                        d.outcome="CONNECTED_AIR_HEAVY_SOFT_TO_SEVERE";
                    }else d.outcome="CONNECTED_AIR_HEAVY_SEVERE_SLAM";
                }
            }
        }
        d.victimActionAfter=wordAt(r,victim+0x60u);
        d.victimProfileAfter=wordAt(r,victim+0xE0u);
        observed=true;
        prevAction=d.victimActionAfter;prevProfile=d.victimProfileAfter;
        prevSubstate=d.substate;prevReaction=d.reaction;prevVictimY=d.y;
        prevAttackNow=attackNow;
        prevAttackerAction=d.attackerAction;
        return d;
    }
};
struct Controller {
    Actor actor[2];bool initialized,enabled;std::FILE* log;
    Controller():initialized(false),enabled(false),log(NULL){}
    ~Controller(){if(log)std::fclose(log);}
    void init(){
        initialized=true;
        const char* on=std::getenv("XM8785_CONNECTED_HEAVY_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8785_CONNECTED_HEAVY_LOG_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\tattacker_pointer\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\trom_signature_matched\tfirst_air_followup\tair_attack_index\theavy_followup\tnew_air_attack\tconnected_hits_before\tconnected_hits_after\tcombo_continuous\thit_confirmed\tvelocity_written\tstate_pair_written\texcluded\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.85 CONNECTED_AIR_HEAVY_READY enabled=%d target=00066200/00132A00 connected_air_heavy_second=1 protected_drill=2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r){
        if(!initialized)init();
        if(!r)return;
        const bool combat=byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(r,i?0x4400u:0x4000u,i?0x4000u:0x4400u,enabled,combat);
            if(!d.hitConfirmed && !d.airAttackStart && !d.launch && !(d.excluded && d.candidate))continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.signatureKnown),unsigned(d.openingFollowup),d.airAttackIndex,
                  unsigned(d.heavyFollowup),unsigned(d.airAttackStart),d.connectedBefore,d.connectedAfter,unsigned(d.comboContinuous),unsigned(d.hitConfirmed),unsigned(d.velocityWritten),unsigned(d.stateWritten),unsigned(d.excluded),d.outcome);
                std::fflush(log);
            }
            if(d.launch)std::fprintf(stderr,"XM87.85 FLIGHT frame=%u victim=P%u bank=%04X action=%04X anim=%04X decision=%s\n",
                tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.outcome);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
} // namespace XM8785Flight
#endif
