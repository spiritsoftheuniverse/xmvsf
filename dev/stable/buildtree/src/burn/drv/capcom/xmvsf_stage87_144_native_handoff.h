#ifndef XMVSF_STAGE87_144_NATIVE_HANDOFF_H
#define XMVSF_STAGE87_144_NATIVE_HANDOFF_H
// Stage 87.144: opt-in first *gameplay* DHC native-action handoff test.
// One controlled Ryu->Ken transaction only. Does NOT touch ordinary tags.
// Uses the ROM's exact Ryu native outgoing action 32/00 (and Ken 2A/0C
// incoming if the game actually swaps); no synthetic incoming actor, ROM
// patch, injected sprite, or altered global controller input.
// To prove a native script transition works, emulate the assignments the
// game's own PC 0x01038E->0x001AE6 performs from actual loaded ROM records.
// If the native swap never happens, abort the DHC transaction without
// touching partner, opponent, meter, or Stable.
#include "xmvsf_stage87_143_super_queue.h"
#include "xmvsf_stage87_159_projectile_cutoff.h"
#include "xmvsf_stage87_176_dhc_input_isolation.h" // pure per-frame DHC P1 input gate
#include "xmvsf_stage87_178_tag_latch_recovery.h" // 87.178 only stale DHC tag latch
#include "xmvsf_stage87_179_delayed_tag_edge.h" // 87.179 delayed native input edge
#include "xmvsf_stage87_183_native_edge_preflight.h" // 87.183 native consumed-punch gate
#include "xmvsf_stage87_184_super_action_lifetime.h" // real ROM super lifetime
#include "xmvsf_stage87_185_action_gate.h" // 87.185 separate recognition and ROM tag acceptance
#include "xmvsf_stage87_186_first_attack_gate.h" // TEST-only first four natural attack frames
#include "xmvsf_stage87_187_opening_front.h" // block stale startup handoff
#include "xmvsf_stage87_188_full_attack_gate.h" // full native 2E08 ATTACK
#include "xmvsf_stage87_189_native_super_family.h" // original ROM 2E family
#include "xmvsf_stage87_191_super_lifetime_gate.h" // no phase-specific DHC availability
#include "xmvsf_stage87_192_reserve_super_ownership.h" // incoming reserve owns captured gesture
#include "xmvsf_stage87_193_deterministic_exit_protocol.h" // actor-addressed deterministic native exit
#include "xmvsf_stage87_194_native_passive_jump_entry.h" // ROM-owned descent, true landing, target steering
#include "xmvsf_stage87_177_dhc_punch_chords.h" // explicit 3P host / native 2P helper dependency
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87144 {
static inline unsigned rb(const std::uint8_t* r,unsigned a){return XM87142::ramB(r,a);}
static inline unsigned rw(const std::uint8_t* r,unsigned a){return XM87142::ramW(r,a);}
static inline unsigned rl(const std::uint8_t* r,unsigned a){return XM87142::ramL(r,a);}
static inline void wb(std::uint8_t* r,unsigned a,unsigned v){r[(a&65535u)^1u]=std::uint8_t(v);}
static inline void ww(std::uint8_t* r,unsigned a,unsigned v){wb(r,a,v>>8u);wb(r,a+1u,v);}
static inline void wl(std::uint8_t* r,unsigned a,unsigned v){ww(r,a,v>>16u);ww(r,a+2u,v);}
// Native 68000 01038E -> 001AE6 first animation frame setup. Preserve
// global and opponent state. Abort for unsupported headers.
static inline bool setNativeScript(std::uint8_t* ram,
        const std::uint8_t* rom,unsigned size,unsigned actor,
        unsigned fighter,unsigned action,unsigned selector){
    XM87142::Lookup lookup=XM87142::resolve(rom,size,fighter,action,selector);
    if(!lookup.valid || lookup.script+16u>size)return false;
    unsigned first=XM87142::romW(rom,size,lookup.script);
    unsigned dur=XM87142::romW(rom,size,lookup.script+2u);
    unsigned header=XM87142::romL(rom,size,lookup.script+4u);
    unsigned payload=XM87142::romL(rom,size,lookup.script+8u);
    unsigned misc=XM87142::romW(rom,size,lookup.script+12u);
    unsigned tableIdx=XM87142::romW(rom,size,lookup.script+14u);
    // Negative/zero duration has its own ROM branching logic; no guess.
    if(!dur || (dur&0x8000u) || tableIdx>0x200u)return false;
    const unsigned table=XM87142::romL(rom,size,0x8AA04u+fighter*2u);
    if(table>=size || size-table<16u || tableIdx*16u>size-table-16u)return false;
    // Native ROM dispatch writes +60/+61, +68, then animation fields.
    wb(ram,actor+0x60u,action);wb(ram,actor+0x61u,selector);
    wl(ram,actor+0x68u,table);
    wl(ram,actor+0x34u,lookup.script);
    ww(ram,actor+0x3Cu,first);
    ww(ram,actor+0x3Eu,dur);
    wl(ram,actor+0x40u,header);
    wl(ram,actor+0x62u,payload);
    ww(ram,actor+0x66u,misc);
    for(unsigned i=0;i<4u;++i){
        const unsigned v=XM87142::romL(rom,size,table+tableIdx*16u+i*4u);
        wl(ram,actor+0x70u+i*4u,v);
    }
    wl(ram,actor+0x38u,lookup.script+16u);
    return true;
}
enum Phase{
    IDLE=0,TAG_OUT=1,WAIT_TAG_CONTROLLER=2,WAIT_NATIVE_SWAP=3,
    WAIT_LANDING=4,WAIT_KEN_EXIT=5,SUPER_STARTED=6,FAILED=7,
    DELIVER_NATIVE_INPUT=8,WAIT_RYU_SUPER_END=9,
    SEND_ROM_TAG_INPUT=10,PRIME_DELAYED_TAG_EDGE=11,
    WAIT_NATIVE_PASSIVE_JUMP=12
};
static inline const char* phaseName(Phase p){
    switch(p){
      case TAG_OUT:return "ROM_TAG_OUT";
      case WAIT_TAG_CONTROLLER:return "WAIT_NATIVE_TAG_CONTROLLER";
      case WAIT_NATIVE_SWAP:return "WAIT_NATIVE_SWAP";
      case WAIT_LANDING:return "WAIT_NATIVE_LANDING";
      case WAIT_NATIVE_PASSIVE_JUMP:return "WAIT_NATIVE_PASSIVE_JUMP";
      case WAIT_KEN_EXIT:return "NATIVE_KEN_KICKLESS_EXIT";
      case SUPER_STARTED:return "NATIVE_KEN_SUPER_START";
      case DELIVER_NATIVE_INPUT:return "NATIVE_KEN_INPUT_SEQUENCE";
      case WAIT_RYU_SUPER_END:return "WAIT_RYU_NATIVE_SUPER_TIMER_08";
      case SEND_ROM_TAG_INPUT:return "PROOF_SEND_HP_HK_NORMAL_TAG";
      case PRIME_DELAYED_TAG_EDGE:return "PRIME_DELAYED_NATIVE_2P_EDGE";
      case FAILED:return "FAILED";
      default:return "IDLE";
    }
}
struct State {
    bool initialized=false,enabled=false,proofNative=false,earlyCancel=false,
         delayedPrimeExperimental=false,unsafeConsumedPunchReplay=false,
         earlyActiveDirectTest=false,recoveryReboundTest=false,
         firstFourAttackTest=false,fullAttackTest=false,familyTest=false,liveLifetimeTest=false,
         reserveOwnershipTest=false,deterministicExitTest=false,
         passiveJumpTest=false,
         chainUsed[2]={false,false};
    Phase phase[2]={IDLE,IDLE};
    bool armedFromFamily[2]={false,false};
    XM87159Projectile::State oldRyuProjectiles;
    unsigned started[2]={0u,0u},outgoing[2]={0u,0u},incoming[2]={0u,0u};
    unsigned cooldownUntil[2]={0u,0u},lastAcceptedAt[2]={~0u,~0u};
    unsigned capturedMode[2]={0u,0u},capturedAction[2]={0u,0u};
    unsigned capturedScript[2]={0u,0u};
    unsigned captured256[2]={0u,0u},captured259[2]={0u,0u};
    unsigned capturedD6[2]={0u,0u};
    unsigned captured06[2]={0u,0u},captured08[2]={0u,0u},
             captured0A[2]={0u,0u};
    unsigned controllerAcceptedAt[2]={0u,0u};
    unsigned kenExitAt[2]={0u,0u},kenSuperAt[2]={0u,0u};
    unsigned kenNeutralAt[2]={0u,0u},inputBegin[2]={0u,0u};
    unsigned primeFrame[2]={0u,0u};
    bool primeSent[2]={false,false};
    unsigned kenKickReassertions[2]={0u,0u};
    bool kenExitObserved[2]={false,false},kenSuperConfirmed[2]={false,false};
    // The first ROM action after handoff must belong to the REQUESTED move,
    // not simply to Ken's large 2Cxx rush/dash action family.
    unsigned kenExpectedAction[2]={0u,0u},kenExpectedScript[2]={0u,0u};
    bool kenFallbackUsed[2]={false,false};
    unsigned kenNeutralSince[2]={0u,0u};
    // Both ROM super targets are independently checked in a single build.
    bool shoryureppaReady[2]={false,false},shinryukenReady[2]={false,false};
    unsigned lastKenTargetAction[2]={0u,0u};
    unsigned lastNormalRef[2]={~0u,~0u};
    unsigned originalX[2]={0u,0u},opponentX[2]={0u,0u};
    XM87143Queue::Command command[2]={XM87143Queue::QCF_PP,XM87143Queue::QCF_PP};
    XM87192Owner::Proposal ownedCommand[2];
    XM87194Jump::Arrival jumpArrival[2];
    unsigned lastFrame=~0u;
    unsigned lastQueuedCheck[2]={~0u,~0u};
    std::FILE* log=nullptr;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* setting=std::getenv("XM87144_DHC_NATIVE_TEST");
        const bool requested=setting && std::strcmp(setting,"1")==0;
        const char* proof=std::getenv("XM87156_PROOF_NATIVE");
        const char* fast=std::getenv("XM87157_EARLY_CANCEL");
        proofNative=requested && proof && std::strcmp(proof,"1")==0;
        earlyCancel=requested && !proofNative && fast &&
                    std::strcmp(fast,"1")==0;
        const char* legacy=std::getenv("XM87144_DHC_LEGACY_TAG_WRITER");
        // Proof: intact natural 87.156 sequence. Early cancel: verified
        // 87.150 late-Ryu tag/first-Ken-arrival cancellation plus 87.156
        // genuine ROM Ken QCF+2P/2K command, never the old guessed 2C dash.
        enabled=requested && (proofNative || earlyCancel ||
                 (legacy && std::strcmp(legacy,"1")==0));
        // The supplied 87.179 movie proves delayed 2P priming is not
        // deterministic: 4/16 queued commands reached Ken, while many
        // primed pulses entered Ryu 0002/2C02/2C04 instead.
        // Default OFF until within-frame input acceptance is understood.
        // Explicit opt-in preserves the existing path for isolated audit.
        const char* primeTest=std::getenv("XM87180_EXPERIMENTAL_DELAYED_PRIME");
        delayedPrimeExperimental=enabled && earlyCancel && primeTest &&
             std::strcmp(primeTest,"1")==0;
        // Two 87.185 TEST-only hypotheses. Never active without a
        // Ryu->Ken early-cancel transaction and dedicated env flag.
        const char* earlyDirect=std::getenv("XM87185_TEST_EARLY_ACTIVE_TAG");
        earlyActiveDirectTest=enabled && earlyCancel && earlyDirect &&
             std::strcmp(earlyDirect,"1")==0;
        const char* rebound=std::getenv("XM87185_TEST_RECOVERY_REBOUND");
        recoveryReboundTest=enabled && earlyCancel && rebound &&
             std::strcmp(rebound,"1")==0;
        const char* full=std::getenv("XM87188_TEST_FULL_ATTACK");
        fullAttackTest=earlyActiveDirectTest && full &&
             std::strcmp(full,"1")==0;
        const char* nativeFamily=std::getenv("XM87189_TEST_NATIVE_SUPER_FAMILY");
        familyTest=enabled && earlyCancel && nativeFamily &&
             std::strcmp(nativeFamily,"1")==0;
        const char* liveLifetime=std::getenv("XM87191_TEST_OPEN_SUPER_LIFETIME");
        liveLifetimeTest=familyTest && liveLifetime &&
            std::strcmp(liveLifetime,"1")==0;
        const char* owned=std::getenv("XM87192_TEST_RESERVE_OWNERSHIP");
        reserveOwnershipTest=liveLifetimeTest && owned &&
            std::strcmp(owned,"1")==0;
        const char* exitProtocol=std::getenv("XM87193_TEST_DETERMINISTIC_EXIT");
        deterministicExitTest=reserveOwnershipTest && exitProtocol &&
            std::strcmp(exitProtocol,"1")==0;
        const char* jump=std::getenv("XM87194_TEST_NATIVE_PASSIVE_JUMP");
        passiveJumpTest=deterministicExitTest && jump &&
            std::strcmp(jump,"1")==0;
        const char* firstFour=std::getenv("XM87186_TEST_FIRST_FOUR_ATTACK");
        firstFourAttackTest=enabled && earlyCancel && firstFour &&
             std::strcmp(firstFour,"1")==0;
        // 87.183: only offline F9 diagnostics may explicitly reproduce
        // the three dangerous 87.182 consumed-edge native attempts.
        // Normal RUN_TEST leaves this OFF to protect working gameplay.
        const char* unsafeReplay=std::getenv("XM87183_REPLAY_UNSAFE_CONSUMED_EDGE");
        unsafeConsumedPunchReplay=enabled && earlyCancel && unsafeReplay &&
             std::strcmp(unsafeReplay,"1")==0;
        const char* path=std::getenv("XM87144_DHC_LOG");
        if(enabled && path && *path){
            log=std::fopen(path,"wb");
            if(log)std::fputs("frame\tside\tphase\tpointId\treserveId"
                "\taction\tmodeA0\tstep06\tstep08\tstep0A"
                "\tcontroller256\tcontroller259\tnativeD6\tscriptFlag3C\tstep4E\ta4\tpartner214"
                "\tscript34\tscript38\tpartnerPtr\tpoint210\treserve210"
                "\tx\ty\tdecision\tken2E00_D6722_ready\tken2E02_D6976_ready"
                "\tken_observed_2E00\tken_observed_2E02\tselected_command\n",log);
        }
        std::fprintf(stderr,
            "XM87.180 Ken DHC enabled=%d proofMode=%d earlyCancel=%d"
            " experimentalDelayedPrime=%d\n",
            enabled?1:0,proofNative?1:0,earlyCancel?1:0,
            delayedPrimeExperimental?1:0);
    }
    void reset(){
        oldRyuProjectiles.reset();
        for(unsigned s=0;s<2u;++s){
            phase[s]=IDLE;chainUsed[s]=false;armedFromFamily[s]=false;
            ownedCommand[s]=XM87192Owner::Proposal();
            jumpArrival[s].reset();
            lastQueuedCheck[s]=~0u;
            lastAcceptedAt[s]=~0u;cooldownUntil[s]=0u;
            lastNormalRef[s]=~0u;
            kenExitAt[s]=kenSuperAt[s]=0u;
            kenNeutralAt[s]=inputBegin[s]=0u;
            primeFrame[s]=0u;primeSent[s]=false;
            kenKickReassertions[s]=0u;
            kenExitObserved[s]=kenSuperConfirmed[s]=false;
            kenExpectedAction[s]=kenExpectedScript[s]=0u;
            kenFallbackUsed[s]=false;kenNeutralSince[s]=0u;
            shoryureppaReady[s]=shinryukenReady[s]=false;
            lastKenTargetAction[s]=0u;
        }
    }
    // A failed DHC attempt must NEVER use up the whole match. Discard the
    // consumed command; the next intentional button/motion press is free
    // to stage another queue entry after a short native action settle.
    void abort(unsigned now,unsigned side,std::uint8_t*ram,
               const char*reason){
        // The failed 87.178 controller-8 attempts consumed +256 and
        // left the DHC's +259=08 latch, even though no tag action 3200
        // occurred. Check ownership BEFORE changing either field.
        const unsigned p=0x4000u+side*0x400u;
        const bool staleHostLatch=XM87178Recovery::staleHostTag259(
                unsigned(phase[side]),enabled,earlyCancel,
                rw(ram,p+0x52u),rw(ram,p+0x852u),
                outgoing[side],incoming[side],rw(ram,p+0x60u),
                rb(ram,p+0xD6u),rb(ram,p+0x256u),
                rb(ram,p+0x259u),captured259[side]);
        // A0=002A was also written by OUR host when controller 8
        // was armed; on 87.178 failures that state persisted 10-20
        // frames. Only restore the captured native mode if BOTH the
        // DHC-owned stale latch and the exact Ryu super state match.
        const bool restoreStaged=staleHostLatch &&
            XM87179Edge::canRestoreStagedMode(unsigned(phase[side]),
                enabled,earlyCancel,rw(ram,p+0x52u),
                rw(ram,p+0x852u),outgoing[side],incoming[side],
                rw(ram,p+0x60u),capturedAction[side],
                rw(ram,p+0xA0u),capturedMode[side],
                rb(ram,p+0xD6u),capturedD6[side],
                rb(ram,p+0x256u),rb(ram,p+0x259u),
                captured259[side]);
        // Controller 8 may have been consumed while leaving the DHC's
        // temporary 002A host mode behind. Restore only our captured
        // mode when the exact original super action/D6 still owns RAM.
        // Never rewind a different ROM-selected action (0002/2C02/etc).
        const bool restoreFamily=staleHostLatch && armedFromFamily[side] &&
            rw(ram,p+0x60u)==capturedAction[side] &&
            rw(ram,p+0xA0u)==0x002Au &&
            rb(ram,p+0xD6u)==capturedD6[side] &&
            rb(ram,p+0x256u)==0u &&
            captured256[side]==0u && captured259[side]==0u &&
            (capturedMode[side]&0xFF00u)==0x2A00u;
        if(staleHostLatch){
            wb(ram,p+0x259u,captured259[side]);
            event(now,side,ram,"DHC_178_STALE_HOST_259_LATCH_CLEARED");
        }
        if(restoreStaged || restoreFamily){
            // Never rewrite the actual ROM-selected action, D6,
            // 68000 button history, timers, projectiles, or script.
            ww(ram,p+0xA0u,capturedMode[side]);
            event(now,side,ram,
                  "DHC_179_ABANDONED_HOST_002A_RESTORED_TO_CAPTURED_MODE");
        }
        phase[side]=FAILED;
        event(now,side,ram,reason);
        if(side==0u)oldRyuProjectiles.abort(now,
            rw(ram,0x4052u)==0x14u && rw(ram,0x4852u)==0x12u);
        phase[side]=IDLE;
        armedFromFamily[side]=false;
        // 87.150 left this true when Ken lost the point slot, so later
        // first supers were queued but could never enter the DHC gate.
        chainUsed[side]=false;
        kenSuperConfirmed[side]=false;
        kenExpectedAction[side]=kenExpectedScript[side]=0u;
        jumpArrival[side].reset();
        ownedCommand[side]=XM87192Owner::Proposal();
        XM87192Owner::state().clear(side);
        kenFallbackUsed[side]=false;kenNeutralSince[side]=0u;
        kenNeutralAt[side]=inputBegin[side]=0u;
        primeFrame[side]=0u;primeSent[side]=false;
        // A brand-new human DHC command is its own debounce. Do not
        // impose a guessed six-frame retry blackout in full-lifetime
        // mode; a real new super may begin immediately after an abort.
        cooldownUntil[side]=liveLifetimeTest?now:now+6u;
        XM87143Queue::state().discard(side);
        event(now,side,ram,"DHC_ABORT_RELEASED_CHAIN_LOCK");
    }
    void event(unsigned now,unsigned side,const std::uint8_t*ram,
               const char*decision){
        if(!log)return;
        const unsigned p=0x4000u+side*0x400u;
        const unsigned reserve=0x4800u+side*0x400u;
        std::fprintf(log,"%u\t%u\t%s\t%04X\t%04X"
            "\t%04X\t%04X\t%04X\t%04X\t%04X"
            "\t%02X\t%02X\t%02X\t%02X\t%04X\t%02X\t%04X"
            "\t%08X\t%08X\t%08X\t%04X\t%04X"
            "\t%u\t%u\t%s\t%u\t%u\t%u\t%u\t%u\n",
            now,side+1u,phaseName(phase[side]),
            rw(ram,p+0x52u),rw(ram,reserve+0x52u),
            rw(ram,p+0x60u),rw(ram,p+0xA0u),
            rw(ram,p+0x06u),rw(ram,p+0x08u),rw(ram,p+0x0Au),
            rb(ram,p+0x256u),rb(ram,p+0x259u),
            rb(ram,p+0xD6u),rb(ram,p+0x3Cu),
            rw(ram,p+0x4Eu),rb(ram,p+0xA4u),rw(ram,p+0x214u),
            rl(ram,p+0x34u),rl(ram,p+0x38u),
            rl(ram,p+0x224u),rw(ram,p+0x210u),rw(ram,reserve+0x210u),
            rw(ram,p+0x0Cu),rw(ram,p+0x10u),decision,
            unsigned(shoryureppaReady[side]),unsigned(shinryukenReady[side]),
            unsigned(rw(ram,p+0x52u)==0x14u && rw(ram,p+0x60u)==0x2E00u &&
                     rl(ram,p+0x34u)>=0xD6722u && rl(ram,p+0x34u)<0xD67A2u),
            unsigned(rw(ram,p+0x52u)==0x14u && rw(ram,p+0x60u)==0x2E02u &&
                     rl(ram,p+0x34u)>=0xD6976u && rl(ram,p+0x34u)<0xD69F6u),
            unsigned(command[side]));
        std::fflush(log);
    }

    // 87.150 succeeded with Ryu's super +4E <= 08; 87.152 corrupted
    // control with +4E=0D. Arm only in that observed late-super window.
    // Called after choosing either Ken super, never for ordinary tags.
    void armOutgoingTag(std::uint8_t*ram,unsigned now,unsigned side,
                        unsigned selectedAt){
        const unsigned p=0x4000u+side*0x400u;
        // A new native controller transaction must not inherit the
        // previous partner's landing/lock state. This was otherwise a
        // persistent active=true after the first successful 87.194 jump.
        jumpArrival[side].reset();
        // Original 87.151 Ryu->Ken handoff: capture the independent
        // P1 projectile objects, not the outgoing Ryu actor's hitboxes.
        if(side==0u && earlyCancel &&
           rw(ram,0x4052u)==0x12u && rw(ram,0x4852u)==0x14u)
            oldRyuProjectiles.arm(ram,now);
                // Capture the ORIGINAL actor state once. The exit
                // protocol is identical for either side and does not
                // know which outgoing/incoming character owns a super.
                const XM87193Exit::Transaction exit=
                    XM87193Exit::beginNativeExit(ram,side);
                if(!exit.begun){
                    abort(now,side,ram,"DHC_193_NATIVE_EXIT_TRANSACTION_FAILED");
                    return;
                }
                const XM87193Exit::Snapshot& original=exit.before;
                capturedMode[side]=original.mode;
                capturedAction[side]=original.action;
                capturedScript[side]=original.script;
                captured256[side]=original.controller256;
                captured259[side]=original.controller259;
                capturedD6[side]=original.d6;
                captured06[side]=original.step06;
                captured08[side]=original.step08;
                captured0A[side]=original.step0A;
                // beginNativeExit already armed controller8. All
                // mirrored exit writes and the snapshot are indivisible
                // in this host callback; the ROM still selects 3200.
                phase[side]=WAIT_TAG_CONTROLLER;
                started[side]=now;
                lastAcceptedAt[side]=selectedAt;
                event(now,side,ram,"DHC_NATIVE_TAG_CONTROLLER_08_ARMED");
                event(now,side,ram,reserveOwnershipTest?
                      "DHC_192_RESERVE_NATIVE_MOVE_UNSELECTED_ROM_OWNS_SUPER":
                      "DHC_KEN_ALTERNATE_ROM_SUPERS_REPPA_2E00_D6722_SHINRYUKEN_2E02_D6976");
    }
    // 87.152: one-shot real CPS-2 controller stream. This is consumed
    // BEFORE CpsRwGetInp in the next real emulation frame, then the host
    // restores the physical SDL button array immediately after that read.
    // It does not set a sprite, A0, D6, or script address. The game's own
    // 68000 command decoder, controller and recovery logic own the super.
    struct InputMask { bool active=false,isolated=false,priming=false;unsigned directions=0,attacks=0; };
    InputMask inputForFrame(const std::uint8_t*ram,unsigned now){
        InputMask m;
        if(!enabled || !ram || rb(ram,0x5000u)!=8u)return m;
        // CPS2 input sampling precedes the host DHC callback by one native
        // frame. The failed 87.178 late-window tags had CA=0000 at the
        // frame AFTER controller 8 was armed; direct tags had a fresh
        // 0030/0050/0060 punch edge. Prime the original input PORT one
        // frame BEFORE arming the delayed tag controller, and never write
        // the native 68000 history or force a super script.
        if(XM87179Edge::eligible(unsigned(phase[0]),
                enabled,earlyCancel,proofNative,
                rw(ram,0x4052u),rw(ram,0x4852u),
                rw(ram,0x4060u),rw(ram,0x40A0u),
                rb(ram,0x40D6u),rb(ram,0x4256u),
                rb(ram,0x4259u),rw(ram,0x40C6u),
                now,primeFrame[0])){
            m.active=true;m.priming=true;
            m.attacks=XM87179Edge::nativePunchPair();
            m.directions=XM87179Edge::facingForward(rb(ram,0x404Bu));
            primeSent[0]=true;
            return m;
        }
        // 87.175 captured queued QCF+PP at 1865 / 3095 / 3843, but the
        // *native* PP edge only reached Ryu at 1866 / 3096 / 3844.
        // Once that input selects DHC, do NOT let the outgoing fighter
        // interpret the very same physical chord a second time.
        // Neutralize the P1 CPS input PORT only during a verified DHC
        // handoff. Do not write the 68000 input-history RAM directly.
        // Preserve earlyCancel's existing 2/2/3-frame Ken QCF sequence.
        if(XM87176Input::neutralize(unsigned(phase[0]),
            rw(ram,0x4052u),rw(ram,0x4852u),enabled,earlyCancel,
            now,inputBegin[0])){
            m.active=true;m.isolated=true;
            return m;
        }
        if(proofNative && phase[0]==SEND_ROM_TAG_INPUT &&
           rw(ram,0x4052u)==0x12u && inputBegin[0] &&
           now>=inputBegin[0]){
            const unsigned t=now-inputBegin[0];
            if(t>=6u)return m;
            m.active=true;
            // 87.155 user trace: real native P1 change P2 (Ken->Ryu):
            // held bit 0x24 at 3052; ROM 3200 at 3053. No RAM writes.
            m.attacks=t<3u?0x24u:0u;
            return m;
        }
        if(phase[0]!=DELIVER_NATIVE_INPUT ||
           rw(ram,0x4052u)!=0x14u || !inputBegin[0] ||
           now<inputBegin[0])return m;
        const unsigned t=now-inputBegin[0];
        if(reserveOwnershipTest && ownedCommand[0].valid){
            // The reserve owns this captured gesture once the ROM swaps.
            // Let the original 68000 parser choose the actual action.
            const bool faceRight=(rb(ram,0x404Bu)&1u)!=0u;
            const auto x=ownedCommand[0].input(t,faceRight);
            m.active=x.active;m.directions=x.directions;
            m.attacks=x.attacks;
            return m;
        }
        const unsigned maxFrames=earlyCancel?9u:17u;
        if(t>=maxFrames)return m;
        const bool faceRight=(rb(ram,0x404Bu)&1u)!=0u;
        const unsigned forward=faceRight?1u:2u;
        // 87.156 confirmed that the game accepts native Ken QCF+2P/2K.
        // FAST mode compresses the already verified sequence, preserving
        // the old kickless arrival's near-immediate cancel timing.
        // We are NOT submitting the game's built-in HP+HK dual super.
        m.active=true;
        if(earlyCancel){
            if(t<2u)m.directions=4u;              // Down
            else if(t<4u)m.directions=4u|forward;  // Down-forward
            else if(t<7u)m.directions=forward;     // Forward
            if(t>=5u && t<7u)
                m.attacks=command[0]==XM87143Queue::QCF_KK?0x18u:
                    XM87177Chords::nativeKenTwoPunchMask();
        }else{
            if(t<4u)m.directions=4u;
            else if(t<8u)m.directions=4u|forward;
            else if(t<14u)m.directions=forward;
            if(t>=11u && t<14u)
                m.attacks=command[0]==XM87143Queue::QCF_KK?0x18u:
                    XM87177Chords::nativeKenTwoPunchMask();
        }
        return m;
    }
    // Call AFTER the ROM has executed a full real frame and the input queue
    // was selected. Never run during runahead, never touch a normal tag.
    void frame(std::uint8_t* ram,const std::uint8_t* rom,unsigned size,
               unsigned now,unsigned superWindowMask){
        init();
        if(!enabled || !ram || !rom || lastFrame==now)return;
        lastFrame=now;
        if(rb(ram,0x5000u)!=8u){reset();return;}
        for(unsigned side=0;side<2u;++side){
            // Stage 87.144 confines executable experiment to P1 Ryu->Ken.
            if(side!=0u)continue;
            const unsigned p=0x4000u+side*0x400u;
            const unsigned reserve=0x4800u+side*0x400u;
            const unsigned pointId=rw(ram,p+0x52u);
            const unsigned reserveId=rw(ram,reserve+0x52u);
            const XM87184Life::Sample ryuLife={
                pointId,rw(ram,p+0x60u),rw(ram,p+0xA0u),
                rw(ram,p+0x08u),rl(ram,p+0x34u),
                rb(ram,p+0xD6u)};
            // The original-ROM 2E tracker was updated by the input
            // observer on this exact post-CPU frame, before DHC runs.
            const auto& originalSuper=XM87189Family::state().side[side];
            // 87.191: native ROM super START -> true original EXIT.
            // No timer, low A0 mode, script step or air-state veto.
            // Only the freshly selected physical DHC command can arm.
            const bool liveSuperWindow=liveLifetimeTest &&
                XM87191Lifetime::isOpen(originalSuper,now,ryuLife);
            const bool familyAttack=familyTest &&
                originalSuper.activeAt(now,ryuLife) &&
                XM87184Life::identify(ryuLife)==XM87184Life::OUTSIDE;
            const bool familyStartup=familyTest &&
                originalSuper.live && originalSuper.lastFrame==now &&
                originalSuper.phase==XM87189Family::STARTUP &&
                XM87184Life::identify(ryuLife)==XM87184Life::OUTSIDE;
            // Never re-enable the five-of-five failing 87.184
            // recovery controller route without its exact rebound
            // bridge. Turning that TEST feature OFF must itself be safe.
            const bool recoveryTest=XM87184Life::isRecoveryTag(
                ryuLife,XM87184Life::recoveryOptIn() &&
                        recoveryReboundTest);
            // Read-only reference from an actual ROM-driven *ordinary*
            // tag to compare against this DHC controller attempt. These
            // are never modified by the experiment unless it has a real
            // queued DHC command and the exact super gate passes.
            if(phase[side]==IDLE && rw(ram,p+0x60u)==0x3200u &&
               (lastNormalRef[side]==~0u || now-lastNormalRef[side]>=4u)){
                event(now,side,ram,"REFERENCE_REAL_ROM_NORMAL_TAG");
                lastNormalRef[side]=now;
            }
            if(phase[side]==IDLE){
                const auto* entry=XM87143Queue::state().selected[side];
                const bool ownedSelected=reserveOwnershipTest &&
                    XM87192Owner::state().fresh(side,reserveId,now);
                if((!ownedSelected && (!entry||!entry->known)) ||
                   chainUsed[side] || now<cooldownUntil[side])continue;
                const unsigned selectedAt=ownedSelected?
                    XM87192Owner::state().pending[side].frame:
                    XM87143Queue::state().selectedFrame[side];
                const bool firstCheck=selectedAt!=lastQueuedCheck[side];
                lastQueuedCheck[side]=selectedAt;
                // 87.186: 2524/2565 STARTUP commands were held for
                // 20/60 frames, then sent with zero physical/native
                // button edge. Never re-arm that stale request.
                // The 2A02 RECOVERY rebound remains untouched.
                // Startup is recognition only, not a deferred controller
                // request. A native 2E action needs fresh same-frame
                // input, including when its mode is 2A1C in the air.
                if((!liveSuperWindow && familyStartup) ||
                   ((liveSuperWindow || familyAttack) && selectedAt!=now)){
                    if(firstCheck)event(now,side,ram,familyStartup?
                        "DHC_189_FAMILY_STARTUP_NO_DEFERRED_TAG":
                        "DHC_189_FAMILY_STALE_PUNCH_NOT_ARMED");
                    lastAcceptedAt[side]=selectedAt;
                    XM87143Queue::state().discard(side);
                    continue;
                }
                const XM87187Front::Disposition front=
                    liveSuperWindow?XM87187Front::KEEP:
                    XM87187Front::check(ryuLife,now,selectedAt);
                if(front!=XM87187Front::KEEP){
                    if(firstCheck)
                        event(now,side,ram,XM87187Front::reason(front));
                    lastAcceptedAt[side]=selectedAt;
                    XM87143Queue::state().discard(side);
                    continue;
                }
                // Explicitly log why a selected command cannot reach the
                // ROM handoff. Stage 87.144 produced a blank DHC log because
                // it silently required +A0=2A02 instead of native 2A28.
                const char* blocked=nullptr;
                if(!(superWindowMask&(1u<<side)))
                    blocked="BLOCK_NO_ACTIVE_SUPER_WINDOW";
                else if(pointId!=0x12u || reserveId!=0x14u)
                    blocked="BLOCK_NOT_RYU_POINT_KEN_RESERVE";
                else if(!ownedSelected && entry->known->fighter!=reserveId)
                    blocked="BLOCK_SELECTED_SUPER_NOT_FOR_RESERVE";
                else if(liveLifetimeTest && !liveSuperWindow)
                    blocked="DHC_191_OUTSIDE_ACTUAL_ROM_SUPER_LIFETIME";
                else if(!liveSuperWindow && !familyAttack &&
                        !XM87184Life::canTag(ryuLife,
                             XM87184Life::recoveryOptIn() &&
                             recoveryReboundTest)){
                    // Native 2E continuation is real super ownership,
                    // but not automatically a safe tag phase. The
                    // captured 2E02 Tatsu loop proves selector+2 may
                    // be active attacks, NOT universal recovery.
                    blocked=familyTest && originalSuper.live &&
                            originalSuper.phase==XM87189Family::CONTINUATION?
                        "DHC_190_ROM_FAMILY_FOLLOWUP_RECOGNIZED_TAG_UNVERIFIED":
                        "BLOCK_OUTSIDE_SAFE_ORIGINAL_SUPER_PHASE";
                }
                else if(!liveSuperWindow && !familyAttack &&
                        rw(ram,p+0xA0u)!=0x2A28u &&
                        rw(ram,p+0xA0u)!=0x2A02u)
                    blocked="BLOCK_NOT_NATIVE_2A28_SUPER_MODE";
                bool hasScript=ownedSelected;
                if(!ownedSelected)for(unsigned i=0;i<entry->count;++i)
                    hasScript |= entry->scripts[i].resolved;
                if(!blocked && !hasScript)
                    blocked="BLOCK_SELECTED_SUPER_HAS_NO_RESOLVED_ROM_SCRIPT";
                // Require a real ROM-verified outgoing Ryu tag script.
                const auto out=XM87142::resolve(rom,size,pointId,0x32u,0u);
                if(!blocked && (!out.valid || out.script!=0xD1DACu))
                    blocked="BLOCK_MISSING_REAL_RYU_OUTGOING_TAG_SCRIPT";
                if(!blocked && selectedAt==lastAcceptedAt[side])
                    blocked="BLOCK_SAME_INPUT_ALREADY_CONSUMED";
                const auto rep=XM87142::resolve(rom,size,0x14u,0x2Eu,0x00u);
                const auto ryu=XM87142::resolve(rom,size,0x14u,0x2Eu,0x02u);
                shoryureppaReady[side]=rep.valid && rep.script==0xD6722u;
                shinryukenReady[side]=ryu.valid && ryu.script==0xD6976u;
                if(!blocked && !ownedSelected && entry->known->command==XM87143Queue::QCF_PP &&
                   !shoryureppaReady[side])blocked="BLOCK_REPPA_2E00_ROM_UNRESOLVED";
                if(!blocked && !ownedSelected && entry->known->command==XM87143Queue::QCF_KK &&
                   !shinryukenReady[side])blocked="BLOCK_SHINRYUKEN_2E02_ROM_UNRESOLVED";
                // 87.183 live 87.182 trace, 23 armed handoffs:
                // 20 successful: native C0=0001, CA=0000.
                // 3245/5111/10756 failures: C0=0011/0031 and
                // CA=0010/0030, i.e. the original CPU already consumed
                // part/all of the same user's punch chord. Attempting
                // controller 8 at this point instead selected 2C04,
                // 2C02, or entered 2A2A and timed out.
                // Never speculate another synthetic pulse or force 3200.
                if(!blocked && XM87183Preflight::shouldDecline(
                        earlyCancel,unsafeConsumedPunchReplay,
                        rw(ram,p+0xC0u),rw(ram,p+0xCAu))){
                    // Queue was prepared but must NOT touch +256/+259/A0.
                    // Keep native super and inputs intact. Requiring a
                    // fresh deliberate motion for a retry is safer than
                    // replaying the same partially consumed chord.
                    event(now,side,ram,
                          "DHC_183_NATIVE_PUNCH_ALREADY_CONSUMED_SKIP_TAG");
                    lastAcceptedAt[side]=selectedAt;
                    XM87143Queue::state().discard(side);
                    cooldownUntil[side]=liveLifetimeTest?now:now+6u;
                    continue;
                }
                if(!blocked && unsafeConsumedPunchReplay &&
                   !XM87183Preflight::freshNativeTagEdge(
                         rw(ram,p+0xC0u),rw(ram,p+0xCAu)))
                    event(now,side,ram,
                          "DHC_183_REPLAY_ONLY_UNSAFE_INPUT_TRACE_BYPASS");
                if(blocked){
                    if(firstCheck)event(now,side,ram,blocked);
                    // A fresh command inside an unverified 2E follow-up
                    // is recognized but NEVER held until the next
                    // super action: that previously allowed false
                    // deferred controller-8 handoffs. The player may
                    // deliberately enter a new command on a verified
                    // phase instead.
                    if((!ownedSelected && entry->known->fighter!=reserveId) ||
                       (liveLifetimeTest && !liveSuperWindow) ||
                       (familyTest && originalSuper.live &&
                        originalSuper.phase==XM87189Family::CONTINUATION &&
                        !XM87184Life::canTag(ryuLife,
                             XM87184Life::recoveryOptIn() &&
                             recoveryReboundTest))){
                        lastAcceptedAt[side]=selectedAt;
                        XM87143Queue::state().discard(side);
                    }
                    continue;
                }
                originalX[side]=rw(ram,p+0x0Cu);
                opponentX[side]=rw(ram,0x4400u+0x0Cu);
                if(ownedSelected){
                    ownedCommand[side]=XM87192Owner::state().pending[side];
                    // This is only a legacy display fallback, never a
                    // super selector in reserve-owned 87.192 mode.
                    command[side]=(ownedCommand[side].buttonMask&0x38u)?
                        XM87143Queue::QCF_KK:XM87143Queue::QCF_PP;
                    event(now,side,ram,
                        "DHC_192_RESERVE_INPUT_OWNER_SELECTED_NO_SUPER_LIST");
                }else command[side]=entry->known->command;
                outgoing[side]=pointId;incoming[side]=reserveId;
                // 87.158 restore the *actual* 87.151 Ken-Hadoken route.
                // 87.151 armed controller 8 at 1857 (+4E=28), ROM 3200
                // at 1858, Ken point swap at 1872, kickless 2A08 at
                // 1873 and Hadoken 2C00 at 1875. Stalling to timer<=8 in
                // 87.157 failed: it never reached Ken (user capture).
                // Do not replace these genuine observations with a
                // guessed late-super or variable-combination command.
                const unsigned remaining=rw(ram,p+0x4Eu);
                // The 87.185 source begins the actual attack at +4E=40
                // but prior 87.185 TEST only allows early tag <=3C.
                // This is a separate, opt-in four-frame ROM acceptance
                // experiment, NEVER startup or recovery or reused input.
                const bool probeFirstFour=firstFourAttackTest &&
                    XM87186FirstAttack::eligible(ryuLife,remaining,
                        rw(ram,p+0xC0u),rw(ram,p+0xCAu));
                lastAcceptedAt[side]=selectedAt;
                lastKenTargetAction[side]=0u;
                XM87143Queue::state().discard(side);
                if(proofNative){
                    // Stage 87.156: observe Ryu's *natural* super completion.
                    // Never write +256/+259/+A0/D6 or fake a 3200 script.
                    phase[side]=WAIT_RYU_SUPER_END;
                    started[side]=now;
                    kenNeutralAt[side]=0u;
                    event(now,side,ram,
                          "PROOF_156_RYU_SUPER_TO_NATIVE_TAG_WAIT_NO_RAM_WRITES");
                    continue;
                }
                if(liveSuperWindow &&
                   XM87191Lifetime::canArm(originalSuper,now,selectedAt,ryuLife)){
                    // 87.191: ORIGINAL ROM super is still alive. That is
                    // the sole action-timing condition for a fresh DHC.
                    // The proven controller 8, original 3200, Ken arrival,
                    // input isolation and abort remain unchanged.
                    armedFromFamily[side]=true;
                    event(now,side,ram,
                        "DHC_191_NATIVE_SUPER_LIFETIME_FRESH_HANDOFF");
                    armOutgoingTag(ram,now,side,selectedAt);
                    continue;
                }
                if(familyAttack){
                    // Shared, original-ROM 2E-family ATTACK inlet.
                    // Reuses the established Ryu->Ken controller script,
                    // input isolation, ROM acceptance, and abort path.
                    // Other fighters remain read-only until their native
                    // tag-entry contract is captured.
                    armedFromFamily[side]=true;
                    event(now,side,ram,
                        "DHC_189_ROM_SUPER_FAMILY_FRESH_ATTACK_TAG_TEST");
                    armOutgoingTag(ram,now,side,selectedAt);
                    continue;
                }
                if((remaining==0u && !recoveryTest) ||
                   (remaining>60u && !probeFirstFour) ||
                   rb(ram,p+0xD6u)!=0x0Au){
                    abort(now,side,ram,
                        "DHC_BLOCK_RYU_SUPER_TIMER_OR_CONTROLLER_INVALID");
                    continue;
                }
                const unsigned verifiedWindow=earlyCancel?0x1Cu:8u;
                if(recoveryTest){
                    // Experimental recovery branch is *still* the ROM's
                    // Shinkuu action 2E0A, but never its terminal script.
                    // Existing native input history guard remains active.
                    event(now,side,ram,
                          "DHC_184_RECOVERY_PHASE_NATIVE_TAG_EXPERIMENT");
                    armOutgoingTag(ram,now,side,selectedAt);
                }else if(probeFirstFour){
                    event(now,side,ram,
                          "DHC_186_TEST_FIRST_FOUR_ATTACK_FRESH_EDGE");
                    armOutgoingTag(ram,now,side,selectedAt);
                }else if(XM87188Attack::eligible(
                       ryuLife,fullAttackTest,remaining,
                       rw(ram,p+0xC0u),rw(ram,p+0xCAu))){
                    // 2A02 and 2A28 both appear in full genuine super
                    // ATTACK lifetimes. Preserve fresh current-frame
                    // input, preflight and wrong native action abort.
                    event(now,side,ram,ryuLife.mode==0x2A02u?
                          "DHC_188_NATIVE_ATTACK_2A02_FRESH_EDGE_TEST":
                          "DHC_188_NATIVE_ATTACK_2A28_FRESH_EDGE_TEST");
                    armOutgoingTag(ram,now,side,selectedAt);
                }else if(XM87185Gate::directActiveAttack(
                       ryuLife,earlyActiveDirectTest,remaining,
                       rw(ram,p+0xC0u),rw(ram,p+0xCAu))){
                    // 87.184: seven early active-phase inputs were
                    // accepted by the matcher but thrown away when the
                    // arbitrary +4E=28 cutoff arrived. All seven were
                    // already original 2E08/step04 with a clean native
                    // punch history at the actual QCF+2P/3P press.
                    // TEST: arm now while the fresh physical input is
                    // available. Do NOT generate the unsafe 87.179
                    // delayed synthetic edge. 87.183 C0/CA preflight
                    // ran above, and the ROM must still accept controller 8.
                    event(now,side,ram,
                          "DHC_185_EARLY_ACTIVE_DIRECT_FRESH_EDGE_TEST");
                    armOutgoingTag(ram,now,side,selectedAt);
                }else if(remaining>verifiedWindow){
                    phase[side]=WAIT_RYU_SUPER_END;
                    started[side]=now;
                    event(now,side,ram,earlyCancel?
                          "DHC_158_WAIT_RYU_TIMER_28_KEN_HADOKEN_BASELINE":
                          "DHC_WAIT_RYU_SUPER_TIMER_08");
                }else{
                    armOutgoingTag(ram,now,side,selectedAt);
                    if(earlyCancel)
                        event(now,side,ram,
                              "DHC_158_RESTORED_151_KEN_ARRIVAL_HANDOFF");
                }
                continue;
            }
            if(phase[side]==WAIT_RYU_SUPER_END){
                if(proofNative){
                    if(pointId!=outgoing[side] || reserveId!=incoming[side]){
                        abort(now,side,ram,"PROOF_RYU_ROSTER_CHANGED");
                    }else if(rw(ram,p+0x60u)==0u &&
                             ((rw(ram,p+0xA0u)&0xFF00u)==0u)){
                        if(!kenNeutralAt[side])kenNeutralAt[side]=now;
                        if(now>=kenNeutralAt[side]+2u){
                            phase[side]=SEND_ROM_TAG_INPUT;
                            inputBegin[side]=now+2u;
                            event(now,side,ram,
                                  "PROOF_156_NATIVE_HP_HK_TAG_INPUT_ARMED");
                        }
                    }else if(now>started[side]+340u){
                        abort(now,side,ram,
                              "PROOF_RYU_SUPER_NEVER_NATURALLY_ENDED");
                    }else kenNeutralAt[side]=0u;
                    continue;
                }
                const unsigned left=rw(ram,p+0x4Eu);
                // No mutation while waiting for the historically verified
                // 87.151 +4E=28 early entry or a separate legacy window.
                if(pointId!=outgoing[side] ||
                   reserveId!=incoming[side] ||
                   // Do not carry an aged input into the recovery
                   // phase. This boundary is exact ROM step/script,
                   // not a fabricated super duration or tag timer.
                   !XM87184Life::canTag(ryuLife,false) ||
                   left==0u || now>started[side]+52u){
                    abort(now,side,ram,"DHC_RYU_SUPER_LEFT_SAFE_WINDOW");
                }else if(left<=(earlyCancel?0x1Cu:8u)){
                    event(now,side,ram,earlyCancel?
                          "DHC_158_RYU_TIMER_28_151_ACCEPTED":
                          "DHC_LATE_SUPER_TIMER_WINDOW_ACCEPTED");
                    if(earlyCancel && !delayedPrimeExperimental){
                        // The old 87.179 speculative prime issued attacks
                        // during Ryu's super and intermittently launched
                        // native 0002/2C02/2C04 instead of tag 3200.
                        // Preserve the live super and release DHC-only
                        // bookkeeping rather than pushing controller 8
                        // without a verified native acceptance signal.
                        event(now,side,ram,
                             "DHC_180_UNVERIFIED_DELAYED_PRIME_SKIPPED");
                        abort(now,side,ram,
                             "DHC_180_DELAYED_HANDOFF_RELEASED_NO_NATIVE_EDGE");
                    }else if(earlyCancel){
                        // Only delayed tags need a fresh native button
                        // edge. There is no pressed edge left by the time
                        // the super reaches +4E<=28 (four failures in
                        // the supplied 87.178 observer trace).
                        phase[side]=PRIME_DELAYED_TAG_EDGE;
                        primeFrame[side]=now+1u;
                        primeSent[side]=false;
                        event(now,side,ram,
                              "DHC_179_DELAYED_TAG_EDGE_PRIME_SCHEDULED");
                    }else{
                        armOutgoingTag(ram,now,side,lastAcceptedAt[side]);
                    }
                }
                continue;
            }
            if(phase[side]==PRIME_DELAYED_TAG_EDGE){
                const bool primed=primeSent[side] &&
                    now==primeFrame[side] &&
                    pointId==outgoing[side] &&
                    reserveId==incoming[side] &&
                    rw(ram,p+0x60u)==0x2E08u &&
                    (rw(ram,p+0xA0u)==0x2A28u ||
                     rw(ram,p+0xA0u)==0x2A02u) &&
                    rb(ram,p+0xD6u)==0x0Au &&
                    rw(ram,p+0x4Eu)>0u &&
                    rw(ram,p+0x4Eu)<=0x1Cu;
                if(!primed){
                    abort(now,side,ram,
                          "DHC_179_DELAYED_TAG_EDGE_PRIME_REJECTED");
                }else{
                    event(now,side,ram,
                          "DHC_179_NATIVE_2P_EDGE_PRIMED_BEFORE_CONTROLLER_08");
                    armOutgoingTag(ram,now,side,lastAcceptedAt[side]);
                    event(now,side,ram,
                          "DHC_179_CONTROLLER_08_ARMED_AFTER_INPUT_PRIME");
                }
                continue;
            }
            if(phase[side]==SEND_ROM_TAG_INPUT){
                // Only the game's native HP+HK input decoder can choose 3200.
                // This is a strict, bounded input-only experiment.
                if(pointId!=outgoing[side] || reserveId!=incoming[side]){
                    abort(now,side,ram,"PROOF_NATIVE_TAG_ROSTER_CHANGED");
                }else if(rw(ram,p+0x60u)==0x3200u &&
                         rw(ram,p+0xA0u)==0x2A00u){
                    phase[side]=WAIT_NATIVE_SWAP;
                    started[side]=now;
                    controllerAcceptedAt[side]=now;
                    event(now,side,ram,
                          "PROOF_156_ROM_HP_HK_NATIVE_3200_ACCEPTED");
                }else if(now>inputBegin[side]+18u){
                    abort(now,side,ram,
                          "PROOF_NATIVE_HP_HK_TAG_NOT_ACCEPTED_NO_RAM_WRITES");
                }
                continue;
            }
            if(phase[side]==WAIT_TAG_CONTROLLER){
                // ROM scheduler must select outgoing action 3200 itself,
                // not merely display Ryu's 3200 sprite for one frame.
                event(now,side,ram,"NATIVE_TAG_CONTROLLER_WAIT_FRAME");
                // All five 87.184 experimental 2E0A handoffs executed
                // the SAME genuine 68000 path: +256 08->00,
                // +A0 002A->2A00, then PC 010392 reselected 2E08
                // from script 0D3070 (D0=2E/D1=08). The old bridge
                // rejected the action change and left a restarted
                // Shinkuu instead of tagging. This TEST-only route
                // recognizes that exact one-frame rebound, then
                // reuses the already resolved original tag script
                // 32/00. It is a HOST bridge after ROM command
                // consumption, not a claim that ROM selected 3200.
                const bool recoveryRebound=
                    XM87185Gate::exactRecoveryRebound(
                       recoveryReboundTest && XM87184Life::recoveryOptIn(),
                       unsigned(phase[side]),pointId,reserveId,
                       capturedAction[side],captured08[side],
                       capturedScript[side],
                       rw(ram,p+0x60u),rw(ram,p+0xA0u),
                       rw(ram,p+0x08u),rl(ram,p+0x34u),
                       rb(ram,p+0xD6u),rb(ram,p+0x256u),
                       rb(ram,p+0x259u),started[side],now);
                if(pointId!=outgoing[side] || reserveId!=incoming[side]){
                    abort(now,side,ram,"CONTROLLER_FIGHTER_CHANGED_BEFORE_TAG");
                }else if(rw(ram,p+0x60u)==0x3200u &&
                         rw(ram,p+0xA0u)==0x2A00u){
                    if(side==0u && earlyCancel)
                        oldRyuProjectiles.activate(ram,now);
                    phase[side]=WAIT_NATIVE_SWAP;
                    controllerAcceptedAt[side]=now;
                    event(now,side,ram,"NATIVE_CONTROLLER_ACCEPTED_OUTGOING_3200");
                }else if(recoveryRebound ||
                          (rw(ram,p+0xA0u)==0x2A00u &&
                           rb(ram,p+0x256u)==0u &&
                           rw(ram,p+0x60u)==capturedAction[side] &&
                           (capturedAction[side]==0x2E08u ||
                            (armedFromFamily[side] &&
                             (capturedAction[side]&0xFF00u)==0x2E00u &&
                             rb(ram,p+0xD6u)==capturedD6[side])) &&
                           // Keep already working 87.183 attack path.
                           (earlyCancel ||
                            (rb(ram,p+0x259u)==0x08u &&
                             rb(ram,p+0xD6u)==capturedD6[side])))){
                    // 87.146 real recording: controller 8 WAS executed by
                    // 68000 at 1778/2365/2756; A0 switched to 2A00 and
                    // controller 256 was consumed (08->00), but +60 stayed
                    // 2E08, +259 stayed 08 and +4E was super timer 0040.
                    // A natural ROM tag at 2886 instead has +60=3200,
                    // +A0=2A00, +259=00 and +4E=001E.
                    //
                    // This is the DHC-only bridge between the genuine
                    // ROM-accepted controller and the native outgoing tag
                    // animation/timer. Never change a normal tag.
                    if(recoveryRebound)
                        event(now,side,ram,
                           "DHC_185_NATIVE_RECOVERY_REBOUND_ROM_CHOSE_2E08_HOST_TAG_BRIDGE");
                    if(!setNativeScript(ram,rom,size,p,pointId,0x32u,0u)){
                        abort(now,side,ram,"NATIVE_3200_SCRIPT_INIT_FAILED");
                        continue;
                    }
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,2u);
                    ww(ram,p+0x0Au,0u);
                    ww(ram,p+0x4Eu,0x001Eu);
                    wb(ram,p+0x259u,0u);
                    // The previous 87.147 bridge played the exact 3200
                    // script, but never switched Ryu's per-fighter *action
                    // controller* +D6 out of the super family. On the
                    // decrypted ROM, Ryu's +D6=0x14 goes via 0x0337BA's
                    // state table to PC 0x0341F6 -> PC 0x011B00.
                    // PC 0x011B1E drives each tag frame, decrements +4E,
                    // then recognizes +3C=01 at script 0x0D1DEC and
                    // invokes native exit controller 2. This is why the
                    // 87.147 Ryu froze forever in the tag pose.
                    // Ken's separate state table *also* uses +D6=0x14
                    // (PC 0x035F28) for the ROM-owned tag handler.
                    wb(ram,p+0xD6u,0x14u);
                    if(side==0u && earlyCancel)
                        oldRyuProjectiles.activate(ram,now);
                    phase[side]=WAIT_NATIVE_SWAP;
                    controllerAcceptedAt[side]=now;
                    event(now,side,ram,recoveryRebound?
                        "DHC_185_RECOVERY_REBOUND_TO_ORIGINAL_3200_SCRIPT":
                        "DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT");

                }else if(rb(ram,p+0x256u)==0u &&
                          rw(ram,p+0x60u)!=capturedAction[side] &&
                          rw(ram,p+0x60u)!=0x3200u){
                    // Original controller sometimes chose 0002 or 2C02,
                    // not 3200. Preserve ROM action; log true outcome.
                    abort(now,side,ram,
                          XM87187Front::wrongAction(rw(ram,p+0x60u)));
                }else if(now-started[side]>12u){
                    // Restore only a verified UNCONSUMED staging command.
                    // Once ROM selected any action, it owns all state.
                    const bool unconsumed=
                        pointId==outgoing[side] && reserveId==incoming[side] &&
                        rw(ram,p+0x60u)==capturedAction[side] &&
                        rw(ram,p+0xA0u)==0x002Au &&
                        rb(ram,p+0x256u)==0x08u &&
                        rb(ram,p+0x259u)==0x08u;
                    if(unconsumed){
                        ww(ram,p+0xA0u,capturedMode[side]);
                        wb(ram,p+0x256u,captured256[side]);
                        wb(ram,p+0x259u,captured259[side]);
                        ww(ram,p+0x06u,captured06[side]);
                        ww(ram,p+0x08u,captured08[side]);
                        ww(ram,p+0x0Au,captured0A[side]);
                        event(now,side,ram,"DHC_UNCONSUMED_COMMAND_RESTORED");
                    }else event(now,side,ram,"DHC_CONSUMED_ROM_ACTION_LEFT_INTACT");
                    abort(now,side,ram,"NATIVE_TAG_CONTROLLER_REJECTED_12F");
                }
                continue;
            }
            if(phase[side]==WAIT_NATIVE_SWAP){
                // Frame-by-frame native scheduler and partner-state capture:
                // if the ROM refuses to swap, one postmortem trace reveals
                // what the point actor actually did after script selection.
                event(now,side,ram,"NATIVE_SWAP_WAIT_FRAME");
                if(pointId==incoming[side] && reserveId==outgoing[side]){
                    if(side==0u && earlyCancel)
                        oldRyuProjectiles.swapped(now);
                    phase[side]=WAIT_LANDING;
                    kenExitObserved[side]=false;
                    kenSuperConfirmed[side]=false;
                    event(now,side,ram,"REAL_ROM_POINT_RESERVE_SWAP");
                }else if(proofNative){
                    if(now>started[side]+120u)
                        abort(now,side,ram,
                              "PROOF_NATIVE_ROM_TAG_SWAP_TIMEOUT_NO_RAM_WRITES");
                }else if(now>controllerAcceptedAt[side]+2u &&
                         rw(ram,p+0x60u)==0x2E08u){
                    // If the game's own super owner reclaims the actor
                    // during a failed DHC, do not leave the DHC-only
                    // outgoing +D6=14 route attached to Ryu's Hadoken.
                    if(pointId==outgoing[side]&&reserveId==incoming[side]){
                        wb(ram,p+0xD6u,capturedD6[side]);
                        ww(ram,p+0xA0u,capturedMode[side]);
                    }
                    abort(now,side,ram,
                          "ROM_REASSERTED_RYU_HADOKEN_AFTER_TAG_3200");
                }else if(now-started[side]>60u){
                    // The 87.147 failure left the outgoing actor
                    // frozen even after this watchdog expired. Request
                    // the same native exit controller 2 that the ROM
                    // itself writes at PC 0x011B38 (never synthetic swap).
                    // Only touch the original Ryu actor if it remains
                    // the point with the same reserve, active action 3200
                    // and DHC-owned tag state.
                    if(pointId==outgoing[side]&&reserveId==incoming[side] &&
                       rw(ram,p+0x60u)==0x3200u &&
                       rb(ram,p+0xD6u)==0x14u){
                        wb(ram,p+0x256u,2u);
                        wb(ram,p+0x259u,2u);
                        event(now,side,ram,"DHC_NATIVE_EXIT_02_WATCHDOG_RECOVERY");
                    }
                    abort(now,side,ram,"NO_ROM_OWNERSHIP_SWAP_WITHIN_60_FRAMES");
                }
                continue;
            }
            if(phase[side]==WAIT_LANDING){
                event(now,side,ram,"NATIVE_KEN_INCOMING_WAIT_FRAME");
                if(pointId!=incoming[side] || reserveId!=outgoing[side]){
                    abort(now,side,ram,"FIGHTER_CHANGED_DURING_DHC_ARRIVAL");
                    continue;
                }
                // User recording 87.149:
                // 1865 ROM ownership swapped; 1866 incoming Ken 2A0C,
                // flying attack hitbox already present, still 64px high.
                // At 1881 Ken was 5px from floor: setting D6=12 alone
                // left 2A0C active through frame 1890. 2A08 only
                // appeared on 1891. At 2690-2695 he held Y=461 (11px
                // above floor) then jumped directly into 2A0E at 2696;
                // the old 8px trigger missed him entirely.
                //
                // For this DHC ONLY: suppress attack on Ken's FIRST
                // ROM-confirmed incoming frame. No floor proximity
                // guesses or waiting until 2A0E has already kicked.
                const unsigned incomingAction=rw(ram,p+0x60u);
                if(proofNative){
                    // Let the genuine Ken entry finish WITHOUT attack,
                    // location, or controller mutation. When Ken's real
                    // action returns to normal, the 2P/2K test may begin.
                    const unsigned a0=rw(ram,p+0xA0u)&0xFF00u;
                    if((incomingAction==0u && a0==0u) ||
                       (incomingAction==0x0002u && a0==0x0200u)){
                        phase[side]=WAIT_KEN_EXIT;
                        kenExitObserved[side]=true;
                        kenExitAt[side]=now;
                        kenNeutralAt[side]=0u;
                        event(now,side,ram,
                              "PROOF_156_KEN_NATIVE_INPUT_READY_NO_KICK_OVERRIDES");
                    }else if(now>started[side]+320u)
                        abort(now,side,ram,
                              "PROOF_KEN_NATIVE_ENTRY_STILL_BUSY");
                    continue;
                }
                const bool kenEntry=(incomingAction==0x2A0Cu ||
                                     incomingAction==0x2A0Eu) &&
                                     rb(ram,p+0xD6u)==0x10u;
                const unsigned floor=rw(ram,p+0x254u);
                // 87.194: retain the authentic native airborne launch
                // Y, fractional positions and native X/Y velocity. The
                // previous path set Y=floor and cleared motion on this
                // very frame; that TELEPORTED Ken and left spurious entry
                // sounds/interactions. Sabretooth's captured native entry
                // naturally hands over to 0000/0000 and stands.
                //
                // Suppress Ken's attacking 2A0C art by selecting his real
                // non-attacking 2A08 script, but do NOT zero his motion,
                // force neutral, place him on the floor, or release his
                // reserve-owned super command before actual landing.
                if(passiveJumpTest && kenEntry){
                    const unsigned foe=rw(
                        ram,0x4000u+(side^1u)*0x400u+0x0Cu);
                    const unsigned x=rw(ram,p+0x0Cu);
                    const unsigned y=rw(ram,p+0x10u);
                    if(!jumpArrival[side].begin(
                           side,pointId,now,int(x),int(y),int(floor),
                           int(foe),originalX[side]<opponentX[side])){
                        abort(now,side,ram,
                              "DHC_194_ROM_AIRBORNE_ENTRY_NOT_CONFIRMED");
                        continue;
                    }
                    const auto leave=XM87142::resolve(
                        rom,size,pointId,0x2Au,0x08u);
                    if(!leave.valid || leave.script!=0xD567Eu ||
                       !setNativeScript(ram,rom,size,p,pointId,0x2Au,0x08u)){
                        abort(now,side,ram,
                              "DHC_194_NONATTACKING_NATIVE_ENTRY_ROM_UNRESOLVED");
                        continue;
                    }
                    wb(ram,p+0xA4u,0u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,2u);ww(ram,p+0x0Au,0u);
                    ww(ram,p+0xA0u,0x2A00u);
                    wb(ram,p+0xD6u,0x12u);
                    // Intentionally leave +0C X, +10 Y and +14..+20
                    // velocity/acceleration EXACTLY as the ROM produced.
                    kenKickReassertions[side]=0u;
                    phase[side]=WAIT_NATIVE_PASSIVE_JUMP;
                    event(now,side,ram,
                        "DHC_194_ROM_AIRBORNE_ARRIVAL_MOTION_PRESERVED");
                    continue;
                }
                if(kenEntry && floor>=128u && floor<=768u){
                    const auto leave=XM87142::resolve(
                        rom,size,pointId,0x2Au,0x08u);
                    if(!leave.valid || leave.script!=0xD567Eu){
                        abort(now,side,ram,"KEN_KICKLESS_2A08_ROM_LOOKUP_FAILED");
                        continue;
                    }
                    // Place the actor at the original Ryu-side target
                    // and the actual native floor. The charging streaks
                    // remain separate and are NOT changed or suppressed.
                    const unsigned foe=rw(ram,0x4400u+0x0Cu);
                    int pos=int(foe)+(originalX[side]<opponentX[side]?-48:48);
                    if(pos<192)pos=192;
                    if(pos>768)pos=768;
                    const unsigned current=rl(ram,p+0x0Cu);
                    wl(ram,p+0x0Cu,(unsigned(pos)<<16u)|(current&65535u));
                    ww(ram,p+0x10u,floor);
                    wb(ram,p+0x4Bu,originalX[side]<opponentX[side]?1u:0u);
                    wl(ram,p+0x14u,0u);wl(ram,p+0x18u,0u);
                    wl(ram,p+0x1Cu,0u);wl(ram,p+0x20u,0u);
                    // D6=12 by itself DID NOT suppress the kick in
                    // 87.149. Initialize the *non-attacking native
                    // animation* immediately, just as Ryu's 3200
                    // required both ROM action+handler ownership.
                    if(!setNativeScript(ram,rom,size,p,pointId,0x2Au,0x08u)){
                        abort(now,side,ram,"KEN_KICKLESS_2A08_INIT_FAILED");
                        continue;
                    }
                    wb(ram,p+0xA4u,0u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,2u);ww(ram,p+0x0Au,0u);
                    ww(ram,p+0xA0u,0x2A00u);
                    wb(ram,p+0xD6u,0x12u);
                    phase[side]=WAIT_KEN_EXIT;
                    kenExitAt[side]=now;
                    kenKickReassertions[side]=0u;
                    kenExitObserved[side]=false;
                    event(now,side,ram,
                        "DHC_KEN_2A0C_ATTACK_SUPPRESSED_IMMEDIATE_2A08");
                }else if(incomingAction==0x2A0Eu){
                    event(now,side,ram,"KEN_LATE_TAG_KICK_FALLBACK_PENDING");
                }
                if(phase[side]==WAIT_LANDING &&
                   now-started[side]>135u)
                    abort(now,side,ram,"KEN_ARRIVAL_TIMEOUT");
                continue;
            }
            if(phase[side]==WAIT_NATIVE_PASSIVE_JUMP){
                if(pointId!=incoming[side] || reserveId!=outgoing[side] ||
                   !jumpArrival[side].active ||
                   jumpArrival[side].fighter!=pointId){
                    abort(now,side,ram,"DHC_194_NATIVE_JUMP_ACTOR_LOST");
                    continue;
                }
                const unsigned actualAction=rw(ram,p+0x60u);
                if(actualAction==0x2A0Cu || actualAction==0x2A0Eu){
                    // The original tag-in scheduler may repeat the entry
                    // attack before completing the native descent.
                    // Keep the real, non-attacking 2A08 animation instead
                    // of letting a kick/charge interrupt the arrival.
                    if(++kenKickReassertions[side]>2u ||
                       !setNativeScript(ram,rom,size,p,pointId,0x2Au,0x08u)){
                        abort(now,side,ram,
                            "DHC_194_NATIVE_ENTRY_KICK_REASSERTION_UNRESOLVED");
                        continue;
                    }
                    wb(ram,p+0xD6u,0x12u);
                    wb(ram,p+0xA4u,0u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    event(now,side,ram,
                        "DHC_194_NATIVE_ENTRY_KICK_REASSERTED_SUPPRESSED");
                }
                const unsigned y=rw(ram,p+0x10u);
                const bool wasDescending=jumpArrival[side].descended;
                const int targetX=jumpArrival[side].position(now,int(y));
                if(jumpArrival[side].descended && !wasDescending)
                    event(now,side,ram,
                        "DHC_194_NATIVE_INCOMING_JUMP_DESCENT_OBSERVED");
                const unsigned priorX=rl(ram,p+0x0Cu);
                if(int(priorX>>16u)!=targetX)
                    wl(ram,p+0x0Cu,
                        (unsigned(targetX)<<16u)|(priorX&65535u));
                // Frame-by-frame X/Y/actions in the existing handoff
                // TSV reveal whether original 2A08 keeps descending.
                // Never hide a stalled ROM entry by forcing Y=floor.
                event(now,side,ram,
                    "DHC_194_ORIGINAL_ROM_JUMP_POSITION_AND_ACTION");
                if(jumpArrival[side].ready(pointId,int(y))){
                    // The original game, not a fabricated elapsed frame
                    // count, confirms Y reached the native floor. Only
                    // NOW enter the original ROM idle script, matching
                    // Sabretooth's passive stand-after-entry contract.
                    const auto idle=XM87142::resolve(rom,size,pointId,0u,0u);
                    if(!idle.valid || idle.script!=0xD39FAu ||
                       !setNativeScript(ram,rom,size,p,pointId,0u,0u)){
                        abort(now,side,ram,
                            "DHC_194_NATIVE_LANDING_IDLE_ROM_UNRESOLVED");
                        continue;
                    }
                    ww(ram,p+0xA0u,0x002Au);
                    wb(ram,p+0xD6u,0x10u);
                    wb(ram,p+0xA4u,0u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,0u);
                    ww(ram,p+0x0Au,0u);
                    // Clearing velocity AT GROUND contact is safe;
                    // the preceding airborne native motion was intact.
                    wl(ram,p+0x14u,0u);wl(ram,p+0x18u,0u);
                    wl(ram,p+0x1Cu,0u);wl(ram,p+0x20u,0u);
                    kenExpectedAction[side]=kenExpectedScript[side]=0u;
                    kenSuperConfirmed[side]=false;
                    kenNeutralSince[side]=0u;
                    inputBegin[side]=now+1u;
                    phase[side]=DELIVER_NATIVE_INPUT;
                    event(now,side,ram,
                        "DHC_194_REAL_NATIVE_LANDING_IDLE_BEFORE_RESERVE_SUPER");
                }else if(now>jumpArrival[side].startFrame+135u){
                    // Only a watchdog, NEVER fake a descent or floor.
                    event(now,side,ram,
                        "DHC_194_NATIVE_ENTRY_NO_DESCENT_DID_NOT_FORCE_LANDING");
                    abort(now,side,ram,"DHC_194_NATIVE_JUMP_WATCHDOG_ABORT");
                }
                continue;
            }
            if(phase[side]==WAIT_KEN_EXIT){
                if(pointId!=incoming[side] || reserveId!=outgoing[side]){
                    abort(now,side,ram,"KEN_EXIT_LOST_FIGHTER_OWNERSHIP");
                    continue;
                }
                const unsigned kenAction=rw(ram,p+0x60u);
                const bool originalKick=kenAction==0x2A0Cu ||
                                        kenAction==0x2A0Eu;
                if(originalKick && !proofNative){
                    // Protect against one frame of the ROM's pending
                    // tag controller reasserting 2A0C. Do not let this
                    // complete into a second kick, and do not repeat
                    // animation overrides indefinitely.
                    event(now,side,ram,"KEN_ENTRY_ATTACK_REASSERTED_AFTER_CANCEL");
                    ++kenKickReassertions[side];
                    if(kenKickReassertions[side]>1u ||
                       now>kenExitAt[side]+2u ||
                       !setNativeScript(ram,rom,size,p,pointId,0x2Au,0x08u)){
                        abort(now,side,ram,"KEN_KICK_SUPPRESSION_REJECTED");
                        continue;
                    }
                    wb(ram,p+0xD6u,0x12u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    wb(ram,p+0xA4u,0u);
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,2u);ww(ram,p+0x0Au,0u);
                    event(now,side,ram,"KEN_KICK_REASSERTION_CANCELLED_ONCE");
                    continue;
                }
                if(kenAction==0x2A08u && !kenExitObserved[side]){
                    kenExitObserved[side]=true;
                    event(now,side,ram,"KEN_ROM_NONATTACKING_2A08_CONFIRMED");
                }
                // 87.151(1) proves Ken QCF+PP enters 2E00/D6722,
                // QCF+KK enters 2E02/D6976, with A0=2A02 via the
                // normal ROM command parser. Our former 2C00+D6=0A
                // handoff ran a Hadoken in an infinite animation loop.
                // Do NOT overwrite D6, A0, or any 2E script here.
                // Ken must finish native 2A08, return to normal input
                // control, and execute his own native motion/chord.
                // Recorded natural Ken supers (87.151(1) 2456/2624)
                // began from 0002/A0=020E, not only from action 0000.
                const unsigned kenMode=rw(ram,p+0xA0u)&0xFF00u;
                const bool actionable=
                    (kenAction==0x0000u && kenMode==0u) ||
                    (kenAction==0x0002u && kenMode==0x0200u);
                const bool neutral=kenExitObserved[side] &&
                   actionable && now>kenExitAt[side]+2u;
                if(earlyCancel && kenExitObserved[side] &&
                   kenAction==0x2A08u &&
                   now>kenExitAt[side] && now<=kenExitAt[side]+3u){
                    // Restore the frame-accurate 87.150 cancellation:
                    // the first ROM Ken tag kick (2A0C) was replaced with
                    // its verified nonattacking 2A08 exit. In 87.150 the
                    // next frame changed D6/A0 to a guessed action and the
                    // ROM instead ran 2C32 dash. In 87.156 we proved that
                    // *genuine neutral Ken* + D6=10/A0=002A accepts
                    // QCF+2P/2K as real supers. Reproduce that actual ROM
                    // state, then let the INPUT parser start 2E00/2E02.
                    const auto idle=XM87142::resolve(rom,size,0x14u,0u,0u);
                    const unsigned target=
                       command[side]==XM87143Queue::QCF_KK?0x02u:0x00u;
                    const unsigned expected=
                       command[side]==XM87143Queue::QCF_KK?0xD6976u:0xD6722u;
                    const auto sup=XM87142::resolve(rom,size,0x14u,0x2Eu,target);
                    if(rb(ram,p+0xD6u)!=0x12u ||
                       rl(ram,p+0x34u)!=0xD567Eu ||
                       !idle.valid || idle.script!=0xD39FAu ||
                       (!reserveOwnershipTest &&
                        (!sup.valid || sup.script!=expected))){
                        abort(now,side,ram,
                              "FAST_KEN_CANCEL_ROM_STATE_UNVERIFIED_NO_OVERRIDE");
                        continue;
                    }
                    // This is ROM-derived +00/00 neutral (from the user's
                    // 87.156 frame 5068), NOT an invented Hadoken/dash.
                    if(!setNativeScript(ram,rom,size,p,0x14u,0u,0u)){
                        abort(now,side,ram,
                              "FAST_KEN_CANCEL_IDLE_ROM_ENTRY_UNAVAILABLE");
                        continue;
                    }
                    ww(ram,p+0xA0u,0x002Au);
                    wb(ram,p+0xD6u,0x10u);
                    wb(ram,p+0x256u,0u);wb(ram,p+0x259u,0u);
                    wb(ram,p+0xA4u,0u);
                    ww(ram,p+0x06u,0u);
                    ww(ram,p+0x08u,0u);
                    ww(ram,p+0x0Au,0u);
                    wl(ram,p+0x14u,0u);
                    wl(ram,p+0x18u,0u);
                    wl(ram,p+0x1Cu,0u);
                    wl(ram,p+0x20u,0u);
                    kenExpectedAction[side]=reserveOwnershipTest?0u:(0x2E00u|target);
                    kenExpectedScript[side]=reserveOwnershipTest?0u:expected;
                    kenSuperConfirmed[side]=false;
                    kenNeutralSince[side]=0u;
                    inputBegin[side]=now+1u;
                    phase[side]=DELIVER_NATIVE_INPUT;
                    event(now,side,ram,
                          "FAST_157_KEN_2A08_CANCEL_TO_VERIFIED_ROM_IDLE");
                    continue;
                }
                if(neutral){
                    if(!kenNeutralAt[side])kenNeutralAt[side]=now;
                    if(now>=kenNeutralAt[side]+2u){
                        const unsigned target=
                          command[side]==XM87143Queue::QCF_KK?0x02u:0x00u;
                        const unsigned expected=
                          command[side]==XM87143Queue::QCF_KK?0xD6976u:0xD6722u;
                        const auto sup=XM87142::resolve(rom,size,pointId,0x2Eu,target);
                        if(!reserveOwnershipTest &&
                           (!sup.valid || sup.script!=expected)){
                            abort(now,side,ram,"KEN_NATIVE_INPUT_SUPER_ROM_UNRESOLVED");
                            continue;
                        }
                        kenExpectedAction[side]=reserveOwnershipTest?0u:(0x2E00u|target);
                        kenExpectedScript[side]=reserveOwnershipTest?0u:expected;
                        kenSuperConfirmed[side]=false;
                        kenNeutralSince[side]=0u;
                        inputBegin[side]=now+2u;
                        phase[side]=DELIVER_NATIVE_INPUT;
                        event(now,side,ram,"DHC_KEN_NATIVE_NEUTRAL_INPUT_ARMED");
                    }
                }else kenNeutralAt[side]=0u;
                if(phase[side]==WAIT_KEN_EXIT &&
                   now>kenExitAt[side]+180u)
                    abort(now,side,ram,"KEN_NATIVE_2A08_NEUTRAL_TIMEOUT");
                continue;
            }
            if(phase[side]==DELIVER_NATIVE_INPUT){
                if(pointId!=incoming[side] || reserveId!=outgoing[side]){
                    abort(now,side,ram,"KEN_NATIVE_INPUT_OWNERSHIP_CHANGED");
                    continue;
                }
                if(reserveOwnershipTest && ownedCommand[side].valid){
                    const unsigned d=XM87192Owner::Proposal::length(
                        ownedCommand[side].motion);
                    if(now==inputBegin[side])
                        event(now,side,ram,
                            "DHC_192_RESERVE_INPUT_NATIVE_PARSER_START");
                    if(now==inputBegin[side]+2u*(d-1u)+1u)
                        event(now,side,ram,
                            "DHC_192_RESERVE_INPUT_NATIVE_BUTTON_EDGE");
                    if(now==inputBegin[side]+2u*d+1u)
                        event(now,side,ram,
                            "DHC_192_RESERVE_INPUT_NATIVE_BUTTON_RELEASE");
                }else{
                    if(now==inputBegin[side])
                        event(now,side,ram,"DHC_KEN_NATIVE_QCF_INPUT_BEGIN");
                    if(now==inputBegin[side]+(earlyCancel?6u:13u))
                        event(now,side,ram,earlyCancel?
                              "FAST_157_KEN_NATIVE_TWO_BUTTON_EDGE":
                              "PROOF_156_KEN_NATIVE_TWO_BUTTON_EDGE");
                    if(now==inputBegin[side]+(earlyCancel?8u:16u))
                        event(now,side,ram,"DHC_KEN_NATIVE_INPUT_RELEASED");
                }
                // 87.192: only the reserve's REAL native ROM decision
                // may enter the super catalog. Nothing selects 2E00,
                // 2E02 or another assumed hyper on the host's behalf.
                // Wrong specials/normals do not count as a super, and
                // the existing abort/chain release handles rejection.
                if(reserveOwnershipTest && ownedCommand[side].valid){
                    const XM87184Life::Sample observed={
                        pointId,rw(ram,p+0x60u),rw(ram,p+0xA0u),
                        rw(ram,p+0x08u),rl(ram,p+0x34u),
                        rb(ram,p+0xD6u)};
                    if(XM87192Owner::state().confirm(
                          side,now,ownedCommand[side],
                          observed,rom,size)){
                        phase[side]=SUPER_STARTED;
                        kenSuperConfirmed[side]=true;
                        chainUsed[side]=true;
                        kenSuperAt[side]=now;
                        kenNeutralSince[side]=0u;
                        event(now,side,ram,
                            "DHC_192_RESERVE_NATIVE_ROM_2E_SUPER_ACCEPTED_AND_CATALOGED");
                        event(now,side,ram,"DHC_NATIVE_INPUT_DISPATCH_ACCEPTED");
                        XM87192Owner::state().clear(side);
                    }else if(now>inputBegin[side]+
                          ownedCommand[side].duration()+8u){
                        XM87192Owner::state().rejected(
                            side,now,ownedCommand[side],observed);
                        event(now,side,ram,
                            "DHC_192_RESERVE_ROM_DID_NOT_SELECT_SUPER_REFUSED");
                        abort(now,side,ram,
                            "DHC_192_NON_SUPER_NATIVE_ACTION_NOT_ACCEPTED");
                        XM87192Owner::state().clear(side);
                    }
                    continue;
                }
                const unsigned act=rw(ram,p+0x60u);
                const unsigned script=rl(ram,p+0x34u);
                const unsigned original=kenExpectedScript[side];
                const bool actualNativeSuper=
                    act==kenExpectedAction[side] &&
                    (rw(ram,p+0xA0u)==0x2A02u ||
                     rw(ram,p+0xA0u)==0x2A28u) &&
                    script>=original && script-original<0x80u &&
                    ((script-original)&15u)==0u;
                // Watch BOTH Ken super addresses concurrently; a valid
                // *other* super is not successful selected DHC execution.
                const bool nativeMode=
                    rw(ram,p+0xA0u)==0x2A02u ||
                    rw(ram,p+0xA0u)==0x2A28u;
                const bool repObserved=nativeMode && act==0x2E00u &&
                    script>=0xD6722u && script-0xD6722u<0x80u;
                const bool shinObserved=nativeMode && act==0x2E02u &&
                    script>=0xD6976u && script-0xD6976u<0x80u;
                if((repObserved||shinObserved) &&
                   lastKenTargetAction[side]!=act){
                    lastKenTargetAction[side]=act;
                    event(now,side,ram,repObserved?
                          "DHC_KEN_REPPA_2E00_ROM_SEEN":
                          "DHC_KEN_SHINRYUKEN_2E02_ROM_SEEN");
                }
                if((repObserved||shinObserved) && !actualNativeSuper){
                    abort(now,side,ram,"DHC_WRONG_KEN_NATIVE_SUPER_TARGET_UNLOCKED");
                }else if(actualNativeSuper){
                    phase[side]=SUPER_STARTED;
                    kenSuperConfirmed[side]=true;
                    chainUsed[side]=true;
                    kenSuperAt[side]=now;
                    kenNeutralSince[side]=0u;
                    event(now,side,ram,"DHC_KEN_NATIVE_SUPER_ROM_ACTION_CONFIRMED");
                    event(now,side,ram,"DHC_NATIVE_INPUT_DISPATCH_ACCEPTED");
                }else if(now>inputBegin[side]+68u){
                    event(now,side,ram,"DHC_NATIVE_INPUT_NO_SUPER_ROM_ACCEPT");
                    abort(now,side,ram,"KEN_INPUT_DISPATCH_TIMEOUT_UNLOCKED");
                }
                continue;
            }
            if(phase[side]==SUPER_STARTED){
                if(pointId!=incoming[side] || reserveId!=outgoing[side]){
                    // Native point swaps must unlock without modifying ROM
                    // fighter ownership or any player's current input.
                    phase[side]=IDLE;chainUsed[side]=false;
                    XM87143Queue::state().discard(side);
                    event(now,side,ram,"DHC_REARM_ON_NATIVE_POINT_SWAP");
                    continue;
                }
                const unsigned act=rw(ram,p+0x60u);
                const unsigned mode=rw(ram,p+0xA0u);
                // Native completion: Ken's real 2E00/2E02 script fully
                // exits to neutral. Never force a script to finish.
                if(act==0u && (mode&0xFF00u)==0u){
                    if(!kenNeutralSince[side])kenNeutralSince[side]=now;
                    if(now>=kenNeutralSince[side]+2u){
                        phase[side]=IDLE;chainUsed[side]=false;
                        XM87143Queue::state().discard(side);
                        event(now,side,ram,"DHC_REARM_AFTER_NATIVE_SUPER_NEUTRAL");
                    }
                }else kenNeutralSince[side]=0u;
                if(phase[side]!=IDLE && now>kenSuperAt[side]+720u){
                    // Bookkeeping timeout ONLY. Never reinitialize/reset
                    // the fighter while the ROM is executing gameplay.
                    phase[side]=IDLE;chainUsed[side]=false;
                    XM87143Queue::state().discard(side);
                    event(now,side,ram,"DHC_REARM_OBSERVATION_WATCHDOG");
                }
            }
            // A terminal DHC exit releases its own lock. Fresh DHCs are
            // still gated by the next verified Ryu native super and a NEW
            // pressed reserve command, never by a held old chord.
        }
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87144
#endif
