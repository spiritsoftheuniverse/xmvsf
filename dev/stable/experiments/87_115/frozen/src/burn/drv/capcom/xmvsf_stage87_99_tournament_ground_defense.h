#ifndef XMVSF_STAGE87_99_TOURNAMENT_GROUND_DEFENSE_H
#define XMVSF_STAGE87_99_TOURNAMENT_GROUND_DEFENSE_H
// XM87.99 — native CPS-2 dizzy-pop ground handoff, Tournament only.
// Source verified against XMVSFU decrypted 68000 ROM; exact bytes guarded.
// Does not touch the on-disk ROM or rewrite fighter states from the host.
// E800: JSR 1038E -> JMP 109C0 (same six bytes, only on dizzy pop ground contact).
// 109C0: verified all-zero 48-byte executable-fetch gap after RTS 109BE.
// The injected code reproduces all original E800..E818 work in order,
// calls stock 16E76 (closes combo, score + stats, clears +135),
// clears generic hit-reaction latch +134, then tail-jumps to
// stock 11FC0 ground action/defense selector.
// This does not alter 87.95 airborne collision patches or 87.96 fallback.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
namespace XM8799Ground {
static constexpr unsigned HOOK=0xE800u,HOOK_N=6u,CAVE=0x109C0u,CAVE_N=48u;
static constexpr std::uint8_t HOOK_ORIG[HOOK_N]={0x4e,0xb9,0,1,3,0x8e};
static constexpr std::uint8_t HOOK_TEST[HOOK_N]={0x4e,0xf9,0,1,9,0xc0};
static constexpr std::uint8_t CODE[CAVE_N]={
  0x4e,0xb9,0x00,0x01,0x03,0x8e, // JSR 1038E original action selection
  0x4e,0xb9,0x00,0x07,0x8d,0x10, // JSR 78D10 original ground helper
  0x4e,0xb9,0x00,0x06,0x21,0xec, // JSR 621EC original ground helper
  0x4e,0xb9,0x00,0x01,0x00,0xf0, // JSR 100F0 original ground helper
  0x54,0x6e,0x00,0x08,           // ADDQ.W #2,(8,A6) original substate advance
  0x4e,0xb9,0x00,0x01,0x6e,0x76, // JSR 16E76 original combo finalizer
  0x42,0x2e,0x01,0x34,           // CLR.B (134,A6), end legacy hit-reaction latch
  0x4e,0xf9,0x00,0x01,0x1f,0xc0, // JMP 11FC0 original ground/guard selector
  0x00,0x00,0x00,0x00            // leave unused final four-byte gap unchanged
};
static_assert(sizeof(CODE)==CAVE_N,"Native stub length changed");
enum class Layout{UNKNOWN,BE,WORD_SWAP};
inline const char* layoutName(Layout l){return l==Layout::BE?"BE":l==Layout::WORD_SWAP?"WORD_SWAP":"UNKNOWN";}
inline bool env(const char* key){const char* p=std::getenv(key);return p&&std::strcmp(p,"1")==0;}
inline unsigned u8(const std::uint8_t* r,unsigned cpu){return r[(cpu&65535u)^1u];}
inline unsigned u16(const std::uint8_t* r,unsigned cpu){return (u8(r,cpu)<<8)|u8(r,cpu+1u);}
inline bool match(const std::uint8_t* p,const std::uint8_t* expect,unsigned n,bool swap){
 for(unsigned i=0;i<n;i+=2){if(p[i]!=(swap?expect[i+1]:expect[i])||p[i+1]!=(swap?expect[i]:expect[i+1]))return false;}
 return true;
}
inline bool zero(const std::uint8_t* p,unsigned n){for(unsigned i=0;i<n;++i)if(p[i])return false;return true;}
inline void store(std::uint8_t* p,const std::uint8_t* src,unsigned n,bool swap){
 for(unsigned i=0;i<n;i+=2){p[i]=src[i+(swap?1u:0u)];p[i+1]=src[i+(swap?0u:1u)];}
}
inline Layout identify(const std::uint8_t* code,unsigned n,bool& patched){
 patched=false;if(!code||n<CAVE+CAVE_N||n<HOOK+HOOK_N)return Layout::UNKNOWN;
 for(Layout l:{Layout::BE,Layout::WORD_SWAP}){
  const bool s=l==Layout::WORD_SWAP;
  if(match(code+HOOK,HOOK_ORIG,HOOK_N,s)&&zero(code+CAVE,CAVE_N))return l;
  if(match(code+HOOK,HOOK_TEST,HOOK_N,s)&&match(code+CAVE,CODE,CAVE_N,s)){
   patched=true;return l;
  }
 }
 return Layout::UNKNOWN;
}
struct Controller {
 bool installed=false,refused=false,initialized=false;
 Layout layout=Layout::UNKNOWN;
 std::FILE* log=nullptr;
 unsigned events=0;
 unsigned prevAction[2]={0,0},prevCombo[2]={0,0},prevFloor[2]={0,0};
 ~Controller(){if(log)std::fclose(log);}
 void init(){if(initialized)return;initialized=true;
  const char* path=std::getenv("XM8799_GROUND_DEFENSE_LOG_FILE");
  if(path&&*path){log=std::fopen(path,"wb");if(log)std::fputs("frame\tevent\tplayer\tmode\tpatched\taction\tprevious_action\tground_y\tposition_y\tstun_b8\tdizzy_135\thit_latch_134\tdef_gate_83\tattacker_combo_110\tattacker_last_combo_111\tdefender_c0\tdefender_c4\tdetail\n",log);}
  std::fputs("XM87.99 GROUND_DEFENSE_READY: native 16E76 combo close + 11FC0 guard selector\n",stderr);
 }
 void record(unsigned frame,const char* kind,int victim,bool tour,const std::uint8_t* r,const char* detail){
  if(!log)return;
  if(!r||victim<0){std::fprintf(log,"%u\t%s\t-\t%u\t%u\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t%s\n",frame,kind,tour?1:0,installed?1:0,detail);std::fflush(log);return;}
  const unsigned b=victim?0x4400u:0x4000u,a=victim?0x4000u:0x4400u;
  const unsigned act=u16(r,b+0x60u);
  std::fprintf(log,"%u\t%s\tP%d\t%u\t%u\t%04X\t%04X\t%u\t%u\t%u\t%02X\t%02X\t%02X\t%u\t%u\t%04X\t%04X\t%s\n",frame,kind,victim+1,tour?1:0,installed?1:0,act,prevAction[victim],u16(r,b+0x254u),u16(r,b+0x10u),u8(r,b+0xb8u),u8(r,b+0x135u),u8(r,b+0x134u),u8(r,b+0x83u),u8(r,a+0x110u),u8(r,a+0x111u),u16(r,b+0xc0u),u16(r,b+0xc4u),detail);
  std::fflush(log);
 }
 bool before(std::uint8_t* code,unsigned length,const char* driver,unsigned frame,bool tournament,bool enabled,bool native95,bool native96){
  init();bool patched=false;Layout candidate=identify(code,length,patched);
  if(candidate==Layout::UNKNOWN){
   if(!refused){char description[260];std::snprintf(description,sizeof(description),"guarded native E800/109C0 signatures mismatched: E800=%02X%02X CAVE=%02X%02X (no write)",code&&length>HOOK+1?code[HOOK]:0u,code&&length>HOOK+1?code[HOOK+1]:0u,code&&length>CAVE+1?code[CAVE]:0u,code&&length>CAVE+1?code[CAVE+1]:0u);record(frame,"SIGNATURE_REFUSED",-1,tournament,nullptr,description);std::fprintf(stderr,"XM87.99 %s\n",description);refused=true;}
   installed=false;layout=Layout::UNKNOWN;return false;
  }
  refused=false;layout=candidate;installed=patched;
  const bool supported=driver&&std::strcmp(driver,"xmvsfu")==0;
  const bool want=supported&&tournament&&enabled&&native95&&native96;
  if(patched==want)return supported;
  const bool swap=candidate==Layout::WORD_SWAP;
  std::uint8_t oldHook[HOOK_N],oldCave[CAVE_N];
  std::memcpy(oldHook,code+HOOK,HOOK_N);std::memcpy(oldCave,code+CAVE,CAVE_N);
  if(want){store(code+CAVE,CODE,CAVE_N,swap);store(code+HOOK,HOOK_TEST,HOOK_N,swap);}
  else{store(code+HOOK,HOOK_ORIG,HOOK_N,swap);std::memset(code+CAVE,0,CAVE_N);}
  bool verified=false;
  if(identify(code,length,verified)!=candidate||verified!=want){
   std::memcpy(code+HOOK,oldHook,HOOK_N);std::memcpy(code+CAVE,oldCave,CAVE_N);
   record(frame,"VERIFY_REFUSED",-1,tournament,nullptr,"write rollback, original runtime bytes restored");return false;
  }
  installed=want;++events;
  record(frame,want?"TOURNAMENT_GROUNDED_NATIVE_HOOK_INSTALLED":"CLASSIC_OR_TEST_NATIVE_HOOK_RESTORED",-1,tournament,nullptr,
    "E800 -> 109C0; original 4 landing helpers; 16E76 combo finalize; clear 134; 11FC0 native guard selector");
  return supported;
 }
 void after(const std::uint8_t* ram,unsigned frame,bool tournament){
  init();if(!ram)return;
  for(int p=0;p<2;++p){
   unsigned b=p?0x4400u:0x4000u,a=p?0x4000u:0x4400u;
   unsigned action=u16(ram,b+0x60u),active=u8(ram,a+0x110u),floor=u16(ram,b+0x254u),y=u16(ram,b+0x10u);
   if(tournament&&installed){
    if((prevAction[p]==0x1c38u||prevAction[p]==0x1c3au)&&y>=floor&&floor>0&&floor<1024u)
      record(frame,"POP_GROUND_CONTACT_OBSERVED",p,tournament,ram,"check native 16E76 combo finalization and 11FC0 action/guard next frame");
    if((prevAction[p]==0x1c38u||prevAction[p]==0x1c3au||prevAction[p]==0x1c36u)&&action!=prevAction[p]&&
       action!=0x1c38u&&action!=0x1c3au&&action!=0x1c36u)
      record(frame,"POP_EXIT_TO_NATIVE_ACTION",p,tournament,ram,"read-only: action/guard/combo after native ROM update");
    if(active!=prevCombo[p]&&((action==0||action==0x1c36u)||(prevAction[p]==0x1c36u)))
      record(frame,"ATTACKER_COMBO_COUNT_CHANGED",p,tournament,ram,"ROM-native count, no RAM count overrides");
   }
   prevAction[p]=action;prevCombo[p]=active;prevFloor[p]=floor;
  }
 }
};
inline Controller& state(){static Controller value;return value;}
} // namespace XM8799Ground
#endif
