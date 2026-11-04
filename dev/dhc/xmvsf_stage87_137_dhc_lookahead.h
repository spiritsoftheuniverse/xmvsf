#ifndef XMVSF_STAGE87_137_DHC_LOOKAHEAD_H
#define XMVSF_STAGE87_137_DHC_LOOKAHEAD_H
// Stage 87.137: zero-visible-startup release test for a DHC super.
// A native adapter must save/restore the FULL emulation state and suppress
// external writes (sound output, logs, autosaves, network, controller edges).
// Never use a wall-clock or a hardcoded number of startup frames.
//
// Test the next released native frame SPECULATIVELY, compare validated
// incoming-super hitboxes before vs after, and restore the exact pre-frame
// emulation state. The following real frame can be released if that preview
// proves the FIRST offensive activation for the requested super.
//
// NOTE: This helper is not the native adapter. It intentionally cannot
// change a fighter, spend meter, run a CPS2 frame or paint a screen.
namespace XM87137Lookahead {
enum Decision { NO_PREVIEW, NOT_READY, READY_NEXT_FRAME, ROLLBACK_FAILED };
struct Attack {
    bool valid=false;
    bool ownSuper=false;           // exact incoming move/action identity validated
    bool firstActive=false;       // per-move native activation predicate, not silhouette
    // Strike: validated offensive box. Projectile: validated initial spawn.
    // Throw/counter/install: validated command-specific activation state.
    bool correctFighter=false;     // source == new point character or its projectile
    unsigned superId=0;
};
struct Hooks {
    void* ctx=nullptr;
    bool (*save)(void*)=nullptr;
    bool (*advanceReleasedFrame)(void*)=nullptr;
    bool (*observe)(void*,Attack*)=nullptr;
    bool (*restore)(void*)=nullptr;
    bool (*restorationVerified)(void*)=nullptr;
};
struct Outcome {
    Decision decision=NO_PREVIEW;
    bool simulated=false;
    bool restored=false;
};
// The caller must already own a DHC blackout and have a charged incoming
// startup frame. The lookahead must execute at a safe post-CPU boundary.
static inline Outcome preview(const Hooks& h,unsigned expectedSuperId) {
    Outcome out;
    if(!h.save || !h.advanceReleasedFrame || !h.observe || !h.restore ||
       !h.restorationVerified || !expectedSuperId)return out;
    Attack before,after;
    if(!h.observe(h.ctx,&before) || !before.valid ||
       !before.correctFighter || !before.ownSuper ||
       before.superId!=expectedSuperId || before.firstActive)
        return out;
    if(!h.save(h.ctx))return out;
    // Step and classify only within the speculative state. ALWAYS restore.
    const bool stepped=h.advanceReleasedFrame(h.ctx);
    out.simulated=stepped;
    const bool sampled=stepped && h.observe(h.ctx,&after);
    const bool rolled=h.restore(h.ctx);
    out.restored=rolled && h.restorationVerified(h.ctx);
    if(!out.restored){out.decision=ROLLBACK_FAILED;return out;}
    if(!sampled || !after.valid || !after.correctFighter ||
       !after.ownSuper || after.superId!=expectedSuperId){
        out.decision=NO_PREVIEW;return out;
    }
    out.decision=after.firstActive?READY_NEXT_FRAME:NOT_READY;
    return out;
}
} // namespace XM87137Lookahead
#endif
