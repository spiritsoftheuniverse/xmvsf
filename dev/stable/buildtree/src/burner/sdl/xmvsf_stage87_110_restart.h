#ifndef XMVSF_STAGE87_110_RESTART_H
#define XMVSF_STAGE87_110_RESTART_H
// Training-only triple Start gesture. Observe each physical Start edge once
// per real input frame, independent of FBNeo's game-input query order.
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
namespace XM87110 {
// Event-only diagnostics follow RUN_TEST.bat's current-version log root.
static inline void log(const char* format,...) {
 char line[512];
 va_list args;va_start(args,format);
 std::vsnprintf(line,sizeof(line),format,args);
 va_end(args);
 std::fprintf(stderr,"XM87.274 RELOCATOR %s\n",line);
 const char* configured=std::getenv("XM87110_LOG_FILE");
 const char* path=(configured && *configured)?configured:"xmvsf_training_relocator.log";
 FILE* f=std::fopen(path,"a");
 if(f){std::fprintf(f,"XM87.274 RELOCATOR %s\n",line);std::fclose(f);}
}
enum Preset { NONE=0, NORMAL=1, RIGHT=2, LEFT=3 };
// TEST 87.274: adjust player to 87.272's repeated taunt marker, keeping
// Cyclops on the original outer edge. Keep mirrored at world width 768.
// Source of truth for runtime placement is XM87111::fighterX().
static const unsigned RIGHT_P1_X=650u;
static const unsigned RIGHT_P2_X=747u;
static const unsigned LEFT_P1_X=118u;
static const unsigned LEFT_P2_X=21u;
static inline unsigned targetX(unsigned preset,unsigned fighter){
 return preset==LEFT ? (fighter==0?LEFT_P1_X:LEFT_P2_X)
                     : (fighter==0?RIGHT_P1_X:RIGHT_P2_X);
}
struct Tap {
 bool down=false;
 unsigned count=0,firstMs=0,lastMs=0;
 void clear(){down=false;count=0;firstMs=lastMs=0;}
};
struct State {
 Tap keys[2];
 unsigned pending=NONE;
 bool releaseArmed=false;
 unsigned lastSelection=NONE;
 void reset(){keys[0].clear();keys[1].clear();pending=NONE;releaseArmed=false;lastSelection=NONE;}
 // Called only for the mapped physical controller's START input (0x87).
 // A successful activation takes exclusive ownership of restart, suppressing
 // the third Start press from Stage18's Right+Start side switch.
 bool observe(unsigned player,bool pressed,bool left,bool right,unsigned now,bool enabled){
  if(player>1)return false;
  if(!enabled){keys[player].clear();return pending!=NONE || releaseArmed;}
  if(pending!=NONE||releaseArmed)return true;
  Tap& t=keys[player];
  if(!pressed){t.down=false;return false;}
  if(t.down)return false;
  t.down=true;
  if(t.count==0 || (unsigned)(now-t.lastMs)>750u || (unsigned)(now-t.firstMs)>2200u){
   t.count=1;t.firstMs=now;
  }else ++t.count;
  t.lastMs=now;
  log("START_EDGE player=%u tap=%u left=%u right=%u ms=%u",player+1,t.count,(unsigned)left,(unsigned)right,now);
  if(t.count<3u)return false;
  pending=(right&&!left)?RIGHT:((left&&!right)?LEFT:NORMAL);
  lastSelection=pending;
  log("START_TRIGGER player=%u preset=%s",player+1,pending==LEFT?"LEFT":pending==RIGHT?"RIGHT":"NORMAL");
  keys[0].clear();keys[1].clear();
  return true;
 }
 unsigned take(){const unsigned p=pending;pending=NONE;return p;}
 void didRestore(){pending=NONE;keys[0].clear();keys[1].clear();releaseArmed=true;}
 // The host must keep blocking even after all buttons are released; its
 // existing Stage25 handoff adds a neutral debounce before gameplay resumes.
 bool gate(bool anythingHeld){
  if(pending!=NONE)return true;
  if(!releaseArmed)return false;
  if(!anythingHeld)releaseArmed=false;
  return true;
 }
};
inline State& state(){static State s;return s;}
}
#endif
