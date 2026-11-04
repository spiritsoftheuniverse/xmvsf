#ifndef XMVSF_NATIVE_HOST_H
#define XMVSF_NATIVE_HOST_H
// Stage 21: first explicit boundary between XMVSF frontend and legacy runtime.
// These are forwarding adapters, deliberately preserving Stage20's working behavior.
// No changes to emulated inputs, clocks, ROM, save states or game code here.
#include "xmvsf_stage20_api.h"
namespace xmvsfHost {
inline void menuStart() { xm20MenuStart(); }
inline void cursorMove() { xm20MenuMovement(); }
inline void confirm() { xm20MenuConfirm(); }
inline void enterGame() { xm20MenuEnterGame(); }
inline void enterVs() { xm20OnCombat(); }
inline void resetClock() { xm20ClockReset(); }
inline int frameDue(int fpsHundredths) { return xm20FrameDue(fpsHundredths); }
}
#endif
