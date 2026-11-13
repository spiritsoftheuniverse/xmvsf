#ifndef XMVSF_STAGE87_340_MORPHUS_NATIVE_2V2_H
#define XMVSF_STAGE87_340_MORPHUS_NATIVE_2V2_H
// 87.341: Morphus native M68K exit port, controlled by the INCOMING
// fighter's REAL original Super command (motion + PP or KK).
// No donor-assigned MP+MK/MP+HK chords or substitute motion mappings.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include "xmvsf_stage87_334_morphus_2v2_precommit.h"
#include <cstdint>
#include <cstring>
#include <cstdlib>
namespace XM87340Donor {
enum Phase {IDLE, CLAIMED, CHAIN_ENTERED, COMMITTED};
enum : unsigned { kExitHelperUS=0x04BF66u,kExitOffsetP1=0x200u,
   kExitOffsetP2=0x300u,kGuestBytes=32u,kPageBytes=4096u,
   kChainPC=0x013AB6u,kIncomingPC=0x011DBEu};
struct Motion {std::uint8_t len;std::uint8_t seq[5];};
static inline const Motion* motions(){
 static const Motion m[]={
 {3,{4,6,2,0,0}}, {3,{4,5,1,0,0}}, {3,{2,4,6,0,0}},
 {3,{1,4,5,0,0}}, {5,{1,5,4,6,2}}, {5,{2,6,4,5,1}},
 {3,{4,0x80,4,0,0}}};return m;
}
// 87.343: close-range original WORLD X for the incoming native Hyper.
// Both sides read the SAME live opponent actor +$0C from original RAM.
// The outgoing fighter's saved X chooses which side of the opponent the
// incoming fighter occupies. Retain an already-closer placement instead
// of teleporting through the opponent. All coordinates stay in original
// 16-bit CPS2 world units; no screen/camera proxy or hard-coded fighter.
static inline unsigned nearOpponentX(unsigned outgoingX,
                                     unsigned opponentX,
                                     unsigned gap=16u){
 const unsigned old=outgoingX&0xFFFFu;
 const unsigned target=opponentX&0xFFFFu;
 const unsigned separation=gap>0xFFFFu?0xFFFFu:gap;
 if(old<=target){
  const unsigned left=target>separation?target-separation:0u;
  return old>=left?old:left;
 }
 const unsigned right=target>0xFFFFu-separation?
                       0xFFFFu:target+separation;
 return old<=right?old:right;
}
static inline unsigned facingDir(unsigned dirs,bool faceRight){
 unsigned v=(dirs&4u)?4u:0u;
 if(dirs&8u)v|=8u;
 if(dirs&(faceRight?1u:2u))v|=2u;
 if(dirs&(faceRight?2u:1u))v|=1u;
 return v;
}
struct Slot {
 Phase phase=IDLE;
 unsigned owner=~0u,point=0u,reserve=0u,d6=0u,x=0u,outgoingD6=0u;
 unsigned lastFrame=~0u,prev=0u,hidden=0u,selected=7u,stock=0u;
 unsigned claimedAt=0u,enteredAt=0u,committedAt=0u;
 bool confirmed=false;
 std::uint8_t step[7]={},timer[7]={};
 unsigned claims=0u,refusals=0u,commits=0u;
 void reset(){*this=Slot();}
 void clearSteps(){std::memset(step,0,sizeof(step));
                   std::memset(timer,0,sizeof(timer));}
 // Donor's seven trackers, 14 original input samples, and priority:
 // HCF,HCB,DP,RDP,QCF,QCB,DD. Most recent completion timer wins.
 unsigned sample(unsigned frame,unsigned who,unsigned bench,unsigned dir,
    unsigned buttons,bool faceRight,unsigned actorD6,bool inHyper,
    bool eligible,unsigned originalX,unsigned stockBefore){
  if(lastFrame==frame)return hidden;
  if(lastFrame!=~0u && frame<lastFrame)reset();
  lastFrame=frame;
  const unsigned mask=buttons&0x3Fu;
  if(owner!=who){
   owner=who;prev=mask;hidden=0u;clearSteps();
   // A genuine original swap resets motion history, but the incoming
   // Hyper remains one-shot until its ROM-owned neutral return.
   return hidden;
  }
  const unsigned pressed=mask&~prev;
  prev=mask;
  if(hidden && !(mask&hidden))hidden=0u;
  if(phase!=IDLE)return hidden;
  if(!inHyper){clearSteps();return hidden;}
  const unsigned d=facingDir(dir,faceRight);
  const Motion* m=motions();
  for(unsigned i=0u;i<7u;++i){
   unsigned n=step[i];
   if(n && timer[i] && !--timer[i]){step[i]=0u;n=0u;}
   if(n>=m[i].len)continue;
   const unsigned expect=m[i].seq[n];
   if((expect==0x80u)?!(d&4u):(d==expect)){
    step[i]=std::uint8_t(n+1u);
    timer[i]=14u;
   }
  }
  // The same authentic command that ordinarily starts the incoming
  // fighter's Hyper now selects the DHC. PP and KK are NOT interchangeable.
  // The donor MP+MK assist-style chord is not a DHC trigger.
  if(!pressed || !XM87334Native::qualifiedChord(mask) ||
     !eligible || !stockBefore || who==bench ||
     bench>0x24u || (bench&1u))return hidden;
  static const unsigned priority[]={4u,5u,2u,3u,0u,1u,6u};
  unsigned best=0u,col=7u;
  XM87334Native::Decision chosen;
  for(unsigned i=0u;i<7u;++i){
   const unsigned j=priority[i];
   if(step[j]<m[j].len || timer[j]<=best)continue;
   const auto candidate=XM87334Native::resolveNativeOnly(
       bench,j+1u,mask);
   if(!candidate.ok())continue; // no alternate donor input substitution
   best=timer[j];col=j;chosen=candidate;
  }
  if(col==7u)return hidden;
  selected=col;
  d6=chosen.hyperD6; // the ONLY Hyper selection consumed by the M68K guest
  outgoingD6=actorD6;
  point=who;reserve=bench;
  x=originalX;stock=stockBefore;claimedAt=frame;phase=CLAIMED;
  hidden=mask; // mask only the physical PP/KK command being consumed
  ++claims;
  return hidden;
 }

};
struct State {Slot side[2];void reset(){side[0].reset();side[1].reset();}};
static inline State& state(){static State s;return s;}
static inline bool enabled(){
 const char* s=std::getenv("XM87340_MORPHUS_NATIVE");
 return s && std::strcmp(s,"1")==0;
}
// US source opcode evidence at $00D59A, original call to $04BF66.
// Never derive every native function from a global Euro->US offset.
static inline bool validateExit(const std::uint8_t* c,unsigned n){
 if(!c||n<0x4C000u)return false;
 static const std::uint8_t site[]={
   0x30,0x3C,0x00,0x02,0x4E,0xB9,0x00,0x04,0xBF,0x66};
 for(unsigned p=0xD570u;p<=0xD620u;p+=2u){
  bool found=true;
  for(unsigned i=0;i<sizeof(site);++i)
   if(c[p+(i^1u)]!=site[i]){found=false;break;}
  if(found)return true;
 }
 return false;
}
// Donor xm3_dhc_chain: native VC-exit helper, then +256/+259=2.
// Preserve A5 at EVERY M68K instruction; return -1 to original caller.
static inline bool emitExit(std::uint8_t* p,unsigned n,unsigned off){
 static const std::uint8_t be[]={
  0x48,0xE7,0xFF,0xFE,0x70,0x04,
  0x4E,0xB9,0x00,0x04,0xBF,0x66,
  0x4C,0xDF,0x7F,0xFF,
  0x1D,0x7C,0x00,0x02,0x02,0x56,
  0x1D,0x7C,0x00,0x02,0x02,0x59,
  0x70,0xFF,0x4E,0x75
 };
 static_assert(sizeof(be)==kGuestBytes,"Native donor exit opcode changed");
 if(!p||n<kPageBytes||off>n||n-off<sizeof(be)||(off&1u))return false;
 for(unsigned i=0;i<sizeof(be);i+=2u){p[off+i]=be[i+1u];p[off+i+1u]=be[i];}
 return true;
}
} // namespace XM87340Donor
#endif
