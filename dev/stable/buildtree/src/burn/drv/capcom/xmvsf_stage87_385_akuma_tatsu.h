#ifndef XMVSF_STAGE87_385_AKUMA_TATSU_H
#define XMVSF_STAGE87_385_AKUMA_TATSU_H
// XMVSF 87.385: Akuma native QCF+LK/MK/HK, 2C40/2C42/2C44.
// User 87.384 F10 evidence, BOTH captures:
// active scripts 0E9C9A/0E9CCE/0E9D02, animation 661C,
// native D6=1C, step 2, attack slot +70=004F;
// opponent 1C16 step 2 receives upward vy FFF64A00 (-9.71)
// even while both characters are airborne. Per-hit vertical-only
// change follows the original ROM hit. Never create a hit, change
// damage, animation, attacker velocity, ROM input or hit stun.
// Native landing tails 2C46/2C48/2C4A are 10-11 frames;
// shorten original animation hold to a positive THREE ticks,
// using the same timer proven for 87.375 Akuma teleport.
// V4 only, US xmvsfu only, regardless of Tournament on/off.
// Optional A/B: XM87385_AKUMA_TATSU=0. No build/replay dependency.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87385Tatsu {
static inline void writeW(std::uint8_t* ram,unsigned p,unsigned v){
 ram[(p&65535u)^1u]=std::uint8_t(v>>8);
 ram[((p+1u)&65535u)^1u]=std::uint8_t(v);
}
static inline void writeL(std::uint8_t* ram,unsigned p,unsigned v){
 writeW(ram,p,v>>16u);writeW(ram,p+2u,v);
}
static inline std::int64_t signedL(const std::uint8_t* ram,unsigned p){
 const std::uint32_t u=XM87142::ramL(ram,p);
 return u&0x80000000u ? std::int64_t(u)-0x100000000LL : std::int64_t(u);
}
static inline bool enabled(){
 const char* v=std::getenv("XM87385_AKUMA_TATSU");
 return !v || std::strcmp(v,"0")!=0;
}
static inline void event(const char* type,unsigned frame,unsigned side,
                         unsigned move,int ay,int vy,
                         std::int64_t before,std::int64_t after){
 std::fprintf(stderr,
     "XM87.385 AKUMA_TATSU_%s frame=%u side=%u action=%04X "
     "attackerY=%d victimY=%d before=%lld after=%lld\n",
     type,frame,side+1u,move,ay,vy,
     (long long)before,(long long)after);
 const char* name=std::getenv("XM87385_AKUMA_TATSU_LOG");
 if(!name||!*name)return;
 FILE* f=std::fopen(name,"ab");
 if(!f)return;
 std::fprintf(f,"%u\t%u\t%s\t%04X\t%d\t%d\t%lld\t%lld\n",
     frame,side+1u,type,move,ay,vy,
     (long long)before,(long long)after);
 std::fclose(f);
}
static inline unsigned attackRecord(unsigned act){
 switch(act){
 case 0x2C40u:return 0x0E9C9Au; // LK
 case 0x2C42u:return 0x0E9CCEu; // MK
 case 0x2C44u:return 0x0E9D02u; // HK
 default:return 0u;
 }
}
static inline unsigned tailRecord(unsigned action){
 switch(action){
 case 0x2C46u:return 0x0E9D16u; // LK landing
 case 0x2C48u:return 0x0E9D2Au; // MK landing
 case 0x2C4Au:return 0x0E9D3Eu; // HK landing
 default:return 0u;
 }
}
struct Side {
 unsigned frame=~0u,previousAction=~0u,previousFighter=~0u;
 unsigned previousVictimFighter=~0u,lastHitFrame=~0u;
 std::int64_t previousVictimVy=0;
 bool primed=false;
};
struct State {
 Side sides[2];
 void reset(){sides[0]=Side{};sides[1]=Side{};}
 void afterCpu(std::uint8_t* ram,const std::uint8_t* rom,
               unsigned romSize,unsigned now,const char* driver,
               bool v4,bool runahead){
  if(!v4||runahead||!enabled()||!ram||!rom||!driver||
     std::strcmp(driver,"xmvsfu")!=0||
     XM87142::ramB(ram,0x5000u)!=8u){
   reset();return;
  }
  // Required ROM addresses must exist; native pointers are validated
  // below against fixed action-script pairs from the uploaded capture.
  if(romSize<0x0E9D4Eu){reset();return;}
  for(unsigned s=0;s<2u;++s){
   Side& st=sides[s];
   if(st.frame==now)continue;
   if(st.frame!=~0u && (now<st.frame||now-st.frame>2u))st=Side{};
   const unsigned p=0x4000u+s*0x400u;
   const unsigned other=0x4000u+(1u-s)*0x400u;
   const unsigned id=XM87142::ramW(ram,p+0x52u);
   const unsigned action=XM87142::ramW(ram,p+0x60u);
   const unsigned victimId=XM87142::ramW(ram,other+0x52u);
   const unsigned y=XM87142::ramW(ram,p+0x10u);
   const unsigned vy=XM87142::ramW(ram,other+0x10u);
   const unsigned floorA=XM87142::ramW(ram,p+0x254u);
   const unsigned floorB=XM87142::ramW(ram,other+0x254u);
   const std::int64_t nativeVy=signedL(ram,other+0x1Cu);
   const unsigned script=XM87142::ramL(ram,p+0x34u);
   const unsigned anim=XM87142::ramW(ram,p+0x42u);
   const unsigned step=XM87142::ramW(ram,p+0x08u);
   if(id==0x001Eu && victimId!=0u && victimId!=0x001Eu &&
      floorA>=32u && floorA<2048u && floorB>=32u && floorB<2048u){
    // Detect actual ROM-launched upward velocity *on a connected kick*
    // rather than continuously writing vy while the move is active.
    // The two supplied captures contain 9 such impulse edges across
    // all three strengths, and no unrelated action matches the gate.
    const unsigned active=attackRecord(action);
    const unsigned victimAction=XM87142::ramW(ram,other+0x60u);
    const unsigned victimStep=XM87142::ramW(ram,other+0x08u);
    const unsigned attackHit=XM87142::ramW(ram,p+0x70u);
    const unsigned attackMode=XM87142::ramW(ram,p+0xA0u);
    const unsigned attackD6=XM87142::ramB(ram,p+0xD6u);
    const int xA=int(XM87142::ramW(ram,p+0x0Cu));
    const int xB=int(XM87142::ramW(ram,other+0x0Cu));
    const int diff=xA-xB;
    const bool close=diff>=-96 && diff<=96;
    const bool newUpImpulse=st.primed &&
       st.previousFighter==id && st.previousVictimFighter==victimId &&
       nativeVy<-(2LL<<16) &&
       st.previousVictimVy-nativeVy>=(4LL<<16) &&
       (st.lastHitFrame==~0u||now-st.lastHitFrame>=4u);
    if(active && script==active && anim==0x661Cu &&
       step==2u && attackMode==0x2A28u && attackD6==0x1Cu &&
       attackHit==0x004Fu && victimAction==0x1C16u &&
       victimStep==2u && y+12u<floorA && vy+12u<floorB &&
       close && newUpImpulse){
      // Native Akuma Tatsu descends at +8.0 pixels/frame in both
      // supplied traces. Pull the opponent toward his *current*
      // vertical plane over ~4 frames while remaining downward.
      // Relative vertical error influences each hit separately.
      const std::int64_t akumaVy=signedL(ram,p+0x1Cu);
      const std::int64_t base=akumaVy>0 && akumaVy<=(12LL<<16)?
          akumaVy:(8LL<<16);
      std::int64_t target=base+std::int64_t(int(y)-int(vy))*16384LL;
      if(target<(5LL<<16))target=5LL<<16;
      if(target>(11LL<<16))target=11LL<<16;
      writeL(ram,other+0x1Cu,std::uint32_t(target));
      st.lastHitFrame=now;
      event("HIT_DOWN",now,s,action,int(y),int(vy),nativeVy,target);
    }
    // When the native kick reaches its landing tail, shorten only
    // the ORIGINAL positive animation countdown, never force neutral.
    const unsigned tail=tailRecord(action);
    if(tail && st.primed && st.previousFighter==id &&
       st.previousAction==action-6u &&
       script==tail && anim==0x4C52u && step==4u &&
       y+12u>=floorA){
      const unsigned t=XM87142::ramW(ram,p+0x3Eu);
      if(t>3u && t<0x1000u){
       writeW(ram,p+0x3Eu,3u);
       event("LAND_3F",now,s,action,int(y),int(vy),t,3u);
      }else{
       event("LAND_TIMER_NOT_POSITIVE",now,s,action,
             int(y),int(vy),t,t);
      }
    }
   }
   st.frame=now;st.previousAction=action;st.previousFighter=id;
   st.previousVictimFighter=victimId;
   st.previousVictimVy=signedL(ram,other+0x1Cu);
   st.primed=true;
  }
 }
};
inline State& state(){static State s;return s;}
} // namespace XM87385Tatsu
#endif
