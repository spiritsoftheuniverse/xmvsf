#ifndef XMVSF_STAGE77_NATIVE_STAGE5_H
#define XMVSF_STAGE77_NATIVE_STAGE5_H
// XM86.48: ONE guarded EARLY selector write to FF5082 only (not FF5113).
// Stage86.46/47 established ROM stage-lookup table slot04 -> native stage05 (forest).
// This selects the game's own native stage and does NOT render Wily art.
// Stage86.42 transition observer remains. Guarded by chosen Wily host menu only.
// The stage-byte FF5113 is NEVER written by this header.
#include "xmvsf_stage77_selection.h"
#include "burnint.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern int xm14TrainingMode;
extern int xm24VersusMode;
extern bool bRunPause;

struct XM8642StageSample {
    unsigned long long frame=0;
    unsigned char status=0, stageByte=0;
    unsigned char nearStage[24]{};  // $FF5108 through $FF511F
};
struct XM8642StageAudit {
    unsigned long long frames=0;
    unsigned totalEvents=0;
    unsigned stageEvents=0;
    unsigned generation=0;
    int previousStatus=-1;
    int previousStage=-1;
    int previousChoice=-1;
    int previousResolved=-1;
    bool initialized=false;
    XM8642StageSample ring[192]{};
};
static XM8642StageAudit xm8642Audit;
struct XM8648EarlyStageLock {
    bool initialized=false;
    unsigned lastGeneration=0;
    bool attempted=false;
    bool selectorWritten=false;
    bool assignmentChecked=false;
    bool combatChecked=false;
    unsigned selectorWrites=0;
};
static XM8648EarlyStageLock xm8648Lock;


