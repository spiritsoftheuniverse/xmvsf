#ifndef XMVSF_STAGE87_189_NATIVE_SUPER_FAMILY_H
#define XMVSF_STAGE87_189_NATIVE_SUPER_FAMILY_H
// 87.189: discover a native 2E-family action from its ORIGINAL ROM entry,
// not from a Shinkuu name, animation snapshot, airborne flag, or timer.
// Ryu air Shinkuu is 2E0C/2E0E, mode 2A1C or 2A28, in actual 87.188
// F9 evidence. Ground Ryu is 2E08/2E0A. Ken 2E00 has D6=0C, so
// do not impose Ryu D6=0A on all fighters.
//
// This is a ROM-verified 2E-family CANDIDATE, not proof every 2E entry
// is a player-triggered hyper. The executable 87.189 native tag remains
// restricted to the independently verified Ryu->Ken roster and a new
// explicit TEST flag. No speculative recovery bridge for other moves.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87189Family {
enum Phase { OUTSIDE=0, STARTUP=1, ATTACK=2, RECOVERY=3 };
enum Event { NOTHING=0, ENTER=1, ATTACK_BEGIN=2,
             RECOVERY_BEGIN=3, ACTION_EXIT=4 };
struct Tracker {
    bool live=false;
    Phase phase=OUTSIDE;
    unsigned lastFrame=~0u;
    unsigned fighter=0u,rootAction=0u,rootScript=0u,nextScript=0u;
    unsigned recoveryAction=0u,recoveryScript=0u,recoveryEnd=0u;
    unsigned originalD6=0u;
    unsigned beginFrame=0u,attackFrame=0u,exitFrame=0u;
    void reset(){*this=Tracker();}
    static bool originalSuperController(const XM87184Life::Sample& s){
        return (s.action&0xFF00u)==0x2E00u &&
            (s.mode&0xFF00u)==0x2A00u && s.d6!=0u;
    }
    bool activeAt(unsigned now,const XM87184Life::Sample& s)const{
        return live && lastFrame==now && phase==ATTACK &&
            s.fighter==fighter && s.action==rootAction &&
            s.d6==originalD6 && s.step>=4u &&
            (s.step&1u)==0u && s.script>rootScript &&
            nextScript>rootScript && s.script<nextScript &&
            originalSuperController(s);
    }
    bool recognizes()const {
        return live && (phase==STARTUP || phase==ATTACK || phase==RECOVERY);
    }
    // Pre-CPU CPS input is sampled at the current host frame before this
    // frame's post-CPU observer callback. It must belong to the same
    // original ROM actor/2E root observed on the preceding frame.
    // Prevent the 3P dash from stealing a valid DHC chord, without
    // applying any protection after the native super has ended.
    bool preCpuActive(unsigned now,const XM87184Life::Sample&s)const{
        return live && (phase==STARTUP || phase==ATTACK) &&
            lastFrame!=~0u && now>=lastFrame && now-lastFrame<=1u &&
            s.fighter==fighter && s.action==rootAction &&
            s.d6==originalD6 && originalSuperController(s) &&
            s.script>=rootScript &&
            nextScript>rootScript && s.script<nextScript &&
            ((phase==STARTUP && s.step==2u) ||
             (phase==ATTACK && s.step>=4u && (s.step&1u)==0u));
    }
    Event update(unsigned now,const XM87184Life::Sample& s,
                 const std::uint8_t*rom,unsigned romSize){
        if(lastFrame==now)return NOTHING;
        if(lastFrame!=~0u && now<lastFrame)reset();
        lastFrame=now;
        const bool inFamily=originalSuperController(s);
        XM87142::Lookup root;
        if(inFamily && s.step==2u)
            root=XM87142::resolve(rom,romSize,s.fighter,0x2Eu,s.action&0xFFu);
        const bool originalRoot=inFamily && s.step==2u &&
            root.valid && root.script==s.script;
        if(originalRoot && (!live || s.fighter!=fighter ||
            s.action!=rootAction || phase!=STARTUP)){
            const unsigned selector=s.action&0xFFu;
            const auto next=selector<=0x4Cu?
                XM87142::resolve(rom,romSize,s.fighter,0x2Eu,selector+2u):
                XM87142::Lookup();
            const auto after=selector<=0x4Au?
                XM87142::resolve(rom,romSize,s.fighter,0x2Eu,selector+4u):
                XM87142::Lookup();
            // Without an ordered, original NEXT script, a root is
            // observable but never executable. No arbitrary +N window.
            live=true;fighter=s.fighter;rootAction=s.action;
            rootScript=s.script;
            nextScript=next.valid && next.script>root.script?next.script:0u;
            recoveryAction=nextScript?s.action+2u:0u;
            recoveryScript=nextScript;
            recoveryEnd=after.valid && after.script>nextScript?
                after.script:0u;
            originalD6=s.d6;phase=STARTUP;beginFrame=now;
            attackFrame=exitFrame=0u;
            return ENTER;
        }
        if(!live)return NOTHING;
        if(!inFamily || s.fighter!=fighter || s.d6!=originalD6){
            live=false;phase=OUTSIDE;exitFrame=now;return ACTION_EXIT;
        }
        const bool ownAction=s.action==rootAction && s.script>=rootScript &&
            (!nextScript || s.script<nextScript);
        if(ownAction && s.step==2u){
            phase=STARTUP;return NOTHING;
        }
        const bool attack=ownAction && s.step>=4u &&
            (s.step&1u)==0u && s.script>rootScript &&
            nextScript>rootScript;
        if(attack){
            const bool justBegan=phase!=ATTACK;
            phase=ATTACK;
            if(justBegan){attackFrame=now;return ATTACK_BEGIN;}
            return NOTHING;
        }
        const bool recovery=recoveryAction &&
            s.action==recoveryAction && s.step>=6u &&
            (s.step&1u)==0u && s.script>=recoveryScript &&
            (!recoveryEnd || s.script<recoveryEnd);
        if(recovery){
            const bool justBegan=phase!=RECOVERY;
            phase=RECOVERY;
            return justBegan?RECOVERY_BEGIN:NOTHING;
        }
        live=false;phase=OUTSIDE;exitFrame=now;return ACTION_EXIT;
    }
};
struct State {
    Tracker side[2];
    void reset(){side[0].reset();side[1].reset();}
};
inline State&state(){static State s;return s;}
} // namespace XM87189Family
#endif
