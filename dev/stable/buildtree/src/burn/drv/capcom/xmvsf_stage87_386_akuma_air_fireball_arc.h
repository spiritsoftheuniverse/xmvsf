#ifndef XMVSF_STAGE87_386_AKUMA_AIR_FIREBALL_ARC_H
#define XMVSF_STAGE87_386_AKUMA_AIR_FIREBALL_ARC_H
// XMVSF 87.386: regular jump -> Akuma air fireball follows its jump arc.
// Native F10 87.385 / 001: the 0202/0204 regular jump uses Y gravity
// +0x00006C00 and X drag 0x00000600 per game frame. At 2C24 (2A06),
// the original ROM reduces both velocities to about 1/8 and reduces
// gravity to +0x000001B0, producing the long suspended fireball.
// Restore ONLY the velocity history of a verified ordinary jump on
// transition to native airborne fireball; keep original ROM animation,
// projectile, actor X/Y position integrator, collision and landing.
// Superjump/unrecognized launch remains original hovering behavior.
// Version 3 unaffected. Opt out: XM87386_AKUMA_AIR_ARC=0.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87386AirArc {
static const std::int64_t GRAVITY=0x00006C00LL;
static const std::int64_t X_DRAG=0x00000600LL;
static const std::int64_t FIXED=0x00010000LL;
static inline bool enabled(){
 const char* v=std::getenv("XM87386_AKUMA_AIR_ARC");
 return !v || std::strcmp(v,"0")!=0;
}
static inline bool normalJumpAction(unsigned action){
 return action==0x0200u || action==0x0202u || action==0x0204u;
}
static inline bool fireballAction(unsigned action){
 return action==0x2C24u || action==0x2C26u || action==0x2C28u;
}
static inline std::int64_t signedFixed(const std::uint8_t* ram,unsigned pos){
 const std::uint32_t raw=XM87142::ramL(ram,pos);
 return (raw&0x80000000u)?
        std::int64_t(raw)-0x100000000LL:std::int64_t(raw);
}
static inline void setFixed(std::uint8_t* ram,unsigned pos,
                            std::int64_t value){
 const std::uint32_t v=std::uint32_t(value);
 ram[(pos&65535u)^1u]=std::uint8_t(v>>24u);
 ram[((pos+1u)&65535u)^1u]=std::uint8_t(v>>16u);
 ram[((pos+2u)&65535u)^1u]=std::uint8_t(v>>8u);
 ram[((pos+3u)&65535u)^1u]=std::uint8_t(v);
}
static inline std::int64_t dragX(std::int64_t v){
 if(v>0)return v>X_DRAG?v-X_DRAG:0LL;
 if(v<0)return v< -X_DRAG?v+X_DRAG:0LL;
 return 0LL;
}
static inline void logEvent(unsigned frame,unsigned side,
                            const char* event,unsigned action,
                            unsigned y,unsigned floor,
                            std::int64_t nativeVx,std::int64_t nativeVy,
                            std::int64_t vx,std::int64_t vy){
 std::fprintf(stderr,
  "XM87.386 AKUMA_AIR_ARC_%s frame=%u side=%u action=%04X "
  "y=%u floor=%u velocity=%lld,%lld -> %lld,%lld\n",
  event,frame,side+1u,action,y,floor,
  (long long)nativeVx,(long long)nativeVy,
  (long long)vx,(long long)vy);
 const char* path=std::getenv("XM87386_AKUMA_AIR_ARC_LOG");
 if(!path||!*path)return;
 FILE* f=std::fopen(path,"ab");
 if(!f)return;
 std::fprintf(f,"%u\t%u\t%s\t%04X\t%u\t%u\t%lld\t%lld\t%lld\t%lld\n",
  frame,side+1u,event,action,y,floor,
  (long long)nativeVx,(long long)nativeVy,
  (long long)vx,(long long)vy);
 std::fclose(f);
}
struct Side {
 unsigned lastFrame=~0u,lastAction=~0u,lastMode=0u,lastId=~0u;
 std::int64_t lastJumpVX=0,lastJumpVY=0;
 std::int64_t arcVX=0,arcVY=0;
 bool ordinaryJump=false,arcActive=false;
};
struct State {
 Side side[2];
 void reset(){side[0]=Side{};side[1]=Side{};}
 void afterCpu(std::uint8_t* ram,const std::uint8_t* rom,
               unsigned romSize,unsigned frame,const char* driver,
               bool version4,bool runahead){
  if(!version4||runahead||!enabled()||!ram||!rom||!driver||
     std::strcmp(driver,"xmvsfu")!=0 ||
     XM87142::ramB(ram,0x5000u)!=8u){
   reset();return;
  }
  // Original US-ROM air-fireball entry (2C24) starts at 0EA8AA,
  // with authored animation 59FA. Avoid touching unknown ROM variants.
  if(romSize<0x0EA8B2u ||
     XM87142::romW(rom,romSize,0x0EA8B0u)!=0x59FAu){
   reset();return;
  }
  for(unsigned i=0u;i<2u;++i){
   Side& s=side[i];
   if(s.lastFrame==frame)continue;
   if(s.lastFrame!=~0u &&
      (frame<s.lastFrame || frame-s.lastFrame>1u))s=Side{};
   const unsigned p=0x4000u+i*0x400u;
   const unsigned id=XM87142::ramW(ram,p+0x52u);
   const unsigned action=XM87142::ramW(ram,p+0x60u);
   const unsigned mode=XM87142::ramW(ram,p+0xA0u);
   const unsigned step=XM87142::ramW(ram,p+0x08u);
   const unsigned script=XM87142::ramL(ram,p+0x34u);
   const unsigned d6=XM87142::ramB(ram,p+0xD6u);
   const unsigned y=XM87142::ramW(ram,p+0x10u);
   const unsigned floor=XM87142::ramW(ram,p+0x254u);
   const std::int64_t nativeVX=signedFixed(ram,p+0x14u);
   const std::int64_t nativeVY=signedFixed(ram,p+0x1Cu);
   const bool actor=id==0x001Eu && floor>=32u && floor<2048u;
   const bool jump=actor && normalJumpAction(action);
   const bool fireball=actor && fireballAction(action) &&
       mode==0x2A06u && d6==8u && y<floor && step>=2u &&
       (script==0x0EA8AAu ||
        (script>=0x0EA8BAu && script<=0x0EA96Au &&
         ((script-0x0EA8BAu)&15u)==0u));
   // First three original takeoff frames: 0202/0204, mode 0402,
   // step zero, about -9.625 to -10 px/frame. Reject superjump,
   // whose initial native launch is not this exact ordinary profile.
   const bool launch=jump && mode==0x0402u && step==0u &&
       y+4u>=floor && y<=floor+4u &&
       nativeVY<=-(8LL*FIXED) && nativeVY>=-(12LL*FIXED);
   if(launch && !s.ordinaryJump){
    s.ordinaryJump=true;
   }
   if(s.arcActive && !fireball){
    logEvent(frame,i,"END",action,y,floor,nativeVX,nativeVY,
             s.arcVX,s.arcVY);
    s.arcActive=false;
   }
   const bool atFireballEntry=fireball &&
        (!fireballAction(s.lastAction) || s.lastId!=id);
   if(!s.arcActive && atFireballEntry &&
      s.ordinaryJump && s.lastId==id &&
      normalJumpAction(s.lastAction) &&
      s.lastMode==0x0604u &&
      s.lastFrame!=~0u && s.lastFrame+1u==frame){
    s.arcActive=true;
    s.arcVX=s.lastJumpVX;
    s.arcVY=s.lastJumpVY;
    logEvent(frame,i,"NORMAL_BEGIN",action,y,floor,
             nativeVX,nativeVY,s.arcVX,s.arcVY);
   }else if(atFireballEntry && !s.arcActive){
    // Preserve the native hover for superjumps or unrecognized moves.
    logEvent(frame,i,"NATIVE_HOVER",action,y,floor,
             nativeVX,nativeVY,nativeVX,nativeVY);
   }
   if(s.arcActive && fireball){
    // Native movement still owns positions, walls and collision. We
    // modify only the existing CPS2 16.16 velocity registers after
    // the ROM's slower air-fireball gravity has run this frame.
    // Normal jump gravity and drag are taken from the F10 trace.
    s.arcVX=dragX(s.arcVX);
    s.arcVY+=GRAVITY;
    if(s.arcVY>12LL*FIXED)s.arcVY=12LL*FIXED;
    if(s.arcVY< -12LL*FIXED)s.arcVY= -12LL*FIXED;
    setFixed(ram,p+0x14u,s.arcVX);
    setFixed(ram,p+0x1Cu,s.arcVY);
    // Light event-only sample verifies that native Y/X coordinates
    // follow the new velocity, without producing per-frame logs.
    if(frame%12u==0u)
     logEvent(frame,i,"ARC_SAMPLE",action,y,floor,
              nativeVX,nativeVY,s.arcVX,s.arcVY);
   }
   if(jump && mode==0x0604u && s.ordinaryJump){
    // Record the last genuine ROM jump velocity immediately before
    // Akuma's fireball cancels it. No artificial launch is created.
    s.lastJumpVX=nativeVX;
    s.lastJumpVY=nativeVY;
   }
   if(!jump && !fireball)s.ordinaryJump=false;
   s.lastFrame=frame;s.lastAction=action;s.lastMode=mode;s.lastId=id;
  }
 }
};
static inline State& state(){static State s;return s;}
} // namespace XM87386AirArc
#endif
