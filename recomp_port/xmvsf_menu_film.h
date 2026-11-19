// XMVSF 87.460: closer, twice-fast, shuffled films with post-text Bayer dither.
#pragma once
#include "xmvsf_home.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace port::xmvsf_home {
class MenuFilmStrips {
    static constexpr int kCanvasWidth = 960, kCanvasHeight = 540;
    // 80px closer than the previous +16px spacing: 64px overlap.
    static constexpr int kMargin = 16, kGap = -64, kPortraitTop = 78;
    static constexpr double kSpeedPixelsPerSecond = 20.0;
    struct Column {
        Image shape;
        SDL_Texture *plain = nullptr;
        std::vector<SDL_Texture *> portraits;
        std::vector<Image> portraitImages;
        std::vector<size_t> order; // shuffled only at load; stable across frames
        int x = 0;
        int revealLevel = -1;
        bool scrollDown = false;
    };
    SDL_Renderer *renderer_ = nullptr;
    Column xmen_, streetFighter_;
    std::vector<uint32_t> ditherPixels_;

    static SDL_Texture *Upload(SDL_Renderer *renderer, const Image &image) {
        if (!renderer || !image.Valid()) return nullptr;
        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STATIC, image.width, image.height);
        if (!texture) return nullptr;
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        if (!SDL_UpdateTexture(texture, nullptr, image.pixels.data(), image.width * 4)) {
            SDL_DestroyTexture(texture);
            return nullptr;
        }
        return texture;
    }

    // Find the widest full-width vertical band inside the *actual* film alpha.
    // This avoids cropping heads against the trapezoid's sloped top/bottom.
    // The 78px original top inset remains the minimum possible starting row.
    static constexpr int kPortraitInset = 8;
    static Rect SafePortraitArea(const Image &film) {
        if (!film.Valid() || film.width <= 2 * kPortraitInset ||
            film.height <= kPortraitTop + 2 * kPortraitInset) return {};
        const int left = kPortraitInset;
        const int right = film.width - kPortraitInset;
        int runStart = -1, bestStart = -1, bestLength = 0;
        for (int y = kPortraitTop; y <= film.height; ++y) {
            bool fullWidth = y < film.height;
            if (fullWidth) {
                for (int x = left; x < right; ++x) {
                    if ((film.pixels[size_t(y) * size_t(film.width) +
                                     size_t(x)] >> 24) < 240u) {
                        fullWidth = false;
                        break;
                    }
                }
            }
            if (fullWidth && runStart < 0) runStart = y;
            else if (!fullWidth && runStart >= 0) {
                const int length = y - runStart;
                if (length > bestLength) {
                    bestStart = runStart;
                    bestLength = length;
                }
                runStart = -1;
            }
        }
        if (bestStart < 0 || bestLength <= 2 * kPortraitInset) return {};
        const int top = std::max(kPortraitTop + kPortraitInset,
                                 bestStart + kPortraitInset);
        const int bottom = bestStart + bestLength - kPortraitInset;
        return bottom > top ? Rect{left, top, right - left, bottom - top} : Rect{};
    }

    // Source alpha bounds remove transparent padding (rather than cropping
    // the head with cover-scaling). Contain-fit the *whole opaque portrait*
    // within the film's guaranteed-visible rectangle, then keep its RGBA
    // and apply the original trapezoid alpha as an extra clipping mask.
    static Image MaskPortrait(const Image &film, const Image &portrait,
                              Rect safe) {
        Image out;
        if (!film.Valid() || !portrait.Valid() || safe.w < 1 || safe.h < 1)
            return out;
        out.width = film.width;
        out.height = film.height;
        out.pixels.assign(size_t(out.width) * size_t(out.height), 0u);
        const Rect visible = VisibleBounds(portrait);
        if (visible.w < 1 || visible.h < 1) return out;

        const double scale = std::min(double(safe.w) / double(visible.w),
                                      double(safe.h) / double(visible.h));
        const int drawW = std::clamp(int(std::floor(double(visible.w) * scale + 0.5)),
                                    1, safe.w);
        const int drawH = std::clamp(int(std::floor(double(visible.h) * scale + 0.5)),
                                    1, safe.h);
        const int dstX = safe.x + (safe.w - drawW) / 2;
        const int dstY = safe.y + (safe.h - drawH) / 2;
        for (int y = 0; y < drawH; ++y) {
            const int sy = visible.y + std::clamp(
                int(std::floor((double(y) + 0.5) / scale)), 0, visible.h - 1);
            const int dy = dstY + y;
            for (int x = 0; x < drawW; ++x) {
                const int sx = visible.x + std::clamp(
                    int(std::floor((double(x) + 0.5) / scale)), 0, visible.w - 1);
                const int dx = dstX + x;
                const size_t at = size_t(dy) * size_t(film.width) + size_t(dx);
                const unsigned maskAlpha = film.pixels[at] >> 24;
                if (maskAlpha == 0u) continue;
                const uint32_t person = portrait.pixels[
                    size_t(sy) * size_t(portrait.width) + size_t(sx)];
                const unsigned portraitAlpha = person >> 24;
                if (portraitAlpha == 0u) continue;
                const unsigned clippedAlpha =
                    (maskAlpha * portraitAlpha + 127u) / 255u;
                // Image::pixels is 0xAABBGGRR, NOT 0xAARRGGBB.
                out.pixels[at] = (person & 0x00ffffffu) |
                                 (uint32_t(clippedAlpha) << 24);
            }
        }
        return out;
    }

    // Match the text compositor's 4x4 Bayer ordering exactly. Update
    // texture pixels only when the 16-step intro phase changes; ordinary
    // gameplay/menu rendering does not read back or re-upload anything.
    void UploadDither(SDL_Texture *texture, const Image &source, int level) {
        if (!texture || !source.Valid()) return;
        constexpr uint8_t bayer[16] = {
            0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5
        };
        const uint32_t *pixels = source.pixels.data();
        if (level < 16) {
            ditherPixels_.resize(source.pixels.size());
            for (int y = 0; y < source.height; ++y) {
                for (int x = 0; x < source.width; ++x) {
                    const size_t at = size_t(y) * size_t(source.width) + size_t(x);
                    const uint32_t pixel = source.pixels[at];
                    ditherPixels_[at] = bayer[(y & 3) * 4 + (x & 3)] < level ?
                        pixel : (pixel & 0x00ffffffu);
                }
            }
            pixels = ditherPixels_.data();
        }
        if (!SDL_UpdateTexture(texture, nullptr, pixels, source.width * 4)) {
            std::fprintf(stderr, "XMVSF 87.460 film dither upload failed: %s\n",
                         SDL_GetError());
        }
    }

    void ApplyReveal(Column &column, int level) {
        level = std::clamp(level, 0, 16);
        if (column.revealLevel == level) return;
        column.revealLevel = level;
        if (level == 0) return; // DrawColumn suppresses invisible films.
        UploadDither(column.plain, column.shape, level);
        for (size_t i = 0; i < column.portraits.size(); ++i) {
            UploadDither(column.portraits[i], column.portraitImages[i], level);
        }
    }

    void LoadColumn(Column &column, const char *shapeFile,
                    const char *prefix, bool down) {
        column.scrollDown = down;
        column.shape = LoadPng(shapeFile);
        if (!column.shape.Valid()) {
            std::fprintf(stderr, "XMVSF 87.460 menu film missing or invalid: %s\n",
                         shapeFile);
            return;
        }
        column.x = down ? kMargin : kCanvasWidth - kMargin - column.shape.width;
        column.plain = Upload(renderer_, column.shape);
        const Rect safe = SafePortraitArea(column.shape);
        if (safe.w < 1 || safe.h < 1) {
            std::fprintf(stderr, "XMVSF 87.460 portrait safe area unavailable: %s\n",
                         shapeFile);
            return;
        }
        for (int i = 1; i <= 9; ++i) {
            const std::string filename = std::string("menu/characters/") +
                prefix + " (" + std::to_string(i) + ").png";
            const Image person = LoadPng(filename.c_str());
            if (!person.Valid()) {
                std::fprintf(stderr, "XMVSF 87.460 missing portrait: %s\n",
                             filename.c_str());
                continue;
            }
            Image clipped = MaskPortrait(column.shape, person, safe);
            SDL_Texture *texture = Upload(renderer_, clipped);
            if (texture) {
                column.portraitImages.push_back(std::move(clipped));
                column.portraits.push_back(texture);
            } else std::fprintf(stderr, "XMVSF 87.460 portrait upload failed: %s\n",
                                SDL_GetError());
        }
        column.order.resize(column.portraits.size());
        std::iota(column.order.begin(), column.order.end(), size_t(0));
        // Fisher-Yates by std::shuffle. Each loaded character appears once
        // before the cycle repeats; no per-frame random flicker.
        std::mt19937 random(std::random_device{}());
        std::shuffle(column.order.begin(), column.order.end(), random);
        std::fprintf(stderr, "XMVSF 87.460 %s film: %zu shuffled portraits at x=%d\n",
                     prefix, column.portraits.size(), column.x);
    }

    void DrawColumn(const Column &column, std::uint64_t now, int reveal) const {
        if (!column.shape.Valid() || reveal == 0) return;
        // 453px panels at 389px pitch: 64px overlap (80px nearer).
        const int step = column.shape.height + kGap;
        const double distance = double(now) * (kSpeedPixelsPerSecond / 1000.0);
        const std::int64_t cycles = std::int64_t(std::floor(distance / double(step)));
        const float phase = float(std::fmod(distance, double(step)));
        // Downward scroll can show the bottom 64px of slot -2 at rollover.
        for (int slot = -2; slot <= 4; ++slot) {
            const int y = int(std::floor(double(slot * step) +
                                  (column.scrollDown ? double(phase) : -double(phase))));
            if (y >= kCanvasHeight || y + column.shape.height <= 0) continue;
            const SDL_FRect dest{float(column.x), float(y),
                                 float(column.shape.width), float(column.shape.height)};
            if (column.plain) SDL_RenderTexture(renderer_, column.plain, nullptr, &dest);
            if (!column.portraits.empty()) {
                const std::int64_t ordinal = std::int64_t(slot) +
                    (column.scrollDown ? -cycles : cycles);
                const auto count = std::int64_t(column.portraits.size());
                SDL_Texture *portrait =
                    column.portraits[column.order[size_t((ordinal % count + count) % count)]];
                if (portrait) SDL_RenderTexture(renderer_, portrait, nullptr, &dest);
            }
        }
    }
    static void Clear(Column &column) {
        for (SDL_Texture *texture : column.portraits)
            if (texture) SDL_DestroyTexture(texture);
        column.portraits.clear();
        column.portraitImages.clear();
        column.order.clear();
        column.revealLevel = -1;
        if (column.plain) SDL_DestroyTexture(column.plain);
        column.plain = nullptr;
        column.shape = {};
    }

public:
    MenuFilmStrips() = default;
    MenuFilmStrips(const MenuFilmStrips &) = delete;
    MenuFilmStrips &operator=(const MenuFilmStrips &) = delete;
    ~MenuFilmStrips() { Release(); }
    void Release() {
        Clear(xmen_);
        Clear(streetFighter_);
        ditherPixels_.clear();
        renderer_ = nullptr;
    }
    void Init(SDL_Renderer *renderer) {
        if (!renderer || renderer_ == renderer) return;
        Release();
        renderer_ = renderer;
        LoadColumn(xmen_, "menu/menufilmL.png", "xm", true);
        LoadColumn(streetFighter_, "menu/menufilm.png", "sf", false);
    }
    void Draw(SDL_Renderer *renderer, std::uint64_t now, int reveal) {
        if (!renderer) return;
        Init(renderer);
        ApplyReveal(xmen_, reveal);
        ApplyReveal(streetFighter_, reveal);
        DrawColumn(xmen_, now, reveal);
        DrawColumn(streetFighter_, now, reveal);
    }
};
} // namespace port::xmvsf_home
