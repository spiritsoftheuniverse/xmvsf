#ifndef XMVSF_STAGE87_189_NATIVE_SUPER_FAMILY_H
#define XMVSF_STAGE87_189_NATIVE_SUPER_FAMILY_H
// 87.190: preserve ROM-verified super lifetime across script loops and
// native 2E action sub-stages, not a guessed 'next = recovery' lifetime.
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
enum Phase { OUTSIDE=0, STARTUP=1, ATTACK=2, RECOVERY=3, CONTINUATION=4 };
enum Event { NOTHING=0, ENTER=1, ATTACK_BEGIN=2,
             RECOVERY_BEGIN=3, ACTION_EXIT=4,
             CONTINUATION_BEGIN=5, REPEATED_ROOT_SCRIPT=6,
             CONTINUATION_STAGE_CHANGE=7 };
struct Tracker {
    bool live=false;
    Phase phase=OUTSIDE;
    unsigned lastFrame=~0u;
    unsigned fighter=0u,rootAction=0u,rootScript=0u,nextScript=0u;
    unsigned stageAction=0u,stageScript=0u,stageEnd=0u;
    unsigned lastObservedScript=0u;
    unsigned originalD6=0u;
    unsigned beginFrame=0u,attackFrame=0u,exitFrame=0u;
    void reset(){*this=Tracker();}
    // Native selector order is not always chronological script order.
    // Bound each stage by the NEXT GREATER original-ROM 2E script
    // address, not by arbitrary frames or sprite-script widths.
    static unsigned nextRomScript(const std::uint8_t*rom,unsigned size,
                                  unsigned fighter,unsigned from,
                                  unsigned action=0x2Eu){
        unsigned next=0u;
        for(unsigned selector=0u;selector<=0x4Eu;selector+=2u){
            const auto x=XM87142::resolve(
                rom,size,fighter,action,selector);
            if(x.valid && x.script>from &&
               (!next || x.script<next))next=x.script;
        }
        return next;
    }
    // The original 68000 has genuine 2C-family Hypers as well as 2E.
    // Observed: Storm, Sabretooth, M. Bison and Charlie, with mode 2A
    // and their actual D6. Do not mistake these for failed native tags.
    static bool native2C(const XM87184Life::Sample& s){
        return (s.action&0xFF00u)==0x2C00u &&
               (s.mode&0xFF00u)==0x2A00u && s.d6!=0u;
    }
    static bool originalSuperController(const XM87184Life::Sample& s){
        return ((s.action&0xFF00u)==0x2E00u || native2C(s)) &&
               (s.mode&0xFF00u)==0x2A00u && s.d6!=0u;
    }
    bool native2CWindowAt(unsigned now,const XM87184Life::Sample& s)const{
        // 2C retains original phase 0 or 2 during its attack: 2E's
        // step >= 4 requirement cannot be generalized. Only accept a
        // stage already identified by the original ROM action/script,
        // actual D6, exact actor, and previous-CPU-frame continuity.
        return live && lastFrame==now && native2C(s) &&
               s.fighter==fighter && s.d6==originalD6 &&
               s.action==stageAction && s.script==lastObservedScript &&
               stageScript!=0u && s.script>=stageScript &&
               (stageEnd<=stageScript || s.script<stageEnd);
    }
    bool activeAt(unsigned now,const XM87184Life::Sample& s)const{
        return live && lastFrame==now && phase==ATTACK &&
            s.fighter==fighter && s.action==rootAction &&
            s.d6==originalD6 && s.step>=4u &&
            (s.step&1u)==0u && s.script>=rootScript &&
            nextScript>rootScript && s.script<nextScript &&
            originalSuperController(s);
    }

