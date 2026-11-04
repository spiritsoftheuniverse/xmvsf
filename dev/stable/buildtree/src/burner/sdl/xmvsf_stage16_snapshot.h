// Stage16: explicit full-game-state snapshot. This is a save state, NOT a replay.
// Training and Versus load the established selection-state snapshot; no manual capture shortcut.
#ifndef XMVSF_STAGE16_SNAPSHOT_H
#define XMVSF_STAGE16_SNAPSHOT_H
#include "xmvsf_stage14_mode.h"
#include <stdio.h>
#include <string.h>
static inline const TCHAR* xm16SnapshotPath() {
    return _T("config/games/xmvsfu_training_select.fs");
}
static inline void xm16SnapshotLog(const char* event, int result) {
    FILE* f=fopen("xmvsf_stage16_snapshot.log","a");
    if (f) {
        fprintf(f,"%s result=%d mode=%d file=config/games/xmvsfu_training_select.fs\n",
                event,result,xm14TrainingMode);
        fclose(f);
    }
    fprintf(stderr,"XMVSF Stage16: %s (%d)\n",event,result);
}
static inline bool xm16OnExpectedDriver() {
    const char* name=BurnDrvGetTextA(DRV_NAME);
    return bDrvOkay && name && strcmp(name,"xmvsfu")==0;
}
static inline void xm16LoadTrainingSnapshot() {
    if (!xm14TrainingMode && !xm24VersusMode) return; /* XMVSF_STAGE24_VERSUS */
    if (!xm16OnExpectedDriver()) {
        xm16SnapshotLog("LOAD_SKIPPED_DRIVER_NOT_READY",-1); return;
    }
    FILE* file=fopen("config/games/xmvsfu_training_select.fs","rb");
    if (!file) { xm16SnapshotLog("SNAPSHOT_MISSING",-1); return; }
    char hdr[4]={};
    const bool good=fread(hdr,1,4,file)==4 && memcmp(hdr,"FB1 ",4)==0;
    fclose(file);
    if (!good) { xm16SnapshotLog("INVALID_SNAPSHOT_HEADER",-2); return; }
    const int rc=BurnStateLoad((TCHAR*)xm16SnapshotPath(),1,NULL);
    xm16SnapshotLog(rc ? "LOAD_FAILED" : "LOAD_OK",rc);
    if (!rc) { xm14InputTarget=0; if(xm14TrainingMode)xm17BeginSelect(); } /* XMVSF_STAGE24_VERSUS */ // XMVSF_STAGE17_AUTO
}
#endif
