#ifndef XMVSF_STAGE87_96_TOURNAMENT_DIZZY_HANDOFF_H
#define XMVSF_STAGE87_96_TOURNAMENT_DIZZY_HANDOFF_H
// Stage 87.96: keep Stage 87.95's confirmed dizzy-pop aerial hit acceptance.
// Suppress the subsequent dizzy STAND by diverting the 68000 initializer
// at PC 012B88 to the existing native ground-recovery selector 011FC0.
// 012B88 original: ST (135,A6); MOVE.B #80,(83,A6) ...
// Tournament: CLR.B (135,A6); JMP $011FC0.  Entire 10-byte slot replaced.
// 011FC0 is also used by the unmodified 1C36 timeout handler at E902.
// During the dizzy pop action 1C38/1C3A/1C36, +135 remains native FF.
// If a COMBO interrupts the pop, clear only +135 on departure from the pop;
// NEVER alter 1C38/1C3A/1C36 actions, HP, velocity, stun meter, or ROM ZIP.
// +135 is the persistent native dizzy flag, CPU-byte address (A6+135).
// In FBNeo's CpsRamFF, its raw index is base+134 (WORD SWAP).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
namespace XM8796Native {
static constexpr unsigned HANDOFF=0x12b88u,HANDOFF_LEN=10u;
static constexpr unsigned POP_FLAG=0x150ceu,POP_FLAG_LEN=4u;
static constexpr std::uint8_t HANDOFF_ORIG[HANDOFF_LEN]={0x50,0xee,0x01,0x35,0x1d,0x7c,0x00,0x80,0x00,0x83};
static constexpr std::uint8_t HANDOFF_TEST[HANDOFF_LEN]={0x42,0x2e,0x01,0x35,0x4e,0xf9,0x00,0x01,0x1f,0xc0};
static constexpr std::uint8_t POP_FLAG_ORIG[POP_FLAG_LEN]={0x50,0xee,0x01,0x35};
enum class Layout { UNKNOWN, BIG, WORD_SWAP };
static inline const char* name(Layout l){return l==Layout::BIG?"BE":l==Layout::WORD_SWAP?"WORD_SWAP":"UNKNOWN";}
static inline unsigned u8(const std::uint8_t* r,unsigned address){return r[(address&65535u)^1u];}
static inline unsigned u16(const std::uint8_t* r,unsigned address){return (u8(r,address)<<8)|u8(r,address+1u);}
static inline bool env(const char* k){const char* s=std::getenv(k);return s&&std::strcmp(s,"1")==0;}
static inline bool matches(const std::uint8_t* c,unsigned n,const std::uint8_t* target,unsigned len,bool swap){
 if(!c||n<len)return false;
 for(unsigned i=0;i<len;i+=2){if(c[i]!=(swap?target[i+1]:target[i])||c[i+1]!=(swap?target[i]:target[i+1]))return false;}
 return true;
}
static inline void write(std::uint8_t* c,const std::uint8_t* src,unsigned len,bool swap){
 for(unsigned i=0;i<len;i+=2){c[i]=src[i+(swap?1u:0u)];c[i+1]=src[i+(swap?0u:1u)];}
}
static inline Layout identify(const std::uint8_t* code,unsigned n,bool& patched){
 patched=false;if(!code||n<HANDOFF+HANDOFF_LEN||n<POP_FLAG+POP_FLAG_LEN)return Layout::UNKNOWN;
 for(Layout l:{Layout::BIG,Layout::WORD_SWAP}){
  bool swap=l==Layout::WORD_SWAP;
  if(!matches(code+POP_FLAG,n-POP_FLAG,POP_FLAG_ORIG,POP_FLAG_LEN,swap))continue;
  if(matches(code+HANDOFF,n-HANDOFF,HANDOFF_ORIG,HANDOFF_LEN,swap))return l;
  if(matches(code+HANDOFF,n-HANDOFF,HANDOFF_TEST,HANDOFF_LEN,swap)){patched=true;return l;}
 }
 return Layout::UNKNOWN;
}
struct Controller {
 bool installed=false,refused=false,initialized=false;
 Layout layout=Layout::UNKNOWN;
 std::FILE* log=nullptr;
 unsigned changes=0,clears=0;
 bool prevPop[2]={false,false},prevDizzyStand[2]={false,false};
 unsigned prevAction[2]={0,0};
 ~Controller(){if(log)std::fclose(log);}
 void init(){
  if(initialized)return;
  initialized=true;
  const char* f=std::getenv("XM8796_DIZZY_HANDOFF_LOG_FILE");
  if(f&&*f){log=std::fopen(f,"wb");if(log)std::fputs("frame\tevent\tplayer\tmode\tpatched\tprevious_action\taction\tsubstate\tdizzy135\tother134\tstun_remaining\tstun_capacity\tf6\tgate83\ty\tlife210\tdetail\n",log);}
  std::fputs("XM87.96 DIZZY_HANDOFF_READY; native 12B88 standing transition; 87.95 pop preserved\n",stderr);
 }
 static unsigned cap(unsigned id){static const unsigned char c[18]={50,60,60,60,60,60,50,60,60,60,60,60,60,60,60,50,60,60};return id<=34&&!(id&1)?c[id>>1]:0;}
 void record(unsigned tick,const char* event,int pl,bool tournament,const std::uint8_t* ram,const char* info){
  if(!log)return;
  if(!ram||pl<0){std::fprintf(log,"%u\t%s\t-\t%u\t%u\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t-\t%s\n",tick,event,tournament?1:0,installed?1:0,info);std::fflush(log);return;}
  unsigned b=pl?0x4400u:0x4000u,id=ram[b+0x52u];
  std::fprintf(log,"%u\t%s\tP%d\t%u\t%u\t%04X\t%04X\t%04X\t%02X\t%02X\t%u\t%u\t%04X\t%02X\t%u\t%u\t%s\n",
   tick,event,pl+1,tournament?1u:0u,installed?1u:0u,prevAction[pl],u16(ram,b+0x60u),u16(ram,b+6u),u8(ram,b+0x135u),u8(ram,b+0x134u),u8(ram,b+0xb8u),cap(id),u16(ram,b+0xf6u),u8(ram,b+0x83u),u16(ram,b+0x10u),u16(ram,b+0x210u),info);
  std::fflush(log);
 }
 static void signature(char* buf,unsigned max,const std::uint8_t* c,unsigned n,unsigned at,unsigned len){
  if(!max)return;
  buf[0]=0;
  if(!c||at+len>n)return;
  unsigned used=0;for(unsigned i=0;i<len&&used+3<max;++i){int got=std::snprintf(buf+used,max-used,"%02X",c[at+i]);if(got<=0)break;used+=(unsigned)got;}
 }
 bool before(std::uint8_t* code,unsigned n,const char* driver,unsigned frame,bool tournament,bool enabled,bool native95){
  init();bool already=false;Layout l=identify(code,n,already);
  const bool game=driver&&!std::strcmp(driver,"xmvsfu");
  if(l==Layout::UNKNOWN){
   if(!refused){char s[48],flag[24],detail[200];signature(s,sizeof(s),code,n,HANDOFF,HANDOFF_LEN);signature(flag,sizeof(flag),code,n,POP_FLAG,POP_FLAG_LEN);
    std::snprintf(detail,sizeof(detail),"unknown native handoff signature @12B88=%s @150CE=%s; no writes",s,flag);
    record(frame,"SIGNATURE_REFUSED",-1,tournament,nullptr,detail);std::fprintf(stderr,"XM87.96 %s\n",detail);refused=true;}
   layout=Layout::UNKNOWN;installed=false;return false;
  }
  refused=false;layout=l;installed=already;
  const bool want=game&&tournament&&enabled&&native95;
  if(already==want)return game;
  const bool swap=l==Layout::WORD_SWAP;
  std::uint8_t saved[HANDOFF_LEN];std::memcpy(saved,code+HANDOFF,HANDOFF_LEN);
  write(code+HANDOFF,want?HANDOFF_TEST:HANDOFF_ORIG,HANDOFF_LEN,swap);
  bool tested=false;
  if(identify(code,n,tested)!=l||tested!=want){std::memcpy(code+HANDOFF,saved,HANDOFF_LEN);record(frame,"PATCH_VERIFICATION_FAILED",-1,tournament,nullptr,"restored prewrite native instruction bytes");return false;}
  installed=want;++changes;
  char detail[192];std::snprintf(detail,sizeof(detail),"%s: @12B88 CLR.B (135,A6); JMP $11FC0 native recovery; original @150CE pop flag ST unchanged",name(l));
  record(frame,want?"TOURNAMENT_NATIVE_HANDOFF_INSTALLED":"CLASSIC_OR_DRIVER_OR_TEST_RESTORED",-1,tournament,nullptr,detail);
  return game;
 }
 static bool isPop(unsigned action){return action==0x1c38u||action==0x1c3au||action==0x1c36u;}
 // Only clears the actual native flag when the ROM has ALREADY selected a
 // different non-dizzy reaction (usually an accepted airborne combo hit).
 // A successful native pop-to-stand bypass at 12B88 clears it on the CPU itself.
 void after(std::uint8_t* ram,unsigned tick,bool tournament,bool enabled){
  init();if(!ram)return;
  const bool combat=u8(ram,0x5000u)==8u||u8(ram,0x5001u)==8u;
  for(int pl=0;pl<2;++pl){
   unsigned b=pl?0x4400u:0x4000u,act=u16(ram,b+0x60u);
   bool pop=combat&&isPop(act),dizzyStand=combat&&act==0x1e00u;
   if(pop&&!prevPop[pl])record(tick,"DIZZY_POP_INTACT",pl,tournament,ram,"No 68000 pop action/velocity rewrite; 87.95 retains airborne collision");
   if(tournament&&enabled&&installed&&prevPop[pl]&&!pop){
    if(dizzyStand){record(tick,"UNEXPECTED_DIZZY_STAND",pl,tournament,ram,"12B88 bypass was not taken; preserve native state and capture trace");}
    else if(u8(ram,b+0x135u)!=0u){
     record(tick,"NATIVE_POP_INTERRUPTED_BY_NEW_ACTION",pl,tournament,ram,"clearing only CPU dizzy marker +135 after ROM changed action");
     ram[(b+0x135u)^1u]=0;++clears;
     record(tick,"DIZZY_MARKER_CLEARED_AFTER_POP_EXIT",pl,tournament,ram,"no changes to action, life, stun, velocity, hurtbox or hitboxes");
    } else record(tick,"POP_EXIT_FLAG_ALREADY_CLEARED",pl,tournament,ram,"native handler or normal recovery cleared +135");
   }
   if(dizzyStand&&!prevDizzyStand[pl])record(tick,"DIZZY_STAND_ENTERED",pl,tournament,ram,"if Tournament and installed, another path is committing 1E00");
   if(!tournament&&prevPop[pl]&&!pop)record(tick,"CLASSIC_POP_EXIT_UNCHANGED",pl,tournament,ram,"no Tournament RAM intervention");
   prevPop[pl]=pop;prevDizzyStand[pl]=dizzyStand;prevAction[pl]=act;
  }
 }
};
static inline Controller& state(){static Controller instance;return instance;}
} // namespace XM8796Native
#endif
