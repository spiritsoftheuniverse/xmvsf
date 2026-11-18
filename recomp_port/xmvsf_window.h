// XMVSF window controls v87.437. Native SDL3 equivalent of our Stage29
// Ctrl +/- presets, plus Ctrl+Backspace maximize/restore.
#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace port::xmvsf_home {
struct WindowSize { int width, height; };
inline constexpr std::array<WindowSize, 6> kWindowSizes{{
    {384, 224}, {768, 448}, {1152, 672}, {1536, 896},
    {1920, 1120}, {2304, 1344}
}};

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
