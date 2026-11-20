// 87.476 XMVSF F10 native CPS-2 OAM capture, frame range 548..1455.
// P2's moving selector is the palette-0x1B OAM object group.
// When P2 cursor moves 0012->0014 at frame 850, this group moves
// in the native OAM at frame 851. After P2's accept at 903/904,
// the palette-0x1B entries disappear. These are the object's ACTUAL
// attribute bits, not speculative decoded graphics pens or tile IDs.
// Renderer only enables suppression in Training character selection.
#pragma once
#include <cstdint>
namespace cps2recomp::native_video {
inline constexpr bool IsP2SelectionObject(std::uint16_t attributes) {
    return (attributes & 0x001Fu) == 0x001Bu;
}
}
