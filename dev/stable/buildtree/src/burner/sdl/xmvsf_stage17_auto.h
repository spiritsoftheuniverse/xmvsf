// Stage17: input-only automatic team selection after the confirmed Stage16 save-state load.
// No ROM edits, no synthetic Start/Coin, no input buffering, no periodic state reload.
#ifndef XMVSF_STAGE17_AUTO_H
#define XMVSF_STAGE17_AUTO_H
#include "xmvsf_stage14_mode.h"
#include "burnint.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xmvsf_stage87_66_select_probe.h" // XM87.66 SELECT_TRACE_TIMER

enum { XM17_OFF=0, XM17_P1=1, XM17_P1_SETTLE=2, XM17_P2=3, XM17_DONE=4 };
struct XM17Auto {
    unsigned int frames;
    unsigned int picksP1;
    unsigned int combatCandidateFrames;
    unsigned char phase;
    unsigned char p2CarryMask;
    FILE* log;
};
extern XM17Auto xm17;
static inline void xm17Log(const char* event) {
    if(xm17.log) {
        fprintf(xm17.log,"f=%u phase=%u p1=%u route=%d %s\n",
                xm17.frames,xm17.phase,xm17.picksP1,xm14InputTarget,event);
        fflush(xm17.log);
    }
}
static inline void xm17BeginSelect() {
    if(xm17.log) { fclose(xm17.log); xm17.log=NULL; }
    memset(&xm17,0,sizeof(xm17));
    xm14InputTarget=0;
    if(!xm14TrainingMode) return;
    xm17.phase=XM17_P1;
    // Keep the Training handoff trace with the current versioned test session.
    char tracePath[1024];
    const char* outputDir=getenv("XMVSF_TEST_OUTPUT_DIR");
    if(outputDir && *outputDir) {
        snprintf(tracePath,sizeof(tracePath),"%s\\xmvsf_stage17_select.log",outputDir);
        xm17.log=fopen(tracePath,"w");
    } else {
        xm17.log=fopen("xmvsf_stage17_select.log","w");
    }
    XM8766::TimerState().Reset();
    xm17Log("SNAPSHOT_RESTORED_P1_SELECT");
}
static inline int xm17BlockSlot(int slot) {
    // Prevent P2's physical controller from stealing character selections from P1.
    // The P1->P2 routing itself remains Stage14's immediate, unbuffered input path.
    return xm14TrainingMode &&
           (xm17.phase==XM17_P1 || xm17.phase==XM17_P1_SETTLE) &&
           xm14InputTarget==0 && slot==1;
}
// XM87.68: FF5183 increments ONLY on ROM-accepted P1 choices in the recorded
// Training selection: 0->1 at native 1086, 1->2 at native 1205. The old
// implementation changed ownership at 1211 after a physical release: six
// unnecessary frames after ROM acceptance. A P2-ready cue must use the ROM
// selection milestone rather than the button edge or an elapsed timer.
// Suppress only a confirmation held across the owner transition. D-pad/stick
// directions have zero handoff lockout, and a newly pressed P2 confirmation
// can pass after its first neutral observation.
static inline int xm17FilterTransferredConfirm(int slot,int sub,int value) {
    if(!xm14TrainingMode || xm17.phase!=XM17_P2 || xm14InputTarget!=1 ||
       slot!=1 || sub<0x80 || sub>0x85) return value;
    const unsigned char bit=(unsigned char)(1u<<(sub-0x80));
    if(xm17.p2CarryMask & bit) {
        if(!value) xm17.p2CarryMask=(unsigned char)(xm17.p2CarryMask & ~bit);
        return 0;
    }
    return value;
} /* XM87.68 TRANSFER_CONFIRM_FILTER */
// XM87.134: Restore the verified ROM character-selection timeout hold without
// enabling XM87.66's 4096-byte-per-frame RAM capture. The existing real-frame
// hook calls this for Arcade, Training, and Versus; only ROM phase 00 is changed.
// 87.67 identified FF515E as the 20->0 countdown; FF5008 is the FIGHT timer.
static inline void xm17KeepCharacterSelectOpen(int ready) {
    if (!ready || bBurnRunAheadFrame) return;
    const char* driver = BurnDrvGetTextA(DRV_NAME);
    if (!driver || strncmp(driver, "xmvsf", 5) != 0) return;
    cpu_core_config* cpu = GetCpuCoreConfig(0);
    if (!cpu || !cpu->open || !cpu->close || !cpu->read || !cpu->write) return;
    cpu->open(0);
    if ((cpu->read(0xFF5000u) & 0xffu) == 0x00u) {
        const unsigned timer = cpu->read(0xFF515Eu) & 0xffu;
        // Do not seed zero/uninitialized or unexpected values.
        if (timer >= 1u && timer < 0x14u)
            cpu->write(0xFF515Eu, 0x14u);
    }
    cpu->close();
}

