#ifndef XMVSF_STAGE20_API_H
#define XMVSF_STAGE20_API_H
// Implemented in aud_sdl.cpp. This header may be included in both the SDL UI
// and gameplay modules; nothing here creates a second audio output.
void xm20MenuStart();
void xm20MenuMovement();
void xm20MenuConfirm();
void xm20MenuIntroSilence();
void xm20MenuIntroMusic();
void xm20MenuCrash();
void xm20MenuWelcome();
void xm20MenuFight();
void xm20VersusPrepare(); // async versus-screen song from audio/music/versusscreen.ogg
void xm20CombatMusicHandoff(); // native combat: stop VS music, never start it late
void xm20FDPrepare(); // async Vorbis decode on Final Destination stage highlight
void xm20GuilePrepare(); // preload single Guile Stage Vorbis loop
void xm20SigmaPrepare(); // preload single Sigma Stage Vorbis loop
void xm20AkumaPrepare(); // preload single Akuma Stage Vorbis loop
void xm20CustomStagePhase(int selectedStage,int nativeStatus); // 0=native 2=FD 3=Guile 4=Sigma 5=Akuma
void xm20MeterCharged(int player); // XM87.118 per-stock event
void xm20MenuEnterGame();
void xm20OnCombat();
void xm20ClockReset();
int xm20FrameDue(int fpsHundredths);
int xm20FrameDueCatchup(int fpsHundredths,int maxFrames);
void xm20AdvanceSoundNoPaint();
#endif
