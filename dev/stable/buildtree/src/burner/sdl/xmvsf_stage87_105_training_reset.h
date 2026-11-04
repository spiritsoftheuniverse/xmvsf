#ifndef XMVSF_STAGE87_105_TRAINING_RESET_H
#define XMVSF_STAGE87_105_TRAINING_RESET_H
// Training KO Enabled: load the match-start savestate when the defeated
// fighter reaches the observed native ground-entry action transition.
// Do not reload inside a CPU-core open() section or during runahead.
#include "xmvsf_stage87_105_ko_reset.h"
#include "xmvsf_stage87_110_restart.h"
#include "xmvsf_stage87_105_training_hp.h"
#include "xmvsf_stage87_102_training_lab.h"
#include "xmvsf_stage87_100_idle_logic.h"
#include <cstdio>
#include <cstring>
extern int xm87102DamageMode;
static inline void xm87105TrainingResetOnLanding(bool ready,unsigned frame){
    XM87105KO::Detector& det=XM87105KO::detector();
    if(!ready || bBurnRunAheadFrame)return;
    const char* driver=BurnDrvGetTextA(DRV_NAME);
    const bool training=xm14TrainingMode!=0 && driver && std::strcmp(driver,"xmvsfu")==0;
    if(!training || xm87102DamageMode!=1){det.reset();return;}
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if(!cpu || !cpu->open || !cpu->close || !cpu->read)return;
    cpu->open(0);
    const unsigned phase=XM87102Lab::read8(cpu,0xFF5000u);
    const unsigned hp[2]={XM87102Lab::read16(cpu,0xFF4210u),XM87102Lab::read16(cpu,0xFF4610u)};
    const unsigned action[2]={XM87102Lab::read16(cpu,0xFF4060u),XM87102Lab::read16(cpu,0xFF4460u)};
    cpu->close();
    XM87102Lab::Capture& capture=XM87102Lab::capture();
    const int landed=det.observe(training,true,capture.saved,phase,hp,action);
    if(landed<0)return;
    // The first-live-frame save state is explicitly required. Never load the
    // older character-selection state as an unverified fallback.
    FILE* file=std::fopen(XM87102Lab::SNAPSHOT,"rb");
    char hdr[4]={};
    const bool valid=file && std::fread(hdr,1,4,file)==4 && std::memcmp(hdr,"FB1 ",4)==0;
    if(file)std::fclose(file);
    if(!valid){
        std::fprintf(stderr,"XM87.105 TRAINING_KO_LANDING_RESET refused: invalid match snapshot, P%d frame=%u\n",landed+1,frame);
        return;
    }
    const int rc=BurnStateLoad((TCHAR*)XM87102Lab::SNAPSHOT,1,NULL);
    if(rc==0)XM8764::checkpoint("TRAINING_KO_LANDING_RELOAD");
    std::fprintf(stderr,"XM87.105 TRAINING_KO_LANDING_RESET P%d frame=%u action=KO_GROUND_ENTRY load_rc=%d\n",landed+1,frame,rc);
    if(rc!=0)return;
    // Host-side configuration persists across savestate reload. Reapply the
    // chosen HP percentages and reset the idle recovery and input handoff.
    capture.reset();
    XM87105HP::state().queueAll();
    XM87100Idle::state().reset();
    det.reset();
    xm18Position.requested[0]=xm18Position.requested[1]=0;
    xm18Position.held[0]=xm18Position.held[1]=0;
    XM87110::state().didRestore();
    xm25BeginInputHandoff();
}
#endif
