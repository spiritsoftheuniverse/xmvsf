#ifndef XMVSF_STAGE86_49_CAMERA_H
#define XMVSF_STAGE86_49_CAMERA_H
#include "xmvsf_stage86_49_state.h"
#include "xmvsf_stage87_108_lifecycle.h"
#include "xmvsf_stage77_selection.h"
#include "burnint.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
extern int xm14TrainingMode;
extern int xm24VersusMode;
extern bool bRunPause;
static inline int xm8649SignedWord(cpu_core_config* c, unsigned a){
 const unsigned u=((c->read(a)&255u)<<8)|(c->read(a+1)&255u);
 return (u&0x8000u)?int(u)-65536:int(u);
}
static inline void xm8649ObserveCamera(bool gameReady){
 static bool previouslyActive=false;
 if(bRunPause)return; // pausing must never reveal the original stage
 // Share the verified stage-05 camera with both custom renderers.
 // Wily remains Training-only; Final Destination works in Training/Versus.
 const bool wily=((xm77StageChoice==0||xm77StageChoice==1) && xm77StageResolved==1 &&
                  xm14TrainingMode==1 && xm24VersusMode==0);
 const bool finalDestination=((xm77StageChoice==0||xm77StageChoice==2) && xm77StageResolved==2 &&
                             (xm14TrainingMode==1 || xm24VersusMode==1));
 const bool guileStage=((xm77StageChoice==0||xm77StageChoice==3) && xm77StageResolved==3 &&
                        (xm14TrainingMode==1 || xm24VersusMode==1));
 if(!gameReady || bBurnRunAheadFrame || (!wily&&!finalDestination&&!guileStage)){
  xm8649Visual.sceneActive=false;return;
 }
 const char* name=BurnDrvGetTextA(DRV_NAME);
 if(!name || std::strncmp(name,"xmvsf",5)!=0){xm8649Visual.sceneActive=false;return;}
 cpu_core_config* c=GetCpuCoreConfig(0);
 if(!c || !c->open || !c->close || !c->read)return;
 c->open(0);
 const unsigned status=c->read(0xFF5000u)&255u;
 const unsigned stage=c->read(0xFF5113u)&15u;
 const unsigned actor=((c->read(0xFFA014u)&255u)<<24)|
   ((c->read(0xFFA015u)&255u)<<16)|((c->read(0xFFA016u)&255u)<<8)|
    (c->read(0xFFA017u)&255u);
 // Stage86.58: the early selector is committed well before battle status 0x07.
 // Begin compositing from the FIRST frame the guest has assigned native stage 05,
 // beginning with the observed pre-fight stage-load phase 0x02.  Do not require
 // the actor pointer (which is unavailable until later).  Statuses below 0x02
 // remain excluded to avoid exposing Wily in character select and the host menu.
 const bool active=(stage==5u && XM87108::StagePhase(status));
 // XMVSF USA stage 05: $FF8000-0x2DC0 = $FF5240; native camera
 // is s16(+0C)+64 horizontally, s16(+10) vertically.
 // This scroll object remains the native camera even when the point
 // pointer changes during startup, tag or entry. The previous code
 // HELD the last-frame camera whenever actor != FF4000, visibly making
 // custom scenery lag behind fighters and then snap back on reacquisition.
 // Read the actual scroll coordinates every frame instead; no virtual
 // camera, interpolation, smoothing or extra frame of delay.
 const int x=active?xm8649SignedWord(c,0xFF524Cu)+64:0;
 const int y=active?xm8649SignedWord(c,0xFF5250u):256;
 c->close();
 xm8649Visual.stage=stage; xm8649Visual.status=status;
 xm8649Visual.pointer=actor; xm8649Visual.sceneActive=active;
 if(active && status>=0x0au){
  static unsigned lastPhase=0xffu;
  if(lastPhase!=status)std::fprintf(stderr,
     "XM87.108 KO_STAGE_LIFETIME status=%02X stage=%02X active=1\n",status,stage);
  lastPhase=status;
 }
 if(active){xm8649Visual.nativeCameraX=x;xm8649Visual.nativeCameraY=y;
            ++xm8649Visual.frames;}
 else if(previouslyActive){xm8649Visual.nativeCameraX=0;xm8649Visual.nativeCameraY=256;}
 if(active!=previouslyActive){
  std::fprintf(stderr,"XM86.49 VISUAL_GATE active=%d status=%02X stage=%02X ptr=%08X camera=%d,%d\n",
      (int)active,status,stage,actor,x,y);
  std::fflush(stderr);previouslyActive=active;
 }
}
#endif
