#ifndef XMVSF_STAGE87_110_RESTART_H
#define XMVSF_STAGE87_110_RESTART_H
// Training-only triple Start gesture. All timing is input-edge based; no
// changes to the ROM's own Start, position switch, or fighter rules.
namespace XM87110 {
enum Preset { NONE=0, NORMAL=1, RIGHT=2, LEFT=3 };
static const unsigned RIGHT_P1_X=568u; // session_20261102_175317_190, settled
static const unsigned RIGHT_P2_X=725u;
static const unsigned LEFT_P1_X=217u; // final 2026-11-03 native capture
static const unsigned LEFT_P2_X=46u;
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
  if(t.count==0 || (unsigned)(now-t.lastMs)>450u || (unsigned)(now-t.firstMs)>1000u){
   t.count=1;t.firstMs=now;
  }else ++t.count;
  t.lastMs=now;
  if(t.count<3u)return false;
  pending=(right&&!left)?RIGHT:((left&&!right)?LEFT:NORMAL);
  lastSelection=pending;
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
