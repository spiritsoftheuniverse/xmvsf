#ifndef XMVSF_STAGE87_401_BOOMER_MOTION_H
#define XMVSF_STAGE87_401_BOOMER_MOTION_H
// Source-independent Boomer animation geometry. No SDL/GIF dependencies:
// this can be validated on Windows via a local C++ test executable.
namespace XM87401Motion {
static constexpr unsigned NearFramesPerPixel=2u;
static constexpr unsigned RotorFramesPerPixel=3u;
static constexpr unsigned ProjectileFramesPerPixel=1u;
// 87.405: rotors may enter anywhere across the full current 384px stage
// view, not just the old 150..220px cluster.
static constexpr int RotorScreenMargin=8;
static constexpr int RotorMinSeparation=96; // minimum top-left X separation
static constexpr int RotorGunGap=38; // clear empty space between both sprites
// 87.405: per-side corrections to 87.404 world-corner placement.
// Mirrored left moves 2px left: at camX=0, screen X=104.
// Original right moves 34px left: at camX=384, screen X=244.
// Do not add saved B wallSpread a second time.
static constexpr int StageWorldW=768;
static constexpr int StageScreenW=384;
static constexpr int WallCornerInset=106;
static constexpr int LeftCorrection=-2;
static constexpr int RightCorrection=-34;
static constexpr int LeftCornerScreenX=WallCornerInset+LeftCorrection; // 104
static constexpr int RightCornerScreenX=StageScreenW-WallCornerInset+RightCorrection; // 244
static inline int gunWorldX(bool mirrored){
    return mirrored?LeftCornerScreenX:StageWorldW-WallCornerInset+RightCorrection;
}
// Compare HORIZONTAL SPRITE BOUNDS with an empty gap between them,
// not just their origins. Applies to every active rotor and gun.
static inline bool horizontallySeparated(int aX,int aW,int bX,int bW,int gap){
    return aX+aW+gap<=bX || bX+bW+gap<=aX;
}
// Central vertical third of the native screen, measured at the
// Triblaster's sprite center. An upward cycle may begin only here.
static constexpr int UpwardCenterMin=StageScreenW*0 + 75;
static constexpr int UpwardCenterMax=149;
static inline bool inMiddleVerticalBand(int gunTopY,int gunHeight){
    const int center=gunTopY+gunHeight/2;
    return center>=UpwardCenterMin && center<=UpwardCenterMax;
}
static inline int screenX(int worldX,int nativeCameraX){
    return worldX-nativeCameraX;
}
// Sprite Y should use the same vertical camera translation as wall B.
// Treat the scene's first camera reading as the stable local spawn origin.
static inline int wallCameraYOffset(int originalCameraY,int currentCameraY){
    return originalCameraY-currentCameraY;
}
static inline int wallShift(unsigned long long born,unsigned long long now){
    return now>=born?int(now/NearFramesPerPixel-born/NearFramesPerPixel):0;
}
static inline int rotorShift(unsigned long long born,unsigned long long now){
    return now>=born?int(now/RotorFramesPerPixel-born/RotorFramesPerPixel):0;
}
static inline int offsetX(bool mirror,int parentW,int projectileW,int localX){
    return mirror?parentW-localX-projectileW:localX;
}
static inline int offsetY(bool verticalFlip,int parentH,int projectileH,int localY){
    return verticalFlip?parentH-localY-projectileH:localY;
}
static inline int projectileDx(bool leftSide,bool shot,unsigned long long elapsed){
    if(!shot)return 0;
    return leftSide?int(elapsed/ProjectileFramesPerPixel):
                    -int(elapsed/ProjectileFramesPerPixel);
}
static inline int projectileDy(bool verticalFlip,bool bullet,
                               unsigned long long born,unsigned long long now){
    const unsigned long long elapsed=now>=born?now-born:0ull;
    return wallShift(born,now)+
        (bullet?(verticalFlip?-int(elapsed/ProjectileFramesPerPixel):
                                      int(elapsed/ProjectileFramesPerPixel)):0);
}
static inline bool verticalFlip(unsigned long long gifCycle){
    return (gifCycle&1ull)!=0ull;
}
}
#endif
