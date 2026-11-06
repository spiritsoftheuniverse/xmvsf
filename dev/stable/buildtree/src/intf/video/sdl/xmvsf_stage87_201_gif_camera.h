#ifndef XMVSF_STAGE87_201_GIF_CAMERA_H
#define XMVSF_STAGE87_201_GIF_CAMERA_H
// Camera registration for a *single complete* animated stage image.
// The CPS2 stage-05 camera is observed (never written). The same bounded
// 86.29 -> Wily world transform is used for GIF backgrounds in both modes.
// Pure math: no SDL objects, ROM access, or per-frame allocations.
#include <algorithm>
#include <cstdint>

namespace XM87201GifCamera {
struct Rect { int x, y, w, h; };
static const int kNativeViewW=384, kNativeViewH=224;
static const int kNativeXMax=384;
static const int kNativeYMin=-14, kNativeYMax=256;
static const int kWorldLeft=-64, kWorldRight=317;
static const int kWorldCameraTravelX=kWorldRight-kWorldLeft; // 381
static const int kWorldCameraTravelY=kNativeYMax-kNativeYMin; // 270
static const int kWorldImageWidth=kNativeViewW+kWorldCameraTravelX; // 765
static const int kWorldImageHeight=kNativeViewH+kWorldCameraTravelY; // 494

inline int clamp(int value,int lo,int hi){
    return std::max(lo,std::min(value,hi));
}
inline int worldCameraX(int nativeX){
    // Identical integer rounding and bounds to Stage86.49 Wily:
    // -64 + (nativeCameraX * 381 + 192) / 384.
    return kWorldLeft+
        (clamp(nativeX,0,kNativeXMax)*kWorldCameraTravelX+192)/kNativeXMax;
}
inline Rect crop(int gifW,int gifH,int nativeX,int nativeY){
    if(gifW<=0 || gifH<=0)return Rect{0,0,0,0};
    // One uniformly scaled world image, containing the entire camera travel
    // plus one 384x224 viewport. This is NOT aspect-only pan interpolation:
    // camera y and x each have independently registered world coordinates.
    // Center any extra artwork on the axis with surplus source pixels.
    const std::int64_t scaleX=(std::int64_t(gifW)<<16)/kWorldImageWidth;
    const std::int64_t scaleY=(std::int64_t(gifH)<<16)/kWorldImageHeight;
    const std::int64_t scale=std::max<std::int64_t>(1,std::min(scaleX,scaleY));
    const int viewW=clamp(int((kNativeViewW*scale+32768)/65536),1,gifW);
    const int viewH=clamp(int((kNativeViewH*scale+32768)/65536),1,gifH);
    const int worldW=int((kWorldImageWidth*scale+32768)/65536);
    const int worldH=int((kWorldImageHeight*scale+32768)/65536);
    const int originX=std::max(0,(gifW-worldW)/2);
    const int originY=std::max(0,(gifH-worldH)/2);
    const int worldX=worldCameraX(nativeX)-kWorldLeft; // [0..381]
    const int worldY=clamp(nativeY,kNativeYMin,kNativeYMax)-kNativeYMin; // [0..270]
    const int sourceX=originX+int((worldX*scale+32768)/65536);
    const int sourceY=originY+int((worldY*scale+32768)/65536);
    return Rect{clamp(sourceX,0,gifW-viewW),
                clamp(sourceY,0,gifH-viewH),viewW,viewH};
}
} // namespace XM87201GifCamera
#endif
