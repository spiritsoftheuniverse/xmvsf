#ifndef XMVSF_STAGE87_274_UNIFIED_DHC_H
#define XMVSF_STAGE87_274_UNIFIED_DHC_H
// 87.274: character-independent, two-sided ROM-owned DHC transaction.
// Experimental Version 4 ONLY (XM87274_UNIFIED_DHC=1). Never a replacement
// for original Version 3 or a reason to overwrite an unverified actor script.
// No Ryu/Ken action, script, animation, timer or fighter-ID shortcuts.
// A native tag must be accepted by the original 68000; the reserve's own
// 68000 input parser must independently select a real 2E ROM super.
// The initial universal path intentionally DOES NOT force an incoming kickless
// landing: other fighters' native tag-entry scripts need captured ROM evidence.
#include "xmvsf_stage87_192_reserve_super_ownership.h"
#include "xmvsf_stage87_329_morphus_dhc_catalog.h"
#include "xmvsf_stage87_334_morphus_2v2_precommit.h"
#include "xmvsf_stage87_193_deterministic_exit_protocol.h"
#include "xmvsf_stage87_262_game_version.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87274Unified {
enum Phase { IDLE=0, TAG_REQUESTED=1, AWAIT_SWAP=2, AWAIT_NEUTRAL=3,
             REPLAY_RESERVE_INPUT=4, INCOMING_SUPER=5 };
