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
 {3,{4,0x80,4,0,0}}, {3,{2,6,4,0,0}}};return m; // motion 8: F,DF,D
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
// 87.346: +$210 is the ORIGINAL resident partner's life field.
// There is no "empty fighter ID": 00 means Wolverine. Refuse the DHC
// only when the stock partner really has no life/identity; never
// interfere with 1v1 ordinary Hyper inputs or a dead teammate's slot.
static inline bool genuineReserve(unsigned who,unsigned bench,
                                  unsigned reserveLife){
 return reserveLife!=0u && who<=0x24u && bench<=0x24u &&
        !(who&1u) && !(bench&1u) && who!=bench;
}
static inline unsigned facingDir(unsigned dirs,bool faceRight){
 unsigned v=(dirs&4u)?4u:0u;
 if(dirs&8u)v|=8u;
 if(dirs&(faceRight?1u:2u))v|=2u;
 if(dirs&(faceRight?2u:1u))v|=1u;
 return v;
}
// 87.350: original character-specific Super commands which CANNOT
// be represented as ordinary QCF/DP + PP/KK. Input is sampled from
// actual P1/P2 physical edges, relative to the outgoing facing.
// No artificial DHC shortcut, no rewriting ROM input, no fake attack.
struct SpecialHistory {
 unsigned demon=0u,demonTime=0u,lastDir=0u;
 unsigned spinLast=8u,spinTurn=0u,spinSweep=0u;
 unsigned spinTime=0u,spinReady=0u;
 bool spinUp=false,spinDown=false;
 void reset(){*this=SpecialHistory();}
 static unsigned sector(unsigned dir){
  // Clockwise order around the original 8-way joystick.
  switch(dir){
  case 2u:return 0u;  // F
  case 6u:return 1u;  // DF
  case 4u:return 2u;  // D
  case 5u:return 3u;  // DB
  case 1u:return 4u;  // B
  case 9u:return 5u;  // UB
  case 8u:return 6u;  // U
  case 10u:return 7u; // UF
  default:return 8u;
  }
 }
 void beginSpin(unsigned pos,unsigned dir){
  spinLast=pos;spinTurn=0u;spinSweep=0u;
  spinUp=(dir&8u)!=0u;spinDown=(dir&4u)!=0u;
  spinTime=36u;spinReady=0u;
 }
 unsigned sample(unsigned dir,unsigned pressed,unsigned held){
  // Shun Goku Satsu: LP, release LP, LP, F, LK, HP.
  // Each new button requires a real rising edge. Pressing an LP+MP
  // chord does not create the two distinct LP taps.
  if(demonTime && !--demonTime)demon=0u;
  bool demonDone=false;
  if(demon==0u && (pressed&1u) && held==1u){
   demon=1u;demonTime=20u;
  }else if(demon==1u && (pressed&1u) && held==1u){
   demon=2u;demonTime=20u;
  }else if(demon==2u && dir==2u && lastDir!=2u){
   demon=3u;demonTime=20u;
  }else if(demon==3u && (pressed&8u)){
   demon=4u;demonTime=20u;
  }else if(demon==4u && (pressed&4u)){
   demonDone=true;demon=0u;demonTime=0u;
  }
  lastDir=dir;
  // Final Atomic Buster (XMvsSF donor source: 360+PP, not an
  // arbitrary QCF DHC alias). Accept a continuous 270+ degrees
  // through BOTH the upper and lower hemispheres, either turning
  // direction, with short adjacent-sector skips for fast pads.
  if(spinTime && !--spinTime)beginSpin(8u,0u);
  if(spinReady)--spinReady;
  const unsigned pos=sector(dir);
  if(pos!=8u){
   if(spinLast==8u){
    beginSpin(pos,dir);
   }else if(pos!=spinLast){
    const unsigned cw=(pos+8u-spinLast)&7u;
    const unsigned ccw=(spinLast+8u-pos)&7u;
    unsigned turn=spinTurn,delta=0u;
    if(!turn){
     if(cw<=2u){turn=1u;delta=cw;}
     else if(ccw<=2u){turn=2u;delta=ccw;}
    }else{
     delta=turn==1u?cw:ccw;
     if(delta>2u)delta=0u;
    }
    if(!delta){
     beginSpin(pos,dir);
    }else{
     spinLast=pos;spinTurn=turn;
     spinSweep=(spinSweep+delta>8u)?8u:spinSweep+delta;
     spinUp=spinUp||((dir&8u)!=0u);
     spinDown=spinDown||((dir&4u)!=0u);
     spinTime=36u;
     if(spinSweep>=6u && spinUp && spinDown)
      spinReady=12u;
    }
   }
  }
  if(demonDone)return 10u; // LP LP F LK HP; actual final HP
  if(spinReady && (pressed&7u) &&
     XM87334Native::qualifiedChord(held))return 9u;
  return 0u;
 }
};
struct Slot {
 Phase phase=IDLE;
 unsigned owner=~0u,point=0u,reserve=0u,d6=0u,x=0u,outgoingD6=0u;
 unsigned lastFrame=~0u,prev=0u,hidden=0u,selected=8u,stock=0u;
 unsigned claimedAt=0u,enteredAt=0u,committedAt=0u;
 bool confirmed=false;
 std::uint8_t step[8]={},timer[8]={},missedDiagonal[8]={};
 unsigned firstFrame[8]={};
 // 87.353: remember an actual physical 3K edge while the last
 // direction of a native motion is still crossing a sampled frame.
 unsigned kkkEdgeAt=~0u;
 bool claimedBufferedKKK=false;
 bool windowReady=false; // DHC input recognition is not active in normal play
 // 87.357: exact Charlie QCB+KKK entered during a native Hyper is
 // remembered until the original M68K cancel gate can authorize it.
 bool charlieQueued=false,claimedQueuedCharlie=false;
 unsigned charlieQueuedD6=0u;
 SpecialHistory special;
 unsigned claims=0u,refusals=0u,commits=0u;
 void reset(){*this=Slot();}
 void clearSteps(){std::memset(step,0,sizeof(step));
                   std::memset(timer,0,sizeof(timer));
                   std::memset(missedDiagonal,0,sizeof(missedDiagonal));
                   std::memset(firstFrame,0,sizeof(firstFrame));
                   kkkEdgeAt=~0u;claimedBufferedKKK=false;
                   charlieQueued=false;claimedQueuedCharlie=false;
                   charlieQueuedD6=0u;special.reset();}
 // 87.354: Never sample DHC motion history during normal play.
 void quiet(unsigned frame,unsigned who,unsigned buttons){
  if(phase!=IDLE)return;
  clearSteps();windowReady=false;owner=who;lastFrame=frame;
  prev=buttons&0x3Fu;hidden=0u;
 }
 // First eligible native Hyper frame arms input, but does NOT use any
 // attack held before that moment. A new DHC command must start here.
 void openWindow(unsigned frame,unsigned who,unsigned buttons,
                 unsigned incoming=~0u,unsigned physicalDir=0u,
                 bool faceRight=true){
  if(phase!=IDLE)return;
  quiet(frame,who,buttons);windowReady=true;
  // 87.355: The FIRST verified native Hyper frame is already a
  // legitimate input frame. Retain just its initial ORIGINAL direction
  // for the two requested 3K supers. Never retain normal-play history,
  // a pre-existing KKK press or any other pre-Hyper button.
  const unsigned d=facingDir(physicalDir,faceRight);
  if(incoming==0x20u && d==4u){
   step[1]=1u;timer[1]=18u;firstFrame[1]=frame; // Charlie D,DB,B
  }else if(incoming==0x0Au && d==2u){
   step[5]=1u;timer[5]=24u;firstFrame[5]=frame; // Sabretooth F,DF,D,DB,B
  }
 }
 // Previously claimed DHC continues the authentic M68K handoff.
 // Update button release only; no new directions/gestures are parsed.
 void pendingInput(unsigned frame,unsigned buttons){
  if(phase==IDLE || lastFrame==frame)return;
  lastFrame=frame;const unsigned mask=buttons&0x3Fu;
  prev=mask;if(hidden && !(mask&hidden))hidden=0u;
 }
 // Donor's seven motions plus original Cyclops F,DF,D+PP.
 // Original forward motions keep 14 input samples; QCB/HCB use 18/24.
 // Cyclops' eighth motion still requires terminal down.
 unsigned sample(unsigned frame,unsigned who,unsigned bench,unsigned dir,
    unsigned buttons,bool faceRight,unsigned actorD6,bool inHyper,
    bool eligible,unsigned originalX,unsigned stockBefore){
  if(lastFrame==frame)return hidden;
  if(lastFrame!=~0u && frame<lastFrame)reset();
  lastFrame=frame;
  const unsigned mask=buttons&0x3Fu;
  if(owner!=who){
   owner=who;prev=mask;hidden=0u;clearSteps();windowReady=false;
   // A genuine original swap resets motion history, but the incoming
   // Hyper remains one-shot until its ROM-owned neutral return.
   return hidden;
  }
  const unsigned pressed=mask&~prev;
  prev=mask;
  if(hidden && !(mask&hidden))hidden=0u;
  if(phase!=IDLE)return hidden;
  // No guessed three-kick input: only a physically observed KKK rising
  // edge, and only while all THREE kicks remain continuously held.
  if(mask!=0x38u)kkkEdgeAt=~0u;
  else if(pressed&0x38u)kkkEdgeAt=frame;
  const bool heldFreshKKK=mask==0x38u &&
      kkkEdgeAt!=~0u && frame>=kkkEdgeAt &&
      frame-kkkEdgeAt<=10u;
  // Called exclusively from an eligible original-ROM Hyper DHC window.
  // Normal play never invokes this directional/KKK/spin parser.
  const unsigned d=facingDir(dir,faceRight);
  const unsigned specialMotion=special.sample(d,pressed,mask);
  const Motion* m=motions();
  for(unsigned i=0u;i<8u;++i){
   unsigned n=step[i];
   if(n && timer[i] && !--timer[i]){
    step[i]=0u;missedDiagonal[i]=0u;firstFrame[i]=0u;n=0u;
   }
   if(n>=m[i].len)continue;
   const unsigned expect=m[i].seq[n];
   // 87.352: consecutive real cardinal samples can straddle joystick
   // diagonals. QCB may miss DB; Cyclops F,DF,D may miss DF.
   // An HCB may miss BOTH diagonals only for a fast F,D,B sweep
   // completed within 12 frames of its first native cardinal sample.
   const unsigned window=i==5u?24u:(i==1u?18u:14u);
   if((expect==0x80u)?!(d&4u):(d==expect)){
    if(!n)firstFrame[i]=frame;
    step[i]=std::uint8_t(n+1u);
    timer[i]=std::uint8_t(window);
   }else if((i==1u||i==5u||i==7u) && n>0u &&
            n+1u<m[i].len && (expect==5u||expect==6u) &&
            d==m[i].seq[n+1u] &&
            missedDiagonal[i]<(i==5u?2u:1u) &&
            (i!=5u || missedDiagonal[i]!=1u ||
             frame-firstFrame[i]<=12u)){
    step[i]=std::uint8_t(n+2u);
    timer[i]=std::uint8_t(window);
    ++missedDiagonal[i];
   }
  }
  // The same authentic command that ordinarily starts the incoming
  // fighter's Hyper now selects the DHC. PP and KK are NOT interchangeable.
  // Cyclops native Super Optic Blast is F,DF,D + PP or PPP;
  // Mega Optic Blast remains QCF+PP. Do not substitute QCB.
  // The donor MP+MK assist-style chord is not a DHC trigger.
  // Input recognition and native DHC authorization are separate.
  // Only a real outgoing 2A/2C-or-2E Hyper can reach this parser.
  // A completed Charlie original QCB + actual KKK before native
  // cancel eligibility is queued for this SAME outgoing Hyper.
  // No synthetic input or arbitrary time-based deadline is used.
  if(charlieQueued && charlieQueuedD6!=actorD6)charlieQueued=false;
  if(bench==0x20u && inHyper && !eligible &&
     step[1]>=m[1].len && timer[1] && mask==0x38u &&
     ((pressed&0x38u)!=0u || heldFreshKKK)){
    charlieQueued=true;charlieQueuedD6=actorD6;
  }
  const bool queuedCharlie=charlieQueued && bench==0x20u &&
      inHyper && eligible && charlieQueuedD6==actorD6;
  if((!pressed && !heldFreshKKK && !queuedCharlie) ||
     !inHyper || !eligible || !stockBefore || who==bench ||
     bench>0x24u || (bench&1u))return hidden;
  static const unsigned priority[]={7u,4u,5u,2u,3u,0u,1u,6u};
  unsigned best=0u,col=8u;
  XM87334Native::Decision chosen;
  if(queuedCharlie){
    chosen=XM87334Native::resolveNativeOnly(bench,2u,0x38u);
    if(chosen.ok())col=1u;
  }
  // Special native Hyper sequences have distinct final buttons.
  // Akuma SGS ends on a SINGLE HP; a generic PP filter must never
  // discard this real command. Zangief FAB ends on real PP/PPP.
  if(bench==0x1Eu && specialMotion==10u){
   chosen=XM87334Native::resolveNativeOnly(bench,10u,0x04u);
   if(chosen.ok())col=9u;
  }else if(bench==0x1Au && specialMotion==9u){
   chosen=XM87334Native::resolveNativeOnly(bench,9u,mask);
   if(chosen.ok())col=8u;
  }
  if(!chosen.ok()){
   if(!XM87334Native::qualifiedChord(mask))return hidden;
   for(unsigned i=0u;i<8u;++i){
    const unsigned j=priority[i];
    if(step[j]<m[j].len || timer[j]<=best)continue;
    // A two-diagonal HCB shortcut expires sooner than an explicitly
    // sampled five-sector rotation. No stale F,D,B can fake a Hyper.
    if(j==5u && missedDiagonal[j]==2u &&
       frame-firstFrame[j]>12u)continue;
    const auto candidate=XM87334Native::resolveNativeOnly(
        bench,j+1u,mask);
    if(!candidate.ok())continue; // real native motion only
    best=timer[j];col=j;chosen=candidate;
   }
  }
  if(!chosen.ok())return hidden;
  claimedQueuedCharlie=queuedCharlie;
  claimedBufferedKKK=!queuedCharlie && !pressed && heldFreshKKK;
  kkkEdgeAt=~0u;charlieQueued=false; // one cue, one ROM handoff
  selected=col;
  d6=chosen.hyperD6; // the ONLY Hyper selection consumed by the M68K guest
  outgoingD6=actorD6;
  point=who;reserve=bench;
  x=originalX;stock=stockBefore;claimedAt=frame;phase=CLAIMED;
  hidden=queuedCharlie?0u:mask; // no interception after key release
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
