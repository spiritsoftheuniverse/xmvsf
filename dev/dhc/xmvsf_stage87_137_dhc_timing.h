#ifndef XMVSF_STAGE87_137_DHC_TIMING_H
#define XMVSF_STAGE87_137_DHC_TIMING_H
// Stage 87.137 experimental DHC timing coordinator. No emulator or game-RAM writes.
// Native host supplies VERIFIABLE signals: this class does not infer a super
// from action +A0 or from the visual portrait alone.
//
// Timing contract:
// * World/opponent/outgoing hit resolution must be held throughout blackout.
// * Incoming reserve completes its native tag arrival and SUPER STARTUP while
//   black is rendered. The host advances only permitted incoming logic.
// * READY means the *next* native incoming animation tick is the first active
//   attack tick, NOT that a guessed timer has expired.
// * Release the world freeze only at the next frame boundary; the first
//   visible, unfrozen native tick must become ACTIVE (zero visible startup).
// * Only one DHC per super chain. Meter is debited exactly once by host's
//   atomic native commit, never here. Invalid transitions require rollback.
namespace XM87137Timing {
enum Phase { IDLE, FIRST_SUPER, BLACKOUT_PENDING, TAG_ENTER,
             STARTUP, RELEASE_ARMED, INCOMING, LOCKED, FAILED };
struct Command {
    unsigned side=0, reserveId=0, superId=0;
    bool combat=false, outgoingSuperVerified=false;
    bool reserveReady=false, commandIsVerifiedReserveSuper=false;
    bool meterForOneMoreSuper=false;
};
struct Signal {
    bool combat=false;
    bool worldHeld=false;          // Opponent/world/global time AND hit resolution held
    bool blackPresented=false;     // At least one blackout frame presented
    bool nativeTagComplete=false;  // The reserve is now the incoming point fighter
    bool incomingSuperStarted=false;
    bool incomingStartupTick=false; // native startup animation advanced under blackout
    bool firstActiveOnNextTick=false; // move-specific script/hitbox transition proved
    bool firstActiveNow=false;       // asserted on the first RELEASED emulator tick
    bool incomingSuperEnded=false;  // native super action truly finished
    bool foreignWorldAdvanced=false; // invariant failure: opponent, hitbox, etc advanced
};
struct Result {
    bool holdWorld=false, drawBlack=false, tickIncomingOnly=false;
    bool releaseBeforeNextNativeFrame=false, rollbackRequired=false;
};
struct Coordinator {
    Phase phase=IDLE;
    unsigned side=0, reserveId=0, superId=0, startupTicks=0, blackoutFrames=0;
    bool didDHC=false, nativeCommitted=false, firstReleaseTick=false;

    void clear(){*this=Coordinator();}
    bool observeFirstSuper(bool combat,bool verifiedSuperStart,bool verifiedSuperEnd){
        if(!combat){clear();return false;}
        if(phase==IDLE && verifiedSuperStart){phase=FIRST_SUPER;return true;}
        if(phase==FIRST_SUPER && verifiedSuperEnd){clear();return true;}
        return false;
    }
    bool request(const Command& c){
        if(phase!=FIRST_SUPER || didDHC || !c.combat || !c.outgoingSuperVerified ||
           !c.reserveReady || !c.commandIsVerifiedReserveSuper ||
           !c.meterForOneMoreSuper || !c.superId || !c.reserveId || c.side>1u) return false;
        side=c.side;reserveId=c.reserveId;superId=c.superId;
        phase=BLACKOUT_PENDING;return true;
    }
    // Call only after an atomic operation that entered native freeze, staged
    // the partner, and charged one extra super stock (except infinite-meter).
    // If any piece fails the host MUST restore its pre-request transaction.
    bool nativeCommit(bool freezeActive,bool tagQueued,bool stockSettled){
        if(phase!=BLACKOUT_PENDING || !freezeActive || !tagQueued || !stockSettled){
            if(phase==BLACKOUT_PENDING)phase=FAILED;
            return false;
        }
        nativeCommitted=true;didDHC=true;phase=TAG_ENTER;return true;
    }
    Result tick(const Signal& s){
        Result r;
        if(!s.combat){clear();return r;}
        if(phase==FAILED){r.holdWorld=true;r.drawBlack=true;r.rollbackRequired=true;return r;}
        if(phase==BLACKOUT_PENDING){r.holdWorld=true;r.drawBlack=true;return r;}
        if(phase==TAG_ENTER || phase==STARTUP || phase==RELEASE_ARMED){
            r.holdWorld=true;r.drawBlack=true;
            // Freeze violation is NOT repairable by merely dropping darkness.
            if(!s.worldHeld || s.foreignWorldAdvanced){
                phase=FAILED;r.rollbackRequired=true;return r;
            }
            if(s.blackPresented)++blackoutFrames;
            if(phase==TAG_ENTER && s.nativeTagComplete)phase=STARTUP;
            if(phase==STARTUP){
                r.tickIncomingOnly=true;
                if(s.incomingStartupTick && s.incomingSuperStarted)++startupTicks;
                if(s.firstActiveOnNextTick){
                    // Do not release from a portrait exit or a guess. Incoming
                    // native super must have actually progressed during blackout.
                    if(s.incomingSuperStarted && startupTicks>0 && blackoutFrames>0)
                        phase=RELEASE_ARMED;
                    else {phase=FAILED;r.rollbackRequired=true;return r;}
                }
            }
            if(phase==RELEASE_ARMED){
                // Called after an emulated hidden startup frame. The host removes
                // darkness + freeze BEFORE the following, now-active frame.
                r.tickIncomingOnly=false;
                r.releaseBeforeNextNativeFrame=true;
                phase=INCOMING;firstReleaseTick=true;
            }
            return r;
        }
        if(phase==INCOMING){
            if(firstReleaseTick){
                firstReleaseTick=false;
                if(!s.firstActiveNow){
                    // First visible native gameplay frame was not an active hit.
                    // Required timing contract was violated.
                    phase=FAILED;r.rollbackRequired=true;return r;
                }
            }
            if(s.incomingSuperEnded)phase=LOCKED;
        }
        if(phase==LOCKED){
            // Fresh DHC cannot begin until a newly verified, separate super
            // has started; host must observe a full return to normal first.
            if(s.incomingSuperEnded){} // lock is not reset by repeated end samples
        }
        return r;
    }
    // Host calls only after verifying normal action has resumed, not on
    // cinematic disappearance or the super's first active hit.
    void newChainAfterNeutral(bool neutralConfirmed){
        if(phase==LOCKED && neutralConfirmed)clear();
    }
    void rollbackComplete(){
        if(phase==FAILED)clear(); // host restored pre-DHC state before this call
    }
};
} // namespace XM87137Timing
#endif
