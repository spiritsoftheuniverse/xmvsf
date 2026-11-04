#ifndef XMVSF_STAGE87_95_NATIVE_DIZZY_AIR_COLLISION_H
#define XMVSF_STAGE87_95_NATIVE_DIZZY_AIR_COLLISION_H
// Stage 87.95: verified XMVSFU native dizzy-air collision eligibility experiment.
// Stage 87.94 only altered the launch initializer at PC 015118, but the 1C38/1C3A
// state handler re-closes the hurtbox at E784/E78A on subsequent frames.
// Tournament ONLY: preserve the existing safe 87.94 initializer operands, and
// NOP the TWO unconditional native airborne state writes at E784 and E78A.
// No action, stun meter, HP, 68000 movement, position, or on-disk ROM edits.
// Exact source signatures verified against SHA256 e6020e76... original CPS2
// decrypted 1 MiB fetch opcodes for xmvsfu; both native fetch buffer layouts.
// Classic restores all original bytes verbatim. Unknown/mixed signatures refuse.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
namespace XM8795Native {
static constexpr unsigned INIT=0x15118u,INITLEN=12u;
static constexpr unsigned AIR=0x0e784u,AIRLEN=22u;
static constexpr std::uint8_t INIT_ORIG[INITLEN]={0x1d,0x7c,0,0,0,0x83,0x3d,0x7c,0,2,0,0xf6};
static constexpr std::uint8_t INIT_TEST[INITLEN]={0x1d,0x7c,0,0x80,0,0x83,0x3d,0x7c,0,0,0,0xf6};
static constexpr std::uint8_t AIR_ORIG[AIRLEN]={0x3d,0x7c,0,2,0,0xf6,0x42,0x2e,0,0x83,0x30,0x2e,0,8,0x32,0x3b,0,6,0x4e,0xfb,0x10,2};
static constexpr std::uint8_t AIR_TEST[AIRLEN]={0x4e,0x71,0x4e,0x71,0x4e,0x71,0x4e,0x71,0x4e,0x71,0x30,0x2e,0,8,0x32,0x3b,0,6,0x4e,0xfb,0x10,2};
static inline unsigned u8(const std::uint8_t* ram,unsigned a){return ram[(a&65535u)^1u];}
static inline unsigned u16(const std::uint8_t* ram,unsigned a){return (u8(ram,a)<<8)|u8(ram,a+1u);}
static inline bool env(const char* name){const char* s=std::getenv(name);return s&&!std::strcmp(s,"1");}
enum class Layout { NONE, BIG, SWAPPED };
static inline const char* layoutName(Layout a){return a==Layout::BIG?"BE":a==Layout::SWAPPED?"WORD_SWAP":"UNKNOWN";}
static inline bool matches(const std::uint8_t* p,const std::uint8_t* expected,unsigned n,bool swapped){
 for(unsigned i=0;i<n;i+=2){if(p[i]!=(swapped?expected[i+1]:expected[i])||p[i+1]!=(swapped?expected[i]:expected[i+1]))return false;}
 return true;
}
static inline bool isPair(const std::uint8_t* c,unsigned n,Layout layout,bool patched){
 if(!c||n<AIR+AIRLEN||n<INIT+INITLEN||layout==Layout::NONE)return false;
 const bool swap=layout==Layout::SWAPPED;
 return matches(c+INIT,patched?INIT_TEST:INIT_ORIG,INITLEN,swap)&&
        matches(c+AIR,patched?AIR_TEST:AIR_ORIG,AIRLEN,swap);
}
static inline Layout identify(const std::uint8_t* c,unsigned n,bool& patched){
 patched=false;
 for(Layout l:{Layout::BIG,Layout::SWAPPED}){
  if(isPair(c,n,l,false))return l;
  if(isPair(c,n,l,true)){patched=true;return l;}
 }
 return Layout::NONE;
}
static inline void writeSequence(std::uint8_t* dest,const std::uint8_t* src,unsigned n,bool swap){
 for(unsigned i=0;i<n;i+=2){dest[i]=src[i+(swap?1u:0u)];dest[i+1]=src[i+(swap?0u:1u)];}
}
struct Controller {
 bool initialized=false,installed=false,refused=false;
 Layout layout=Layout::NONE;
 unsigned changes=0,airFrames[2]={0,0};
 std::FILE* log=nullptr;
 bool prevAir[2]={false,false};unsigned prevGate[2]={0,0},prevTimer[2]={0,0};
 ~Controller(){if(log)std::fclose(log);}
 void init(){
  if(initialized)return;
  initialized=true;
  const char* name=std::getenv("XM8795_NATIVE_HIT_LOG_FILE");
  if(name&&*name){log=std::fopen(name,"wb");if(log){
   std::fputs("frame\tevent\tplayer\tmode\tpatched\tfighter_id\tstun_capacity\tstun_remaining\tdizzy134\taction\tsubstate06\tx\ty\tgate83\tf6\tdefensive_box74\tcontact87\tattacker_id\tattacker_action\tattacker_attack_box70\tattacker_gate82\tattacker_latch84\tattacker_x\tattacker_y\tdetail\n",log);
  }}
  std::fputs("XM87.95 NATIVE_DIZZY_AIR_COLLISION_READY; 68000 native tick override (Tournament only)\n",stderr);
 }
 static unsigned cap(unsigned id){static const unsigned char capacities[18]={50,60,60,60,60,60,50,60,60,60,60,60,60,60,60,50,60,60};return id<=34&&!(id&1)?capacities[id>>1]:0;}
 void record(unsigned tick,const char* what,unsigned pl,bool mode,const std::uint8_t* ram,const char* detail){
  if(!log)return;
  if(!ram){std::fprintf(log,"%u\t%s\t-\t%u\t%u\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t%s\n",tick,what,mode?1:0,installed?1:0,detail);std::fflush(log);return;}
  const unsigned v=pl?0x4400u:0x4000u,a=pl?0x4000u:0x4400u;
  const unsigned id=ram[v+0x52u];
  std::fprintf(log,"%u\t%s\tP%u\t%u\t%u\t%u\t%u\t%u\t%02X\t%04X\t%04X\t%u\t%u\t%02X\t%04X\t%04X\t%02X\t%u\t%04X\t%04X\t%02X\t%02X\t%u\t%u\t%s\n",
   tick,what,pl+1u,mode?1u:0u,installed?1u:0u,id,cap(id),ram[v+0xb8u],ram[v+0x134u],
   u16(ram,v+0x60u),u16(ram,v+0x06u),u16(ram,v+0x0cu),u16(ram,v+0x10u),u8(ram,v+0x83u),u16(ram,v+0xf6u),
   u16(ram,v+0x74u),u8(ram,v+0x87u),ram[a+0x52u],u16(ram,a+0x60u),u16(ram,a+0x70u),u8(ram,a+0x82u),u8(ram,a+0x84u),u16(ram,a+0x0cu),u16(ram,a+0x10u),detail);
  std::fflush(log);
 }
 static void sig(char* output,unsigned max,const std::uint8_t* code,unsigned n,unsigned at,unsigned count){
  if(!max)return;
  unsigned used=0;output[0]=0;
  for(unsigned i=0;i<count&&code&&at+i<n&&used+4<max;++i){int len=std::snprintf(output+used,max-used,"%02X",code[at+i]);if(len<=0)break;used+=(unsigned)len;}
 }
 bool before(std::uint8_t* code,unsigned n,const char* driver,unsigned frame,bool tournament,bool test){
  init();bool patched=false;Layout found=identify(code,n,patched);
  const bool correctDriver=driver&&!std::strcmp(driver,"xmvsfu");
  if(found==Layout::NONE){if(!refused){
   char s1[90],s2[96],detail[300];sig(s1,sizeof(s1),code,n,INIT,INITLEN);sig(s2,sizeof(s2),code,n,AIR,AIRLEN);
   std::snprintf(detail,sizeof(detail),"REFUSED: INIT@15118=%s AIR@E784=%s; no writes",s1,s2);
   record(frame,"SIGNATURE_REFUSED",0,tournament,nullptr,detail);std::fprintf(stderr,"XM87.95 %s\n",detail);refused=true;
  }installed=false;layout=Layout::NONE;return false;}
  refused=false;layout=found;installed=patched;
  // Restore if no longer in XMVSFU; never leave an in-memory patch on a mode/driver change.
  const bool want=correctDriver&&tournament&&test;
  if(installed==want)return correctDriver;
  const bool swap=found==Layout::SWAPPED;
  std::uint8_t saveInit[INITLEN],saveAir[AIRLEN];
  std::memcpy(saveInit,code+INIT,INITLEN);std::memcpy(saveAir,code+AIR,AIRLEN);
  writeSequence(code+INIT,want?INIT_TEST:INIT_ORIG,INITLEN,swap);
  writeSequence(code+AIR,want?AIR_TEST:AIR_ORIG,AIRLEN,swap);
  if(!isPair(code,n,found,want)){
   std::memcpy(code+INIT,saveInit,INITLEN);std::memcpy(code+AIR,saveAir,AIRLEN);
   record(frame,"PATCH_VERIFICATION_FAILED",0,tournament,nullptr,"Original two signatures restored from pre-write snapshot");return false;
  }
  installed=want;++changes;
  char detail[150];std::snprintf(detail,sizeof(detail),"fetch_order=%s; INIT 015118/01511E; native tick E784/E78A; original branch/jump table untouched",layoutName(found));
  record(frame,want?"TOURNAMENT_INIT_AND_TICK_INSTALLED":"CLASSIC_OR_DRIVER_OR_TEST_RESTORED",0,tournament,nullptr,detail);
  return correctDriver;
 }
 void after(const std::uint8_t* ram,unsigned tick,bool tournament){
  init();if(!ram)return;
  const bool combat=u8(ram,0x5000u)==8u||u8(ram,0x5001u)==8u;
  for(unsigned pl=0;pl<2;++pl){
   const unsigned b=pl?0x4400u:0x4000u,a=pl?0x4000u:0x4400u,act=u16(ram,b+0x60u);
   const bool air=combat&&(act==0x1c38u||act==0x1c3au);
   const unsigned gate=u8(ram,b+0x83u),timer=u16(ram,b+0xf6u),atk=u16(ram,a+0x70u),contact=u8(ram,b+0x87u);
   if(air){
    ++airFrames[pl];
    if(!prevAir[pl]||gate!=prevGate[pl]||timer!=prevTimer[pl]||atk||contact||tick%15u==0u)
      record(tick,!prevAir[pl]?"NATIVE_DIZZY_POP_ENTER":contact?"CONTACT_REGISTERED":atk?"ATTACK_BOX_ACTIVE":gate!=prevGate[pl]||timer!=prevTimer[pl]?"DEFENSE_STATE_CHANGED":"AIRFRAME_SAMPLE",pl,tournament,ram,"read-only post-CPU; contact87 transient and zero does not prove no damage");
   }else if(prevAir[pl])record(tick,"NATIVE_DIZZY_POP_EXIT",pl,tournament,ram,"native action progressed without rollback");
   prevAir[pl]=air;prevGate[pl]=gate;prevTimer[pl]=timer;
  }
 }
};
static inline Controller& state(){static Controller c;return c;}
} // namespace XM8795Native
#endif
