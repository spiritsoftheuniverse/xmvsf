#ifndef XMVSF_STAGE87_57_NATIVE_FLASH_OVERLAY_H
#define XMVSF_STAGE87_57_NATIVE_FLASH_OVERLAY_H
// Stage87.57: restore ROM-authored native white wipe above custom Wily stage.
// Captured from final native raster BEFORE the two-fill OBJ isolation.
// Gameplay and ROM states are not modified. No synthetic flash timer.
#include "xmvsf_stage87_57_flash_rules.h"
static std::vector<UINT8> xm8757NativeFlashBgr;
static std::vector<UINT8> xm8757NativeFlashRgba;
static bool xm8757NativeFlashActive=false;
static bool xm8757NativeFlashPreviously=false;
static unsigned xm8757NativeFlashFrame=0;
static void xm8757CaptureNativeFlash(){
 xm8757NativeFlashActive=false;
 if(xm77StageResolved!=1 || !xm8649Visual.sceneActive ||
    xm8649Visual.stage!=5 || Cps!=2 || !pBurnDraw || nBurnBpp!=4 ||
    nCpsScreenWidth!=384 || nCpsScreenHeight!=224 || nBurnPitch<384*4)return;
 const auto coverage=xm8757flash::classify(pBurnDraw,384,224,nBurnPitch);
 xm8757NativeFlashActive=coverage.active;
 xm8757NativeFlashFrame=(unsigned)GetCurrentFrame();
 if(xm8757NativeFlashActive){
  xm8757NativeFlashBgr.resize(384u*224u*4u);
  xm8757NativeFlashRgba.resize(384u*224u*4u);
  for(unsigned y=0;y<224u;++y){
   std::memcpy(xm8757NativeFlashBgr.data()+y*384u*4u,
               pBurnDraw+(std::size_t)y*nBurnPitch,384u*4u);
  }
 }
 if(xm8757NativeFlashActive!=xm8757NativeFlashPreviously){
  std::fprintf(stderr,"XM87.57 NATIVE_FLASH frame=%u active=%d white=%u near=%u rows=%u\n",
    xm8757NativeFlashFrame,int(xm8757NativeFlashActive),coverage.white,
    coverage.nearWhite,coverage.rows);
  xm8757NativeFlashPreviously=xm8757NativeFlashActive;
 }
}
static void xm8757FinalizeNativeFlash(){
 if(!xm8757NativeFlashActive || xm8757NativeFlashBgr.empty())return;
 // Keep actual native-colored FX atop white; avoid copying the native forest
 // in the non-white part of the scanline-wipe. The Wily scenery stays below.
 const UINT8* sprite=(xm77SpriteValid && xm77SpriteWidth==384 &&
                       xm77SpriteHeight==224)?xm77SpritePixels():nullptr;
 xm8757flash::compose(xm8757NativeFlashBgr.data(),384*4,sprite,
                      xm8757NativeFlashRgba.data(),384,224);
}
const UINT8* xm8757FlashPixels(){
 return xm8757NativeFlashActive && xm8757NativeFlashRgba.size()==384u*224u*4u
    ?xm8757NativeFlashRgba.data():nullptr;
}
#endif