static inline const char* phaseName(Phase phase){
    switch(phase){
    case IDLE:return "IDLE";
    case TAG_REQUESTED:return "NATIVE_TAG_REQUESTED";
    case AWAIT_SWAP:return "AWAIT_ORIGINAL_ROM_SWAP";
    case AWAIT_NEUTRAL:return "AWAIT_NATIVE_RESERVE_CONTROL";
    case REPLAY_RESERVE_INPUT:return "REPLAY_RESERVE_ORIGINAL_COMMAND";
    case INCOMING_SUPER:return "ROM_VERIFIED_RESERVE_SUPER";
    default:return "UNKNOWN";
    }
}
struct Input {
    bool active=false;
    unsigned directions=0u,attacks=0u;
};
struct Slot {
    Phase phase=IDLE;
    unsigned point=0u,reserve=0u,started=0u,accepted=0u;
    unsigned inputStart=0u,superAt=0u,neutralSince=0u;
    unsigned originalAction=0u,originalMode=0u,originalD6=0u;
    XM87193Exit::Snapshot snapshot;
    XM87192Owner::Proposal request;
    XM87334Native::Pending nativePrecommit; // opt-in, host-side only
    void reset(){*this=Slot();}
};
struct State {
    Slot slot[2];
    unsigned lastFrame=~0u;
    std::FILE* log=nullptr;
    bool logChecked=false;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    bool enabled()const{
        const char* v=std::getenv("XM87274_UNIFIED_DHC");
        return XM87262Game::version4() && v && std::strcmp(v,"1")==0;
    }
    void reset(){
        slot[0].reset();slot[1].reset();lastFrame=~0u;
    }
    void event(unsigned now,unsigned side,const char*what){
        if(!logChecked){
            logChecked=true;
            const char* p=std::getenv("XM87274_UNIFIED_DHC_LOG");
            if(p && *p){
                log=std::fopen(p,"wb");
                if(log)std::fputs("frame\tside\tphase\tpoint\treserve\tdecision\n",log);
            }
        }
        if(log && side<2u){
            const Slot& s=slot[side];
            std::fprintf(log,"%u\t%u\t%s\t%02X\t%02X\t%s\n",
                         now,side+1u,phaseName(s.phase),s.point,s.reserve,what);
            std::fflush(log);
        }
    }
    // These are ORIGINAL 68000 post-CPU observations, not host-generated
    // action IDs. No tag is "confirmed" just because controller 8 was armed.
    static bool neutral(const std::uint8_t*ram,unsigned p){
        const unsigned a=XM87142::ramW(ram,p+0x60u);
        const unsigned m=XM87142::ramW(ram,p+0xA0u)&0xFF00u;
        return (a==0u && m==0u) || (a==2u && m==0x0200u);
    }
    static bool validPair(unsigned p,unsigned r){
        return p<=0x40u && r<=0x40u && !(p&1u) && !(r&1u) && p!=r;
    }
    // Called before CpsRwGetInp on each REAL frame. Physical port inputs
    // are saved/restored by cps_run.cpp, never by this policy object.
    Input inputForFrame(const std::uint8_t*ram,unsigned side,unsigned now,
                        unsigned physicalDir,unsigned physicalButtons,
                        bool superCandidate)const{
        Input out;
        if(!enabled()||!ram||side>=2u ||
           XM87142::ramB(ram,0x5000u)!=8u)return out;
        const Slot& s=slot[side];
        const unsigned p=XM87193Exit::actorBase(side);
        if(s.phase==REPLAY_RESERVE_INPUT && now>=s.inputStart &&
           XM87142::ramW(ram,p+0x52u)==s.reserve){
            const bool facingRight=(XM87142::ramB(ram,p+0x4Bu)&1u)!=0u;
            const XM87192Owner::Mask m=s.request.input(
                now-s.inputStart,facingRight);
            out.active=m.active;out.directions=m.directions;
            out.attacks=m.attacks;return out;
        }
        if(s.phase==TAG_REQUESTED || s.phase==AWAIT_SWAP){
            // The outgoing fighter must not consume the same chord twice.
            if(XM87142::ramW(ram,p+0x52u)==s.point){
                out.active=true;return out;
            }
        }
        if(s.phase!=IDLE||!superCandidate)return out;
        const XM87184Life::Sample native={
            XM87142::ramW(ram,p+0x52u),XM87142::ramW(ram,p+0x60u),
            XM87142::ramW(ram,p+0xA0u),XM87142::ramW(ram,p+0x08u),
            XM87142::ramL(ram,p+0x34u),XM87142::ramB(ram,p+0xD6u)};
        const auto& super=XM87189Family::state().side[side];
        if(!super.preCpuActive(now,native))return out;
        // Unbounded terminal 2E tails remain read-only, including at the
        // pre-CPU physical input boundary. Do not shield their 3P chords.
        if(super.phase==XM87189Family::CONTINUATION &&
           !(super.stageScript && super.stageEnd>super.stageScript))return out;
        const XM87193Exit::PreCpu plan=XM87193Exit::beforeCpu(
            true,true,true,physicalButtons);
        if(plan.active){
            out.active=true;out.directions=physicalDir;
            out.attacks=plan.nativeButtons;
        }
        return out;
    }
    void fail(unsigned side,unsigned now,const char*why){
        event(now,side,why);
        slot[side].reset();
        XM87192Owner::state().clear(side);
    }
    void frame(std::uint8_t*ram,const std::uint8_t*rom,unsigned romSize,
               unsigned now){
        if(!enabled()||!ram||!rom)return;
        if(lastFrame!=~0u && now<lastFrame)reset();
        if(lastFrame==now)return;
        lastFrame=now;
        if(XM87142::ramB(ram,0x5000u)!=8u){reset();return;}
        for(unsigned side=0u;side<2u;++side){
            Slot& s=slot[side];
            const unsigned p=XM87193Exit::actorBase(side);
            const unsigned r=XM87193Exit::reserveBase(side);
            const unsigned point=XM87142::ramW(ram,p+0x52u);
            const unsigned reserve=XM87142::ramW(ram,r+0x52u);
            if(s.phase==IDLE){
                // Physical DHC proposals arrive AFTER the current CPU frame.
                // Log why one cannot enter the real native tag path instead of
                // remaining silently IDLE (87.309 capture, frames 2579-2651).
                if(!XM87192Owner::state().fresh(side,reserve,now))continue;
                if(!validPair(point,reserve)){
                    event(now,side,"REJECT_INVALID_ACTOR_RESERVE_PAIR");
                    XM87192Owner::state().clear(side);
                    continue;
                }
                const XM87184Life::Sample native={
                    point,XM87142::ramW(ram,p+0x60u),
                    XM87142::ramW(ram,p+0xA0u),
                    XM87142::ramW(ram,p+0x08u),
                    XM87142::ramL(ram,p+0x34u),
                    XM87142::ramB(ram,p+0xD6u)};
                const auto& super=XM87189Family::state().side[side];
                // The 87.309 trace proved proposals during genuine 2E
                // continuation (e.g. action 2E22), not only root ATTACK.
                // Permit ONLY the exact ROM-bounded current stage; a terminal
                // tail or an unmatched actor/script still cannot arm tag 8.
                if(!super.unifiedDhcWindowAt(now,native)){
                    char reason[160];
                    std::snprintf(reason,sizeof(reason),
                        "REJECT_ROM_SUPER_WINDOW phase=%u action=%04X step=%u script=%06X root=%04X stage=%04X end=%06X",
                        unsigned(super.phase),native.action,native.step,native.script,
                        super.rootAction,super.stageAction,super.stageEnd);
                    event(now,side,reason);
                    XM87192Owner::state().clear(side);
                    continue;
                }
                if(super.phase==XM87189Family::CONTINUATION)
                    event(now,side,"ROM_BOUNDED_2E_CONTINUATION_WINDOW_ACCEPTED");
                if(!XM87142::ramB(ram,p+0x214u)){
                    event(now,side,"REJECT_NO_ADDITIONAL_STOCK");
                    XM87192Owner::state().clear(side);continue;
                }
                const auto tag=XM87142::resolve(rom,romSize,point,0x32u,0u);
                if(!tag.valid){
                    event(now,side,"REJECT_NO_ORIGINAL_32_00_TAG_SCRIPT");
                    XM87192Owner::state().clear(side);continue;
                }
                // Morphus 87.329 EURO roster catalog: experimental
                // pre-tag refusal only. Do not infer an incoming super or
                // write an unverified +D6/action into the US ROM from it.
                if(XM87329Morphus::enabled()){
                    const auto decision=XM87329Morphus::review(
                        reserve,unsigned(XM87192Owner::state().pending[side].motion));
                    event(now,side,XM87329Morphus::reason(decision));
                    if(XM87329Morphus::rejectBeforeTag(decision)){
                        XM87192Owner::state().clear(side);
                        continue; // original outgoing Hyper continues; no swap
                    }
                }
                if(XM87334Native::enabled()){
                    const auto& pending=XM87192Owner::state().pending[side];
                    const auto chosen=XM87334Native::resolve(
                        reserve,unsigned(pending.motion),pending.buttonMask);
                    if(!chosen.ok()){
                        event(now,side,XM87334Native::reasonName(chosen.reason));
                        XM87192Owner::state().clear(side);
                        continue; // no original tag or meter debit
                    }
                    if(!s.nativePrecommit.stage(
                            side,point,reserve,now,chosen)){
                        event(now,side,"MORPHUS_D6_PRECOMMIT_REJECTED");
                        XM87192Owner::state().clear(side);
                        continue;
                    }
                    char detail[112];
                    std::snprintf(detail,sizeof(detail),
                        "MORPHUS_D6_PREQUALIFIED_ONLY target=%02X d6=%02X throw=%u",
                        reserve,unsigned(chosen.hyperD6),
                        chosen.terminalThrow?1u:0u);
                    event(now,side,detail);
                    // Capture the current original outgoing point, not a
                    // guessed camera/ground coordinate for the native entry.
                    s.nativePrecommit.x=XM87142::ramW(ram,p+0x0Cu);
                    s.nativePrecommit.stockBefore=
                        XM87142::ramB(ram,p+0x214u);
                }
                s.request=XM87192Owner::state().pending[side];
                s.point=point;s.reserve=reserve;s.started=now;
                const auto tx=XM87193Exit::beginNativeExit(ram,side);
                if(!tx.begun){fail(side,now,"NATIVE_CONTROLLER_8_UNAVAILABLE");continue;}
                s.snapshot=tx.before;
                s.originalAction=tx.before.action;
                s.originalMode=tx.before.mode;
                s.originalD6=tx.before.d6;
                s.phase=TAG_REQUESTED;
                XM87192Owner::state().clear(side);
                event(now,side,"ROM_CONTROLLER_8_ARMED");
                continue;
            }
            if(s.phase==TAG_REQUESTED){
                if(point!=s.point||reserve!=s.reserve){
                    fail(side,now,"ABORT_OWNERSHIP_CHANGED_BEFORE_NATIVE_TAG");
                    continue;
                }
                const unsigned action=XM87142::ramW(ram,p+0x60u);
                const unsigned mode=XM87142::ramW(ram,p+0xA0u);
                if(action==0x3200u && mode==0x2A00u){
                    s.phase=AWAIT_SWAP;s.accepted=now;
                    event(now,side,"ORIGINAL_ROM_32_00_TAG_ACCEPTED");
                }else if(action!=s.originalAction){
                    // Native 68000 selected a different action: do not
                    // overwrite it with a fabricated 3200 tag animation.
                    fail(side,now,"ABORT_WRONG_NATIVE_ACTION_NO_SCRIPT_WRITE");
                }else if(now-s.started>12u){
                    // Restore a request ONLY when 68000 never consumed it.
                    if(XM87142::ramB(ram,p+0x256u)==8u &&
                       XM87142::ramB(ram,p+0x259u)==8u &&
                       mode==0x002Au){
                        XM87193Exit::bput(ram,p+0x256u,s.snapshot.controller256);
                        XM87193Exit::bput(ram,p+0x259u,s.snapshot.controller259);
                        XM87193Exit::wput(ram,p+0xA0u,s.snapshot.mode);
                        XM87193Exit::wput(ram,p+0x06u,s.snapshot.step06);
                        XM87193Exit::wput(ram,p+0x08u,s.snapshot.step08);
                        XM87193Exit::wput(ram,p+0x0Au,s.snapshot.step0A);
                    }
                    fail(side,now,"ABORT_NATIVE_TAG_NOT_ACCEPTED_12F");
                }
                continue;
            }
            if(s.phase==AWAIT_SWAP){
                if(point==s.reserve && reserve==s.point){
                    if(XM87334Native::enabled() &&
                       !s.nativePrecommit.matchesAfterOriginalSwap(
                           side,point,reserve)){
                        fail(side,now,"MORPHUS_D6_WRONG_OR_EXPIRED_SWAP");
                        continue;
                    }
                    const char* direct=std::getenv("XM87335_NATIVE_DHC");
                    if(direct && std::strcmp(direct,"1")==0 &&
                       XM87334Native::enabled()){
                        // We requested a direct native commit. If the actual
                        // 68000 instruction hook did not run, do not silently
                        // pretend that a neutral-wait replay is DHC. Leave
                        // the original tag alone and fail only bookkeeping.
                        fail(side,now,"NATIVE_335_NO_ORIGINAL_INCOMING_HOOK_NO_INPUT_REPLAY");
                        continue;
                    }
                    if(XM87334Native::enabled())
                        event(now,side,"MORPHUS_D6_ORIGINAL_SWAP_CONFIRMED_LEGACY_INPUT_PATH");
                    s.phase=AWAIT_NEUTRAL;s.neutralSince=0u;
                    event(now,side,"ORIGINAL_ROM_POINT_RESERVE_SWAPPED");
                }else if((point!=s.point||reserve!=s.reserve) ||
                         now-s.accepted>90u){
                    fail(side,now,"ABORT_NATIVE_SWAP_FAILED");
                }
                continue;
            }
            if(point!=s.reserve||reserve!=s.point){
                fail(side,now,"ABORT_INCOMING_OWNERSHIP_LOST");continue;
            }
            if(s.phase==AWAIT_NEUTRAL){
                // Do not inject Ken's 2A08 or Ryu's 00/00 into other
                // fighters. First implementation waits for REAL native
                // control return. Kickless arrival needs per-ROM evidence.
                if(neutral(ram,p)){
                    if(!s.neutralSince)s.neutralSince=now;
                    if(now-s.neutralSince>=2u){
                        s.phase=REPLAY_RESERVE_INPUT;s.inputStart=now+1u;
                        event(now,side,"NATIVE_RESERVE_NEUTRAL_INPUT_READY");
                    }
                }else s.neutralSince=0u;
                if(now-s.started>320u)fail(side,now,"ABORT_ENTRY_NO_NEUTRAL_320F");
                continue;
            }
            if(s.phase==REPLAY_RESERVE_INPUT){
                const XM87184Life::Sample native={
                    point,XM87142::ramW(ram,p+0x60u),
                    XM87142::ramW(ram,p+0xA0u),
                    XM87142::ramW(ram,p+0x08u),
                    XM87142::ramL(ram,p+0x34u),
                    XM87142::ramB(ram,p+0xD6u)};
                // Do not accept a guessed 2C special or another actor's
                // action. The incoming actor's own original ROM decides.
                if(XM87192Owner::state().confirm(
                       side,now,s.request,native,rom,romSize)){
                    s.phase=INCOMING_SUPER;s.superAt=now;
                    s.neutralSince=0u;
                    if(s.nativePrecommit.armed){
                        const unsigned executedD6=XM87142::ramB(ram,p+0xD6u);
                        char message[128];
                        std::snprintf(message,sizeof(message),
                            "MORPHUS_D6_POST_SWAP_ROM_ACCEPT expected=%02X actual=%02X %s",
                            unsigned(s.nativePrecommit.d6),executedD6,
                            executedD6==s.nativePrecommit.d6?
                            "MATCH":"MISMATCH_REVISION_OR_COMMAND");
                        event(now,side,message);
                        s.nativePrecommit.markConsumed();
                    }
                    event(now,side,"INCOMING_ORIGINAL_ROM_2E_SUPER_ACCEPTED");
                }else if(now>s.inputStart+s.request.duration()+12u){
                    XM87192Owner::state().rejected(side,now,s.request,native);
                    fail(side,now,"ABORT_RESERVE_INPUT_NOT_A_ROM_SUPER");
                }
                continue;
            }
            if(s.phase==INCOMING_SUPER){
                // Direct 87.335 guest must still PROVE a real original-ROM
                // 2E action. Guest entry is not proof of a successful Hyper.
                // All checks are read-only post-CPU, after native helpers.
                if(s.nativePrecommit.directUsed && !s.nativePrecommit.romSeen){
                    const XM87184Life::Sample native={
                        point,XM87142::ramW(ram,p+0x60u),
                        XM87142::ramW(ram,p+0xA0u),
                        XM87142::ramW(ram,p+0x08u),
                        XM87142::ramL(ram,p+0x34u),
                        XM87142::ramB(ram,p+0xD6u)};
                    const bool accepted=XM87192Owner::State::nativeSuper(
                        rom,romSize,native);
                    if(accepted && native.d6==s.nativePrecommit.d6){
                        s.nativePrecommit.romSeen=true;
                        const unsigned stockNow=XM87142::ramB(ram,p+0x214u);
                        char detail[156];
                        std::snprintf(detail,sizeof(detail),
                            "NATIVE_335_68000_ROM_2E_CONFIRMED d6=%02X stock_before=%u stock_now=%u",
                            unsigned(native.d6),s.nativePrecommit.stockBefore,stockNow);
                        event(now,side,detail);
                    }else if(now>s.superAt+16u){
                        // Native entry failed: never fabricate a super or
                        // roll back a genuine fighter swap.
                        fail(side,now,"NATIVE_335_GUEST_NO_ORIGINAL_ROM_SUPER");
                        continue;
                    }else{
                        continue; // no false successful re-arm during startup
                    }
                }
                // No second DHC during the incoming hyper. Re-arm only
                // after its original ROM action returns to neutral.
                if(neutral(ram,p)){
                    if(!s.neutralSince)s.neutralSince=now;
                    if(now-s.neutralSince>=2u){
                        event(now,side,"ORIGINAL_SUPER_EXIT_REARMED");
                        s.reset();
                    }
                }else s.neutralSince=0u;
                // No timeout that forcibly clears an executing ROM super.
            }
        }
    }
};
inline State&state(){static State s;return s;}
} // namespace XM87274Unified
#endif
