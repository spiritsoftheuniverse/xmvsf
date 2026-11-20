// F10 capture 87.476: verify native OAM attrs and P1 phase timing.
#include "../../recomp/video/selector_oam.h"
#include "../../recomp/games/xmvsf/src/select_phase.h"
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x001B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x011B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x121B));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001C));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001D));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x000D));
static_assert(!cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x01));
static_assert(!cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x02));
static_assert(!cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x03));
static_assert(cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x04));
static_assert(!cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x05));
static_assert(cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x06));
static_assert(!cps2recomp::xmvsf::NativeP1SecondPickCommitted(0x07));
int main() { return 0; }