    // 87.310 experimental universal DHC: accept only an original-ROM,
    // bounded 2E continuation belonging to this exact actor/controller.
    // Terminal table tails have no successor script and remain observer-only.
    // This is NOT used by Version 3 or the legacy Ryu/Ken DHC executor.
    bool boundedContinuationAt(unsigned now,const XM87184Life::Sample& s)const{
        return live && lastFrame==now && phase==CONTINUATION &&
            s.fighter==fighter && s.action==stageAction &&
            s.d6==originalD6 && s.step>=4u && (s.step&1u)==0u &&
            stageScript!=0u && stageEnd>stageScript &&
            s.script>=stageScript && s.script<stageEnd &&
            s.script==lastObservedScript && originalSuperController(s);
    }
    bool unifiedDhcWindowAt(unsigned now,const XM87184Life::Sample& s)const{
        return activeAt(now,s) || boundedContinuationAt(now,s) ||
               native2CWindowAt(now,s);
    }
    bool recognizes()const {
        return live && (phase==STARTUP || phase==ATTACK ||
                        phase==RECOVERY || phase==CONTINUATION);
    }
    // Pre-CPU CPS input is sampled at the current host frame before this
    // frame's post-CPU observer callback. It must belong to the same
    // original ROM actor/2E root observed on the preceding frame.
    // Prevent the 3P dash from stealing a valid DHC chord, without
    // applying any protection after the native super has ended.
    bool preCpuActive(unsigned now,const XM87184Life::Sample&s)const{
        // The original 68000 input port is read BEFORE this frame's
        // post-CPU native lifecycle sample. Reuse the immediately
        // preceding, exact ROM-owned action/script and controller as
        // evidence. Apply the same 3P->2P protection to startup,
        // attacking root, and every observed 2E continuation stage.
        // No move ID, animation mode, fixed timer, or air/ground test.
        return live && recognizes() && lastFrame!=~0u &&
            now>=lastFrame && now-lastFrame<=1u &&
            s.fighter==fighter && s.action==stageAction &&
            s.d6==originalD6 && originalSuperController(s) &&
            s.script==lastObservedScript;
    }
    Event update(unsigned now,const XM87184Life::Sample& s,
                 const std::uint8_t*rom,unsigned romSize){
        if(lastFrame==now)return NOTHING;
        if(lastFrame!=~0u && now<lastFrame)reset();
        lastFrame=now;
        const bool inFamily=originalSuperController(s);
        const bool is2C=native2C(s);
        const bool rootStep=is2C?(s.step<=2u):s.step==2u;
        XM87142::Lookup root;
        if(inFamily && rootStep)
            root=XM87142::resolve(rom,romSize,s.fighter,
                is2C?0x2Cu:0x2Eu,s.action&0xFFu);
        const bool originalRoot=inFamily && rootStep &&
            root.valid && root.script==s.script;
        const bool newNative2CRoot=is2C && (!live ||
            s.fighter!=fighter || s.d6!=originalD6 ||
            (rootAction&0xFF00u)!=0x2C00u);
        if(originalRoot && (is2C?newNative2CRoot:
            (!live || s.fighter!=fighter ||
             s.action!=rootAction || phase!=STARTUP))){
            // Without an ordered, original NEXT script, a root is
            // observable but never executable. No arbitrary +N window.
            live=true;fighter=s.fighter;rootAction=s.action;
            rootScript=s.script;
            nextScript=nextRomScript(rom,romSize,s.fighter,root.script,
                                    is2C?0x2Cu:0x2Eu);
            stageAction=s.action;stageScript=s.script;
            stageEnd=nextScript;lastObservedScript=s.script;
            originalD6=s.d6;phase=STARTUP;beginFrame=now;
            attackFrame=exitFrame=0u;
            return ENTER;
        }
        if(!live)return NOTHING;
        if(!inFamily || s.fighter!=fighter || s.d6!=originalD6){
            live=false;phase=OUTSIDE;exitFrame=now;return ACTION_EXIT;
        }
        // Original 2C Hypers remain in phase 0 or 2 and progress ROM
        // script records. Track the REAL actor-owned action and bounded
        // native stage without demanding a Ryu/Ken 2E phase transition.
        if(is2C){
            if(s.action==rootAction && s.script>=rootScript &&
               (nextScript<=rootScript || s.script<nextScript)){
                const bool begun=phase==STARTUP && now>beginFrame;
                stageAction=rootAction;stageScript=rootScript;
                stageEnd=nextScript;lastObservedScript=s.script;
                if(begun)phase=ATTACK;
                return begun?ATTACK_BEGIN:NOTHING;
            }
            if(s.action!=stageAction){
                const auto stage=XM87142::resolve(
                    rom,romSize,fighter,0x2Cu,s.action&0xFFu);
                if(stage.valid && s.script>=stage.script){
                    stageAction=s.action;stageScript=stage.script;
                    stageEnd=nextRomScript(
                        rom,romSize,fighter,stage.script,0x2Cu);
                }
            }
            if(s.action==stageAction && stageScript!=0u &&
               s.script>=stageScript &&
               (stageEnd<=stageScript || s.script<stageEnd)){
                const bool changed=phase!=CONTINUATION;
                lastObservedScript=s.script;phase=CONTINUATION;
                return changed?CONTINUATION_BEGIN:NOTHING;
            }
            live=false;phase=OUTSIDE;exitFrame=now;return ACTION_EXIT;
        }
        // Step04 may rewind to the native root script while the same
        // hyper is STILL ATTACKING. 87.189 misclassified Ryu's native
        // 2E00 Tatsu at frame 8352 as action EXIT on that rewind.
        const bool ownAction=s.action==rootAction &&
            nextScript>rootScript &&
            s.script>=rootScript && s.script<nextScript;
        if(ownAction && s.step==2u){
            phase=STARTUP;lastObservedScript=s.script;return NOTHING;
        }
        if(ownAction && s.step>=4u && (s.step&1u)==0u){
            // If the ROM loops from a continuation into the root,
            // the pre-CPU shield must follow its actual action again.
            stageAction=rootAction;
            const bool justBegan=phase!=ATTACK;
            const bool rewound=s.script==rootScript && phase==ATTACK &&
                lastObservedScript!=s.script;
            lastObservedScript=s.script;
            phase=ATTACK;
            if(justBegan){attackFrame=now;return ATTACK_BEGIN;}
            return rewound?REPEATED_ROOT_SCRIPT:NOTHING;
        }
        // Another original 2E action with the same ROM controller is
        // continuation of THIS native super, not automatically recovery.
        // Tatsu 2E02/step06 loops as an attacking stage, later running
        // 2E10/step08, 2E04/step0A and 2E06/step0C.
        // Observe it; do not arm controller 8 from an unproven stage.
        if(s.action!=rootAction && s.step>=4u &&
           (s.step&1u)==0u && (s.action&0xFF00u)==0x2E00u){
            const bool changedStage=s.action!=stageAction;
            if(changedStage){
                const auto x=XM87142::resolve(
                    rom,romSize,fighter,0x2Eu,s.action&0xFFu);
                stageAction=s.action;
                stageScript=x.valid?x.script:0u;
                stageEnd=stageScript?
                    nextRomScript(rom,romSize,fighter,stageScript):0u;
            }
            // A terminal 2E selector may have NO higher ROM-script
            // root in this fighter's 2E table (Ryu air 2E0E/D322C).
            // Observe such a terminal stage ONLY as read-only super
            // CONTINUATION while the original action/D6 still owns it.
            // Require valid, 16-byte-aligned original script records;
            // NEVER allow direct native DHC from this tail. No guessed
            // max-frame count or fabricated next-selector sentinel.
            const bool terminalTail=stageScript && !stageEnd &&
               s.script>=stageScript && (s.script-stageScript)%16u==0u &&
               s.script<=romSize && romSize-s.script>=16u;
            const bool boundedStage=stageScript && stageEnd>stageScript &&
               s.script>=stageScript && s.script<stageEnd;
            if(boundedStage || terminalTail){
                const bool first=phase!=CONTINUATION;
                phase=CONTINUATION;lastObservedScript=s.script;
                return first?CONTINUATION_BEGIN:
                    changedStage?CONTINUATION_STAGE_CHANGE:NOTHING;
            }
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
