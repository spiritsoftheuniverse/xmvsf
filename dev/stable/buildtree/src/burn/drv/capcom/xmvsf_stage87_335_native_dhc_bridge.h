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
#include "xmvsf_stage87_336_native_d6_dispatch.h"
#include "xmvsf_stage87_340_morphus_native_2v2.h"
#include "../../../cpu/m68k/m68k.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87335Native {
static inline bool enabled(){
    const char* setting=std::getenv("XM87335_NATIVE_DHC");
    const bool original=setting && std::strcmp(setting,"1")==0 &&
           XM87334Native::enabled() && XM87274Unified::state().enabled();
    // Exact Morphus tag hook and native exit are a separate opt-in
    // transaction, not an extension of host controller 8.
    return original || (XM87340Donor::enabled() &&
                        XM87274Unified::state().enabled());
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
    bool donorArmed[2]={false,false};
    unsigned nativeDispatch[2]={0u,0u};
    const std::uint8_t* ram=nullptr;
    int (*prior)(int)=nullptr;
    unsigned installedAt=0,entries=0,refusals=0;

    bool prepare(const std::uint8_t* code,unsigned codeLength,
                 const std::uint8_t* rom,unsigned romLength,
                 const std::uint8_t* currentRam,const char* driver,
                 unsigned frame,bool runahead){
        ready=false;armed[0]=armed[1]=false;
        donorArmed[0]=donorArmed[1]=false;
        nativeDispatch[0]=nativeDispatch[1]=0u;
        if(!enabled() || runahead || !code || !rom || !currentRam ||
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
        if(XM87340Donor::enabled()){
            // The donor's code must execute ORIGINAL M68K tag-out tail
            // $4BF54 (verified original US callsite $4BF66), NOT the
            // legacy forced controller 8 with a synthetic 3200 wait.
            if(!XM87340Donor::validateExit(code,codeLength)){
                ++refusals;
                return false;
            }
            for(unsigned side=0u;side<2u;++side){
                auto& claim=XM87340Donor::state().side[side];
                if(claim.phase!=XM87340Donor::CLAIMED &&
                   claim.phase!=XM87340Donor::CHAIN_ENTERED)continue;
                const auto hyper=XM87336Dispatch::resolve(
                    code,codeLength,rom,romLength,claim.reserve,claim.d6);
                if(!hyper.valid()){
                    // 87.344: $011DBE is the ORIGINAL tag-in selector,
                    // not a fighter-specific D6-to-Hyper dispatcher.
                    // The 87.343 fallback called the tag routine twice,
                    // causing all seven tagged-without-Super outcomes.
                    // Fail closed here: leave the stock original ROM input
                    // and outgoing active Hyper untouched. A true original
                    // D6 dispatcher is mandatory before native swap.
                    if(claim.phase==XM87340Donor::CLAIMED){
                        XM87274Unified::state().event(frame,side,
                            "NATIVE_344_UNVERIFIED_D6_NO_TAG");
                        claim.phase=XM87340Donor::IDLE;
                        ++claim.refusals;
                    }
                    continue;
                }
                const unsigned offset=side?0x100u:0u;
                const unsigned exitOffset=side?
                    XM87340Donor::kExitOffsetP2:
                    XM87340Donor::kExitOffsetP1;
                if(!XM87335Guest::emit(nativeFetch,sizeof(nativeFetch),
                                       offset,claim.d6,claim.x,
                                       hyper.nativeDispatcher) ||
                   !XM87340Donor::emitExit(
                                       nativeFetch,sizeof(nativeFetch),
                                       exitOffset))continue;
                nativeDispatch[side]=hyper.nativeDispatcher;
                donorArmed[side]=true;
            }
        }else for(unsigned side=0u;side<2u;++side){
            const auto& s=XM87274Unified::state().slot[side];
            if(s.phase!=XM87274Unified::AWAIT_SWAP ||
               !s.nativePrecommit.armed || s.nativePrecommit.consumed ||
               s.nativePrecommit.side!=side ||
               !s.nativePrecommit.d6)continue;
            const auto native=XM87336Dispatch::resolve(
                code,codeLength,rom,romLength,
                s.nativePrecommit.reserve,s.nativePrecommit.d6);
            if(!native.valid())continue; // never call an unverified handler
            const unsigned offset=side?0x100u:0u;
            if(!XM87335Guest::emit(nativeFetch,sizeof(nativeFetch),offset,
                                   s.nativePrecommit.d6,s.nativePrecommit.x,
                                   native.nativeDispatcher))
                continue;
            nativeDispatch[side]=native.nativeDispatcher;
            armed[side]=true;
        }
        if(!armed[0] && !armed[1] && !donorArmed[0] &&
           !donorArmed[1])return false;
        // This range is unmapped in cps_mem.cpp for this native driver;
        // map a private M68K fetch/read/write page, not a guessed ROM cave.
        if(SekMapMemory(nativeFetch,XM87335Guest::kGuestBase,
                        XM87335Guest::kGuestBase+XM87335Guest::kPageBytes-1u,
                        MAP_RAM)!=0) {
            armed[0]=armed[1]=false;
            donorArmed[0]=donorArmed[1]=false;
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
        if(XM87340Donor::enabled()){
            const unsigned pc=(unsigned)m68k_get_reg(nullptr,M68K_REG_PC);
            if(pc!=XM87340Donor::kChainPC &&
               pc!=XM87340Donor::kIncomingPC)return;
            const unsigned a6=(unsigned)m68k_get_reg(nullptr,M68K_REG_A6);
            const unsigned side=a6==0xFF4000u?0u:
                                a6==0xFF4400u?1u:2u;
            if(side>=2u || !donorArmed[side])return;
            auto& claim=XM87340Donor::state().side[side];
            const unsigned p=XM87193Exit::actorBase(side);
            const unsigned r=XM87193Exit::reserveBase(side);
            if(XM87142::ramB(ram,0x5000u)!=8u)return;
            const unsigned who=XM87142::ramW(ram,p+0x52u);
            const unsigned bench=XM87142::ramW(ram,r+0x52u);
            if(pc==XM87340Donor::kChainPC){
                // Literal donor xm3_hook_tag / xm3_dhc_chain:
                // consume the incoming fighter's ORIGINAL PP/KK Hyper
                // command (already decoded to one real D6 byte),
                // invoke verified original exit helper, set +256/259=2,
                // return D0=-1. No controller-8 handoff is written.
                if(claim.phase!=XM87340Donor::CLAIMED ||
                   who!=claim.point || bench!=claim.reserve ||
                   XM87142::ramB(ram,p+0x221u) ||
                   XM87142::ramB(ram,p+0x257u) ||
                   XM87142::ramB(ram,p+0x256u) ||
                   XM87142::ramB(ram,p+0x259u) ||
                   !XM87142::ramB(ram,p+0x214u) ||
                   // Compare the EXACT outgoing native Hyper ID captured
                   // when its original-ROM cancel window was eligible.
                   // The donor's character DHC table must not veto an
                   // otherwise genuine original Super at the tag hook.
                   XM87142::ramB(ram,p+0xD6u)!=claim.outgoingD6)
                    return;
                m68k_set_reg(M68K_REG_PC,
                    XM87335Guest::kGuestBase+
                    (side?XM87340Donor::kExitOffsetP2:
                          XM87340Donor::kExitOffsetP1));
                claim.phase=XM87340Donor::CHAIN_ENTERED;
                claim.enteredAt=installedAt;
                XM87274Unified::state().event(installedAt,side,
                    "NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED");
                return;
            }
            // Donor xm3_hook_dhc_commit: the original ROM MUST have
            // swapped fighter identities before the incoming M68K
            // guest can commit its chosen +D6.
            if(pc==XM87340Donor::kIncomingPC &&
               claim.phase==XM87340Donor::CHAIN_ENTERED &&
               who==claim.reserve && bench==claim.point &&
               XM87142::ramW(ram,p+0x08u)==0u){
                // Resolve the entry against the LIVE opponent at the
                // actual original M68K swap/commit, not the earlier
                // outgoing point's position or a camera coordinate.
                const unsigned enemyBase=XM87193Exit::actorBase(side^1u);
                const unsigned enemy=XM87142::ramW(ram,enemyBase+0x52u);
                const unsigned opponentX=XM87142::ramW(
                    ram,enemyBase+0x0Cu);
                const unsigned previousX=claim.x;
                if(enemy<=0x24u && !(enemy&1u))
                    claim.x=XM87340Donor::nearOpponentX(
                        previousX,opponentX);
                const unsigned guestOffset=side?0x100u:0u;
                if(!XM87335Guest::emit(nativeFetch,sizeof(nativeFetch),
                        guestOffset,claim.d6,claim.x,
                        nativeDispatch[side]) ||
                   !XM87335Guest::isImage(nativeFetch,
                        sizeof(nativeFetch),guestOffset,claim.d6,claim.x,
                        nativeDispatch[side])){
                    ++refusals;return;
                }
                char placement[198];
                std::snprintf(placement,sizeof(placement),
                    "NATIVE_343_ENTRY_POSITION side=%u incoming=%02X enemy=%02X oldX=%u enemyX=%u entryX=%u d6=%02X route=%s",
                    side+1u,who,enemy,previousX,opponentX,claim.x,
                    claim.d6,
                    "VERIFIED_D6");
                XM87274Unified::state().event(installedAt,side,placement);
                m68k_set_reg(M68K_REG_PC,side?
                    XM87335Guest::kP2Entry:XM87335Guest::kP1Entry);
                claim.phase=XM87340Donor::COMMITTED;
                claim.committedAt=installedAt;
                claim.confirmed=false;
                ++claim.commits;
                donorArmed[side]=false;
                XM87274Unified::state().event(installedAt,side,
                    "MORPHUS_340_NATIVE_11DBE_AFTER_ORIGINAL_SWAP_COMMIT");
            }
            return;
        }
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
        // The original 68000 D6 state dispatcher must enter phase zero,
        // as shown in the US 87.174 static native-control-flow proof.
        if(XM87142::ramW(ram,p+0x08u)!=0u)return;
        if(!XM87335Guest::isImage(nativeFetch,sizeof(nativeFetch),
                                 side?0x100u:0u,
                                 s.nativePrecommit.d6,s.nativePrecommit.x,
                                 nativeDispatch[side])){
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
        char detail[144];
        std::snprintf(detail,sizeof(detail),
            "NATIVE_336_ORIGINAL_D6_DISPATCH_ENTERED handler=%06X D6=%02X",
            nativeDispatch[side],unsigned(s.nativePrecommit.d6));
        XM87274Unified::state().event(installedAt,side,detail);
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
        donorArmed[0]=donorArmed[1]=false;
        nativeDispatch[0]=nativeDispatch[1]=0u;
    }
    ~State(){end();}
};
inline State& state(){static State s;return s;}
} // namespace XM87335Native
#endif
