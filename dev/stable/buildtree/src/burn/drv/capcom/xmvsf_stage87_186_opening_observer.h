#ifndef XMVSF_STAGE87_186_OPENING_OBSERVER_H
#define XMVSF_STAGE87_186_OPENING_OBSERVER_H
// 87.186 read-only beginning-gate investigation. The 87.185 capture has
// sixteen real Shinkuu roots, a 66/67-frame startup and no controller-8
// attempt before action age 91. No earlier acceptance is implied.
// This captures ALL original native action frames, and requests bounded
// Musashi PPC samples at entry, on pressed-input startup frames, and
// across the first 25 native attack frames. It never injects buttons,
// changes CPU state, arms controller 8, or alters the 87.185 tag logic.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87186Opening {
struct State {
    bool initialized=false,enabled=false;
    std::FILE*log=nullptr;
    XM87184Life::Tracker lifetime;
    unsigned lastFrame=~0u,root=0u,attack=0u,epoch=0u;
    unsigned lastObservedFrame=~0u,preC0=0u,preCA=0u;
    unsigned traceRequestFrame=~0u;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init() {
        if(initialized)return;
        initialized=true;
        const char*enabledText=std::getenv("XM87186_OPENING_OBSERVER");
        const char*path=std::getenv("XM87186_OPENING_OBSERVER_LOG");
        if(!enabledText || std::strcmp(enabledText,"1")!=0 ||
           !path || !*path)return;
        log=std::fopen(path,"ab");
        if(!log) {
            std::fprintf(stderr,"XM87.186 opening observer cannot open %s\n",path);
            return;
        }
        enabled=true;
        std::fputs("# XM87.186 OPENING SESSION read-only native lifecycle\n",log);
        std::fputs("frame\tboundary\tepoch\tage_from_root\tage_from_attack"
                   "\tphase\tevent\tactor_action\tstep08\tscript34\tmodeA0"
                   "\tD6\ttimer4E\tC0\tC2\tC6\tCA\tctrl256\tctrl259"
                   "\tdhc_phase\tp1_dir\tp1_buttons\tPPC_requested\n",log);
        std::fflush(log);
    }
    static unsigned rb(const std::uint8_t*ram,unsigned address) {
        return ram[(address&0xffffu)^1u];
    }
    static unsigned rw(const std::uint8_t*ram,unsigned address) {
        return (rb(ram,address)<<8u)|rb(ram,address+1u);
    }
    static const char*phaseName(XM87184Life::Phase p) {
        switch(p) {
        case XM87184Life::STARTUP:return "STARTUP";
        case XM87184Life::ATTACK:return "ATTACK";
        case XM87184Life::RECOVERY:return "RECOVERY";
        default:return "OUTSIDE";
        }
    }
    static const char*eventName(XM87184Life::Event p) {
        switch(p) {
        case XM87184Life::ENTER:return "ACTUAL_NATIVE_ROOT";
        case XM87184Life::ATTACK_BEGIN:return "ACTUAL_ATTACK_START";
        case XM87184Life::RECOVERY_BEGIN:return "ACTUAL_RECOVERY_START";
        case XM87184Life::ACTION_EXIT:return "ORIGINAL_ACTION_EXIT_OR_HOST_INTERRUPT";
        default:return "SAMPLE";
        }
    }
    // before CPU, after CPU but BEFORE host DHC, after host DHC.
    // This deliberately does not infer command acceptance from a button.
    void sample(const std::uint8_t*ram,unsigned now,const char*boundary,
                unsigned dhcPhase,unsigned direction,unsigned buttons) {
        init();
        if(!enabled || !ram)return;
        if(lastFrame!=~0u && now<lastFrame) {
            lifetime.reset();
            root=attack=epoch=0u;
            std::fputs("# FRAME_RESET new native lifecycle epoch\n",log);
        }
        lastFrame=now;
        if(rb(ram,0x5000u)!=8u || rw(ram,0x4052u)!=0x12u ||
           rw(ram,0x4852u)!=0x14u) {
            lifetime.reset();
            traceRequestFrame=~0u;
            return;
        }
        const XM87184Life::Sample a(0x12u,rw(ram,0x4060u),
            rw(ram,0x40A0u),rw(ram,0x4008u),
            (rw(ram,0x4034u)<<16u)|rw(ram,0x4036u),
            rb(ram,0x40D6u));
        const XM87184Life::Event ev=lifetime.update(now,a);
        if(ev==XM87184Life::ENTER){root=now;attack=0u;++epoch;}
        if(ev==XM87184Life::ATTACK_BEGIN)attack=now;
        if(!lifetime.live && ev!=XM87184Life::ACTION_EXIT)return;
        const XM87184Life::Phase phase=XM87184Life::identify(a);
        const unsigned c0=rw(ram,0x40C0u),ca=rw(ram,0x40CAu);
        if(std::strcmp(boundary,"PRE_CPU")==0) {
            preC0=c0;preCA=ca;traceRequestFrame=~0u;
            // First four full startup frames provide an executed-ROM
            // anchor. Every startup input involving punches also
            // receives a real PPC trace. The earliest 25 ATTACK frames
            // receive PPC traces to test the known 0x40..0x3C gap.
            // These offsets affect OBSERVATION ONLY, never DHC allowance.
            const bool initialRoot=phase==XM87184Life::STARTUP &&
                                   now>=root && now-root<=3u;
            const bool pressedDuringStartup=phase==XM87184Life::STARTUP &&
                (((c0|ca)&0x0070u)!=0u || (buttons&0x07u)!=0u);
            const bool attackOpening=phase==XM87184Life::ATTACK &&
                attack && now>=attack && now-attack<=24u;
            if(initialRoot || pressedDuringStartup || attackOpening)
                traceRequestFrame=now;
        }
        const unsigned age=now>=root?now-root:0u;
        const unsigned attackAge=attack&&now>=attack?now-attack:0u;
        std::fprintf(log,"%u\t%s\t%u\t%u\t%u\t%s\t%s"
            "\t%04X\t%04X\t%08X\t%04X\t%02X\t%04X"
            "\t%04X\t%04X\t%04X\t%04X\t%02X\t%02X"
            "\t%u\t%02X\t%02X\t%u\n",
            now,boundary,epoch,age,attackAge,phaseName(phase),
            eventName(ev),a.action,a.step,a.script,a.mode,a.d6,
            rw(ram,0x404Eu),c0,rw(ram,0x40C2u),rw(ram,0x40C6u),ca,
            rb(ram,0x4256u),rb(ram,0x4259u),
            dhcPhase,direction,buttons,traceRequestFrame==now?1u:0u);
        if(lastObservedFrame!=now) {
            lastObservedFrame=now;
            std::fflush(log); // small, crash-resistant, read-only log
        }
    }
    bool wantsPcTrace(unsigned frame)const {
        return enabled && traceRequestFrame==frame;
    }
};
inline State&state(){static State obj;return obj;}
} // namespace XM87186Opening
#endif
