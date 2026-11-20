// 87.478: verify native OAM, FF5183 accept count, and global xmvsf namespace.
#include "../../recomp/video/selector_oam.h"
// This constant represents declarations in the game's GLOBAL ::xmvsf header.
// An accidental nested cps2recomp::xmvsf shadows it in Training's scope.
namespace xmvsf { inline constexpr unsigned kNamespaceRegressionPlayer = 0xFF4000u; }
#include "../../recomp/games/xmvsf/src/select_accept.h"
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x001B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x011B));
static_assert(cps2recomp::native_video::IsP2SelectionObject(0x121B));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001C));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x001D));
static_assert(!cps2recomp::native_video::IsP2SelectionObject(0x000D));
static_assert(!xmvsf::NativeP1TeamAccepted(0));
static_assert(!xmvsf::NativeP1TeamAccepted(1));
static_assert(xmvsf::NativeP1TeamAccepted(2));
static_assert(!xmvsf::NativeP1TeamAccepted(3));
static_assert(!xmvsf::NativeP1TeamAccepted(255));
namespace cps2recomp {
namespace {
    using namespace xmvsf; // identical lookup context to training.cpp
    static_assert(kNamespaceRegressionPlayer == 0xFF4000u);
    static_assert(NativeP1TeamAccepted(2));
}
}
int main() { return 0; }
