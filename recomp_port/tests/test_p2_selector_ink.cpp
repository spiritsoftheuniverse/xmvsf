// 87.477: verify native P2 OAM attrs + FBNeo's FF5183 accept-count gate.
#include "../../recomp/video/selector_oam.h"
#include "../../recomp/games/xmvsf/src/select_accept.h"
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x001B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x011B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x121B));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001C));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001D));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x000D));
static_assert(!cps2recomp::xmvsf::NativeP1TeamAccepted(0));
static_assert(!cps2recomp::xmvsf::NativeP1TeamAccepted(1));
static_assert(cps2recomp::xmvsf::NativeP1TeamAccepted(2));
static_assert(!cps2recomp::xmvsf::NativeP1TeamAccepted(3));
static_assert(!cps2recomp::xmvsf::NativeP1TeamAccepted(255));
int main() { return 0; }
