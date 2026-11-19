// XMVSF 87.458: F6 native pixel capture for home/Options/arcade menus.
#pragma once

#include "png_write.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace port::xmvsf_home {

class NativeCapture {
    bool requested_ = false;
    std::string folder_;

    void Failed(const char *message) {
        requested_ = false;
        std::fprintf(stderr, "F6 native capture FAILED: %s (%s)\n",
                     message, SDL_GetError());
    }

public:
    explicit NativeCapture(std::string folder) : folder_(std::move(folder)) {}
    void Request() { requested_ = true; }
    bool Pending() const { return requested_; }
    void Consume() { requested_ = false; } // Native gameplay uses Port::Screenshot().

    // Save exactly the given source pixels. Never include window scaling,
    // desktop chrome, presentation letterbox bars or shader output.
    bool SavePixels(const char *screen, const uint32_t *pixels,
                    int width, int height) {
        if (!requested_) return true;
        requested_ = false;
        if (!pixels || width < 1 || height < 1) {
            std::fprintf(stderr, "F6 native capture: empty %s image\n", screen);
            return false;
        }
        if (!SDL_CreateDirectory(folder_.c_str()) &&
            !std::filesystem::is_directory(std::filesystem::u8path(folder_))) {
            std::fprintf(stderr, "F6 native capture: cannot create %s\n",
                         folder_.c_str());
            return false;
        }
        char stamp[32] = {};
        const std::time_t now = std::time(nullptr);
        if (const std::tm *local = std::localtime(&now))
            std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H.%M.%S", local);
        const std::string base = std::string(GAME_ID) + " " + screen + " " + stamp;
        std::string name = base;
        for (int index = 2;
             std::filesystem::exists(std::filesystem::u8path(folder_ + name + ".png"));
             ++index) {
            name = base + " " + std::to_string(index);
        }
        const std::string path = folder_ + name + ".png";
        if (!WritePng(path.c_str(), pixels, uint32_t(width), uint32_t(height))) {
            std::fprintf(stderr, "F6 native capture: could not write %s\n", path.c_str());
            return false;
        }
        std::fprintf(stderr, "F6 NATIVE SCREENSHOT SAVED: %s (%dx%d)\n",
                     path.c_str(), width, height);
        return true;
    }

    // Render only once on the F6 frame into a 960x540 offscreen target,
    // then read the fully composited SDL3 menu at logical-native resolution.
    // Each SDL render target has independent logical presentation/viewport
    // state; restoring the window target leaves its letterboxing intact.
    bool SaveMenu(SDL_Renderer *renderer, const char *screen,
                  const std::function<bool()> &draw) {
        if (!requested_) return true;
        constexpr int kWidth = 960, kHeight = 540;
        if (!renderer) {
            Failed("SDL renderer unavailable");
            return false;
        }
        SDL_Texture *target = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_TARGET, kWidth, kHeight);
        if (!target) {
            Failed("960x540 capture target unavailable");
            return false;
        }
        SDL_Texture *previous = SDL_GetRenderTarget(renderer);
        if (!SDL_SetRenderTarget(renderer, target)) {
            SDL_DestroyTexture(target);
            Failed("cannot select offscreen target");
            return false;
        }
        SDL_SetRenderLogicalPresentation(renderer, 0, 0,
                                         SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderViewport(renderer, nullptr);
        SDL_SetRenderClipRect(renderer, nullptr);
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        const bool painted = draw();
        SDL_Surface *raw = painted ? SDL_RenderReadPixels(renderer, nullptr) : nullptr;
        // Restore the *window* target (including its own 960x540 logical mode)
        // before drawing and presenting the normal visible menu frame.
        SDL_SetRenderTarget(renderer, previous);
        SDL_DestroyTexture(target);
        if (!raw) {
            Failed("menu draw/readback failed");
            return false;
        }
        SDL_Surface *rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ABGR8888);
        SDL_DestroySurface(raw);
        if (!rgba || rgba->w != kWidth || rgba->h != kHeight || !rgba->pixels) {
            if (rgba) SDL_DestroySurface(rgba);
            Failed("menu pixel conversion failed");
            return false;
        }
        std::vector<uint32_t> pixels(size_t(kWidth) * kHeight);
        for (int y = 0; y < kHeight; ++y) {
            std::memcpy(pixels.data() + size_t(y) * kWidth,
                        static_cast<const uint8_t *>(rgba->pixels) +
                            size_t(y) * rgba->pitch,
                        size_t(kWidth) * sizeof(uint32_t));
        }
        SDL_DestroySurface(rgba);
        return SavePixels(screen, pixels.data(), kWidth, kHeight);
    }
};

} // namespace port::xmvsf_home
