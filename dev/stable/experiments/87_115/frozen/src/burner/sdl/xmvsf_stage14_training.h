// XM87.100 Training health: real native HP damage and 120 consecutive frames
// of standing neutral before genuine HP + HUD gauge restoration. KO is handled
// separately by the native opcode hook at $015B7E (Training only).
#ifndef XMVSF_STAGE14_TRAINING_H
#define XMVSF_STAGE14_TRAINING_H
#include "xmvsf_stage14_mode.h"
#include "xmvsf_stage87_100_idle_logic.h"
#include "xmvsf_stage87_105_training_hp.h"
extern int xm87102DamageMode;
#include "burnint.h"
#include <cstdio>

static inline unsigned xm87100ReadWord(cpu_core_config* cpu,unsigned addr){
 return ((cpu->read(addr)&255u)<<8)|(cpu->read(addr+1u)&255u);
}
static inline unsigned xm87100ReadLong(cpu_core_config* cpu,unsigned addr){
 return (xm87100ReadWord(cpu,addr)<<16)|xm87100ReadWord(cpu,addr+2u);
}
static inline void xm87100WriteWord(cpu_core_config* cpu,unsigned addr,unsigned n){
 cpu->write(addr,(n>>8)&255u);cpu->write(addr+1u,n&255u);
}
static inline void xm14ApplyTrainingCheats(bool noDizzyTraining=false){
 XM87100Idle::State& idle=XM87100Idle::state();
 if(!xm14TrainingMode||bBurnRunAheadFrame||!bDrvOkay){
  if(!xm14TrainingMode||!bDrvOkay)idle.reset();
  return;
 }
 cpu_core_config* cpu=GetCpuCoreConfig(0);
 if(!cpu||!cpu->read||!cpu->write||!cpu->open||!cpu->close)return;
 cpu->open(0);
 const unsigned phase=cpu->read(0xFF5000u)&255u;
 const unsigned actor=xm87100ReadLong(cpu,0xFFA014u);
 const bool screenReady=phase>=2u&&phase<=8u&&(phase&1u)==0u&&actor==0x00FF4000u;
 const unsigned bases[2]={0xFF4000u,0xFF4400u};
 if(!screenReady){XM87105HP::state().seeCombat(false);idle.reset();cpu->close();return;}
 XM87105HP::State& presets=XM87105HP::state();
 presets.seeCombat(phase==8u);
 if(phase==8u){
  // XM87.107: ONLY P2 Dummy 1 and Dummy 2 receive HP slider writes.
  // Resolve the native P2 point/reserve slots by their character indices.
  for(unsigned slot=0;slot<2u;++slot){
   const unsigned base=XM87105HP::p2Base(slot);
   const unsigned hp=xm87100ReadWord(cpu,base+0x210u);
   const unsigned id=cpu->read(base+0x53u)&255u;
   const bool valid=hp>=1u&&hp<=XM87105HP::MAX_HP&&id<36u&&(id&1u)==0u;
   const unsigned owner=presets.observe(slot,id,valid);
   if(!valid)continue;
   unsigned dummy=2u;
   if(!presets.takeSlot(slot,dummy))continue;
   const unsigned n=presets.target(dummy);
   xm87100WriteWord(cpu,base+0x210u,n);
   xm87100WriteWord(cpu,base+0x21au,n);
   if(slot==0u)idle.fighters[1].reset();
   std::fprintf(stderr,"XM87.107 TRAINING_DUMMY_HP D%u slot=%u base=%06X id=%u HP=%u\n",
      owner+1u,slot+1u,base,id,n);
  }
 }
 const unsigned hp0=xm87100ReadWord(cpu,bases[0]+0x210u);
 const unsigned hp1=xm87100ReadWord(cpu,bases[1]+0x210u);
 const bool ready=hp0>=1u&&hp0<=XM87100Idle::MAX_HP&&hp1>=1u&&hp1<=XM87100Idle::MAX_HP;
 if(!ready){idle.reset();cpu->close();return;}
 if(!idle.enabled){idle.reset();idle.enabled=true;}
 // Original infinite-round timer stays; do not touch actual fighter HP here.
 if((cpu->read(0xFF5008u)&255u)<=0x99u)cpu->write(0xFF5008u,0x99u);
 if(noDizzyTraining){
  static const unsigned char cap[18]={50,60,60,60,60,60,50,60,60,60,60,60,60,60,60,50,60,60};
  for(unsigned i=0;i<2u;++i){
   const unsigned base=bases[i],id=cpu->read(base+0x53u)&255u;
   if((id&1u)==0u&&id<36u)cpu->write(base+0xB9u,cap[id>>1]);
  }
 }
 if(xm87102DamageMode!=0){idle.reset();cpu->close();return;} // XM87.102 native KO: no regeneration
 if(idle.lastPhase!=phase){
  std::fprintf(stderr,"XM87.100 TRAINING phase=%02X realHP=P1:%u P2:%u timer=120 idle frames noDizzy=%u\n",
   phase,hp0,hp1,(unsigned)noDizzyTraining);
  idle.lastPhase=phase;
 }
 // Only active match frames count towards the idle refill.
 if(phase!=8u){idle.fighters[0].reset();idle.fighters[1].reset();cpu->close();return;}
 for(unsigned p=0;p<2u;++p){
  const unsigned base=bases[p];
  XM87100Idle::Fighter& f=idle.fighters[p];
  const unsigned hp=xm87100ReadWord(cpu,base+0x210u);
  const unsigned action=xm87100ReadWord(cpu,base+0x60u);
  const unsigned x=xm87100ReadWord(cpu,base+0x0cu);
  const unsigned y=xm87100ReadWord(cpu,base+0x10u);
  const unsigned contactPtr=xm87100ReadLong(cpu,base+0x88u);
  // Native collision handler writes +88 when contact occurs. A new nonzero
  // pointer or changed opponent pointer resets the timer, whether blocked or hit.
  const bool contact=contactPtr!=0u&&contactPtr!=f.previousContact;
  f.previousContact=contactPtr;
  if(f.tick(true,action,hp,x,y,contact,p==0u?XM87100Idle::MAX_HP:presets.activeTarget())){
   // This is genuine fighter RAM, not a HUD-only overlay: both native HP and
   // the damage-gauge value are restored together in this fighter only.
   xm87100WriteWord(cpu,base+0x210u,p==0u?XM87100Idle::MAX_HP:presets.activeTarget());
   xm87100WriteWord(cpu,base+0x21au,p==0u?XM87100Idle::MAX_HP:presets.activeTarget());
   std::fprintf(stderr,"XM87.100 TRAINING_IDLE_REFILL P%u HP=%u after 120 idle frames\n",
     p+1u,p==0u?XM87100Idle::MAX_HP:presets.activeTarget());
  }
 }
 cpu->close();
}
#endif
