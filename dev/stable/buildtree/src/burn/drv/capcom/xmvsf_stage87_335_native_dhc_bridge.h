#ifndef XMVSF_STAGE87_335_NATIVE_DHC_BRIDGE_H
#define XMVSF_STAGE87_335_NATIVE_DHC_BRIDGE_H
// XMVSF 87.335: actual M68K guest execution from the original incoming
// $011DBE handoff, not a post-tag neutral/input replay.
// Morphus56k's 68000 actor-state commit adapted to original 2v2 +D6 ownership.
// The stock tag itself is still performed by the original XMVSF 68000.
// A CPU post-instruction callback redirects the NEXT fetch at exactly
// $011DBE to a mapped 68000 guest routine only for the genuine reserve
// identity and an armed Version-4 DHC. Ordinary tags execute the original
// bytes verbatim. No original CpsCode or on-disk ROM patch is made.
// Requires explicitly enabled experimental flags; not a tested universal DHC.
#include "xmvsf_stage87_274_unified_dhc.h"
#include "xmvsf_stage87_333_morphus_native_dhc_abi.h"
#include "xmvsf_stage87_335_native_dhc_guest.h"
#include "../../../cpu/m68k/m68k.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87335Native {
static inline bool enabled(){
    const char* setting=std::getenv("XM87335_NATIVE_DHC");
    return setting && std::strcmp(setting,"1")==0 &&
           XM87334Native::enabled() && XM87274Unified::state().enabled();
}
struct State;
inline State& state();
struct State {
    // Two pages are not necessary: one 4KiB M68K RAM/FETCH page at
    // unmapped $500000, with independent P1/P2 entrypoints.
    // It is never used by any original XMVSF ROM fetch or game controller.
    std::uint8_t nativeFetch[XM87335Guest::kPageBytes]={};
    bool active=false,ready=false,attempted=false;
    bool armed[2]={false,false};
    const std::uint8_t* ram=nullptr;
    int (*prior)(int)=nullptr;
    unsigned installedAt=0,entries=0,refusals=0;

    bool prepare(const std::uint8_t* code,unsigned codeLength,
                 const std::uint8_t* currentRam,const char* driver,
                 unsigned frame,bool runahead){
        ready=false;armed[0]=armed[1]=false;
        if(!enabled() || runahead || !code || !currentRam ||
           !driver || std::strcmp(driver,"xmvsfu")!=0)return false;
        // These are the exact original US instructions, compared with
        // word-swapped CpsCode. No patching on an unknown ROM revision.
        if(!XM87333NativeDHC::verify(code,codeLength).all()){
            if(!attempted){
                std::fputs("XM87.335: native DHC refused; US hook ABI changed\n",stderr);
                attempted=true;
            }
            ++refusals;
            return false;
        }
        for(unsigned side=0u;side<2u;++side){
            const auto& s=XM87274Unified::state().slot[side];
            if(s.phase!=XM87274Unified::AWAIT_SWAP ||
               !s.nativePrecommit.armed || s.nativePrecommit.consumed ||
               s.nativePrecommit.side!=side ||
               !s.nativePrecommit.d6)continue;
            const unsigned offset=side?0x100u:0u;
            if(!XM87335Guest::emit(nativeFetch,sizeof(nativeFetch),offset,
                                   s.nativePrecommit.d6,s.nativePrecommit.x))
                continue;
            armed[side]=true;
        }
        if(!armed[0] && !armed[1])return false;
        // This range is unmapped in cps_mem.cpp for this native driver;
        // map a private M68K fetch/read/write page, not a guessed ROM cave.
        if(SekMapMemory(nativeFetch,XM87335Guest::kGuestBase,
                        XM87335Guest::kGuestBase+XM87335Guest::kPageBytes-1u,
                        MAP_RAM)!=0) {
            armed[0]=armed[1]=false;
            ++refusals;
            return false;
        }
        ram=currentRam;ready=true;installedAt=frame;
        return true;
    }
    void begin(bool runahead){
        if(active||!ready||runahead)return;
        prior=m68k_get_insn_cb();
        m68k_set_insn_cb(&State::bridge);
        active=true;
    }
    void onInstruction(){
        if(!active || !ram || !ready)return;
        // Musashi's callback runs *after* the last instruction; PC is the
        // address of the NEXT opcode to execute. This is not SekGetPPC.
        if((unsigned)m68k_get_reg(nullptr,M68K_REG_PC)!=
           XM87333NativeDHC::kUSIncoming)return;
        const unsigned a6=(unsigned)m68k_get_reg(nullptr,M68K_REG_A6);
        unsigned side=2u;
        if(a6==0xFF4000u)side=0u;
        if(a6==0xFF4400u)side=1u;
        if(side>=2u || !armed[side])return;
        auto& s=XM87274Unified::state().slot[side];
        const unsigned p=XM87193Exit::actorBase(side);
        const unsigned r=XM87193Exit::reserveBase(side);
        // Require genuine original fighter ownership exchange. Host-side
        // inference from a controller-8 request is never enough.
        if(s.phase!=XM87274Unified::AWAIT_SWAP ||
           !s.nativePrecommit.matchesAfterOriginalSwap(
               side,XM87142::ramW(ram,p+0x52u),
                    XM87142::ramW(ram,r+0x52u)))return;
        if(XM87142::ramB(ram,0x5000u)!=8u)return;
        if(!XM87335Guest::isImage(nativeFetch,sizeof(nativeFetch),
                                 side?0x100u:0u,
                                 s.nativePrecommit.d6,s.nativePrecommit.x)){
            ++refusals;return;
        }
        // All native guest instructions run through the original Musashi
        // CPU, starting on the NEXT instruction. Register preservation
        // and RTS are performed by 68000 MOVEM, not by host RAM snapshots.
        m68k_set_reg(M68K_REG_PC,side?
            XM87335Guest::kP2Entry:XM87335Guest::kP1Entry);
        s.phase=XM87274Unified::INCOMING_SUPER;
        s.superAt=installedAt;
        s.neutralSince=0u;
        s.nativePrecommit.markConsumed();
        s.nativePrecommit.directUsed=true;
        armed[side]=false;
        ++entries;
        XM87274Unified::state().event(installedAt,side,
            "NATIVE_335_US_11DBE_DIRECT_M68K_COMMIT_ENTERED");
    }
    static int bridge(int cycles){
        State& s=state();
        if(s.prior && s.prior!=&State::bridge)s.prior(cycles);
        if(s.active)s.onInstruction();
        return 0; // no cycle modification
    }
    void end(){
        if(active){
            active=false;
            m68k_set_insn_cb(prior);
            prior=nullptr;
        }
        ready=false;ram=nullptr;
        armed[0]=armed[1]=false;
    }
    ~State(){end();}
};
inline State& state(){static State s;return s;}
} // namespace XM87335Native
#endif
