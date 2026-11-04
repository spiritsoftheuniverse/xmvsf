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
    DELIVER_NATIVE_INPUT=8
};
static inline const char* phaseName(Phase p){
    switch(p){
      case TAG_OUT:return "ROM_TAG_OUT";
      case WAIT_TAG_CONTROLLER:return "WAIT_NATIVE_TAG_CONTROLLER";
      case WAIT_NATIVE_SWAP:return "WAIT_NATIVE_SWAP";
      case WAIT_LANDING:return "WAIT_NATIVE_LANDING";
      case WAIT_KEN_EXIT:return "NATIVE_KEN_KICKLESS_EXIT";
      case SUPER_STARTED:return "NATIVE_KEN_SUPER_START";
      case DELIVER_NATIVE_INPUT:return "NATIVE_KEN_INPUT_SEQUENCE";
      case FAILED:return "FAILED";
      default:return "IDLE";
    }
}
struct State {
    bool initialized=false,enabled=false,chainUsed[2]={false,false};
    Phase phase[2]={IDLE,IDLE};
    unsigned started[2]={0u,0u},outgoing[2]={0u,0u},incoming[2]={0u,0u};
    unsigned cooldownUntil[2]={0u,0u},lastAcceptedAt[2]={~0u,~0u};
    unsigned capturedMode[2]={0u,0u},capturedAction[2]={0u,0u};
    unsigned captured256[2]={0u,0u},captured259[2]={0u,0u};
    unsigned capturedD6[2]={0u,0u};
    unsigned captured06[2]={0u,0u},captured08[2]={0u,0u},
             captured0A[2]={0u,0u};
    unsigned controllerAcceptedAt[2]={0u,0u};
    unsigned kenExitAt[2]={0u,0u},kenSuperAt[2]={0u,0u};
    unsigned kenNeutralAt[2]={0u,0u},inputBegin[2]={0u,0u};
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
    unsigned lastFrame=~0u;
    unsigned lastQueuedCheck[2]={~0u,~0u};
    std::FILE* log=nullptr;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* setting=std::getenv("XM87144_DHC_NATIVE_TEST");
        enabled=setting && std::strcmp(setting,"1")==0;
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
        std::fprintf(stderr,"XM87.144 native DHC proof-of-concept enabled=%d\n",enabled?1:0);
    }
    void reset(){
        for(unsigned s=0;s<2u;++s){
            phase[s]=IDLE;chainUsed[s]=false;lastQueuedCheck[s]=~0u;
            lastAcceptedAt[s]=~0u;cooldownUntil[s]=0u;
            lastNormalRef[s]=~0u;
            kenExitAt[s]=kenSuperAt[s]=0u;
            kenNeutralAt[s]=inputBegin[s]=0u;
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
    void abort(unsigned now,unsigned side,const std::uint8_t*ram,
               const char*reason){
        phase[side]=FAILED;
        event(now,side,ram,reason);
        phase[side]=IDLE;
        // 87.150 left this true when Ken lost the point slot, so later
        // first supers were queued but could never enter the DHC gate.
        chainUsed[side]=false;
        kenSuperConfirmed[side]=false;
        kenExpectedAction[side]=kenExpectedScript[side]=0u;
        kenFallbackUsed[side]=false;kenNeutralSince[side]=0u;
        kenNeutralAt[side]=inputBegin[side]=0u;
        cooldownUntil[side]=now+6u;
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
    // 87.152: one-shot real CPS-2 controller stream. This is consumed
    // BEFORE CpsRwGetInp in the next real emulation frame, then the host
    // restores the physical SDL button array immediately after that read.
    // It does not set a sprite, A0, D6, or script address. The game's own
    // 68000 command decoder, controller and recovery logic own the super.
    struct InputMask { bool active=false;unsigned directions=0,attacks=0; };
    InputMask inputForFrame(const std::uint8_t*ram,unsigned now)const{
        InputMask m;
        if(!enabled || !ram || rb(ram,0x5000u)!=8u ||
           phase[0]!=DELIVER_NATIVE_INPUT ||
           rw(ram,0x4052u)!=0x14u || !inputBegin[0] ||
           now<inputBegin[0])return m;
        const unsigned t=now-inputBegin[0];
        if(t>=17u)return m;
        const bool faceRight=(rb(ram,0x404Bu)&1u)!=0u;
        const unsigned forward=faceRight?1u:2u;
        // CPS-2 native port bits: 0=RIGHT 1=LEFT 2=DOWN 3=UP.
        // 4f D -> 4f DF -> 3f F -> 3f F+3P/3K -> 3f release.
        // This matches actual 87.151(1) input chords 07/38, rather than
        // calling an isolated 2C animation or forging D6/A0 state.
        m.active=true;
        if(t<4u)m.directions=4u;
        else if(t<8u)m.directions=4u|forward;
        else if(t<14u)m.directions=forward;
        if(t>=11u && t<14u)
            m.attacks=command[0]==XM87143Queue::QCF_KK?0x38u:0x07u;
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
                if(!entry || chainUsed[side] || !entry->known ||
                   now<cooldownUntil[side])continue;
                const unsigned selectedAt=XM87143Queue::state().selectedFrame[side];
                const bool firstCheck=selectedAt!=lastQueuedCheck[side];
                lastQueuedCheck[side]=selectedAt;
                // Explicitly log why a selected command cannot reach the
                // ROM handoff. Stage 87.144 produced a blank DHC log because
                // it silently required +A0=2A02 instead of native 2A28.
                const char* blocked=nullptr;
                if(!(superWindowMask&(1u<<side)))
                    blocked="BLOCK_NO_ACTIVE_SUPER_WINDOW";
                else if(pointId!=0x12u || reserveId!=0x14u)
                    blocked="BLOCK_NOT_RYU_POINT_KEN_RESERVE";
                else if(entry->known->fighter!=reserveId)
                    blocked="BLOCK_SELECTED_SUPER_NOT_FOR_RESERVE";
                else if(rw(ram,p+0x60u)!=0x2E08u)
                    blocked="BLOCK_NOT_RYU_2E08_SUPER_ACTION";
                else if(rw(ram,p+0xA0u)!=0x2A28u &&
                        rw(ram,p+0xA0u)!=0x2A02u)
                    blocked="BLOCK_NOT_NATIVE_2A28_SUPER_MODE";
                bool hasScript=false;
                for(unsigned i=0;i<entry->count;++i)
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
                if(!blocked && entry->known->command==XM87143Queue::QCF_PP &&
                   !shoryureppaReady[side])blocked="BLOCK_REPPA_2E00_ROM_UNRESOLVED";
                if(!blocked && entry->known->command==XM87143Queue::QCF_KK &&
                   !shinryukenReady[side])blocked="BLOCK_SHINRYUKEN_2E02_ROM_UNRESOLVED";
                if(blocked){
                    if(firstCheck)event(now,side,ram,blocked);
                    // Never replay an old roster slot's super as Ken's
                    // simply because both use QCF+PP.
                    if(entry->known->fighter!=reserveId)
                        XM87143Queue::state().discard(side);
                    continue;
                }
                originalX[side]=rw(ram,p+0x0Cu);
                opponentX[side]=rw(ram,0x4400u+0x0Cu);
                command[side]=entry->known->command;
                outgoing[side]=pointId;incoming[side]=reserveId;
                capturedMode[side]=rw(ram,p+0xA0u);
                capturedAction[side]=rw(ram,p+0x60u);
                captured256[side]=rb(ram,p+0x256u);
                captured259[side]=rb(ram,p+0x259u);
                capturedD6[side]=rb(ram,p+0xD6u);
                captured06[side]=rw(ram,p+0x06u);
                captured08[side]=rw(ram,p+0x08u);
                captured0A[side]=rw(ram,p+0x0Au);
                // Native 68000 013B8E: partner tag command acceptance
                // writes controller +256/+259=8. The PC 00C4E6
                // dispatcher routes controller 8 to 011E50, which resets
                // the state scheduler and selects a fighter-specific tag
                // routine. Action 32/00 is selected downstream in native
                // ROM code PC 011B00/011B16. No host sprite-script write.
                // The 87.145 test wrote script 32/00 but left the actor
                // in super controller +A0=2A28, so the ROM immediately
                // rewrote 2E08 on the NEXT frame.
                //
                // Real native frames 2758->2759:
                //   PRE-TAG Ryu +A0=002A -> TAG Ryu +A0=2A00.
                // At PC 033762 Ryu's controller 8 copies current A0
                // high byte to A1 then sets A0 high=2A. We must enter
                // with high=0, or the native code would produce 2A2A.
                // Do not force the final 2A00 too early. Prepare the
                // observed PRE-TAG state and let the ROM perform 2A00.
                wb(ram,p+0x256u,0x08u);
                wb(ram,p+0x259u,0x08u);
                ww(ram,p+0x06u,0u);
                ww(ram,p+0x08u,0u);
                ww(ram,p+0x0Au,0u);
                ww(ram,p+0xA0u,0x002Au);
                phase[side]=WAIT_TAG_CONTROLLER;
                started[side]=now;
                lastAcceptedAt[side]=selectedAt;
                event(now,side,ram,"DHC_NATIVE_TAG_CONTROLLER_08_ARMED");
                event(now,side,ram,
                      "DHC_DUAL_KEN_TARGETS_REPPA_2E00_D6722_SHINRYUKEN_2E02_D6976");
                XM87143Queue::state().discard(side);
                continue;
            }
            if(phase[side]==WAIT_TAG_CONTROLLER){
                // ROM scheduler must select outgoing action 3200 itself,
                // not merely display Ryu's 3200 sprite for one frame.
                event(now,side,ram,"NATIVE_TAG_CONTROLLER_WAIT_FRAME");
                if(pointId!=outgoing[side] || reserveId!=incoming[side]){
                    abort(now,side,ram,"CONTROLLER_FIGHTER_CHANGED_BEFORE_TAG");
                }else if(rw(ram,p+0x60u)==0x3200u &&
                         rw(ram,p+0xA0u)==0x2A00u){
                    phase[side]=WAIT_NATIVE_SWAP;
                    controllerAcceptedAt[side]=now;
                    event(now,side,ram,"NATIVE_CONTROLLER_ACCEPTED_OUTGOING_3200");
                }else if(rw(ram,p+0xA0u)==0x2A00u &&
                         rb(ram,p+0x256u)==0u &&
                         rb(ram,p+0x259u)==0x08u &&
                         rb(ram,p+0xD6u)==capturedD6[side] &&
                         rw(ram,p+0x60u)==0x2E08u){
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
                    phase[side]=WAIT_NATIVE_SWAP;
                    controllerAcceptedAt[side]=now;
                    event(now,side,ram,"DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT");

                }else if(rb(ram,p+0x256u)==0u &&
                          rw(ram,p+0x60u)!=0x2E08u &&
                          rw(ram,p+0x60u)!=0x3200u){
                    // 87.152: controller consumed but Ryu ran 2C04, not
                    // tag 3200. Never paste stale A0/step values over 2C04.
                    abort(now,side,ram,
                          "DHC_152_ROM_2C04_WRONG_ACTION_ABORT_NO_ROLLBACK");
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
                    phase[side]=WAIT_LANDING;
                    kenExitObserved[side]=false;
                    kenSuperConfirmed[side]=false;
                    event(now,side,ram,"REAL_ROM_POINT_RESERVE_SWAP");
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
                const bool kenEntry=(incomingAction==0x2A0Cu ||
                                     incomingAction==0x2A0Eu) &&
                                     rb(ram,p+0xD6u)==0x10u;
                const unsigned floor=rw(ram,p+0x254u);
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
            if(phase[side]==WAIT_KEN_EXIT){
                if(pointId!=incoming[side] || reserveId!=outgoing[side]){
                    abort(now,side,ram,"KEN_EXIT_LOST_FIGHTER_OWNERSHIP");
                    continue;
                }
                const unsigned kenAction=rw(ram,p+0x60u);
                const bool originalKick=kenAction==0x2A0Cu ||
                                        kenAction==0x2A0Eu;
                if(originalKick){
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
                if(neutral){
                    if(!kenNeutralAt[side])kenNeutralAt[side]=now;
                    if(now>=kenNeutralAt[side]+2u){
                        const unsigned target=
                          command[side]==XM87143Queue::QCF_KK?0x02u:0x00u;
                        const unsigned expected=
                          command[side]==XM87143Queue::QCF_KK?0xD6976u:0xD6722u;
                        const auto sup=XM87142::resolve(rom,size,pointId,0x2Eu,target);
                        if(!sup.valid || sup.script!=expected){
                            abort(now,side,ram,"KEN_NATIVE_INPUT_SUPER_ROM_UNRESOLVED");
                            continue;
                        }
                        kenExpectedAction[side]=0x2E00u|target;
                        kenExpectedScript[side]=expected;
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
                if(now==inputBegin[side])
                    event(now,side,ram,"DHC_KEN_NATIVE_QCF_INPUT_BEGIN");
                if(now==inputBegin[side]+13u)
                    event(now,side,ram,"DHC_KEN_NATIVE_THREE_BUTTON_EDGE");
                if(now==inputBegin[side]+16u)
                    event(now,side,ram,"DHC_KEN_NATIVE_INPUT_RELEASED");
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
