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
#include "xmvsf_stage87_262_game_version.h"

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
// TEST 87.226: two independent native-reference flying-screen writers.
// (1) Cyclops down+MP severe variant: 1C18/0230 and 0x00132A00 dive.
static inline void applyCyclopsSevereFlight(std::uint8_t* r,unsigned victim,
                                             std::uint32_t horizontal){
    setWord(r,victim+0x60u,SEVERE_ACTION);
    setWord(r,victim+0xE0u,SEVERE_PROFILE);
    setFixed(r,victim+0x14u,horizontal);
    setFixed(r,victim+0x1Cu,TARGET_DOWN);
}
// (2) Storm original-style flying-screen: native Storm 68000
// reaction/index plus measured, gentle X/Y launch velocity.
// Keep separate from the Cyclops severe slam; do not mix their Y values.
static inline void applyStormFlyingScreen(std::uint8_t* r,unsigned victim,
                                          std::uint32_t horizontal,
                                          std::uint32_t vertical){
    setWord(r,victim+0x60u,XM87223Flight::flightAction());
    setWord(r,victim+0xE0u,XM87223Flight::flightIndex());
    setFixed(r,victim+0x14u,horizontal);
    setFixed(r,victim+0x1cu,vertical);
}
// Original 68000 Charlie fighter ID=0020 and ROM bank=0028.
// 87.225 recording confirms air HP=1804 with button-edge 0004 and
// his super-jump HK=1A04 with physical button-edge 0020.
// The shared 1A04 action alone is NOT proof of HK (Storm's differs).
static inline bool charlieNativeAirHP(unsigned fighter,unsigned bank,
                                      unsigned action){
    return fighter==0x0020u && bank==0x0028u && action==0x1804u;
}
static inline bool charlieAirHK(unsigned fighter,unsigned bank,
                                unsigned action,unsigned button){
    return fighter==0x0020u && bank==0x0028u &&
           button==(1u<<5u) &&
           (action==0x1a04u || action==0x180au || action==0x1a0au);
}
static inline bool isDrillCandidate(unsigned action){return action==0x2C1Eu || action==0x2C46u;}
static inline bool normalAirAttack(unsigned action){
    return ((action&0xFFF0u)==0x1800u || (action&0xFFF0u)==0x1A00u) &&
           (action&1u)==0 && (action&0x000Eu)<=0x000Au;
}
static inline bool heavyAirAttack(unsigned action){
    return action==0x1804u || action==0x180Au || action==0x1A04u || action==0x1A0Au;
}
// Ground normals are 100x/120x/140x, never super-jump 1Axx.
// Include ordinary light/medium hits in the PREVIOUS CONNECTED HIT
// ledger so a landed MP between HP and HK correctly breaks the chain.
static inline bool groundedNormalAttack(unsigned action){
    const unsigned group=action&0xfff0u;
    const unsigned suffix=action&0x000eu;
    return (group==0x1000u || group==0x1200u || group==0x1400u) &&
           suffix<=0x000au && !(action&1u);
}
static inline bool groundedNormalHeavy(unsigned action){
    const unsigned suffix=action&0x000eu;
    return groundedNormalAttack(action) &&
           (suffix==0x0004u || suffix==0x000au);
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
    unsigned previousHitButton=0u,groundedAttacker=0u,activeFS=0u;
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
    // TEST 87.230: the previous CONNECTED hit, not merely the preceding
    // button press, decides whether an HK during live FS becomes severe.
    // Grounded normal HP/HK during FS is a second independent rule.
    bool postFlightNeedsNativeLanding=false;
    bool fsConditionalPending=false,fsPendingGround=false,fsPendingSevere=false,fsPendingCharlieHK=false;
    bool fsPendingOriginGrounded=false;
    unsigned fsPendingAge=1000u,fsPendingAction=0u,fsPendingSerial=0u;
    unsigned fsPendingPreviousHitButton=0u;
    unsigned fsPendingAttacker=~0u,fsPendingVictim=~0u;
    unsigned lastConnectedButton=0u,lastConnectedAttacker=~0u,lastConnectedVictim=~0u;
    unsigned prevAction,prevProfile,prevSubstate,prevReaction,prevVictimY;
    unsigned prevAttackerAction,attackSerial,lastConfirmedSerial,connectedAirHits,connectedStormHeavyHits,attackAge;
    unsigned pendingAge,pendingAction,pendingButton,pendingBank,pendingAttackerId,lastAttackerId,lastVictimId;
    unsigned previousButtons,recentButton,recentButtonAge,pressSequence,consumedSequence;
    Actor():observed(false),prevAttackNow(false),pendingSevere(false),nativeLaunched(false),pendingStormRepeatHK(false),prevAction(0),prevProfile(0),prevSubstate(0),
      prevReaction(0),prevVictimY(0),prevAttackerAction(0),attackSerial(0),
      lastConfirmedSerial(0),connectedAirHits(0),connectedStormHeavyHits(0),attackAge(1000),pendingAge(1000),pendingAction(0),pendingButton(0),pendingBank(0),pendingAttackerId(0),lastAttackerId(0),lastVictimId(0),
      previousButtons(0),recentButton(0),recentButtonAge(1000),pressSequence(0),consumedSequence(0){}
    void clearChain(){
        connectedAirHits=0;connectedStormHeavyHits=0;attackSerial=0;
        lastConfirmedSerial=0;attackAge=1000;prevAttackNow=false;
        pendingSevere=false;pendingStormRepeatHK=false;
        pendingAge=1000;pendingButton=0;nativeLaunched=false;
        fsConditionalPending=false;fsPendingSevere=false;fsPendingCharlieHK=false;
        fsPendingAge=1000u;
        fsPendingAction=0u;fsPendingSerial=0u;fsPendingGround=false;
        // This air-sequence gate deliberately survives ordinary combo
        // gaps. Only tag-out, a confirmed landing, or match exit clears
        // postFlightNeedsNativeLanding.
    }
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
        if(observed && d.victimId!=lastVictimId){
            stormFlightUsedUntilLanding=false; // a tagged-in victim is new
            postFlightNeedsNativeLanding=false;
        }
        if(observed && (d.attackerId!=lastAttackerId ||
                        d.victimId!=lastVictimId)){
            lastConnectedButton=0u;
            lastConnectedAttacker=~0u;
            lastConnectedVictim=~0u;
        }
        if(observed && (d.attackerId!=lastAttackerId || d.victimId!=lastVictimId))clearChain();
        lastAttackerId=d.attackerId;lastVictimId=d.victimId;
        // A single synthetic flight boost per continuous aerial sequence.
        // Re-arm only at a REAL native landing, not upon hitstun transitions.
        const unsigned floor=wordAt(r,victim+0x254u);
        const unsigned airFlag=wordAt(r,victim+0xaau)>>8u;
        const bool nativeLanded=floor>0u && floor<2048u &&
            wordAt(r,victim+0x10u)>=floor && airFlag==0u;
        if(!combat || nativeLanded){
            stormFlightUsedUntilLanding=false;
            postFlightNeedsNativeLanding=false;
            fsConditionalPending=false;
            fsPendingSevere=false;fsPendingCharlieHK=false;
            fsPendingAge=1000u;
            lastConnectedButton=0u;
            lastConnectedAttacker=~0u;
            lastConnectedVictim=~0u;
        }
        d.attackerBank=wordAt(r,attacker+0x40u);
        d.attackerAction=wordAt(r,attacker+0x60u);
        d.attackerPointer=wordAt(r,attacker+0x42u);
        d.victimActionBefore=wordAt(r,victim+0x60u);
        d.victimProfileBefore=wordAt(r,victim+0xE0u);
        d.afterX=d.beforeX;d.afterY=d.beforeY;
        const unsigned attackerY=wordAt(r,attacker+0x10u);
        const unsigned attackerFloor=wordAt(r,attacker+0x254u);
        const unsigned groundY=attackerFloor>0u && attackerFloor<2048u?
            attackerFloor:472u;
        const bool attackerAir=attackerY>0u && attackerY<472u;
        const bool attackerGrounded=(attackerY>=groundY ||
            attackerY==0u) && !attackerAir &&
            wordAt(r,attacker)!=0u &&
            (wordAt(r,attacker+0xaau)>>8u)==0u;
        const bool groundAttackNow=attackerGrounded &&
            groundedNormalAttack(d.attackerAction);
        const bool groundHeavyNow=groundAttackNow &&
            groundedNormalHeavy(d.attackerAction);
        d.groundedAttacker=unsigned(attackerGrounded);
        const bool attackNow=(attackerAir &&
            (normalAirAttack(d.attackerAction)||isDrillCandidate(d.attackerAction))) ||
            groundAttackNow;
        // A gap in native reaction, victim landing or attacker landing ends the
        // linked air combo; one later heavy cannot inherit an old hit counter.
        const bool continuity=observed && combat && attackerAir && d.y<472u &&
          d.reaction==0x000Cu && prevReaction==0x000Cu && prevVictimY<472u;
        d.comboContinuous=continuity;
        // Once an original ROM impact is confirmed, do not discard its
        // pending conversion merely because the attacker lands/jumps while
        // the SAME move is settling. Eligibility was frozen at hit onset;
        // identity/action/serial changes still cancel below.
        const bool samePendingAttack=fsConditionalPending && combat &&
            !nativeLanded && d.attackerId==fsPendingAttacker &&
            d.victimId==fsPendingVictim &&
            d.attackerAction==fsPendingAction;
        if((!combat || (!attackerAir && !groundAttackNow) ||
            d.y>=472u || (observed && d.reaction!=0x000Cu)) &&
            !samePendingAttack)clearChain();
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
        // A real ROM-native severe flight also qualifies as existing FS,
        // but only when its velocity is inside the recorded FS range.
        // No plain 1C18/0230 knockdown is enough on its own.
        if(combat && !nativeLanded && severe && d.reaction==0x000cu &&
           d.y<472u && magnitude(signedFixed(d.beforeX))>=0x00060000LL &&
           magnitude(signedFixed(d.beforeX))<=0x00300000LL)
            postFlightNeedsNativeLanding=true;
        d.activeFS=unsigned(stormFlightUsedUntilLanding ||
                             postFlightNeedsNativeLanding);
        d.previousHitButton=lastConnectedButton;
        const bool stateChanged=!observed || prevAction!=d.victimActionBefore || prevProfile!=d.victimProfileBefore;
        // Fresh victim hit onset: game transition from native hit substate 2 to 0,
        // or a new hit reaction. It must coincide with a NEW airborne move.
        const bool freshReaction=observed && d.substate==0u &&
          (prevSubstate==2u || prevReaction!=d.reaction) &&
          (stateChanged || prevSubstate==2u);
        d.hitConfirmed=combat && attackNow && d.reaction==0x000Cu &&
          d.y<472u && freshReaction && attackAge<=24u && attackSerial!=lastConfirmedSerial;
        if(d.hitConfirmed){
            // Any new confirmed hit cancels the older pending decision:
            // an old HP/HK cannot leak into a subsequent LP/LK/MP.
            fsConditionalPending=false;
            fsPendingSevere=false;fsPendingCharlieHK=false;
            fsPendingAge=1000u;
            // A new hit invalidates any older pending Storm conversion.
            pendingSevere=false;pendingStormRepeatHK=false;pendingAge=1000u;pendingButton=0u;
            d.buttonAge=recentButtonAge;
            d.buttonEvidence=pressSequence!=consumedSequence &&
                recentButtonAge<=30u && recentButton!=0u;
            d.taggedButton=d.buttonEvidence?recentButton:0u;
            consumedSequence=pressSequence;
            const bool previousWasHP=lastConnectedButton==(1u<<2u) &&
                lastConnectedAttacker==d.attackerId &&
                lastConnectedVictim==d.victimId;
            const bool flightAlready=postFlightNeedsNativeLanding ||
                stormFlightUsedUntilLanding;
            const bool hpThenHK=flightAlready && previousWasHP &&
                d.buttonEvidence && d.taggedButton==(1u<<5u) &&
                attackerAir && heavyAirAttack(d.attackerAction) &&
                !isDrillCandidate(d.attackerAction);
            const bool groundedFSHeavy=flightAlready && groundHeavyNow &&
                d.buttonEvidence &&
                (d.taggedButton==(1u<<2u) ||
                 d.taggedButton==(1u<<5u)) &&
                !excludedCharacterHP(d.attackerId,d.attackerAction,
                                     d.taggedButton);
            // Regardless of whether this hit becomes a finisher, the
            // preceding-hit identity is now this confirmed native hit.
            // A connecting light or medium breaks the HP->HK sequence.
            // A special such as Wolverine's Drill Claw does not count
            // as a preceding NORMAL HP, even if the physical HP button
            // happens to be pressed at impact.
            lastConnectedButton=(normalAirAttack(d.attackerAction) ||
                groundAttackNow)?d.taggedButton:0u;
            lastConnectedAttacker=d.attackerId;
            lastConnectedVictim=d.victimId;
            // 87.231: FS is a FLOOR, not a once-per-landing permission
            // to hit the victim. EVERY newly connected normal during an
            // existing FS gets its own Storm baseline at that hit's native
            // substate 2. Only the two explicit finishers upgrade to Cyclops.
            // Keep protected original native moves untouched.
            const Signature* fsSignature=match(d.attackerBank,d.attackerAction,
                d.attackerPointer,d.victimActionBefore,d.victimProfileBefore);
            const bool protectedFSNative=
                charlieNativeAirHP(d.attackerId,d.attackerBank,d.attackerAction) ||
                excludedCharacterHP(d.attackerId,d.attackerAction,d.taggedButton) ||
                isDrillCandidate(d.attackerAction) ||
                (fsSignature && fsSignature->preserveNative);
            const bool fsStormBaseline=flightAlready && !protectedFSNative &&
                (normalAirAttack(d.attackerAction) || groundAttackNow);
            if(enabled && fsStormBaseline){
                fsConditionalPending=true;
                // Charlie's separately accepted first HK severe is an
                // intentional exception to the general HP->HK upgrade.
                fsPendingCharlieHK=charlieAirHK(d.attackerId,d.attackerBank,
                                                d.attackerAction,d.taggedButton);
                fsPendingSevere=hpThenHK || groundedFSHeavy || fsPendingCharlieHK;
                fsPendingGround=groundedFSHeavy;
                fsPendingOriginGrounded=attackerGrounded;
                fsPendingPreviousHitButton=d.previousHitButton;
                fsPendingAge=0u;
                fsPendingAction=d.attackerAction;
                fsPendingSerial=attackSerial;
                fsPendingAttacker=d.attackerId;
                fsPendingVictim=d.victimId;
            }
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
            if(fsConditionalPending){
                d.candidate=true;
                d.heavyFollowup=fsPendingSevere;
                d.outcome=!fsPendingSevere?
                    "FS_STORM_HIT_CONFIRMED_PENDING":
                    (fsPendingCharlieHK?"FS_CHARLIE_HK_CONFIRMED_PENDING":
                    (fsPendingGround?"FS_GROUNDED_HEAVY_CONFIRMED_PENDING":
                    "FS_HP_THEN_HK_CONFIRMED_PENDING"));
            }else if(isDrillCandidate(d.attackerAction)){
                d.excluded=true;d.outcome="DRILL_CLAW_PRESERVED";
            }else if(stormMedium){
                d.excluded=true;d.outcome="STORM_MK_BUTTON_PRESERVED";
            }else if(stormUnidentified){
                d.excluded=true;d.outcome="STORM_BUTTON_UNIDENTIFIED_PRESERVED";
            }else if(preserveCyclops){
                d.excluded=true;d.outcome="CYCLOPS_NATIVE_SLAM_PRESERVED";
            }else if(d.characterHPExcluded){
                d.excluded=true;d.outcome="SABRETOOTH_CAMMY_HP_NATIVE_PRESERVED";
            }else if(charlieNativeAirHP(d.attackerId,d.attackerBank,d.attackerAction)){
                // Charlie HP must remain EXACTLY ROM-original: no Storm
                // substitution, no elevated velocity, no artificial FS.
                // The connected-air-hit counter still advances normally,
                // so a later confirmed HK can use Cyclops severe.
                d.excluded=true;d.outcome="CHARLIE_AIR_HP_NATIVE_UNCHANGED";
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
            }else if(stormFlightUsedUntilLanding ||
                     postFlightNeedsNativeLanding){
                // This fallback is only for unexpected/unsupported moves.
                // Every supported connected FS normal now takes the
                // 87.231 Storm floor / severe upgrade above.
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
                pendingButton=d.taggedButton;
                pendingBank=d.attackerBank;
                pendingAttackerId=d.attackerId;
                d.outcome=enabled?"CONNECTED_HEAVY_HIT_LATCHED":"TEST_DISABLED_PRESERVED";
            }
        }else{
            d.connectedAfter=connectedAirHits;
            if(d.airAttackStart)d.outcome="AIR_ATTACK_ATTEMPT_UNCONFIRMED";
        }
        // 87.231: resolve EACH FS hit on its own original ROM impact.
        // Normal FS hits receive Storm's gentle knockdown; only the
        // confirmed HP->HK and grounded HP/HK cases receive Cyclops severe.
        // Never carry either variant into the next move/attack serial.
        if(fsConditionalPending && !d.hitConfirmed){
            if(fsPendingAge<1000u)++fsPendingAge;
            const bool sameHit=d.attackerId==fsPendingAttacker &&
                d.victimId==fsPendingVictim &&
                d.attackerAction==fsPendingAction &&
                attackSerial==fsPendingSerial && !d.airAttackStart;
            const bool nativePair=paired ||
                nativeLaunchReaction(d.victimActionBefore,
                                     d.victimProfileBefore);
            const bool expired=fsPendingAge>24u || !enabled || !combat ||
                !sameHit || nativeLanded ||
                !(postFlightNeedsNativeLanding || stormFlightUsedUntilLanding) ||
                d.reaction!=0x000cu || d.y>=472u;
            if(expired){
                fsConditionalPending=false;
                fsPendingSevere=false;fsPendingCharlieHK=false;
                fsPendingAge=1000u;
                d.outcome="FS_HIT_CONVERSION_CANCELLED";
            }else if(d.substate==2u && nativePair){
                d.candidate=true; // also log native-steep preservation
                // Report the preceding hit from the originating substate-0
                // snapshot, not the now-updated lastConnectedButton ledger.
                d.previousHitButton=fsPendingPreviousHitButton;
                d.groundedAttacker=unsigned(fsPendingOriginGrounded);
                const bool originalSteep=severe &&
                    signedFixed(d.beforeY)>=0x00100000LL &&
                    magnitude(signedFixed(d.beforeX))<=0x000a0000LL;
                if(originalSteep){
                    // Native already-severe is always authoritative.
                    d.excluded=true;
                    d.outcome="FS_ALREADY_NATIVE_SEVERE_PRESERVED";
                }else{
                    const std::int64_t vx=signedFixed(d.beforeX);
                    const std::uint32_t ax=rawFixed(r,attacker+0x0cu);
                    const std::uint32_t vxpos=rawFixed(r,victim+0x0cu);
                    const bool left=vx<0 || (vx==0 && vxpos<ax);
                    if(fsPendingSevere){
                        d.afterX=left?TARGET_LEFT:TARGET_RIGHT;
                        d.afterY=TARGET_DOWN;
                        applyCyclopsSevereFlight(r,victim,d.afterX);
                        d.outcome=fsPendingCharlieHK?
                            "CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN":
                            (fsPendingGround?"FS_GROUNDED_HEAVY_CYCLOPS_SEVERE":
                            "FS_HP_THEN_HK_CYCLOPS_SEVERE");
                    }else{
                        const unsigned stormX=XM87223Flight::flightVX();
                        const unsigned stormY=XM87223Flight::flightVY();
                        d.afterX=left?(0u-stormX):stormX;
                        const std::int64_t oldY=signedFixed(d.beforeY);
                        d.afterY=(oldY>std::int64_t(stormY) &&
                                  oldY<0x00080000LL)?d.beforeY:stormY;
                        applyStormFlyingScreen(r,victim,d.afterX,d.afterY);
                        d.outcome="FS_STORM_KNOCKDOWN";
                    }
                    d.launch=true;
                    d.velocityWritten=true;
                    d.stateWritten=d.victimActionBefore!=SEVERE_ACTION ||
                        d.victimProfileBefore!=SEVERE_PROFILE;
                    stormFlightUsedUntilLanding=true;
                    postFlightNeedsNativeLanding=true;
                }
                fsConditionalPending=false;
                fsPendingSevere=false;fsPendingCharlieHK=false;
                fsPendingAge=1000u;
            }
        }
        if(pendingSevere && !d.hitConfirmed){
            ++pendingAge;
            // A source-native steep Cyclops slam must reach the
            // existing NATIVE_STEEP_SLAM_PRESERVED branch below. Do not
            // cancel it merely because its genuine 1C18/0230+VX pair
            // was recognized as active FS this frame.
            const bool alreadyNativeSteep=severe &&
                signedFixed(d.beforeY)>=0x00100000LL &&
                magnitude(signedFixed(d.beforeX))<=0x000a0000LL;
            const bool stale=stormFlightUsedUntilLanding ||
              (postFlightNeedsNativeLanding && !alreadyNativeSteep) ||
              pendingAge>24u || !combat || !attackerAir ||
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
                    // TEST 87.226: distinct native flying-screen variants.
                    // Charlie's connected HK uses the same Cyclops severe
                    // knockdown as Down+MP; Charlie HP is explicitly excluded
                    // earlier and remains ROM-original.
                    d.launch=true;
                    const bool charlieHK=charlieAirHK(
                        d.attackerId,d.attackerBank,pendingAction,pendingButton);
                    const std::uint32_t attackerX=rawFixed(r,attacker+0x0cu);
                    const std::uint32_t victimX=rawFixed(r,victim+0x0cu);
                    const bool left=speed<0 || (speed==0 && victimX<attackerX);
                    if(charlieHK){
                        // Cyclops severe: the original Tournament 87.86
                        // horizontal knockback plus the strong downward hit,
                        // not the new Storm x1.5 experimental velocity.
                        d.afterX=left?TARGET_LEFT:TARGET_RIGHT;
                        d.afterY=TARGET_DOWN;
                        applyCyclopsSevereFlight(r,victim,d.afterX);
                        d.outcome="CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN";
                    }else{
                        // Everyone else keeps the 87.223 Storm reference.
                        const unsigned magnitudeX=XM87223Flight::flightVX();
                        const unsigned magnitudeY=XM87223Flight::flightVY();
                        d.afterX=left?(0u-magnitudeX):magnitudeX;
                        const std::int64_t oldY=signedFixed(d.beforeY);
                        d.afterY=(oldY>std::int64_t(magnitudeY) &&
                                  oldY<0x00080000LL)?d.beforeY:magnitudeY;
                        applyStormFlyingScreen(r,victim,d.afterX,d.afterY);
                        d.outcome=XM87223Flight::flightLearned()?
                            "STORM_HP_RECORDED_FS_INDEX_AND_VELOCITY":
                            "STORM_HP_NATIVE_SIGNATURE_FS_INDEX_AND_VELOCITY";
                    }
                    stormFlightUsedUntilLanding=true;
                    // Both authorized variants require an ACTUAL native
                    // landing before a fresh 1C14/0210 relaunch is legal.
                    // Charlie's first HK remains the untouched Cyclops
                    // severe write; only subsequent native relaunches
                    // receive this new anti-infinite recovery.
                    postFlightNeedsNativeLanding=true;
                    d.velocityWritten=true;
                    const unsigned fsAction=charlieHK?
                        SEVERE_ACTION:XM87223Flight::flightAction();
                    const unsigned fsIndex=charlieHK?
                        SEVERE_PROFILE:XM87223Flight::flightIndex();
                    d.stateWritten=(d.victimActionBefore!=fsAction ||
                                    d.victimProfileBefore!=fsIndex);
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
            if(log)std::fputs("frame\tvictim\tattacker_bank\tattacker_action\tattacker_pointer\treaction\tsubstate\ty\tvictim_action_before\tvictim_profile_before\toriginal_vx\toriginal_vy\tfinal_vx\tfinal_vy\tvictim_action_after\tvictim_profile_after\trom_signature_matched\tfirst_air_followup\tair_attack_index\theavy_followup\tnew_air_attack\tconnected_hits_before\tconnected_hits_after\tcombo_continuous\thit_confirmed\tvelocity_written\tstate_pair_written\texcluded\tbutton_bits\tbutton_tag\tbutton_age\tbutton_evidence\tstorm_mk\tstorm_heavy\tdecision\tattacker_id\tvictim_id\tobserved_native_launch\tcharacter_hp_excluded\tprevious_confirmed_hit_button\tattacker_grounded\tfs_active\n",log);
        }
        std::fprintf(stderr,"XM87.231 FS_STORM_PER_HIT_SEVERE_PRIORITY_READY enabled=%d fallback_x=0010AA00 fallback_y=0000B000 index=0230 fs_latch_until_landing=1 protected_drill=2C1E,2C46\n",int(enabled));
    }
    void frame(unsigned tick,std::uint8_t* r,unsigned p1Buttons,unsigned p2Buttons){
        if(!initialized)init();
        // XM87.123: this controller has RAM-writing flying-screen overrides.
        // A runner may enable its experiment flag in either ruleset, so the
        // actual live Classic/Tournament selection MUST also authorize writes.
        // Re-evaluate each frame because the Options menu changes the mode
        // without restarting the process. Missing/unknown means Classic.
        const bool tournament=XM87262Game::tournamentKnockdowns();
        // No external TEST flag is required for the selected ruleset.
        // Tournament controls exactly this knockdown writer.
        enabled=tournament;
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
            if(!d.hitConfirmed && !d.airAttackStart && !d.launch &&
               !d.velocityWritten && !(d.excluded && d.candidate) &&
               std::strcmp(d.outcome,"FS_HIT_CONVERSION_CANCELLED")!=0)
                continue;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%u\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X\t%04X\t%04X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\t%04X\t%04X\t%u\t%u\t%u\t%u\t%u\n",
                  tick,i+1,d.attackerBank,d.attackerAction,d.attackerPointer,d.reaction,d.substate,d.y,
                  d.victimActionBefore,d.victimProfileBefore,d.beforeX,d.beforeY,d.afterX,d.afterY,
                  d.victimActionAfter,d.victimProfileAfter,unsigned(d.signatureKnown),unsigned(d.openingFollowup),d.airAttackIndex,
                  unsigned(d.heavyFollowup),unsigned(d.airAttackStart),d.connectedBefore,d.connectedAfter,unsigned(d.comboContinuous),unsigned(d.hitConfirmed),unsigned(d.velocityWritten),unsigned(d.stateWritten),unsigned(d.excluded),d.buttonBits,d.taggedButton,d.buttonAge,
                  unsigned(d.buttonEvidence),unsigned(d.stormMediumButton),unsigned(d.stormHeavyButton),d.outcome,
                  d.attackerId,d.victimId,unsigned(d.nativeLaunched),
                   unsigned(d.characterHPExcluded),
                   d.previousHitButton,d.groundedAttacker,d.activeFS);
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
