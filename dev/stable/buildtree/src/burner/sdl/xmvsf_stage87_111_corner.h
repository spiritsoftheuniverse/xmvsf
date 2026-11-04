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
static const unsigned WORLD_LEFT=CAMERA_MIN; // left visible world edge = 192
static const unsigned WORLD_RIGHT=CAMERA_MAX+VIEWPORT_WIDTH; // 768
static const unsigned DUMMY_CORNER_INSET=WORLD_RIGHT-725u; // 43, observed dummy
static const unsigned FIGHTER_SPACING=725u-568u; // 157, observed separation
static const unsigned LEFT_INNER_X=462u-32u; // 430, previous inner fighter position
static const unsigned LEFT_EDGE_X=WORLD_LEFT+DUMMY_CORNER_INSET; // 235, left corner
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
 const bool right=selection==2u;
 const unsigned dummy=right? WORLD_RIGHT-DUMMY_CORNER_INSET : WORLD_LEFT+DUMMY_CORNER_INSET;
 return right?(fighter==1u?dummy:dummy-FIGHTER_SPACING):
              (fighter==0u?LEFT_EDGE_X:LEFT_INNER_X);
}
static inline unsigned cameraX(unsigned selection) {return selection==2u?CAMERA_MAX:CAMERA_MIN;}
static inline bool flipFacing(unsigned selection,bool snapshotP1IsLeft){
 const bool targetP1IsLeft=fighterX(selection,0u)<fighterX(selection,1u);
 return targetP1IsLeft!=snapshotP1IsLeft;
}
static inline unsigned changedFacing(unsigned nativeByte,bool flip){return flip?(nativeByte^1u):nativeByte;}
struct CameraCheck {
 unsigned expected=0, remaining=0;
 void arm(unsigned x){expected=x;remaining=8u;}
 void clear(){expected=remaining=0;}
};
static inline CameraCheck& cameraCheck(){static CameraCheck c;return c;}
}
#endif
