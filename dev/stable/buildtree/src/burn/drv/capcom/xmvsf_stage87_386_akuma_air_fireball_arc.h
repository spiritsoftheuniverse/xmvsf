#ifndef XMVSF_STAGE87_386_AKUMA_AIR_FIREBALL_ARC_H
#define XMVSF_STAGE87_386_AKUMA_AIR_FIREBALL_ARC_H
// XMVSF 87.387: correct neutral/backward normal-jump signatures and
// release native air fireball on actual floor contact.
// Native F10 87.385 / 001: the 0200/0202/0204 regular jump uses Y gravity
// +0x00006C00 and X drag 0x00000600 per game frame. At 2C24 (2A06),
// the original ROM reduces both velocities to about 1/8 and reduces
// gravity to +0x000001B0, producing the long suspended fireball.
// Restore normal-jump velocity without changing the ROM actor X/Y
// position integrator or projectile. On exact floor contact, advance
// the original animation record so the ROM can perform landing.
// 87.386 F10: Akuma reaches y=floor=472 while still in 2C24/2A06
// (e.g. frame 2961), then stays in the special for 16 more frames.
// End the source ROM animation on contact through the verified final
// 0EA96A script record; let the guest transition to its normal
// 0212/0214/0216 then 0206 landing. No position or action writes.
// Superjump/unrecognized launch retains original hover in mid-air.
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
static inline bool normalLaunch(unsigned action,unsigned mode,unsigned step,
                                unsigned y,unsigned floor,unsigned script,
                                unsigned anim,unsigned d6,
                                std::int64_t vx,std::int64_t vy){
 // F10 87.386: native normal starts in 0400 / 0402 / 0408.
 // 87.386 only accepted 0402, silently excluding straight-up and
 // many backward takeoffs. Verify exact ROM script and 16.16 launch
 // velocities to exclude running/superjump states (e.g. 0414 dash).
 if(step!=0u || d6!=8u || anim!=0x4FAEu ||
    (mode!=0x0400u && mode!=0x0402u && mode!=0x0408u) ||
    y+4u<floor || y>floor+4u)return false;
 switch(action){
 case 0x0200u:return script==0x0E81CEu &&
                vx==0 && vy==-0x00098000LL;
 case 0x0202u:return script==0x0E8242u &&
                vx==0x0003C000LL && vy==-0x0009A000LL;
 case 0x0204u:return script==0x0E82D6u &&
                vx==-0x00042000LL && vy==-0x000A0000LL;
 default:return false;
 }
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
static inline void setWord(std::uint8_t* ram,unsigned pos,unsigned value){
 ram[(pos&65535u)^1u]=std::uint8_t(value>>8);
 ram[((pos+1u)&65535u)^1u]=std::uint8_t(value);
}
static inline void setLong(std::uint8_t* ram,unsigned pos,unsigned value){
 setWord(ram,pos,value>>16u);
 setWord(ram,pos+2u,value);
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
  "XM87.387 AKUMA_AIR_ARC_%s frame=%u side=%u action=%04X "
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
 unsigned lastY=~0u,landingFrame=~0u;
 std::int64_t lastJumpVX=0,lastJumpVY=0;
 std::int64_t arcVX=0,arcVY=0;
 bool ordinaryJump=false,arcActive=false;
 bool landingArmed=false,landingWaitReported=false;
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
  // Source US ROM: first 2C24 script 0EA8AA (anim 59FA),
  // terminal record 0EA96A (anim 59FA), looping next pointer
  // 0EA8AA. All three are present in captured native F10 frames.
  // Do not bypass original animation on any unverified ROM.
  if(romSize<0x0EA972u ||
     XM87142::romW(rom,romSize,0x0EA8B0u)!=0x59FAu ||
     XM87142::romW(rom,romSize,0x0EA970u)!=0x59FAu){
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
   const unsigned anim=XM87142::ramW(ram,p+0x42u);
   const unsigned d6=XM87142::ramB(ram,p+0xD6u);
   const unsigned y=XM87142::ramW(ram,p+0x10u);
   const unsigned floor=XM87142::ramW(ram,p+0x254u);
   const std::int64_t nativeVX=signedFixed(ram,p+0x14u);
   const std::int64_t nativeVY=signedFixed(ram,p+0x1Cu);
   const bool actor=id==0x001Eu && floor>=32u && floor<2048u;
   const bool jump=actor && normalJumpAction(action);
   const bool fireballPose=actor && action==0x2C24u &&
       mode==0x2A06u && d6==8u && step>=2u &&
       (script==0x0EA8AAu ||
        (script>=0x0EA8BAu && script<=0x0EA96Au &&
         ((script-0x0EA8BAu)&15u)==0u));
   const bool fireball=fireballPose && y<floor;
   const bool launch=jump && normalLaunch(action,mode,step,y,floor,
                                           script,anim,d6,nativeVX,nativeVY);
   if(launch && !s.ordinaryJump)s.ordinaryJump=true;
   // Native Y has reached the EXACT fighter floor while action is
   // still 2C24 / mode 2A06. Its hit/position integrator clamps Y
   // but animation continues for 2-16f. That is floor contact, not
   // the guest's landing-action transition.
   const bool floorContact=fireballPose && y>=floor &&
       nativeVY>=0 && s.lastId==id && s.lastAction==action &&
       s.lastMode==0x2A06u && s.lastY<floor &&
       s.lastFrame!=~0u && s.lastFrame+1u==frame;
   if(s.landingArmed){
    if(!fireballPose || id!=s.lastId){
     logEvent(frame,i,"LAND_NATIVE_EXIT",action,y,floor,
              nativeVX,nativeVY,nativeVX,nativeVY);
     s.landingArmed=false;
    }else if(!s.landingWaitReported &&
             frame>=s.landingFrame+3u){
     logEvent(frame,i,"LAND_PENDING",action,y,floor,
              nativeVX,nativeVY,nativeVX,nativeVY);
     s.landingWaitReported=true;
    }
   }
   if(floorContact && !s.landingArmed){
    // Advance the existing 68000 sprite-script runner to its genuine
    // final 16-byte record with a one-tick positive countdown.
    // The ROM, not C++, will choose 0212/0214/0216 and 0206.
    // 0 is the native runner's sentinel and must never be written.
    setLong(ram,p+0x34u,0x0EA96Au);
    setLong(ram,p+0x38u,0x0EA8AAu);
    setWord(ram,p+0x3Eu,1u);
    setWord(ram,p+0x42u,0x59FAu);
    s.landingArmed=true;s.landingWaitReported=false;
    s.landingFrame=frame;
    s.arcActive=false;s.ordinaryJump=false;
    logEvent(frame,i,"LAND_SCRIPT_FINISH",action,y,floor,
             nativeVX,nativeVY,nativeVX,nativeVY);
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
   if(s.arcActive && fireball && !s.landingArmed){
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
   s.lastFrame=frame;s.lastAction=action;s.lastMode=mode;
   s.lastId=id;s.lastY=y;
  }
 }
};
static inline State& state(){static State s;return s;}
} // namespace XM87386AirArc
#endif
