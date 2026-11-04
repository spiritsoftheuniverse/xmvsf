// Stage16: Remove Stage15's speculative coin/start pulses and attack-edge handoff.
// Training begins from an explicit user-recorded full CPS-2 save state.
#ifndef XMVSF_STAGE15_MACHINE_H
#define XMVSF_STAGE15_MACHINE_H
#include "xmvsf_stage14_mode.h"
struct XM15Machine { unsigned int reserved; };
extern XM15Machine xm15;
static inline void xm15Reset() { xm14InputTarget=0; }
static inline int xm15SyntheticButton(int,int) { return 0; }
static inline void xm15ObserveInput(int,int,int) {}
static inline void xm15OnRealFrame(int) {}
#endif
