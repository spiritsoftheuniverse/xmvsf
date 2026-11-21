// 87.495: source-verified paired P2 selector concealment; training-only FF5183 gate.
#include "../../recomp/video/selector_oam.h"
#include "../../recomp/video/dummy_selector_sources.h"
using cps2recomp::native_video::IsTrainingDummySelectorSource;
using cps2recomp::native_video::HideUntilDummySelection;
// F6 frame 1222: exact source tile+full attribute pairs, NOT
// mutable OAM slots or raw visible-ink counts.
static_assert(IsTrainingDummySelectorSource(0x1BB30u, 0x121Bu)); // INK249
static_assert(IsTrainingDummySelectorSource(0x1BB28u, 0x011Bu));
static_assert(IsTrainingDummySelectorSource(0x1BB23u, 0x001Bu));
static_assert(IsTrainingDummySelectorSource(0x1BB33u, 0x001Bu));
static_assert(IsTrainingDummySelectorSource(0x1BB34u, 0x001Bu));
static_assert(IsTrainingDummySelectorSource(0x1BB25u, 0x121Cu)); // INK427
static_assert(IsTrainingDummySelectorSource(0x1BB28u, 0x011Cu));
static_assert(IsTrainingDummySelectorSource(0x1B819u, 0x003Cu));
static_assert(IsTrainingDummySelectorSource(0x1BB23u, 0x001Cu));
static_assert(IsTrainingDummySelectorSource(0x1BB2Cu, 0x001Cu));
static_assert(IsTrainingDummySelectorSource(0x1BB2Du, 0x001Cu));
static_assert(IsTrainingDummySelectorSource(0x1BB2Eu, 0x001Cu));
static_assert(IsTrainingDummySelectorSource(0x1BB2Fu, 0x001Cu));
// Other palette-28 character-select art must remain untouched!
static_assert(!IsTrainingDummySelectorSource(0x1BAC0u, 0x021Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BAC3u, 0x021Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BAAFu, 0x001Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BAB1u, 0x001Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BAB6u, 0x001Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BB9Au, 0x121Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BB25u, 0x121Bu));
static_assert(!IsTrainingDummySelectorSource(0x1BB30u, 0x121Cu));
static_assert(!IsTrainingDummySelectorSource(0x1BB30u, 0x001Bu));
static_assert(!IsTrainingDummySelectorSource(0x1BB25u, 0x121Du));
// Training on + P1 FF5183 below 2 = hide both sets.
// FF5183=2 (already routed dummy ownership) = reveal them immediately.
// Outside Training choosing = never suppress.
static_assert(!HideUntilDummySelection(false, false));
static_assert(!HideUntilDummySelection(false, true));
static_assert(HideUntilDummySelection(true, false));
static_assert(!HideUntilDummySelection(true, true));

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
