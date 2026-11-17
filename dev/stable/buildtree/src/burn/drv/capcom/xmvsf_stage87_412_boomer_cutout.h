#ifndef XMVSF_STAGE87_412_BOOMER_CUTOUT_H
#define XMVSF_STAGE87_412_BOOMER_CUTOUT_H
// 87.412: Keep original native HP/timer while hiding native actor/FX OBJ.
// The SDL stun meter is independent. No native ROM or framebuffer mutation.
#include "xmvsf_stage87_392_hud_shock.h"
namespace XM87412 {
// One key-9 policy covers all seven host-custom stages (not native ROM stages).
inline bool customStage(unsigned stage){return stage>=1u && stage<=7u;}
inline bool hudOnlyCapture(unsigned stage,bool actorsVisible){
 return customStage(stage) && !actorsVisible;
}
struct Controls {
 bool actors=true,stun=true;
 void reset(){actors=true;stun=true;}
 bool toggle(unsigned key){
  if(key==9u){actors=!actors;return true;}
  if(key==8u){stun=!stun;return true;}
  return false;
 }
};
inline bool needsMatchReset(unsigned seenGeneration,unsigned generation,
                            unsigned seenActivation,unsigned activation,
                            unsigned long long lastFrame,
                            unsigned long long frame){
 return seenGeneration!=generation || seenActivation!=activation ||
        (lastFrame!=~0ull && frame<lastFrame);
}
inline bool nativeHud(unsigned priority,unsigned palette,unsigned tile,
                      int x,int y,unsigned width,unsigned height){
 // 9 is a SOURCE cutout, not the broad 06/08 shock classifier.
 // Preserve only verified HUD objects so native forest cannot reappear.
 return XM87392::topHudObject(priority,palette,tile,x,y,width,height,
                             false,false,true);
}
}
#endif