static inline void xm17OnRealFrame(int ready) {
    xm17KeepCharacterSelectOpen(ready); // XM87.134 independent of diagnostic tracing

    if(!xm14TrainingMode || !ready || bBurnRunAheadFrame ||
       xm17.phase==XM17_OFF || xm17.phase==XM17_DONE)return;
    ++xm17.frames;
    // Read the ROM's *accepted* P1 pick count rather than counting button
    // transitions; a physical confirm can be ignored while ROM is busy.
    if(xm17.phase==XM17_P1) {
        cpu_core_config* selcpu=GetCpuCoreConfig(0);
        if(selcpu && selcpu->open && selcpu->read && selcpu->close) {
            selcpu->open(0);
            const unsigned gamePhase=selcpu->read(0xFF5000u)&255u;
            const unsigned accepted=selcpu->read(0xFF5183u)&255u;
            selcpu->close();
            if(gamePhase<8u && accepted<=2u && accepted>xm17.picksP1) {
                xm17.picksP1=accepted;
                xm17Log("XM87.68 ROM_P1_ACCEPTED");
                if(accepted==2u) {
                    xm17.p2CarryMask=0x3fu;
                    xm14InputTarget=1;
                    xm17.phase=XM17_P2;
                    xm17Log("XM87.68 P2_CONTROL_READY_ON_ROM_ACCEPT_NO_DELAY");
                }
            }
        }
    }
    // Stage28: finish ownership only when the running game enters combat.
    // An arbitrary number of P2 button presses is never evidence of a
    // character being selectable, or of a completed team.
    // Primary signal is the same verified match signature used by Stage23/18.
    // Secondary signal uses the *existing* Stage14 match-only health/time
    // predicate plus Stage18's active-fighter checks, held for 3 real frames.
    // No writes, no new ROM offsets, no extra global input-block latch.
    if(xm17.phase!=XM17_OFF){
        bool match=false, secondary=false;
        cpu_core_config* cpu=GetCpuCoreConfig(0);
        if(cpu && cpu->open && cpu->read && cpu->close){
            cpu->open(0);
            const unsigned status=cpu->read(0xFF5000)&255u;
            const unsigned ptr=((cpu->read(0xFFA014)&255u)<<24)|
                 ((cpu->read(0xFFA015)&255u)<<16)|
                 ((cpu->read(0xFFA016)&255u)<<8)|(cpu->read(0xFFA017)&255u);
            match=status==0x08 && ptr==0x00FF4000u;
            const unsigned hp1=cpu->read(0xFF4211)&255u;
            const unsigned hp2=cpu->read(0xFF4611)&255u;
            const unsigned timer=cpu->read(0xFF5008)&255u;
            // Presence of the two real fighter objects is required: the
            // character-select screen must not unlock on health/timer alone.
            const unsigned actor1=cpu->read(0xFF4000)&255u;
            const unsigned actor2=cpu->read(0xFF4400)&255u;
            const unsigned x1=((cpu->read(0xFF400C)&255u)<<8)|(cpu->read(0xFF400D)&255u);
            const unsigned x2=((cpu->read(0xFF440C)&255u)<<8)|(cpu->read(0xFF440D)&255u);
            const int separation=(int)x1-(int)x2;
            secondary=actor1!=0 && actor2!=0 &&
                separation!=0 && separation>=-2048 && separation<=2048 &&
                hp1>0 && hp1<=0x90 && hp2>0 && hp2<=0x90 &&
                (timer>>4)<10 && (timer&15)<10;
            cpu->close();
        }
        // The backup predicate is only relevant while waiting for the dummy's
        // selection. Never promote a stray value during P1 selection.
        const bool candidate=match || (xm17.phase==XM17_P2 && secondary);
        if(candidate){
            if(xm17.combatCandidateFrames<3)++xm17.combatCandidateFrames;
        }else xm17.combatCandidateFrames=0;
        if(xm17.combatCandidateFrames>=3){
            xm14InputTarget=0; // Restore independent physical P1 and P2 devices.
            xm17Log(match?"COMBAT_CONFIRMED_RESTORE_BOTH_CONTROLLERS":
                         "FIGHTER_STATE_CONFIRMED_RESTORE_BOTH_CONTROLLERS");
            // Terminal state consumed by xm20_vs_transition.h. Unlike old
            // versions, P2 button counts can NEVER reach XM17_DONE.
            xm17.phase=XM17_DONE;
            xm17.p2CarryMask=0;
            return;
        }
    }
    XM8766::SampleTimer(xm17.frames,xm17.phase); // Training-only guarded ROM countdown observation
}

#endif
