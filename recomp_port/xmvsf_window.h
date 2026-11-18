// XMVSF window controls v87.437. Native SDL3 equivalent of our Stage29
// Ctrl +/- presets, plus Ctrl+Backspace maximize/restore.
#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <algorithm>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace port::xmvsf_home {
struct WindowSize { int width, height; };
// Every preset contains an integer number of 480x270 authored menu pixels.
// The CPS-2 scene itself remains 384x224 and is centered at an integer factor.
inline constexpr std::array<WindowSize, 6> kWindowSizes{{
    {480, 270}, {960, 540}, {1440, 810}, {1920, 1080},
    {2400, 1350}, {2880, 1620}
}};


struct PixelView {
    int x = 0, y = 0, w = 0, h = 0, sourceW = 480, sourceH = 270, divisor = 2;
};
// 960x540 authoring -> exact nearest-neighbour downsample (normally 2x)
// -> integer-scale output. Fractional RGB/blurred edge widths never arise.
inline PixelView MenuViewport(int outputW, int outputH) {
    PixelView view;
    outputW = std::max(1, outputW);
    outputH = std::max(1, outputH);
    while (view.divisor < 12 &&
           (960 / view.divisor > outputW || 540 / view.divisor > outputH))
        ++view.divisor;
    view.sourceW = 960 / view.divisor;
    view.sourceH = 540 / view.divisor;
    const int zoom = std::max(1, std::min(outputW / view.sourceW, outputH / view.sourceH));
    view.w = view.sourceW * zoom;
    view.h = view.sourceH * zoom;
    view.x = (outputW - view.w) / 2;
    view.y = (outputH - view.h) / 2;
    return view;
}
inline void DownsampleHome(const std::vector<uint32_t> &source,
                           std::vector<uint32_t> &out, const PixelView &view) {
    if (source.size() != size_t(960) * 540) {
        out.clear();
        return;
    }
    out.resize(size_t(view.sourceW) * view.sourceH);
    for (int y = 0; y < view.sourceH; ++y) {
        const int sy = std::min(539, y * view.divisor + view.divisor / 2);
        for (int x = 0; x < view.sourceW; ++x) {
            const int sx = std::min(959, x * view.divisor + view.divisor / 2);
            out[size_t(y) * view.sourceW + x] = source[size_t(sy) * 960 + sx];
        }
    }
}

inline bool WindowShortcut(SDL_Window *window, SDL_Scancode scancode, SDL_Keymod modifiers) {
    if (!window || !(modifiers & SDL_KMOD_CTRL) || (modifiers & SDL_KMOD_ALT))
        return false;

    const bool increase = scancode == SDL_SCANCODE_EQUALS ||
                          scancode == SDL_SCANCODE_KP_PLUS;
    const bool decrease = scancode == SDL_SCANCODE_MINUS ||
                          scancode == SDL_SCANCODE_KP_MINUS;
    const bool toggleMax = scancode == SDL_SCANCODE_BACKSPACE;
    if (!increase && !decrease && !toggleMax) return false;

    const SDL_WindowFlags flags = SDL_GetWindowFlags(window);
    if (toggleMax) {
        if (flags & SDL_WINDOW_FULLSCREEN) SDL_SetWindowFullscreen(window, false);
        if (flags & SDL_WINDOW_MAXIMIZED) SDL_RestoreWindow(window);
        else SDL_MaximizeWindow(window);
        return true;
    }
    // Game and menu rendering retain their own aspect ratios. We change
    // the physical window only; fullscreen presentation is left untouched.
    if (flags & SDL_WINDOW_FULLSCREEN) return true;
    int currentW = 0, currentH = 0;
    SDL_GetWindowSize(window, &currentW, &currentH);
    int closest = 0;
    int64_t difference = std::numeric_limits<int64_t>::max();
    for (int i = 0; i < int(kWindowSizes.size()); ++i) {
        const int64_t dw = int64_t(currentW) - kWindowSizes[size_t(i)].width;
        const int64_t dh = int64_t(currentH) - kWindowSizes[size_t(i)].height;
        const int64_t d = dw * dw + dh * dh;
        if (d < difference) { closest = i; difference = d; }
    }
    const int target = closest + (increase ? 1 : -1);
    if (target < 0 || target >= int(kWindowSizes.size())) return true;
    if (flags & SDL_WINDOW_MAXIMIZED) SDL_RestoreWindow(window);
    SDL_SetWindowSize(window, kWindowSizes[size_t(target)].width,
                     kWindowSizes[size_t(target)].height);
    return true;
}
} // namespace port::xmvsf_home
