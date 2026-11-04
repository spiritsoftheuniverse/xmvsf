#ifndef XMVSF_STAGE87_125_STUN_REARM_LOCK_H
#define XMVSF_STAGE87_125_STUN_REARM_LOCK_H
// XM87.125: Tournament-only per-victim stun-resistance rearm lock.
// Evidence: native XMVSFU fighter +B8 is remaining stun resistance (50/60),
// dizzy-pop is action 1C38/1C3A/1C36 with flag +135 (raw +134) == FF;
// fighter floor Y is CPU word +254. The 87.119 captured victim landing changed
// y 463->472, action 1C32->1C34 and air flag +AA 1->0 on ONE frame.
// Ground Y alone is NEVER a valid unlock (87.99 logs showed false positives).
//
// No 68000 ROM modifications. Post-CPU stabilization of +B8 cannot suppress
// a theoretical single native hit that empties an entire 50/60 resistance
// capacity inside one emulated frame; this is explicitly audited in the TSV.
// No modifications to life, damage, collision, action, velocity or dizzy flag.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87125Stun {
static constexpr unsigned BASE[2]={0x4000u,0x4400u};
static constexpr std::uint8_t CAPACITY[18]={50,60,60,60,60,60,50,60,60,60,60,60,60,60,60,50,60,60};
static inline unsigned cap(unsigned id){return (id<=34u && !(id&1u))?CAPACITY[id>>1]:0u;}
static inline unsigned b8(const std::uint8_t* r,unsigned at){return r[(at&0xffffu)^1u];}
static inline unsigned w16(const std::uint8_t* r,unsigned at){return (b8(r,at)<<8)|b8(r,at+1u);}
static inline bool combat(const std::uint8_t* r){return r&&(b8(r,0x5000u)==8u||b8(r,0x5001u)==8u);}
static inline bool dizzyPop(unsigned act){return act==0x1c38u||act==0x1c3au||act==0x1c36u;}
static inline bool floorValid(unsigned floor){return floor>=128u&&floor<=768u;}
static inline bool groundMode(unsigned mode){return mode==0u||mode==2u||mode==8u;}
static inline bool nativeLandingEdge(unsigned priorAction,unsigned afterAction,
                                    unsigned priorMode,unsigned afterMode,
                                    unsigned priorAir,unsigned afterAir){
 return (priorAction!=afterAction)||(priorAir!=0u&&afterAir==0u)||
        (priorMode!=afterMode&&groundMode(afterMode));
}
struct Sample {
 bool valid=false;
 unsigned id=0,capacity=0,remaining=0,flag=0,action=0,y=0,floor=0,mode=0,air=0;
};
struct Lock {
 bool armed=false,seenAir=false,pending=false;
 unsigned since=0,remaining=0,contactFrame=0,contactAction=0,contactMode=0,contactAir=0;
 unsigned suppressed=0;
};
struct Controller {
 Lock victims[2][18];
 Sample preFrame[2];
 bool initialized=false,previousEnabled=false,haveFrame=false;
 unsigned lastFrame=0;
 std::FILE* log=nullptr;
 ~Controller(){if(log)std::fclose(log);}
 void init(){
  if(initialized)return;
  initialized=true;
  const char* path=std::getenv("XM87125_STUN_LOCK_LOG_FILE");
  if(path&&*path){log=std::fopen(path,"wb");if(log)
   std::fputs("frame\tevent\tvictim\tcharacter_id\tlocked\tpre_remaining\tpost_remaining\theld_remaining\tpre_y\tpost_y\tfloor\tpre_action\tpost_action\tpre_mode\tpost_mode\tpre_air\tpost_air\tair_seen\tdetail\n",log);
  }
  std::fputs("XM87.125 STUN_REARM_LOCK_READY native dizzy +B8; require actual air-to-floor transition AND native landing state change\n",stderr);
 }
 void reset(){
  for(auto& side:victims)for(auto& v:side)v=Lock{};
  for(auto& s:preFrame)s=Sample{};
 }
 static Sample sample(const std::uint8_t* r,unsigned p){
  Sample s;if(!r)return s;
  const unsigned b=BASE[p];s.id=r[b+0x52u];s.capacity=cap(s.id);
  if(!s.capacity)return s;
  s.valid=true;s.remaining=r[b+0xb8u];s.flag=r[b+0x134u];
  s.action=w16(r,b+0x60u);s.y=w16(r,b+0x10u);
  s.floor=w16(r,b+0x254u);s.mode=b8(r,b+0xa0u);s.air=b8(r,b+0xaau);
  return s;
 }
 void event(unsigned frame,const char* what,unsigned p,const Sample& prior,
            const Sample& now,const Lock& lock,const char* detail){
  if(!log)return;
  std::fprintf(log,"%u\t%s\tP%u\t%02X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%04X\t%04X\t%02X\t%02X\t%u\t%u\t%u\t%s\n",
   frame,what,p+1u,now.id,unsigned(lock.armed),prior.remaining,now.remaining,lock.remaining,
   prior.y,now.y,now.floor,prior.action,now.action,prior.mode,now.mode,prior.air,now.air,
   unsigned(lock.seenAir),detail);
  std::fflush(log);
 }
 // Called once, after inputs and before this frame's emulated 68000 work.
 // 'enabled' must be live Tournament mode with the verified 87.95 native
 // dizzy-pop hook installed. In Classic, this controller is read-only.
 void before(std::uint8_t* r,unsigned frame,bool enabled){
  init();
  if((haveFrame&&frame<lastFrame)||!enabled||!combat(r)){
   if(previousEnabled||(haveFrame&&frame<lastFrame))reset();
   previousEnabled=false;
  }
  haveFrame=true;lastFrame=frame;
  const bool active=enabled&&combat(r);
  previousEnabled=active;
  for(unsigned p=0;p<2u;++p){
   preFrame[p]=Sample{};
   if(!active)continue;
   const Sample s=sample(r,p);
   if(!s.valid)continue;
   preFrame[p]=s;
   Lock& v=victims[p][s.id>>1];
   if(v.armed){
    // Keep the native field at the resistance value restored by dizzy.
    // Never change the action, damage, state, or collision controls.
    r[BASE[p]+0xb8u]=static_cast<std::uint8_t>(v.remaining);
   }
  }
 }
 // Called immediately after the ROM frame, before 87.96 clears the dizzy
 // marker on pop exit. A landing on the same frame as dizzy onset is NOT enough.
 void after(std::uint8_t* r,unsigned frame,bool enabled){
  if(!initialized)init();
  if(!r||!enabled||!previousEnabled||!combat(r))return;
  for(unsigned p=0;p<2u;++p){
   const Sample prior=preFrame[p];
   if(!prior.valid)continue;
   const Sample now=sample(r,p);
   if(!now.valid||now.id!=prior.id||now.capacity!=prior.capacity)continue;
   Lock& v=victims[p][now.id>>1];
   const bool newDizzy=now.flag==0xffu&&dizzyPop(now.action)&&
       (prior.flag!=0xffu||!dizzyPop(prior.action));
   if(newDizzy&&!v.armed){
    v=Lock{};v.armed=true;v.since=frame;v.remaining=now.capacity;
    v.seenAir=floorValid(now.floor)&&now.y<now.floor;
    r[BASE[p]+0xb8u]=static_cast<std::uint8_t>(v.remaining);
    Sample held=now;held.remaining=v.remaining;
    event(frame,"DIZZY_STUN_LOCK_ARMED",p,prior,held,v,
          "Native dizzy action + marker confirmed; hold reset capacity until later landing");
    continue;
   }
   if(!v.armed)continue;
   // Post-frame repair is required even when a grab or hit interrupts the pop.
   // Deliberately retain the resistance on the landing frame itself.
   if(now.remaining!=v.remaining){
    if(now.remaining<v.remaining)++v.suppressed;
    r[BASE[p]+0xb8u]=static_cast<std::uint8_t>(v.remaining);
    Sample held=now;held.remaining=v.remaining;
    event(frame,"STUN_RESISTANCE_HELD",p,prior,held,v,
          "ROM hit/grab state intact; only native +B8 restored");
   }
   if(newDizzy){
    event(frame,"POSSIBLE_IN_FRAME_REDIZZY",p,prior,now,v,
          "ROM reacted to another dizzy while lock was armed; capture for CPU-level guard");
   }
   if(frame<=v.since)continue;
   const bool sameFloor=floorValid(prior.floor)&&floorValid(now.floor)&&
                        prior.floor==now.floor;
   if(sameFloor&&now.y<now.floor){v.seenAir=true;v.pending=false;}
   const bool physicalContact=sameFloor&&v.seenAir&&prior.y<prior.floor&&
                    now.y>=now.floor&&now.y>prior.y;
   const bool nativeTransition=nativeLandingEdge(prior.action,now.action,
                                    prior.mode,now.mode,prior.air,now.air);
   bool confirmed=false;
   if(physicalContact){
    if(nativeTransition){confirmed=true;}
    else {
     v.pending=true;v.contactFrame=frame;v.contactAction=now.action;
     v.contactMode=now.mode;v.contactAir=now.air;
     event(frame,"CONTACT_AWAITING_NATIVE_STATE",p,prior,now,v,
           "Physical floor crossing seen; awaiting ROM action/mode/air transition");
    }
   }
   else if(v.pending){
    if(frame>v.contactFrame+4u||!sameFloor||now.y<now.floor){
     v.pending=false;
    }else if(now.y>=now.floor&&nativeLandingEdge(v.contactAction,now.action,
                                   v.contactMode,now.mode,v.contactAir,now.air)){
     confirmed=true;
    }
   }
   if(confirmed){
    v.armed=false;v.pending=false;v.seenAir=false;
    event(frame,"LANDING_CONFIRMED_STUN_REARM",p,prior,now,v,
         "Observed later air-to-floor crossing AND ROM landing-state transition; next frame may build stun");
   }
  }
 }
};
static inline Controller& state(){static Controller c;return c;}
} // namespace XM87125Stun
#endif
