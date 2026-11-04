#ifndef XMVSF_STAGE87_137_DHC_TIMING_H
#define XMVSF_STAGE87_137_DHC_TIMING_H
// Stage 87.137 experimental DHC timing coordinator. No emulator or game-RAM writes.
// Native host supplies VERIFIABLE signals: this class does not infer a super
// from action +A0 or from the visual portrait alone.
//
// Timing contract:
// * World/opponent/outgoing hit resolution must be held throughout DHC freeze.
// * The incoming fighter remains DRAWN: land at the outgoing actor's recorded
//   takeover position, then play native super startup in the darkened scene.
// * BYPASS the incoming DHC super's native portrait animation AND its timer,
//   not merely the sprite visibility. Keep native super startup and attack logic.
// * Suppress any residual incoming DHC portrait/splash sprites, never the
//   fighter, hitboxes, HUD, or ordinary non-DHC super portraits.
// * Host advances permitted incoming logic; not the frozen match.
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
    bool darkScenePresented=false; // Stage dimming actually presented; actors remain visible
    bool incomingPortraitVisible=false; // DHC-specific portrait filter failed if true
    bool nativePortraitIntroBypassed=false; // no hidden portrait timer may delay startup
    bool nativeTagComplete=false;  // The reserve is now the incoming point fighter
    bool incomingGrounded=false;   // Native tag entry completed its landing
    bool incomingAtTakeoverPosition=false; // Native world-space X matches outgoing point
    bool incomingSuperStarted=false;
    bool incomingStartupTick=false; // native startup animation advanced under blackout
    bool firstActiveOnNextTick=false; // move-specific script/hitbox transition proved
    bool firstActiveNow=false;       // asserted on the first RELEASED emulator tick
    bool incomingSuperEnded=false;  // native super action truly finished
    bool foreignWorldAdvanced=false; // invariant failure: opponent, hitbox, etc advanced
};
struct Result {
    bool holdWorld=false, dimStage=false, suppressIncomingPortrait=false;
    bool bypassIncomingPortraitIntro=false; // request native DHC-only intro bypass
    bool showIncomingFighter=false, tickIncomingOnly=false;
    bool releaseBeforeNextNativeFrame=false, rollbackRequired=false;
};
struct Coordinator {
    Phase phase=IDLE;
    unsigned side=0, reserveId=0, superId=0, startupTicks=0, cinematicFrames=0;
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
        // This is a DHC-scoped presentation policy. The host must identify
        // the incoming portrait objects precisely; don't suppress every P7
        // sprite or normal super's splash. World is dim, not opaque black.
        if(didDHC && phase>=TAG_ENTER && phase<=INCOMING)
            r.suppressIncomingPortrait=true;
        if(phase==FAILED){r.holdWorld=true;r.dimStage=true;r.rollbackRequired=true;return r;}
        if(phase==BLACKOUT_PENDING){r.holdWorld=true;r.dimStage=true;return r;}
        if(phase==TAG_ENTER || phase==STARTUP || phase==RELEASE_ARMED){
            r.holdWorld=true;r.dimStage=true;r.showIncomingFighter=true;
            r.bypassIncomingPortraitIntro=true;
            // The DHC presentation must not show the native portrait, even
            // transiently; its sprite group needs a precise owner-aware filter.
            // Freeze violation is NOT repairable by dropping the darkening.
            if(!s.worldHeld || s.foreignWorldAdvanced ||
               s.incomingPortraitVisible){
                phase=FAILED;r.rollbackRequired=true;return r;
            }
            if(s.darkScenePresented)++cinematicFrames;
            if(phase==TAG_ENTER && s.nativeTagComplete && s.incomingGrounded &&
               s.incomingAtTakeoverPosition)phase=STARTUP;
            if(phase==STARTUP){
                // Native intro must not merely run invisibly while the
                // fighter waits. Super startup is the only allowed sequence.
                if(s.incomingSuperStarted && !s.nativePortraitIntroBypassed){
                    phase=FAILED;r.rollbackRequired=true;return r;
                }
                r.tickIncomingOnly=true;
                if(s.incomingStartupTick && s.incomingSuperStarted)++startupTicks;
                if(s.firstActiveOnNextTick){
                    // Do not release from a visual transition or fixed timer.
                    // Incoming native super must actually progress in the
                    // visible darkened scene, with no portrait drawn.
                    if(s.incomingSuperStarted && startupTicks>0 && cinematicFrames>0)
                        phase=RELEASE_ARMED;
                    else {phase=FAILED;r.rollbackRequired=true;return r;}
                }
            }
            if(phase==RELEASE_ARMED){
                // The next real native frame is ACTIVE. Remove dimming + match
                // freeze before it, not before the completed startup. Keep
                // suppressing the incoming portrait through this transition.
                r.tickIncomingOnly=false;
                r.releaseBeforeNextNativeFrame=true;
                phase=INCOMING;firstReleaseTick=true;
            }
            return r;
        }
        if(phase==INCOMING){
            if(s.incomingPortraitVisible){
                phase=FAILED;r.rollbackRequired=true;return r;
            }
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
