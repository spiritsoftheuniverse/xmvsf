#ifndef XMVSF_STAGE87_63_SUPER_INTRO_H
#define XMVSF_STAGE87_63_SUPER_INTRO_H
// Read-only, experimental recognition of a CPS2 super's cinematic-intro EXIT.
// The original unfiltered native object list undergoes a pronounced ~43/44
// frame suppression, then returns sharply when the super portrait/darkening
// screen clears.  This is corroborating renderer evidence, NOT a verified
// universal ROM move ID. Never write game RAM and never use a fixed duration
// to terminate a super: the duration range is only a *qualification* for
// the intro observation, while the owner action family supplies the exit.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
namespace XM8763 {
struct Probe {
 bool initialized=false, armed=false, dark=false, active=false;
 bool ownerContact=false, transitionalSix=false;
 int owner=-1;
 unsigned armedAt=0, darknessAt=0, baseline=0, prevCount=0,lastFrame=~0u;
 unsigned prevAction[2]={0,0};
 std::array<unsigned,12> history{{}};
 unsigned histSize=0,histNext=0;
 static unsigned R(const unsigned char* r,unsigned at){return r[at^1u];}
 void Reset(){*this=Probe();}
 unsigned Median() const {
   if(!histSize)return 0;
   std::array<unsigned,12> x=history;
   std::sort(x.begin(),x.begin()+histSize);
   return x[histSize/2];
 }
 void Save(unsigned n){history[histNext]=n;histNext=(histNext+1u)%history.size();if(histSize<history.size())++histSize;}
 bool Observe(const unsigned char* ram,int phase,unsigned frame,unsigned n){
   if(!ram || phase!=8 || n>1024u){
     if(active)std::fprintf(stderr,"XM87.63 DF_SUPER_INTRO_END frame=%u reason=phase\n",frame);
     Reset();return false;
   }
   if(initialized && frame==lastFrame)return active;
   if(initialized && (frame<lastFrame || frame-lastFrame>5u))Reset();
   initialized=true;lastFrame=frame;
   unsigned action[2]={R(ram,0x40A0u),R(ram,0x44A0u)};
   bool valid[2]={ram[0x4000u]!=0,ram[0x4400u]!=0};
   for(int i=0;i<2;++i){
     // An owner is assigned at the action-edge, but 0x2A alone must never
     // enable the light: only the cinematic intro's restoration can.
     if(valid[i] && action[i]==0x2Au && prevAction[i]!=0x2Au && !active){
       owner=i;armed=true;dark=false;armedAt=frame;baseline=Median();
     }
     prevAction[i]=action[i];
   }
   if(armed && !active){
     if(frame-armedAt>110u){armed=false;dark=false;}
     else if(!dark && baseline>=165u && n*100u<=baseline*50u){
       dark=true;darknessAt=frame;
       std::fprintf(stderr,"XM87.63 DF_SUPER_INTRO_DARK frame=%u owner=P%d count=%u baseline=%u\n",
         frame,owner+1,n,baseline);
     }else if(dark){
       unsigned elapsed=frame-darknessAt;
       if(elapsed>54u){armed=false;dark=false;}
       else if(elapsed>=36u && n*100u>=baseline*75u &&
               n>=prevCount+55u && owner>=0 && valid[owner]){
         active=true;armed=false;dark=false;ownerContact=false;
         transitionalSix=(action[owner]==0x06u);
         std::fprintf(stderr,"XM87.63 DF_SUPER_INTRO_EXIT frame=%u owner=P%d count=%u previous=%u after=%u brightness=90 hue_saturation=100\n",
           frame,owner+1,n,prevCount,elapsed);
       }
     }
   }
   if(active){
     const int j=owner^1;
     if(action[owner]==0x32u && valid[j] && action[j]==0x34u)
       ownerContact=true;
     const bool hold=(action[owner]==0x2Au) ||
       (action[owner]==0x32u && ownerContact) ||
       (transitionalSix && action[owner]==0x06u);
     if(!valid[owner] || !hold){
       std::fprintf(stderr,"XM87.63 DF_SUPER_INTRO_END frame=%u owner=P%d action=%02X\n",
         frame,owner+1,action[owner]);
       active=false;owner=-1;ownerContact=false;transitionalSix=false;
     }
   }
   // No baseline poisoning during the cinematic suppression.  In ordinary
   // play the rolling median follows slow changes in sprite complexity.
   if(!armed && !active)Save(n);
   prevCount=n;
   return active;
 }
};
static inline Probe& State(){static Probe p;return p;}
} // namespace XM8763
#endif
