#ifndef XMVSF_STAGE87_340_MORPHUS_NATIVE_2V2_H
#define XMVSF_STAGE87_340_MORPHUS_NATIVE_2V2_H
// Morphus56k/Daniel Plas Rivera dhc.s 7-motion input & native exit port.
// Target two-member teams: MP+MK selects reserve A; MP+HK (B) is unavailable.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include "xmvsf_stage87_329_morphus_dhc_catalog.h"
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
static inline bool sourceHyper(unsigned fighter,unsigned d6,bool noThrow){
 if(fighter>0x24u||(fighter&1u)||!d6)return false;
 const auto& r=XM87329Morphus::rows()[fighter/2u];
 for(unsigned i=0;i<7u;++i)if(r.hyper[i]==d6 &&
     (!noThrow || !(r.throwing&(1u<<i))))return true;
 return false;
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
 unsigned owner=~0u,point=0u,reserve=0u,d6=0u,x=0u;
 unsigned lastFrame=~0u,prev=0u,hidden=0u,selected=7u,stock=0u;
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
   owner=who;prev=mask;hidden=mask&0x32u;clearSteps();
   if(phase==COMMITTED)phase=IDLE;
   return hidden;
  }
  const unsigned pressed=mask&~prev;prev=mask;
  if(!(mask&0x32u))hidden=0u;
  if(phase!=IDLE)return hidden;
  if(!inHyper){clearSteps();return hidden;}
  const unsigned d=facingDir(dir,faceRight);
  const Motion* m=motions();
  for(unsigned i=0;i<7u;++i){
   unsigned n=step[i];
   if(n && timer[i] && !--timer[i]){step[i]=0u;n=0u;}
   if(n>=m[i].len)continue;
   const unsigned expect=m[i].seq[n];
   const bool match=(expect==0x80u)?!(d&4u):(d==expect);
   if(match){step[i]=std::uint8_t(n+1u);timer[i]=14u;}
  }
  if(hidden || !(pressed&0x32u) || !sourceHyper(who,actorD6,true))
    return hidden;
  // Native donor MP+MK is reserve A; MP+HK and both selects B/ambiguous.
  if(mask!=0x12u && mask!=0x22u && mask!=0x32u)return hidden;
  hidden=0x32u;++claims;
  if(mask!=0x12u || !eligible || !stockBefore || who==bench ||
     bench>0x24u || (bench&1u)){++refusals;return hidden;}
  const auto& row=XM87329Morphus::rows()[bench/2u];
  static const unsigned priority[]={4u,5u,2u,3u,0u,1u,6u};
  unsigned best=0u,col=7u;
  for(unsigned i=0;i<7u;++i){
   const unsigned j=priority[i];
   if(!row.hyper[j] || (row.air|row.conditional)&(1u<<j) ||
      step[j]<m[j].len || timer[j]<=best)continue;
   best=timer[j];col=j;
  }
  if(col==7u){++refusals;return hidden;}
  selected=col;d6=row.hyper[col];point=who;reserve=bench;
  x=originalX;stock=stockBefore;phase=CLAIMED;
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
