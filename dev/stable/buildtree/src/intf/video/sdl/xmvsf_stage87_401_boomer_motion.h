#ifndef XMVSF_STAGE87_401_BOOMER_MOTION_H
#define XMVSF_STAGE87_401_BOOMER_MOTION_H
// Source-independent Boomer animation geometry. No SDL/GIF dependencies:
// this can be validated on Windows via a local C++ test executable.
namespace XM87401Motion {
static constexpr unsigned NearFramesPerPixel=2u;
static constexpr unsigned RotorFramesPerPixel=3u;
static constexpr unsigned ProjectileFramesPerPixel=1u;
static constexpr int RotorXMin=150;
static constexpr int RotorXMax=220;
// 87.404: Triblasters belong to the outer WORLD corners, not the
// initial 384-wide camera viewport. Exact top-left screen X at corners:
// mirrored (left) = 106 when cameraX=0; original (right) = 278 when
// cameraX=384. Do not add saved B wallSpread a second time.
static constexpr int StageWorldW=768;
static constexpr int StageScreenW=384;
static constexpr int WallCornerInset=106;
static constexpr int RightCornerScreenX=StageScreenW-WallCornerInset; // 278
static constexpr int LeftCornerScreenX=WallCornerInset; // 106
static inline int gunWorldX(bool mirrored){
    return mirrored?WallCornerInset:StageWorldW-WallCornerInset;
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
