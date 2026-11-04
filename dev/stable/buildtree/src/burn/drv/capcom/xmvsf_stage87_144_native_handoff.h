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
    WAIT_LANDING=4,SUPER_STARTED=5,FAILED=6
};
static inline const char* phaseName(Phase p){
    switch(p){
      case TAG_OUT:return "ROM_TAG_OUT";
      case WAIT_TAG_CONTROLLER:return "WAIT_NATIVE_TAG_CONTROLLER";
      case WAIT_NATIVE_SWAP:return "WAIT_NATIVE_SWAP";
      case WAIT_LANDING:return "WAIT_NATIVE_LANDING";
      case SUPER_STARTED:return "NATIVE_SUPER_QUEUED";
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
    unsigned captured06[2]={0u,0u},captured08[2]={0u,0u},
             captured0A[2]={0u,0u};
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
                "\tcontroller256\tcontroller259\tstep4E\ta4\tpartner214"
                "\tscript34\tscript38\tpartnerPtr\tpoint210\treserve210"
                "\tx\ty\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.144 native DHC proof-of-concept enabled=%d\n",enabled?1:0);
    }
    void reset(){
        for(unsigned s=0;s<2u;++s){
            phase[s]=IDLE;chainUsed[s]=false;lastQueuedCheck[s]=~0u;
            lastAcceptedAt[s]=~0u;cooldownUntil[s]=0u;
            lastNormalRef[s]=~0u;
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
        cooldownUntil[side]=now+6u;
        XM87143Queue::state().discard(side);
    }
    void event(unsigned now,unsigned side,const std::uint8_t*ram,
               const char*decision){
        if(!log)return;
        const unsigned p=0x4000u+side*0x400u;
        const unsigned reserve=0x4800u+side*0x400u;
        std::fprintf(log,"%u\t%u\t%s\t%04X\t%04X"
            "\t%04X\t%04X\t%04X\t%04X\t%04X"
            "\t%02X\t%02X\t%04X\t%02X\t%04X"
            "\t%08X\t%08X\t%08X\t%04X\t%04X"
            "\t%u\t%u\t%s\n",
            now,side+1u,phaseName(phase[side]),
            rw(ram,p+0x52u),rw(ram,reserve+0x52u),
            rw(ram,p+0x60u),rw(ram,p+0xA0u),
            rw(ram,p+0x06u),rw(ram,p+0x08u),rw(ram,p+0x0Au),
            rb(ram,p+0x256u),rb(ram,p+0x259u),
            rw(ram,p+0x4Eu),rb(ram,p+0xA4u),rw(ram,p+0x214u),
            rl(ram,p+0x34u),rl(ram,p+0x38u),
            rl(ram,p+0x224u),rw(ram,p+0x210u),rw(ram,reserve+0x210u),
            rw(ram,p+0x0Cu),rw(ram,p+0x10u),decision);
        std::fflush(log);
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
                else if(rw(ram,p+0x60u)!=0x2E08u)
                    blocked="BLOCK_NOT_RYU_2E08_SUPER_ACTION";
                else if(rw(ram,p+0xA0u)!=0x2A28u)
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
                if(blocked){
                    if(firstCheck)event(now,side,ram,blocked);
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
                // Real Ryu normal tag capture at 2759: action 3200,
                // mode +A0=2A00. Preserve the native controller path;
                // DO NOT force an outgoing sprite script from the host.
                wb(ram,p+0x256u,0x08u);
                wb(ram,p+0x259u,0x08u);
                ww(ram,p+0x06u,0u);
                ww(ram,p+0x08u,0u);
                ww(ram,p+0x0Au,0u);
                ww(ram,p+0xA0u,0x2A00u);
                phase[side]=WAIT_TAG_CONTROLLER;
                started[side]=now;
                lastAcceptedAt[side]=selectedAt;
                event(now,side,ram,"DHC_NATIVE_TAG_CONTROLLER_08_ARMED");
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
                    event(now,side,ram,"NATIVE_CONTROLLER_ACCEPTED_OUTGOING_3200");
                }else if(now-started[side]>12u){
                    // No native tag controller was established. Restore
                    // its command/status bytes only when Ryu is STILL the
                    // same point fighter, and retain all native animation/
                    // script and opponent state as the ROM left them.
                    if(pointId==outgoing[side]&&reserveId==incoming[side]){
                        ww(ram,p+0xA0u,capturedMode[side]);
                        wb(ram,p+0x256u,captured256[side]);
                        wb(ram,p+0x259u,captured259[side]);
                        ww(ram,p+0x06u,captured06[side]);
                        ww(ram,p+0x08u,captured08[side]);
                        ww(ram,p+0x0Au,captured0A[side]);
                    }
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
                    event(now,side,ram,"REAL_ROM_POINT_RESERVE_SWAP");
                }else if(now-started[side]>60u){
                    abort(now,side,ram,"NO_ROM_OWNERSHIP_SWAP_WITHIN_60_FRAMES");
                }
                continue;
            }
            if(phase[side]==WAIT_LANDING){
                event(now,side,ram,"NATIVE_LANDING_WAIT_FRAME");
                if(pointId!=incoming[side]){
                    abort(now,side,ram,"FIGHTER_CHANGED_DURING_DHC_LANDING");
                    continue;
                }
                // Ken's actual native 2A0C tag jump transitions into 2A0E
                // as he touches ground (captured at 87.144 frame 2802). Intervene
                // only on THIS DHC-specific landing, never an ordinary tag.
                if(rw(ram,p+0x10u)>=472u &&
                   rw(ram,p+0x60u)==0x2A0Eu){
                    const unsigned chosenAction=command[side]==XM87143Queue::QCF_KK?0x2Eu:0x2Cu;
                    const unsigned chosenSub=command[side]==XM87143Queue::QCF_KK?0u:4u;
                    const auto sup=XM87142::resolve(rom,size,pointId,chosenAction,chosenSub);
                    if(!sup.valid){
                        abort(now,side,ram,"MISSING_INCOMING_ROM_SCRIPT");continue;
                    }
                    // Establish a fully valid script BEFORE changing incoming
                    // location or velocities. A failed ROM header must leave
                    // Ken's actual native landing untouched.
                    if(!setNativeScript(ram,rom,size,p,pointId,chosenAction,chosenSub)){
                        abort(now,side,ram,"NATIVE_SCRIPT_INIT_REJECTED");
                        continue;
                    }
                    // Fixed location: same side of opponent as outgoing
                    // Ryu at trigger, 48 native units away, in stage bounds.
                    const unsigned foe=rw(ram,0x4400u+0x0Cu);
                    int pos=int(foe)+(originalX[side]<opponentX[side]?-48:48);
                    if(pos<192)pos=192;
                    if(pos>768)pos=768;
                    const unsigned current=rl(ram,p+0x0Cu);
                    wl(ram,p+0x0Cu,(unsigned(pos)<<16u)|(current&65535u));
                    wb(ram,p+0x4Bu,originalX[side]<opponentX[side]?1u:0u);
                    wl(ram,p+0x14u,0u);wl(ram,p+0x18u,0u);
                    wl(ram,p+0x1Cu,0u);wl(ram,p+0x20u,0u);
                    ww(ram,p+0x06u,0u);ww(ram,p+0x08u,0u);
                    // Actual Ken super captures use native +A0=2A28.
                    // Incoming tag's +A0=2A00 must not be retained after
                    // switching from tag kick to the selected super.
                    ww(ram,p+0xA0u,0x2A28u);
                    phase[side]=SUPER_STARTED;
                    chainUsed[side]=true;
                    event(now,side,ram,"DHC_NATIVE_SUPER_REPLACES_KICK__UNVERIFIED_FX");
                }else if(now-started[side]>135u){
                    abort(now,side,ram,"NATIVE_LANDING_TIMEOUT");
                }
                continue;
            }
            if(phase[side]==SUPER_STARTED){
                if(now-started[side]>150u){
                    event(now,side,ram,"DHC_SEQUENCE_OBSERVATION_COMPLETE");
                    phase[side]=IDLE;
                    // One DHC per chain, not one DHC per whole match.
                    // The incoming super owns this chain; rearm only after
                    // the native action returns to non-super neutral.
                }
            }
            if(phase[side]==IDLE && chainUsed[side] &&
               rw(ram,p+0xA0u)!=0x2A28u &&
               rw(ram,p+0x60u)!=0x2E08u &&
               rw(ram,p+0x60u)!=0x2C04u &&
               rw(ram,p+0x60u)!=0x2E00u){
                chainUsed[side]=false;
                event(now,side,ram,"DHC_REARM_NEXT_INDEPENDENT_SUPER");
            }
        }
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87144
#endif
