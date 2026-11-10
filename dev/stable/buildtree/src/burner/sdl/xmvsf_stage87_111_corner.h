#ifndef XMVSF_STAGE87_111_CORNER_H
#define XMVSF_STAGE87_111_CORNER_H
// Stage 05 geometry reconstructed from XM87.109 live positions and XM86.54
// native-camera logs in session_20261102_175317_190. Do not pretend these
// bounds are verified for other CPS-2 stages. Unsafe stages fail closed.
namespace XM87111 {
static const unsigned STAGE=5u;
static const unsigned VIEWPORT_WIDTH=384u;
static const unsigned CAMERA_MIN=192u; // XM86.54 WILY_PRESENT frame=2
static const unsigned CAMERA_MAX=384u; // XM86.54 WILY_PRESENT frame=601
static const unsigned CAMERA_BIAS=64u; // XM86.49: s16(FF524C)+64
static const unsigned WORLD_LEFT=CAMERA_MIN; // unchanged conservative initial-centering gate
static const unsigned WORLD_RIGHT=CAMERA_MAX+VIEWPORT_WIDTH; // 768
// 87.272 training_side_switch.log: P1 repeatedly taunted around X=118
// on LEFT and X=647 on RIGHT. Use the LEFT marker as the canonical
// fighter position and mirror it across world X=384 (768-X).
// Cyclops/P2 retains his stable 21/747 corner anchor. This increases
// center-to-center spacing from 58 to 97 native pixels without touching CENTER.
static const unsigned MIRROR_SUM=WORLD_RIGHT;
static const unsigned LEFT_P1_X=118u;
static const unsigned LEFT_P2_X=21u;
static const unsigned RIGHT_P1_X=MIRROR_SUM-LEFT_P1_X; // 650
static const unsigned RIGHT_P2_X=MIRROR_SUM-LEFT_P2_X; // 747
// Compatibility aliases used by older geometry helpers: now 97px/21px.
static const unsigned FIGHTER_SPACING=LEFT_P1_X-LEFT_P2_X;
static const unsigned DUMMY_CORNER_INSET=WORLD_RIGHT-RIGHT_P2_X;
static const unsigned LEFT_CAMERA_X=0u;
static const unsigned CORNER_Y=472u;
// Normal match-start X coordinates measured from 87.109 live capture, frame 1534.
static const unsigned DEFAULT_P1_X=462u;
static const unsigned DEFAULT_P2_X=514u;
static inline unsigned centerCameraX(unsigned p1,unsigned p2){
 // Place the midpoint of the recorded fighters at the middle of the viewport;
 // clamp to the measured stage-05 camera bounds.
 const unsigned middle=(p1/2u)+(p2/2u)+((p1&1u)&(p2&1u));
 const unsigned half=VIEWPORT_WIDTH/2u;
 const unsigned wanted=middle>half?middle-half:0u;
 return wanted<CAMERA_MIN?CAMERA_MIN:(wanted>CAMERA_MAX?CAMERA_MAX:wanted);
}
static inline unsigned fighterX(unsigned selection,unsigned fighter) {
 // Called only for stage 05, SIDE=RIGHT(2) or LEFT(3).
 const unsigned left=(fighter==0u?LEFT_P1_X:LEFT_P2_X);
 return selection==2u?MIRROR_SUM-left:left;
}
// The LEFT camera target is outside the older center-only estimate [192,384].
// The native camera word is signed: target 0 is stored as -64 (0xFFC0).
static inline unsigned cameraX(unsigned selection) {return selection==2u?CAMERA_MAX:LEFT_CAMERA_X;}
static inline unsigned nativeCameraX(unsigned word) {
 return (unsigned)((int)(short)(word&0xffffu)+(int)CAMERA_BIAS);
}
static inline bool flipFacing(unsigned selection,bool snapshotP1IsLeft){
 const bool targetP1IsLeft=fighterX(selection,0u)<fighterX(selection,1u);
 return targetP1IsLeft!=snapshotP1IsLeft;
}
static inline unsigned changedFacing(unsigned nativeByte,bool flip){return flip?(nativeByte^1u):nativeByte;}
struct CameraCheck {
 unsigned expected=0, remaining=0;
 bool actors=false;
 unsigned p1x=0,p2x=0;
 void arm(unsigned x){expected=x;remaining=8u;actors=false;}
 void armActors(unsigned x,unsigned a,unsigned b){
  arm(x);actors=true;p1x=a;p2x=b;
 }
 void clear(){expected=remaining=0;actors=false;p1x=p2x=0;}
};
static inline CameraCheck& cameraCheck(){static CameraCheck c;return c;}
}
#endif
