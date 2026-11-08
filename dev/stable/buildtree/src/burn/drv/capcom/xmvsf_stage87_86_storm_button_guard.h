#ifndef XMVSF_STAGE87_86_STORM_BUTTON_GUARD_H
#define XMVSF_STAGE87_86_STORM_BUTTON_GUARD_H
// Stage87.86: connected-air-hit qualifier layered on ROM bank/action telemetry.
// No claim that these addresses are a universal CPS-2 flying-screen flag.
// The positive signatures below were observed at native hit-state transitions
// AND subsequently reached |VX| >= 6 within 20 recorded frames.
// Negative examples: 21/1802/AD7E and 21/1808/2CDE entered soft 1C16/0220,
// but never reached true flying speed in the corresponding 20-frame windows.
// An initial aerial hit immediately after native upward launch 1C14/0210 is
// conservatively protected, independently of its button strength; the ROM
// recording does not yet expose a reliable cross-character heavy-button flag.
// Stage87.86: Storm 1A04 can describe multiple button strengths. We read
// real P1/P2 CPS-2 button inputs (six face buttons) at the driver, preserve
// the latest unconsumed press edge, and qualify bank22 action1A04 ONLY when
// the confirmed hit follows an earlier connected airborne hit AND a recent
// HP/HK press is associated with this attack. An MK press cannot inherit HK.
// Ambiguous or absent button evidence fails closed; no guessing from the
// animation pointer or Stage87.80's previously overwritten launch velocity.
// This closes the 87.84 shortcut where a Storm soft-reaction medium could be
// converted solely through an exact animation pointer and victim velocity.
// Normal heavy action codes are provisional until ROM/input verification.
// Original Cyclops down+MP, Wolverine Drill Claw and protected Stable unchanged.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xmvsf_stage87_223_storm_native_flight.h" // native HP velocity/index reference