static inline void xm8642StageDump(cpu_core_config* c,unsigned base,unsigned length) {
    std::fprintf(stderr,"XM86.42 RAM %06X:",base);
    for(unsigned i=0;i<length;i++) std::fprintf(stderr," %02X",c->read(base+i)&255u);
    std::fprintf(stderr,"\n");
}
static inline void xm8642StageHistory(unsigned long long frame) {
    // Rolling pre-transition context; no speculation about camera offsets.
    const unsigned ageList[]={180,150,120,90,60,45,30,20,10,5,4,3,2,1,0};
    for(unsigned age:ageList){
        if(frame<=age)continue;
        const XM8642StageSample& s=xm8642Audit.ring[(frame-1u-age)%192u];
        if(!s.frame || s.frame!=frame-age)continue;
        std::fprintf(stderr,"XM86.42 HISTORY f=%llu age=%u status=%02X stage=%02X near5113=",
            s.frame,age,(unsigned)s.status,(unsigned)s.stageByte);
        for(unsigned i=0;i<24;i++)std::fprintf(stderr,"%02X",(unsigned)s.nearStage[i]);
        std::fprintf(stderr,"\n");
    }
}
static inline void xm77ApplyNativeStage5(bool gameReady,bool beforeFrame) {
    if(beforeFrame || !gameReady || bRunPause || bBurnRunAheadFrame ||
       (!xm14TrainingMode && !xm24VersusMode))return;
    const char* name=BurnDrvGetTextA(DRV_NAME);
    if(!name || std::strncmp(name,"xmvsf",5)!=0)return;
    // New stage/round: the boot submenu rolls the generation once per selection.
    // Reset BOTH the audit and the single-write latch for in-process return-to-menu.
    const unsigned generation=xm77StageRollGeneration();
    if(!xm8648Lock.initialized || xm8648Lock.lastGeneration!=generation) {
        xm8642Audit=XM8642StageAudit{};
        xm8648Lock=XM8648EarlyStageLock{};
        xm8648Lock.initialized=true;
        xm8648Lock.lastGeneration=generation;
        std::fprintf(stderr,"XM86.48 SESSION generation=%u mode=%d WilyChoice=%d WilyResolved=%d\n",
            generation,xm14TrainingMode,xm77StageChoice,xm77StageResolved);
    }
    cpu_core_config* c=GetCpuCoreConfig(0);
    // Never modify stage byte FF5113; ONLY FF5082 can be written, once.

    if(!c || !c->open || !c->close || !c->read)return;
    c->open(0);
    const unsigned status=c->read(0xFF5000u)&255u;
    const unsigned stageByte=c->read(0xFF5113u)&255u;
    const unsigned p1=((c->read(0xFFA014u)&255u)<<24) |
                      ((c->read(0xFFA015u)&255u)<<16) |
                      ((c->read(0xFFA016u)&255u)<<8) |
                       (c->read(0xFFA017u)&255u);
    const unsigned long long frame=++xm8642Audit.frames;
    // Window is deliberately restricted to the FIRST real host frame only.
    // Stage86.46 validated selector=FF, status=00, stage=00 before game frame1.
    // Here we do the same at the end of frame1: well ahead of stage assignment
    // (~frame450) and without ever replacing an already-assigned stage value.
    if(frame==1 && !xm8648Lock.attempted) {
        xm8648Lock.attempted=true;
        const unsigned selector=c->read(0xFF5082u)&255u;
        const bool customWily=(xm77StageChoice==1 && xm77StageResolved==1 &&
                               xm14TrainingMode==1 && xm24VersusMode==0);
        const bool customFinal=(xm77StageChoice==2 && xm77StageResolved==2 &&
                                (xm14TrainingMode==1 || xm24VersusMode==1));
        const bool optIn=(customWily||customFinal) &&
                          std::getenv("XM8648_NATIVE05_OFF")==nullptr;
        if(optIn && status==0 && stageByte==0 && selector==0xFF && c->write) {
            c->write(0xFF5082u,4u);  // ONLY mutation: guest FF5082=04.
            ++xm8648Lock.selectorWrites;
            const unsigned readback=c->read(0xFF5082u)&255u;
            xm8648Lock.selectorWritten=(readback==4u);
            std::fprintf(stderr,"XM86.48 EARLY_SELECTOR_REQUEST old=%02X new=%02X readback=%02X status=%02X stage=%02X writes=%u result=%s\n",
                selector,4u,readback,status,stageByte,xm8648Lock.selectorWrites,
                xm8648Lock.selectorWritten?"PASS":"FAIL");
        } else {
            std::fprintf(stderr,"XM86.48 EARLY_SELECTOR_SKIPPED optIn=%d status=%02X stage=%02X selector=%02X hasWrite=%d (no retry)\n",
                (int)optIn,status,stageByte,selector,(int)(c->write!=nullptr));
        }
    }
    XM8642StageSample& sample=xm8642Audit.ring[(frame-1u)%192u];
    sample.frame=frame;
    sample.status=(unsigned char)status;
    sample.stageByte=(unsigned char)stageByte;
    for(unsigned i=0;i<24;i++)sample.nearStage[i]=(unsigned char)(c->read(0xFF5108u+i)&255u);
    const int priorStatus=xm8642Audit.previousStatus;
    const int priorStage=xm8642Audit.previousStage;
    const bool edge=priorStatus!=(int)status || priorStage!=(int)stageByte;
    const bool choiceEdge=xm8642Audit.previousChoice!=xm77StageChoice ||
                       xm8642Audit.previousResolved!=xm77StageResolved;
    if(!xm8642Audit.initialized) {
        xm8642Audit.initialized=true;
        std::fprintf(stderr,"XM86.48 STAGE OBSERVER START: stage-byte FF5113 writes=0; selector FF5082 writes=%u; mode=Training:%d Versus:%d choice=%d resolved=%d\n",
            xm8648Lock.selectorWrites,xm14TrainingMode,xm24VersusMode,xm77StageChoice,xm77StageResolved);
    }
    if(choiceEdge){
        std::fprintf(stderr,"XM86.42 HOST CHOICE f=%llu choice=%d resolved=%d (NO ROM CHANGE)\n",
            frame,xm77StageChoice,xm77StageResolved);
    }
    if(edge){
        ++xm8642Audit.totalEvents;
        if(priorStage!=(int)stageByte)++xm8642Audit.stageEvents;
        std::fprintf(stderr,"XM86.42 TRANSITION f=%llu oldStatus=%02X newStatus=%02X oldStage=%02X newStage=%02X p1ptr=%08X\n",
            frame,priorStatus<0?255u:(unsigned)priorStatus,status,priorStage<0?255u:(unsigned)priorStage,
            stageByte,p1);
        // On the decisive first stage commitment, preserve pre-transition history.
        // On later transitions, dump the current RAM windows for comparison.
        if(status==0x02 && priorStatus!=0x02) xm8642StageHistory(frame);
        if(xm8642Audit.totalEvents<=32) {
            xm8642StageDump(c,0xFF5000u,32);
            xm8642StageDump(c,0xFF5100u,64);
        }
    }
    if(xm8648Lock.selectorWritten && !xm8648Lock.assignmentChecked && stageByte!=0u){
        xm8648Lock.assignmentChecked=true;
        std::fprintf(stderr,"XM86.48 NATIVE_STAGE_ASSIGNMENT f=%llu stage=%02X status=%02X expected05=%s\n",
            frame,stageByte,status,stageByte==5u?"PASS":"FAIL");
    }
    if(xm8648Lock.selectorWritten && !xm8648Lock.combatChecked && status==8u){
        xm8648Lock.combatChecked=true;
        std::fprintf(stderr,"XM86.48 COMBAT_STAGE f=%llu stage=%02X expected05=%s\n",
            frame,stageByte,stageByte==5u?"PASS":"FAIL");
    }
    if(frame==1 || frame%300u==0u){
        std::fprintf(stderr,"XM86.42 HEARTBEAT f=%llu status=%02X stage=%02X actorPtr=%08X events=%u\n",
            frame,status,stageByte,p1,xm8642Audit.totalEvents);
    }
    xm8642Audit.previousStatus=(int)status;
    xm8642Audit.previousStage=(int)stageByte;
    xm8642Audit.previousChoice=xm77StageChoice;
    xm8642Audit.previousResolved=xm77StageResolved;
    std::fflush(stderr);
    c->close();
}
static inline void xm8642AuditFinish() {
    std::fprintf(stderr,"XM86.48 SUMMARY stageByteWrites=0 selectorWrites=%u selectorSuccess=%d assignmentChecked=%d combatChecked=%d frames=%llu transitions=%u stageChanges=%u lastStatus=%02X lastStage=%02X\n",
        xm8648Lock.selectorWrites,(int)xm8648Lock.selectorWritten,
        (int)xm8648Lock.assignmentChecked,(int)xm8648Lock.combatChecked,
        xm8642Audit.frames,xm8642Audit.totalEvents,xm8642Audit.stageEvents,
        xm8642Audit.previousStatus<0?255u:(unsigned)xm8642Audit.previousStatus,
        xm8642Audit.previousStage<0?255u:(unsigned)xm8642Audit.previousStage);
    std::fflush(stderr);
}
#endif
