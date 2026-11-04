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
enum Phase{IDLE=0,TAG_OUT=1,WAIT_NATIVE_SWAP=2,WAIT_LANDING=3,SUPER_STARTED=4,FAILED=5};
static inline const char* phaseName(Phase p){
    switch(p){
      case TAG_OUT:return "ROM_TAG_OUT";
      case WAIT_NATIVE_SWAP:return "WAIT_NATIVE_SWAP";
      case WAIT_LANDING:return "WAIT_NATIVE_LANDING";
      case SUPER_STARTED:return "NATIVE_SUPER_QUEUED";
      case FAILED:return "FAILED";
      default:return "IDLE";
    }
}
struct State {
    bool initialized=false,enabled=false,used[2]={false,false};
    Phase phase[2]={IDLE,IDLE};
    unsigned started[2]={0u,0u},outgoing[2]={0u,0u},incoming[2]={0u,0u};
    unsigned originalX[2]={0u,0u},opponentX[2]={0u,0u};
    XM87143Queue::Command command[2]={XM87143Queue::QCF_PP,XM87143Queue::QCF_PP};
    unsigned lastFrame=~0u;
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
                "\taction\tscript\tx\ty\tdecision\n",log);
        }
        std::fprintf(stderr,"XM87.144 native DHC proof-of-concept enabled=%d\n",enabled?1:0);
    }
    void reset(){
        for(unsigned s=0;s<2u;++s){phase[s]=IDLE;used[s]=false;}
    }
    void event(unsigned now,unsigned side,const std::uint8_t*ram,
               const char*decision){
        if(!log)return;
        const unsigned p=0x4000u+side*0x400u;
        const unsigned reserve=0x4800u+side*0x400u;
        std::fprintf(log,"%u\t%u\t%s\t%04X\t%04X\t%04X\t%08X"
            "\t%u\t%u\t%s\n",now,side+1u,phaseName(phase[side]),
            rw(ram,p+0x52u),rw(ram,reserve+0x52u),
            rw(ram,p+0x60u),rl(ram,p+0x34u),
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
            if(phase[side]==IDLE){
                const auto* entry=XM87143Queue::state().selected[side];
                if(!entry || used[side] || !entry->known ||
                   !(superWindowMask&(1u<<side)) ||
                   pointId!=0x12u || reserveId!=0x14u ||
                   rw(ram,p+0x60u)!=0x2E08u ||
                   rw(ram,p+0xA0u)!=0x2A28u)
                    continue;
                bool hasScript=false;
                for(unsigned i=0;i<entry->count;++i)
                    hasScript |= entry->scripts[i].resolved;
                if(!hasScript)continue;
                // Require a real ROM-verified outgoing Ryu tag script.
                const auto out=XM87142::resolve(rom,size,pointId,0x32u,0u);
                if(!out.valid || out.script!=0xD1DACu)continue;
                originalX[side]=rw(ram,p+0x0Cu);
                opponentX[side]=rw(ram,0x4400u+0x0Cu);
                command[side]=entry->known->command;
                outgoing[side]=pointId;incoming[side]=reserveId;
                if(!setNativeScript(ram,rom,size,p,pointId,0x32u,0u))continue;
                // 87.144 capture proves the native OUTGOING Ryu tag stays
                // in +A0=2A28. +A0=2A00 appears only AFTER the ROM swaps
                // ownership and starts Ken's incoming tag. Preserve Ryu's
                // native outgoing super-family state here.
                // Do NOT overwrite +A0 on this side of the swap.
                // The action scheduler starts at step zero.
                ww(ram,p+0x06u,0u);ww(ram,p+0x08u,0u);
                phase[side]=WAIT_NATIVE_SWAP;started[side]=now;used[side]=true;
                event(now,side,ram,"DHC_APPROVED_ROM_OUTGOING_ACTION");
                XM87143Queue::state().discard(side);
                continue;
            }
            if(phase[side]==WAIT_NATIVE_SWAP){
                if(pointId==incoming[side] && reserveId==outgoing[side]){
                    phase[side]=WAIT_LANDING;
                    event(now,side,ram,"REAL_ROM_POINT_RESERVE_SWAP");
                }else if(now-started[side]>60u){
                    phase[side]=FAILED;
                    event(now,side,ram,"NO_NATIVE_SWAP_WITHIN_60_FRAMES");
                }
                continue;
            }
            if(phase[side]==WAIT_LANDING){
                if(pointId!=incoming[side]){phase[side]=FAILED;event(now,side,ram,"FIGHTER_CHANGED");continue;}
                // Ken's actual native 2A0C tag jump transitions into 2A0E
                // as he touches ground (captured at frame 3018). Intervene
                // only on THIS DHC-specific landing, never an ordinary tag.
                if(rw(ram,p+0x10u)>=472u &&
                   rw(ram,p+0x60u)==0x2A0Eu){
                    const unsigned chosenAction=command[side]==XM87143Queue::QCF_KK?0x2Eu:0x2Cu;
                    const unsigned chosenSub=command[side]==XM87143Queue::QCF_KK?0u:4u;
                    const auto sup=XM87142::resolve(rom,size,pointId,chosenAction,chosenSub);
                    if(!sup.valid){phase[side]=FAILED;event(now,side,ram,"MISSING_INCOMING_ROM_SCRIPT");continue;}
                    // Fixed location: same side of opponent as outgoing
                    // Ryu at trigger, 48 native units away, in stage bounds.
                    const unsigned foe=rw(ram,0x4400u+0x0Cu);
                    int pos=int(foe)+(originalX[side]<opponentX[side]?-48:48);
                    if(pos<192)pos=192;if(pos>768)pos=768;
                    const unsigned current=rl(ram,p+0x0Cu);
                    wl(ram,p+0x0Cu,(unsigned(pos)<<16u)|(current&65535u));
                    wb(ram,p+0x4Bu,originalX[side]<opponentX[side]?1u:0u);
                    wl(ram,p+0x14u,0u);wl(ram,p+0x18u,0u);
                    wl(ram,p+0x1Cu,0u);wl(ram,p+0x20u,0u);
                    if(setNativeScript(ram,rom,size,p,pointId,chosenAction,chosenSub)){
                        ww(ram,p+0x06u,0u);ww(ram,p+0x08u,0u);
                        ww(ram,p+0xA0u,0x2A00u);
                        phase[side]=SUPER_STARTED;
                        event(now,side,ram,"DHC_NATIVE_SUPER_REPLACES_KICK__UNVERIFIED_FX");
                    }else{
                        phase[side]=FAILED;
                        event(now,side,ram,"NATIVE_SCRIPT_INIT_REJECTED");
                    }
                }else if(now-started[side]>135u){
                    phase[side]=FAILED;event(now,side,ram,"NATIVE_LANDING_TIMEOUT");
                }
                continue;
            }
            if(phase[side]==SUPER_STARTED){
                if(now-started[side]>150u){
                    event(now,side,ram,"DHC_SEQUENCE_OBSERVATION_COMPLETE");
                    phase[side]=IDLE;
                }
            }
        }
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87144
#endif