namespace XM8786Flight {
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
// Native Cyclops down+MP severe flight: knockdown reaction and flight speed
// form one operation. 1C18/0230 must not be omitted on a severe write.
// Historical Cyclops-like severe writer retained for the source audit only.
// TEST 87.223 never calls this for an ordinary Tournament aerial finisher.
static inline void applyCyclopsSevereFlight(std::uint8_t* r,unsigned victim,std::uint32_t horizontal){
    setWord(r,victim+0x60u,SEVERE_ACTION);
    setWord(r,victim+0xE0u,SEVERE_PROFILE);
    setFixed(r,victim+0x14u,horizontal);
    setFixed(r,victim+0x1Cu,TARGET_DOWN);
}
// Storm-like flying-screen movement, NOT a fabricated special knockdown.
// Preserve the original 68000 victim action/index and all ground-hit gates;
// forcing 1C18/0230 was the reason the previous severe variant lost OTG.
static inline void applyStormLikeMotion(std::uint8_t* r,unsigned victim,
                                       std::uint32_t horizontal,std::uint32_t vertical){
    setFixed(r,victim+0x14u,horizontal);
    setFixed(r,victim+0x1cu,vertical);
}
// TEST 87.224: Charlie's native actor ID 0x20, action bank 0x28.
// The supplied 87.223 trace confirms real flying-screen hits on 1804
// (air HP) and 1A04 (super-jump HK). A move's 1A04 label is shared
// with other characters, so NEVER identify Charlie by action alone.
static inline bool charlieAirHeavyFS(unsigned fighter,unsigned bank,
                                      unsigned action){
    return fighter==0x0020u && bank==0x0028u &&
           (action==0x1804u || action==0x1a04u);
}
// Exact 1.5x fixed-point multiplier. Saturate before applying so a
// corrupted learned Storm reference cannot yield an extreme FS vector.
static inline std::uint32_t charlieFS150(std::uint32_t positive,
                                         std::uint32_t maximum){
    const std::uint64_t scaled=(std::uint64_t(positive)*3u)/2u;
    return scaled>maximum?maximum:std::uint32_t(scaled);
}
static inline bool isDrillCandidate(unsigned action){return action==0x2C1Eu || action==0x2C46u;}
static inline bool normalAirAttack(unsigned action){
    return ((action&0xFFF0u)==0x1800u || (action&0xFFF0u)==0x1A00u) &&
           (action&1u)==0 && (action&0x000Eu)<=0x000Au;
}
static inline bool heavyAirAttack(unsigned action){
    return action==0x1804u || action==0x180Au || action==0x1A04u || action==0x1A0Au;
}
// XM87.124: fighter +0x52 is the ROM's character-id field. Values below
// follow the native roster convention (Cyclops=0x02 in 87.119 snapshots).
// Unknown button evidence on a heavy action fails closed for these two.
static inline bool excludedCharacterHP(unsigned fighterId,unsigned action,unsigned button){
    if(fighterId!=0x000Au && fighterId!=0x0022u)return false; // Sabretooth / Cammy
    if(button==(1u<<5))return false; // verified HK edge is not HP
    if(button==(1u<<2))return true; // confirmed HP edge
    return heavyAirAttack(action); // ambiguous => preserve native reaction
}
static inline bool nativeLaunchReaction(unsigned action,unsigned profile){
    return action==0x1C14u && profile==0x0210u;
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
    bool buttonEvidence,stormMediumButton,stormHeavyButton,buttonEdge;
    bool nativeLaunched,characterHPExcluded;
    unsigned attackerBank,attackerAction,attackerPointer,airAttackIndex,attackerId,victimId,buttonBits,taggedButton,buttonAge,victimActionBefore,victimActionAfter;
    unsigned victimProfileBefore,victimProfileAfter,substate,reaction,y,connectedBefore,connectedAfter;
    std::uint32_t beforeX,beforeY,afterX,afterY;
    const char* outcome;
    Decision():onset(false),candidate(false),launch(false),excluded(false),velocityWritten(false),stateWritten(false),
       signatureKnown(false),openingFollowup(false),heavyFollowup(false),airAttackStart(false),
       hitConfirmed(false),hadConnectedAirHit(false),comboContinuous(false),provisionalHeavy(false),
       buttonEvidence(false),stormMediumButton(false),stormHeavyButton(false),buttonEdge(false),
       nativeLaunched(false),characterHPExcluded(false),
       attackerBank(0),attackerAction(0),attackerPointer(0),airAttackIndex(0),attackerId(0),victimId(0),
       buttonBits(0),taggedButton(0),buttonAge(1000),
       victimActionBefore(0),victimActionAfter(0),victimProfileBefore(0),victimProfileAfter(0),
       substate(0),reaction(0),y(0),connectedBefore(0),connectedAfter(0),
       beforeX(0),beforeY(0),afterX(0),afterY(0),outcome("NO_HIT"){}
};
struct Actor {
    // Per-victim, per-combo state; only a fresh connected airborne hit counts.
    bool observed,prevAttackNow,pendingSevere,nativeLaunched,pendingStormRepeatHK;
    bool stormFlightUsedUntilLanding=false;
    unsigned prevAction,prevProfile,prevSubstate,prevReaction,prevVictimY;
    unsigned prevAttackerAction,attackSerial,lastConfirmedSerial,connectedAirHits,connectedStormHeavyHits,attackAge;
    unsigned pendingAge,pendingAction,pendingBank,pendingAttackerId,lastAttackerId,lastVictimId;
    unsigned previousButtons,recentButton,recentButtonAge,pressSequence,consumedSequence;
    Actor():observed(false),prevAttackNow(false),pendingSevere(false),nativeLaunched(false),pendingStormRepeatHK(false),prevAction(0),prevProfile(0),prevSubstate(0),
      prevReaction(0),prevVictimY(0),prevAttackerAction(0),attackSerial(0),
      lastConfirmedSerial(0),connectedAirHits(0),connectedStormHeavyHits(0),attackAge(1000),pendingAge(1000),pendingAction(0),pendingBank(0),pendingAttackerId(0),lastAttackerId(0),lastVictimId(0),
      previousButtons(0),recentButton(0),recentButtonAge(1000),pressSequence(0),consumedSequence(0){}
    void clearChain(){connectedAirHits=0;connectedStormHeavyHits=0;attackSerial=0;lastConfirmedSerial=0;attackAge=1000;prevAttackNow=false;pendingSevere=false;pendingStormRepeatHK=false;pendingAge=1000;nativeLaunched=false;}
    // Bit order is LP MP HP LK MK HK. These are driver-level button states,
    // sampled after CpsRwGetInp, not guessed from a ROM action code.
    Decision evaluate(std::uint8_t* r,unsigned victim,unsigned attacker,bool enabled,bool combat,unsigned buttons=0u){
        Decision d;
        if(!r)return d;
        buttons&=63u;
        d.buttonBits=buttons;
        const unsigned rising=buttons&~previousButtons;
        d.buttonEdge=rising!=0u;
        previousButtons=buttons;
        if(rising){
            ++pressSequence;
            // A simultaneous button edge does not identify the attack button.
            recentButton=(rising&(rising-1u))?0u:rising;
            recentButtonAge=0u;
        }else if(recentButtonAge<1000u)++recentButtonAge;
        d.reaction=wordAt(r,victim+6u);d.substate=wordAt(r,victim+8u);
        d.y=wordAt(r,victim+0x10u);
        d.beforeX=rawFixed(r,victim+0x14u);d.beforeY=rawFixed(r,victim+0x1Cu);
        d.attackerId=wordAt(r,attacker+0x52u);
        d.victimId=wordAt(r,victim+0x52u);
        // A tag must not inherit another fighter's combo/launch latch.
        if(observed && d.victimId!=lastVictimId)
            stormFlightUsedUntilLanding=false; // a tagged-in victim is new
        if(observed && (d.attackerId!=lastAttackerId || d.victimId!=lastVictimId))clearChain();
        lastAttackerId=d.attackerId;lastVictimId=d.victimId;
        // A single synthetic flight boost per continuous aerial sequence.
        // Re-arm only at a REAL native landing, not upon hitstun transitions.
        const unsigned floor=wordAt(r,victim+0x254u);
        const unsigned airFlag=wordAt(r,victim+0xaau)>>8u;
        const bool nativeLanded=floor>0u && floor<2048u &&
            wordAt(r,victim+0x10u)>=floor && airFlag==0u;
        if(!combat || nativeLanded)stormFlightUsedUntilLanding=false;
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
        // Require an observed native launch action/profile, not just a hit
        // counter, airborne Y, or a soft hit-reaction profile.
        if(combat && d.reaction==0x000Cu && d.y<472u &&
           nativeLaunchReaction(d.victimActionBefore,d.victimProfileBefore))nativeLaunched=true;
        d.nativeLaunched=nativeLaunched;
        d.airAttackStart=attackNow && (!prevAttackNow || d.attackerAction!=prevAttackerAction || d.buttonEdge);
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
            // A new hit invalidates any older pending severe-velocity write.
            pendingSevere=false;pendingStormRepeatHK=false;pendingAge=1000u;
            // Bind each button press to at most one confirmed hit. Do not let a
            // prior HK be reused when the player subsequently performs an MK.
            d.buttonAge=recentButtonAge;
            d.buttonEvidence=pressSequence!=consumedSequence && recentButtonAge<=30u && recentButton!=0u;
            d.taggedButton=d.buttonEvidence?recentButton:0u;
            consumedSequence=pressSequence;
            d.stormMediumButton=d.taggedButton==(1u<<4); // native MK
            d.stormHeavyButton=d.taggedButton==(1u<<2) || d.taggedButton==(1u<<5); // HP/HK
            d.onset=true;
            d.hadConnectedAirHit=continuity && connectedAirHits>0u;
            d.openingFollowup=!d.hadConnectedAirHit;
            d.provisionalHeavy=heavyAirAttack(d.attackerAction);
            d.characterHPExcluded=excludedCharacterHP(d.attackerId,d.attackerAction,d.taggedButton);
            // The 87.84 soft-profile medium shortcut is gone. Even exact ROM
            // signatures are descriptive only, never an independent write gate.
            const bool medium=d.attackerAction==0x1802u || d.attackerAction==0x1808u ||
                              d.attackerAction==0x1A02u || d.attackerAction==0x1A08u;
            const Signature* known=match(d.attackerBank,d.attackerAction,d.attackerPointer,
                                         d.victimActionBefore,d.victimProfileBefore);
            d.signatureKnown=known!=NULL;
            const bool preserveCyclops=known && known->preserveNative;
            // Stage87.85 excluded all Storm 1A04, which also blocked HK.
            // Driver button edges distinguish MK from HP/HK without relying on
            // the shared action/pointer. Absent evidence remains protected.
            const bool stormShared=d.attackerBank==0x0022u && d.attackerAction==0x1A04u;
            if(stormShared)d.provisionalHeavy=d.stormHeavyButton;
            const bool stormMedium=stormShared && d.stormMediumButton;
            const bool stormUnidentified=stormShared && !d.stormHeavyButton && !d.stormMediumButton;
            // Update the count AFTER evaluating candidate status: a first
            // connected heavy is still the FIRST hit, never a finisher.
            // Storm: confirmed connected HP/HK, followed by HK in one aerial
            // combo, must finish with native severe knockdown even if no 1C14/0210.
            // Other fighters and Storm's first heavy/medium hits remain native.
            const bool stormConfirmedHeavy=d.attackerId==0x0004u && d.attackerBank==0x0022u &&
                d.stormHeavyButton && d.buttonEvidence &&
                (d.attackerAction==0x1A04u || d.attackerAction==0x1804u);
            const bool stormRepeatHK=stormConfirmedHeavy && d.taggedButton==(1u<<5) &&
                connectedStormHeavyHits>0u && d.hadConnectedAirHit;
            d.heavyFollowup=d.hadConnectedAirHit && (d.nativeLaunched || stormRepeatHK) &&
                d.provisionalHeavy && !medium && paired &&
                !isDrillCandidate(d.attackerAction) && !d.characterHPExcluded;
            if(stormConfirmedHeavy)++connectedStormHeavyHits;
            d.candidate=d.heavyFollowup;
            ++connectedAirHits;
            lastConfirmedSerial=attackSerial;
            d.connectedAfter=connectedAirHits;
            if(isDrillCandidate(d.attackerAction)){
                d.excluded=true;d.outcome="DRILL_CLAW_PRESERVED";
            }else if(stormMedium){
                d.excluded=true;d.outcome="STORM_MK_BUTTON_PRESERVED";
            }else if(stormUnidentified){
                d.excluded=true;d.outcome="STORM_BUTTON_UNIDENTIFIED_PRESERVED";
            }else if(preserveCyclops){
                d.excluded=true;d.outcome="CYCLOPS_NATIVE_SLAM_PRESERVED";
            }else if(d.characterHPExcluded){
                d.excluded=true;d.outcome="SABRETOOTH_CAMMY_HP_NATIVE_PRESERVED";
            }else if(!d.nativeLaunched && !stormRepeatHK){
                d.excluded=true;d.outcome="VICTIM_NOT_NATIVE_LAUNCHED";
            }else if(medium){
                d.excluded=true;d.outcome="MEDIUM_NATIVE_PRESERVED";
            }else if(!d.hadConnectedAirHit){
                d.excluded=true;d.outcome="FIRST_CONNECTED_AIR_HIT_PRESERVED";
            }else if(!d.provisionalHeavy){
                d.excluded=true;d.outcome="NOT_HEAVY_NATIVE_PRESERVED";
            }else if(!paired){
                d.excluded=true;d.outcome="UNKNOWN_REACTION_PRESERVED";
            }else if(stormFlightUsedUntilLanding){
                d.excluded=true;
                d.outcome="STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING";
            }else if(!enabled){
                d.outcome="TEST_DISABLED_PRESERVED";
            }else{
                // Confirm the combo NOW but wait for the game's native launch
                // state before writing. At substate 0, the original game may
                // still overwrite velocity on the following frames.
                pendingSevere=enabled;
                pendingStormRepeatHK=stormRepeatHK;
                pendingAge=0;
                pendingAction=d.attackerAction;
                pendingBank=d.attackerBank;
                pendingAttackerId=d.attackerId;
                d.outcome=enabled?"CONNECTED_HEAVY_HIT_LATCHED":"TEST_DISABLED_PRESERVED";
            }
        }else{
            d.connectedAfter=connectedAirHits;
            if(d.airAttackStart)d.outcome="AIR_ATTACK_ATTEMPT_UNCONFIRMED";
        }
        if(pendingSevere && !d.hitConfirmed){
            ++pendingAge;
            const bool stale=stormFlightUsedUntilLanding || pendingAge>24u || !combat || !attackerAir ||
              d.reaction!=0x000Cu || d.y>=472u ||
              d.attackerAction!=pendingAction || d.attackerBank!=pendingBank ||
              d.attackerId!=pendingAttackerId || (!nativeLaunched && !pendingStormRepeatHK);
            if(stale){
                pendingSevere=false;
                pendingStormRepeatHK=false;
                d.outcome="LATCH_CANCELLED_BEFORE_NATIVE_LAUNCH";
            }else if(d.substate==2u && paired){
                pendingSevere=false;
                pendingStormRepeatHK=false;
                d.heavyFollowup=true;
                d.candidate=true;
                const std::int64_t speed=signedFixed(d.beforeX);
                const bool nativeSteep=severe && signedFixed(d.beforeY)>=0x00100000LL &&
                  magnitude(speed)<=0x000A0000LL;
                if(nativeSteep){
                    d.excluded=true;
                    d.outcome="NATIVE_STEEP_SLAM_PRESERVED";
                }else{
                    // Read the native Storm HP velocity captured BEFORE this
                    // host TEST writer, or use the previously observed 87.80
                    // Storm horizontal/vertical pair until a live HP ref exists.
                    // The original ROM's victim reaction and +E0 flight index
                    // stay authoritative: no synthetic Cyclops Down+MP slam.
                    d.launch=true;
                    // Charlie's native 68000 pre-host reactions in 87.223
                    // are ordinary 1C16/0220 soft hit; no verified
                    // character-specific severe-knockdown writer appears
                    // in this capture. Test the requested 1.5x motion
                    // on ONLY his two confirmed flying-screen heavies.
                    const bool charlieFS=charlieAirHeavyFS(
                        d.attackerId,d.attackerBank,pendingAction);
                    const unsigned magnitudeX=charlieFS?
                        charlieFS150(XM87223Flight::flightVX(),0x00300000u):
                        XM87223Flight::flightVX();
                    const unsigned magnitudeY=XM87223Flight::flightVY();
                    const std::uint32_t attackerX=rawFixed(r,attacker+0x0cu);
                    const std::uint32_t victimX=rawFixed(r,victim+0x0cu);
                    const bool left=speed<0 || (speed==0 && victimX<attackerX);
                    d.afterX=left?(0u-magnitudeX):magnitudeX;
                    // Keep stronger existing *downward* native momentum,
                    // but do not add the old forced 19.16-pixel dive.
                    const std::int64_t oldY=signedFixed(d.beforeY);
                    d.afterY=(oldY>std::int64_t(magnitudeY) &&
                              oldY<0x00080000LL)?d.beforeY:magnitudeY;
                    if(charlieFS)
                        d.afterY=charlieFS150(d.afterY,0x000c0000u);
                    // Reuse Storm's NATIVE 68000 severe reaction pair,
                    // including the flight index, but NOT the Cyclops
                    // Down+MP 0x00132A00 downward override. This is an
                    // experimental fairness test, not an OTG permission patch.
                    // The native match-start hit gates and floor handling
                    // remain exactly as they were.
                    const unsigned fsAction=XM87223Flight::flightAction();
                    const unsigned fsIndex=XM87223Flight::flightIndex();
                    setWord(r,victim+0x60u,fsAction);
                    setWord(r,victim+0xe0u,fsIndex);
                    applyStormLikeMotion(r,victim,d.afterX,d.afterY);
                    stormFlightUsedUntilLanding=true;
                    d.velocityWritten=true;
                    d.stateWritten=(d.victimActionBefore!=fsAction ||
                                    d.victimProfileBefore!=fsIndex);
                    d.outcome=charlieFS?
                        "CHARLIE_AIR_HEAVY_FS_150PCT_NATIVE_INDEX":
                        (XM87223Flight::flightLearned()?
                        "STORM_HP_RECORDED_FS_INDEX_AND_VELOCITY":
                        "STORM_HP_NATIVE_SIGNATURE_FS_INDEX_AND_VELOCITY");
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
    Actor actor[2];bool initialized,enabled;int lastMode;std::FILE* log;
    Controller():initialized(false),enabled(false),lastMode(-1),log(NULL){}
    ~Controller(){if(log)std::fclose(log);}
    void init(){
        initialized=true;
        const char* on=std::getenv("XM8786_STORM_BUTTON_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        const char* path=std::getenv("XM8786_STORM_BUTTON_LOG_FILE");
        if(path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\tattacker_pointer\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\trom_signature_matched\tfirst_air_followup\tair_attack_index\theavy_followup\tnew_air_attack\tconnected_hits_before\tconnected_hits_after\tcombo_continuous\thit_confirmed\tvelocity_written\tstate_pair_written\texcluded\tbutton_bits\tbutton_tag\tbutton_age\tbutton_evidence\tstorm_mk\tstorm_heavy\tdecision\tattacker_id\tvictim_id\tobserved_native_launch\tcharacter_hp_excluded\n",log);
        }
        std::fprintf(stderr,"XM87.223 STORM_NATIVE_FS_LAUNCH_READY enabled=%d fallback_x=0010AA00 fallback_y=0000B000 index=0230 one_launch_until_landing=1 protected_drill=2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r,unsigned p1Buttons,unsigned p2Buttons){
        if(!initialized)init();
        // XM87.123: this controller has RAM-writing flying-screen overrides.
        // A runner may enable its experiment flag in either ruleset, so the
        // actual live Classic/Tournament selection MUST also authorize writes.
        // Re-evaluate each frame because the Options menu changes the mode
        // without restarting the process. Missing/unknown means Classic.
        const char* activeMode=std::getenv("XM8755_TOURNAMENT_MODE");
        const bool tournament=activeMode && std::strcmp(activeMode,"1")==0;
        if(lastMode!=int(tournament)){
            // Never carry a connected-hit latch between different rulesets.
            actor[0]=Actor();actor[1]=Actor();
            lastMode=int(tournament);
            std::fprintf(stderr,"XM87.123 CLASSIC_FLYING_SCREEN_GUARD mode=%s launch_override=%u\n",
                tournament?"TOURNAMENT":"CLASSIC",unsigned(tournament && enabled));
        }
        // In Classic, ROM-native reaction, velocity and flying-screen state
        // are authoritative; do not even evaluate this experimental writer.
        if(!tournament)return;
        if(!r)return;
        const bool combat=byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u;
        for(unsigned i=0;i<2u;++i){
            Decision d=actor[i].evaluate(r,i?0x4400u:0x4000u,i?0x4000u:0x4400u,enabled,combat,i?p1Buttons:p2Buttons);
            if(!d.hitConfirmed && !d.airAttackStart && !d.launch && !(d.excluded && d.candidate))continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\t%04X\t%04X\t%u\t%u\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.signatureKnown),unsigned(d.openingFollowup),d.airAttackIndex,
                  unsigned(d.heavyFollowup),unsigned(d.airAttackStart),d.connectedBefore,d.connectedAfter,unsigned(d.comboContinuous),unsigned(d.hitConfirmed),unsigned(d.velocityWritten),unsigned(d.stateWritten),unsigned(d.excluded),d.buttonBits,d.taggedButton,d.buttonAge,
                  unsigned(d.buttonEvidence),unsigned(d.stormMediumButton),unsigned(d.stormHeavyButton),d.outcome,
                  d.attackerId,d.victimId,unsigned(d.nativeLaunched),unsigned(d.characterHPExcluded));
                std::fflush(log);
            }
            if(d.launch)std::fprintf(stderr,"XM87.86 FLIGHT frame=%u victim=P%u bank=%04X action=%04X anim=%04X decision=%s\n",
                tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.outcome);
        }
    }
};
static inline Controller& state(){static Controller s;return s;}
} // namespace XM8786Flight
#endif
