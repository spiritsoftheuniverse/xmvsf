// XMVSF 87.468: dissolve native character select in from the last SDL3 menu pixels.
// Reuses the 87.464 film's curl noise, not a black fade or ROM tile filtering.
#pragma once
#include "xmvsf_film_edge_dither.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

namespace port::xmvsf_home {
class SelectReveal {
    static constexpr int kWidth = 960, kHeight = 540;
    static constexpr uint64_t kDurationMs = 580;
    std::vector<uint32_t> outgoing_, overlay_;
    std::vector<uint8_t> thresholds_;
    int outputW_ = 0, outputH_ = 0;
    uint64_t start_ = 0;
    bool active_ = false, started_ = false;

    static uint8_t Threshold(int x, int y, uint64_t timestamp) {
        const auto curl = FilmEdgeDither::Curl(double(x), double(y), timestamp,
                                                0x4a1c7e2dULL, true);
        const int bx = (x + int(std::floor(curl.x))) & 7;
        const int by = (y + int(std::floor(curl.y))) & 7;
        const double ordered = (FilmEdgeDither::kBayer8[size_t(by * 8 + bx)] + .5) / 64.;
        const double coarse = (FilmEdgeDither::Lattice(x / 32, y / 32, 0xf02a531bULL) + 1.) * .5;
        return static_cast<uint8_t>(std::clamp(int(255. * (ordered * .72 + coarse * .28)), 0, 254));
    }
public:
    void Cancel() {
        outgoing_.clear();
        overlay_.clear();
        thresholds_.clear();
        outputW_ = outputH_ = 0;
        start_ = 0;
        active_ = started_ = false;
    }
    bool Active() const { return active_; }
    int Width() const { return outputW_; }
    int Height() const { return outputH_; }
    const uint32_t *Pixels() const { return overlay_.data(); }

    // Repaint the actual selected thumbnail/film frame into a temporary
    // native menu-sized target. SDL3 maintains presentation state per target.
    bool Capture(SDL_Renderer *renderer, uint64_t timestamp,
                 const std::function<bool()> &paint) {
        Cancel();
        if (!renderer || !paint) return false;
        int w = 0, h = 0;
        if (!SDL_GetRenderOutputSize(renderer, &w, &h) || w <= 0 || h <= 0)
            return false;
        SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                                 SDL_TEXTUREACCESS_TARGET, kWidth, kHeight);
        if (!target) return false;
        SDL_Texture *previous = SDL_GetRenderTarget(renderer);
        if (!SDL_SetRenderTarget(renderer, target)) {
            SDL_DestroyTexture(target);
            return false;
        }
        SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderViewport(renderer, nullptr);
        SDL_SetRenderClipRect(renderer, nullptr);
        SDL_SetRenderScale(renderer, 1.f, 1.f);
        const bool painted = paint();
        SDL_Surface *raw = painted ? SDL_RenderReadPixels(renderer, nullptr) : nullptr;
        SDL_SetRenderTarget(renderer, previous);
        SDL_DestroyTexture(target);
        if (!raw) {
            std::fprintf(stderr, "87.468 selection capture failed: %s\n", SDL_GetError());
            return false;
        }
        SDL_Surface *rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ABGR8888);
        SDL_DestroySurface(raw);
        if (!rgba || rgba->w != kWidth || rgba->h != kHeight || !rgba->pixels) {
            if (rgba) SDL_DestroySurface(rgba);
            return false;
        }
        std::vector<uint32_t> last(size_t(kWidth) * kHeight);
        for (int y = 0; y < kHeight; ++y)
            std::memcpy(last.data() + size_t(y) * kWidth,
                        static_cast<const uint8_t *>(rgba->pixels) + size_t(y) * rgba->pitch,
                        size_t(kWidth) * sizeof(uint32_t));
        SDL_DestroySurface(rgba);

        // Match the menu's 960x540 physical letterbox, even in ultrawide.
        // The outgoing image is a full-window overlay above native gameplay.
        const float factor = std::min(float(w) / kWidth, float(h) / kHeight);
        const int fw = std::max(1, int(kWidth * factor));
        const int fh = std::max(1, int(kHeight * factor));
        const int left = (w - fw) / 2, top = (h - fh) / 2;
        outputW_ = w; outputH_ = h;
        outgoing_.assign(size_t(w) * h, 0xff000000u);
        thresholds_.assign(outgoing_.size(), 255);
        std::vector<uint8_t> logicalNoise(size_t(kWidth) * kHeight);
        for (int y = 0; y < kHeight; ++y)
            for (int x = 0; x < kWidth; ++x)
                logicalNoise[size_t(y) * kWidth + x] = Threshold(x, y, timestamp);
        for (int y = 0; y < fh; ++y) {
            const int py = top + y;
            if (py < 0 || py >= h) continue;
            const int sy = std::min(kHeight - 1, y * kHeight / fh);
            for (int x = 0; x < fw; ++x) {
                const int px = left + x;
                if (px < 0 || px >= w) continue;
                const int sx = std::min(kWidth - 1, x * kWidth / fw);
                const size_t dest = size_t(py) * w + px;
                outgoing_[dest] = last[size_t(sy) * kWidth + sx] | 0xff000000u;
                thresholds_[dest] = logicalNoise[size_t(sy) * kWidth + sx];
            }
        }
        overlay_ = outgoing_;
        active_ = true;
        return true;
    }

    // Only begin the reveal when the hidden fast boot reaches character select.
    // All old pixels are fully opaque until individually replaced by new ones.
    bool Frame(uint64_t timestamp) {
        if (!active_) return false;
        if (!started_) {
            started_ = true;
            start_ = timestamp;
        }
        const uint64_t elapsed = timestamp >= start_ ? timestamp - start_ : 0;
        if (elapsed >= kDurationMs) {
            Cancel();
            return false;
        }
        const double t = double(elapsed) / double(kDurationMs);
        const unsigned reveal = static_cast<unsigned>(255. * t * t * (3. - 2. * t));
        for (size_t i = 0; i < outgoing_.size(); ++i)
            overlay_[i] = thresholds_[i] < reveal ? 0u : outgoing_[i];
        return true;
    }
};
} // namespace port::xmvsf_home
