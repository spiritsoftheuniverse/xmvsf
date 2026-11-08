#ifndef XMVSF_STAGE20_API_H
#define XMVSF_STAGE20_API_H
// Implemented in aud_sdl.cpp. This header may be included in both the SDL UI
// and gameplay modules; nothing here creates a second audio output.
void xm20MenuStart();
void xm20MenuMovement();
void xm20MenuConfirm();
void xm20MenuCrash();
void xm20MenuWelcome();
void xm20MenuFight();
void xm20MeterCharged(int player); // XM87.118 per-stock event
void xm20MenuEnterGame();
void xm20OnCombat();
void xm20ClockReset();
int xm20FrameDue(int fpsHundredths);
#endif
