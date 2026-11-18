// Local-only 87.438 pixel geometry regression. Build with the existing UCRT64
// SDL3 headers; no emulator or ROM needed. Never use GitHub Actions.
#include "../xmvsf_window.h"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace port::xmvsf_home;

int main() {
    {
        const PixelView p = MenuViewport(960, 540);
        assert(p.x == 0 && p.y == 0 && p.w == 960 && p.h == 540);
        assert(p.sourceW == 480 && p.sourceH == 270 && p.divisor == 2);
    }
    {
        const PixelView p = MenuViewport(1920, 1080);
        assert(p.x == 0 && p.y == 0 && p.w == 1920 && p.h == 1080);
        assert(p.sourceW == 480 && p.sourceH == 270);
    }
    {
        // The game's smallest old 384x224 window remains sharply letterboxed.
        const PixelView p = MenuViewport(384, 224);
        assert(p.w == 320 && p.h == 180 && p.x == 32 && p.y == 22);
        assert(p.sourceW == 320 && p.sourceH == 180 && p.divisor == 3);
    }
    {
        const PixelView p = MenuViewport(2400, 1350);
        assert(p.w == 2400 && p.h == 1350);
        assert(p.sourceW == 480 && p.sourceH == 270);
    }
    {
        std::vector<uint32_t> source(size_t(960) * 540);
        for (int y = 0; y < 540; ++y)
            for (int x = 0; x < 960; ++x)
                source[size_t(y) * 960 + x] = uint32_t((x & 0xFFF) | ((y & 0xFFF) << 12));
        std::vector<uint32_t> result;
        DownsampleHome(source, result, MenuViewport(960, 540));
        assert(result.size() == size_t(480) * 270);
        assert(result[0] == source[size_t(1) * 960 + 1]);
        assert(result[479] == source[size_t(1) * 960 + 959]);
        assert(result.back() == source[size_t(539) * 960 + 959]);
    }
    return 0;
}
