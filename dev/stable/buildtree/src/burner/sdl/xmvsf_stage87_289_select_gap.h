#ifndef XMVSF_STAGE87_289_SELECT_GAP_H
#define XMVSF_STAGE87_289_SELECT_GAP_H
// 87.289: remove the native first-pick confirmation pause without changing
// either team's accepted character count or P1/P2 input ownership.
//
// USA 961023 choosing records: P1 FF5180, P2 FF51A0 (0x20 bytes each).
// +00 is the ROM selection step, +03 is the accepted character count.
// Native step 2 takes the first character; step 3 is its transition;
// step 4 resumes choosing. Advance only 3 -> 4 with exactly one pick.
// No other step or timer changes. Never run during combat/runahead.
#include "burnint.h"
#include "xmvsf_stage87_55_rule_mode.h" // V3 stays native; new selection behavior is V4 only.
#include <cstdio>
#include <cstring>

namespace XM87289Select {
struct Session {
 bool skipped[2]={false,false};
};
static inline Session& state(){static Session s;return s;}
static inline void onNativeFrame(bool gameReady){
 if(!gameReady||bBurnRunAheadFrame||XM8755Mode::version()!=4)return;
 const char* driver=BurnDrvGetTextA(DRV_NAME);
 if(!driver||std::strncmp(driver,"xmvsf",5)!=0)return;
 cpu_core_config* cpu=GetCpuCoreConfig(0);
 if(!cpu||!cpu->open||!cpu->close||!cpu->read||!cpu->write)return;
 cpu->open(0);
 const unsigned phase=cpu->read(0xFF5000u)&255u;
 const unsigned gameStep=((cpu->read(0xFF5104u)&255u)<<8) |
                          (cpu->read(0xFF5105u)&255u);
 if(phase!=0u||gameStep!=4u){
  state().skipped[0]=state().skipped[1]=false;
  cpu->close();return;
 }
 for(unsigned side=0u;side<2u;++side){
  const unsigned base=0xFF5180u+side*0x20u;
  const unsigned step=cpu->read(base)&255u;
  const unsigned picks=cpu->read(base+3u)&255u;
  if(picks!=1u){
   state().skipped[side]=false;
   continue;
  }
  // Work only after the ROM has committed the first character and
  // entered the intermediate step. Both P1 and P2 have the same layout.
  if(!state().skipped[side] && step==3u){
   cpu->write(base,4u);
   state().skipped[side]=true;
   std::fprintf(stderr,
      "XM87.289 SELECT_NO_FIRST_TO_SECOND_GAP side=%u picks=1 step=03->04\n",
      side+1u);
  }
 }
 cpu->close();
}
}
#endif
