#ifndef XMVSF_STAGE86_49_STATE_H
#define XMVSF_STAGE86_49_STATE_H
// Stage86.49: read-only native game observations and an opt-in video pass.
// No modifications to emulated stage selection, camera, CPU, or sound.
struct XM8649VisualState {
 bool sceneActive=false;
 unsigned stage=0,status=0,pointer=0;
 int nativeCameraX=0,nativeCameraY=256;
 // Unlike frames, this increments at each inactive->active custom stage.
 // A second match may have a larger frame number with the SAME generation.
 unsigned activationSerial=0;
 unsigned long long frames=0;
};
extern XM8649VisualState xm8649Visual;
#endif
