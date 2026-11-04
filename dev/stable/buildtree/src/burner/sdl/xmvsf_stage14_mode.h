// Training mode selection and input routing, shared by SDL2 frontend modules.
#ifndef XMVSF_STAGE14_MODE_H
#define XMVSF_STAGE14_MODE_H
extern int xm14TrainingMode; // 0=original Arcade, 1=Training
extern int xm14InputTarget;  // 0=normal ownership, 1=P1 controls P2 select
static inline int xm14ReadControllerFor(int slot) {
    if (xm14TrainingMode && xm14InputTarget) {
        if (slot == 0) return -1; // no simultaneous duplicate P1 inputs
        if (slot == 1) return 0;  // P1's SDL device feeds P2 game slot
    }
    return slot;
}
#endif
