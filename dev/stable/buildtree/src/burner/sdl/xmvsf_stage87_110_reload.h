#ifndef XMVSF_STAGE87_110_RELOAD_H
#define XMVSF_STAGE87_110_RELOAD_H
// Installed after the KO reset and training capture headers are included.
// Runs post-frame only; no save state loads within open 68000 CPU scope.
#include "xmvsf_stage87_110_restart.h"
#include "xmvsf_stage87_111_corner.h"
#include "xmvsf_stage87_135_roster_snapshot.h"
#include "xmvsf_stage87_105_training_reset.h"
#include <cstdio>
#include <cstring>
static inline void xm87111CheckCameraAfterFrame(bool ready,unsigned frame) {
 XM87111::CameraCheck& ch=XM87111::cameraCheck();
 if(!xm14TrainingMode){ch.clear();return;}
 if(!ready||bBurnRunAheadFrame||ch.remaining==0u)return;
 cpu_core_config* cpu=GetCpuCoreConfig(0);
 if(!cpu||!cpu->open||!cpu->close||!cpu->read)return;
 cpu->open(0);
 const unsigned stage=XM87102Lab::read8(cpu,0xFF5113u)&15u;
 const unsigned cam=XM87111::nativeCameraX(XM87102Lab::read16(cpu,0xFF524Cu));
 cpu->close();
 std::fprintf(stderr,"XM87.111 CAMERA_VERIFY frame=%u stage=%u native_camera=%u expected=%u remaining=%u\n",
    frame,stage,cam,ch.expected,ch.remaining);
 --ch.remaining;
}
static inline void xm87114CenterInitialTrainingView(bool ready,unsigned frame){
 static bool centered=false;
 if(!xm14TrainingMode){centered=false;return;}
 if(!ready || bBurnRunAheadFrame || centered)return;
 const char* driver=BurnDrvGetTextA(DRV_NAME);
 if(!driver || std::strcmp(driver,"xmvsfu")!=0)return;
 XM87102Lab::Capture& c=XM87102Lab::capture();
 if(!c.saved || !c.live){centered=false;return;}
 cpu_core_config* cpu=GetCpuCoreConfig(0);
 if(!cpu||!cpu->open||!cpu->close||!cpu->read||!cpu->write)return;
 cpu->open(0);
 const unsigned phase=XM87102Lab::read8(cpu,0xFF5000u);
 const unsigned stage=XM87102Lab::read8(cpu,0xFF5113u)&15u;
 const unsigned ptr=XM87102Lab::read32(cpu,0xFFA014u);
 const unsigned p1=xm18Read32(cpu,0xFF400Cu)>>16;
 const unsigned p2=xm18Read32(cpu,0xFF440Cu)>>16;
 if(phase==8u && stage==XM87111::STAGE && ptr==0x00FF4000u &&
    XM87102Lab::read8(cpu,0xFF4000u)!=0u && XM87102Lab::read8(cpu,0xFF4400u)!=0u &&
    p1!=p2 && p1>=XM87111::WORLD_LEFT && p1<=XM87111::WORLD_RIGHT &&
    p2>=XM87111::WORLD_LEFT && p2<=XM87111::WORLD_RIGHT){
   const unsigned target=XM87111::centerCameraX(p1,p2);
   xm18Write16(cpu,0xFF524Cu,target-XM87111::CAMERA_BIAS);
   std::fprintf(stderr,"XM87.114 CENTER_CAMERA_SYNC frame=%u initial=1 stage=%u camera=%u p1=%u p2=%u\n",
       frame,stage,target,p1,p2);
   centered=true;
 }
 cpu->close();
}
static inline bool xm87110RestartAfterFrame(bool ready,unsigned frame){
 xm87114CenterInitialTrainingView(ready,frame);
 xm87111CheckCameraAfterFrame(ready,frame);
 XM87110::State& s=XM87110::state();
 if(!xm14TrainingMode){s.reset();return false;}
 if(!ready || bBurnRunAheadFrame || s.pending==XM87110::NONE)return false;
 const char* driver=BurnDrvGetTextA(DRV_NAME);
 if(!driver || std::strcmp(driver,"xmvsfu")!=0){s.take();return false;}
 const unsigned selection=s.take();
 XM87111::cameraCheck().clear(); // normal/unsupported restarts must not inherit a previous camera check
 XM87102Lab::Capture& c=XM87102Lab::capture();
 if(!c.saved){std::fprintf(stderr,"XM87.111 RESTART_REFUSED frame=%u no match-start snapshot\n",frame);return false;}
 // The first-frame snapshot can contain an older character team or a
 // different point fighter after tagging. Never overwrite the live roster.
 XM87135::Team liveTeam;
 if(!XM87135::fromCPU(liveTeam)){
  std::fprintf(stderr,"XM87.135 RESTART_REFUSED frame=%u live roster unavailable\n",frame);
  return false;
 }
 std::string loadPath;
 if(!XM87135::prepare(XM87102Lab::SNAPSHOT,liveTeam,loadPath)){
  std::fprintf(stderr,"XM87.135 RESTART_REFUSED frame=%u no matching first-frame roster\n",frame);
  return false;
 }
 FILE* f=std::fopen(loadPath.c_str(),"rb");
 char sig[4]={};
 const bool valid=f && std::fread(sig,1,4,f)==4 && std::memcmp(sig,"FB1 ",4)==0;
 if(f)std::fclose(f);
 if(!valid){std::fprintf(stderr,"XM87.111 RESTART_REFUSED frame=%u invalid snapshot\n",frame);return false;}
 // Load only at the safe post-CPU real-frame boundary.
 const int rc=BurnStateLoad((TCHAR*)loadPath.c_str(),1,NULL);
 if(rc){std::fprintf(stderr,"XM87.111 RESTART_REFUSED frame=%u load_rc=%d\n",frame,rc);return false;}
 bool positioned=selection==XM87110::NORMAL;
 {
  cpu_core_config* cpu=GetCpuCoreConfig(0);
  if(cpu && cpu->open && cpu->close && cpu->read && cpu->write){
   cpu->open(0);
   const unsigned phase=XM87102Lab::read8(cpu,0xFF5000u);
   const unsigned ptr=XM87102Lab::read32(cpu,0xFFA014u);
   const unsigned stage=XM87102Lab::read8(cpu,0xFF5113u)&15u;
   const unsigned bases[2]={0xFF4000u,0xFF4400u};
   const unsigned x1=xm18Read32(cpu,bases[0]+0x0Cu)>>16;
   const unsigned x2=xm18Read32(cpu,bases[1]+0x0Cu)>>16;
   if(phase==8u && ptr==0x00FF4000u && stage==XM87111::STAGE &&
      XM87102Lab::read8(cpu,bases[0])!=0u && XM87102Lab::read8(cpu,bases[1])!=0u &&
      x1!=x2 && x1>=XM87111::WORLD_LEFT && x1<=XM87111::WORLD_RIGHT &&
      x2>=XM87111::WORLD_LEFT && x2<=XM87111::WORLD_RIGHT){
     if(selection==XM87110::NORMAL){
       // The original snapshot is the authoritative DEFAULT fighter position.
       // Only re-center the camera: never move either fighter for NORMAL.
       const unsigned newCamera=XM87111::centerCameraX(x1,x2);
       const unsigned oldCamera=XM87111::nativeCameraX(XM87102Lab::read16(cpu,0xFF524Cu));
       xm18Write16(cpu,0xFF524Cu,newCamera-XM87111::CAMERA_BIAS);
       XM87111::cameraCheck().arm(newCamera);
       std::fprintf(stderr,
         "XM87.114 CENTER_CAMERA_SYNC frame=%u stage=%u old_camera=%u target_camera=%u p1=%u p2=%u\n",
         frame,stage,oldCamera,newCamera,x1,x2);
       positioned=true;
     }else{
     // Both LEFT and RIGHT use explicit positions. LEFT is the final
     // 217/46, Y=472, camera=0 target, never a snapshot-only fallback.
     // Keep each actor and its fractional X. NEVER swap the fighters' data.
     const bool changeFacing=XM87111::flipFacing(selection,x1<x2);
     const unsigned oldCamera=XM87102Lab::read16(cpu,0xFF524Cu)+XM87111::CAMERA_BIAS;
     const unsigned oldFace1=XM87102Lab::read8(cpu,bases[0]+0x4Bu);
     const unsigned oldFace2=XM87102Lab::read8(cpu,bases[1]+0x4Bu);
     for(unsigned i=0;i<2u;++i){
      const unsigned oldFraction=xm18Read32(cpu,bases[i]+0x0Cu)&65535u;
      xm18Write32(cpu,bases[i]+0x0Cu,(XM87111::fighterX(selection,i)<<16)|oldFraction);
      if(selection==XM87110::LEFT)
       xm18Write16(cpu,bases[i]+0x10u,XM87111::CORNER_Y);
      const unsigned original=XM87102Lab::read8(cpu,bases[i]+0x4Bu);
      cpu->write(bases[i]+0x4Bu,XM87111::changedFacing(original,changeFacing));
     }
     // The native stage-05 scroll object is $FF5240. Its X word lives at
     // +0x0C, and the visible left edge is this word plus 64 (XM86.49).
     // Restoring the snapshot camera unchanged is why preset 110 appeared
     // centered or off-corner even when the fighter coordinates were right.
     xm18Write16(cpu,0xFF524Cu,XM87111::cameraX(selection)-XM87111::CAMERA_BIAS);
     XM87111::cameraCheck().arm(XM87111::cameraX(selection));
     std::fprintf(stderr,
       "XM87.111 CAMERA_CORNER_SYNC frame=%u stage=%u old_camera=%u target_camera=%u "
       "old_x=%u,%u target_x=%u,%u native_face=%02X,%02X face_flipped=%u\n",
       frame,stage,oldCamera,XM87111::cameraX(selection),x1,x2,
       XM87111::fighterX(selection,0),XM87111::fighterX(selection,1),
       oldFace1,oldFace2,(unsigned)changeFacing);
     positioned=true;
     } // corner-only placement
   }else{
    // Do not write stage-05 camera bounds into an unknown native stage.
    std::fprintf(stderr,"XM87.111 CORNER_REFUSED frame=%u stage=%u phase=%u ptr=%08X p1=%u p2=%u\n",
       frame,stage,phase,ptr,x1,x2);
   }
   cpu->close();
  }
 }
 // Verify all four native identities, not just the on-screen pair.
 XM87135::Team restored;
 if(!XM87135::fromCPU(restored) || !XM87135::identical(restored,liveTeam)){
  std::fprintf(stderr,"XM87.135 ROSTER_POSTLOAD_MISMATCH frame=%u path=%s\n",
      frame,loadPath.c_str());
 }else{
  std::fprintf(stderr,"XM87.135 ROSTER_VERIFIED frame=%u p1=%02X/%02X p2=%02X/%02X state=%s\n",
      frame,restored.id[0],restored.id[2],restored.id[1],restored.id[3],loadPath.c_str());
 }
 xm18Position.requested[0]=xm18Position.requested[1]=0;
 xm18Position.held[0]=xm18Position.held[1]=0;
 XM87105KO::detector().reset();
 XM87100Idle::state().reset();
 XM87105HP::state().queueAll();
 c.live=true;c.saved=true;c.tried=true;c.postCombatFrames=0;
 s.didRestore();
 xm25BeginInputHandoff();
 std::fprintf(stderr,"XM87.111 TRAINING_TRIPLE_START frame=%u preset=%s positioned=%u input_release_gate=1\n",
    frame,selection==XM87110::NORMAL?"NORMAL":selection==XM87110::RIGHT?"RIGHT":"LEFT",(unsigned)positioned);
 return true;
}

#endif
