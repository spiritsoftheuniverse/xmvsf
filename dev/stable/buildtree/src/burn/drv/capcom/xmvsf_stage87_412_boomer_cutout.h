#ifndef XMVSF_STAGE87_412_BOOMER_CUTOUT_H
#define XMVSF_STAGE87_412_BOOMER_CUTOUT_H
// 87.412: Keep original native HP/timer while hiding native actor/FX OBJ.
// The SDL stun meter is independent. No native ROM or framebuffer mutation.
#include "xmvsf_stage87_392_hud_shock.h"
namespace XM87412 {
struct Controls {
 bool actors=true,stun=true;
 void reset(){actors=true;stun=true;}
 bool toggle(unsigned key){
  if(key==9u){actors=!actors;return true;}
  if(key==8u){stun=!stun;return true;}
  return false;
 }
};
inline bool nativeHud(unsigned priority,unsigned palette,unsigned tile,
                      int x,int y,unsigned width,unsigned height){
 return XM87392::object(priority,palette,tile,x,y,width,height,false,false);
}
}
#endif
