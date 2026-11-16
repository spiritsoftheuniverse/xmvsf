// TEST 87.408: standalone C++ syntax test for screenshot -> stage registry.
// No SDL, ROM, renderer, screen capture or file mutations when compiled
// with -fsyntax-only. BUILD_TEST.bat invokes this before the native make.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_408_capture_catalog.h"
#include <type_traits>
static_assert(std::is_same<
    decltype(XM87408Catalog::append((const char*)nullptr,0u,
        XM87408Catalog::Snapshot{})),bool>::value,
    "Native screenshot stage catalog append API changed");
int main(){
    const XM87408Catalog::Snapshot s{};
    return s.observed?1:0;
}
