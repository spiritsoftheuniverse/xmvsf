// XMVSF 87.461: tiled 50/50 rainbow films with per-panel pan and hue cycling.
#pragma once
#include "xmvsf_home.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>
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
    static constexpr std::uint64_t kFxRefreshMs = 75u;
    // Seven slot textures per column avoid per-frame create/destroy churn.
    static constexpr uint8_t kBayer[16] = {
        0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7
    };
    struct FxCache {
        SDL_Texture *texture = nullptr;
        std::int64_t ordinal = std::numeric_limits<std::int64_t>::min();
        std::uint64_t lastUpdate = 0;
        int reveal = -1;
        bool failed = false;
    };
    struct Column {
        Image shape;
        SDL_Texture *plain = nullptr;
        std::vector<SDL_Texture *> portraits;
        std::vector<Image> portraitImages;
        std::vector<size_t> order; // shuffled only at load; stable across frames
        std::array<FxCache, 7> fx;
        int x = 0;
        int revealLevel = -1;
        bool scrollDown = false;
    };
    SDL_Renderer *renderer_ = nullptr;
    Column xmen_, streetFighter_;
    Image rainbow_;
    std::vector<uint32_t> ditherPixels_, rainbowPixels_;

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
            std::fprintf(stderr, "XMVSF 87.461 film dither upload failed: %s\n",
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
            std::fprintf(stderr, "XMVSF 87.461 menu film missing or invalid: %s\n",
                         shapeFile);
            return;
        }
        column.x = down ? kMargin : kCanvasWidth - kMargin - column.shape.width;
        column.plain = Upload(renderer_, column.shape);
        const Rect safe = SafePortraitArea(column.shape);
        if (safe.w < 1 || safe.h < 1) {
            std::fprintf(stderr, "XMVSF 87.461 portrait safe area unavailable: %s\n",
                         shapeFile);
            return;
        }
        for (int i = 1; i <= 9; ++i) {
            const std::string filename = std::string("menu/characters/") +
                prefix + " (" + std::to_string(i) + ").png";
            const Image person = LoadPng(filename.c_str());
            if (!person.Valid()) {
                std::fprintf(stderr, "XMVSF 87.461 missing portrait: %s\n",
                             filename.c_str());
                continue;
            }
            Image clipped = MaskPortrait(column.shape, person, safe);
            SDL_Texture *texture = Upload(renderer_, clipped);
            if (texture) {
                column.portraitImages.push_back(std::move(clipped));
                column.portraits.push_back(texture);
            } else std::fprintf(stderr, "XMVSF 87.461 portrait upload failed: %s\n",
                                SDL_GetError());
        }
        column.order.resize(column.portraits.size());
        std::iota(column.order.begin(), column.order.end(), size_t(0));
        // Fisher-Yates by std::shuffle. Each loaded character appears once
        // before the cycle repeats; no per-frame random flicker.
        std::mt19937 random(std::random_device{}());
        std::shuffle(column.order.begin(), column.order.end(), random);
        std::fprintf(stderr, "XMVSF 87.461 %s film: %zu shuffled portraits at x=%d\n",
                     prefix, column.portraits.size(), column.x);
    }

    // Stable per-panel motion: never reroll when a slot wraps or F6
    // renders the same frame twice. Distinct hash seeds per XM/SF column.
    static std::uint64_t Hash(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    struct PanelMotion {
        int dx, dy;
        double panSpeed, hueSpeed, huePhase;
    };
    static PanelMotion Motion(bool down, std::int64_t ordinal) {
        constexpr int directions[8][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
        };
        const std::uint64_t seed = Hash(std::uint64_t(ordinal) ^
            (down ? 0xa5a5e7d37fa7c15bULL : 0x3c6ef372fe94f82bULL));
        const int direction = int(seed & 7u);
        return {directions[direction][0], directions[direction][1],
                18.0 + double((seed >> 8) % 33u),
                35.0 + double((seed >> 20) % 76u),
                double((seed >> 37) % 360u)};
    }
    static int Wrap(int x, int size) {
        const int rem = x % size;
        return rem < 0 ? rem + size : rem;
    }

    // Approximate hue rotation with one 3x3 matrix per panel update,
    // rather than doing trigonometry separately for every texture pixel.
    static std::array<float, 9> HueMatrix(double degrees) {
        const double angle = std::fmod(degrees, 360.0) *
                             (3.14159265358979323846 / 180.0);
        const float c = float(std::cos(angle)), s = float(std::sin(angle));
        return {
            .213f + .787f*c - .213f*s, .715f - .715f*c - .715f*s,
            .072f - .072f*c + .928f*s,
            .213f - .213f*c + .143f*s, .715f + .285f*c + .140f*s,
            .072f - .072f*c - .283f*s,
            .213f - .213f*c - .787f*s, .715f - .715f*c + .715f*s,
            .072f + .928f*c + .072f*s
        };
    }

    // Blend red/blue source plate 50/50 with tiled, moving rainbowblobs.
    // Preserve the *source trapezoid alpha* as the output alpha: rainbow
    // never bleeds outside the mask even when its UVs pan.
    bool UpdateRainbow(Column &column, FxCache &cache,
                       std::int64_t ordinal, std::uint64_t now, int reveal) {
        if (!rainbow_.Valid() || !column.shape.Valid() || cache.failed)
            return false;
        if (!cache.texture) {
            cache.texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STREAMING, column.shape.width, column.shape.height);
            if (!cache.texture) {
                cache.failed = true;
                std::fprintf(stderr, "XMVSF 87.461 rainbow texture create: %s\n",
                             SDL_GetError());
                return false;
            }
            SDL_SetTextureBlendMode(cache.texture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(cache.texture, SDL_SCALEMODE_NEAREST);
        }
        // F6's offscreen compositor and the on-screen draw can share
        // identical textures without rebuilding them twice per frame.
        if (cache.ordinal == ordinal && cache.reveal == reveal &&
            now >= cache.lastUpdate && now - cache.lastUpdate < kFxRefreshMs)
            return true;

        const PanelMotion motion = Motion(column.scrollDown, ordinal);
        const double seconds = double(now) / 1000.0;
        const double directionScale = (motion.dx && motion.dy) ? .7071067811865476 : 1.0;
        const double travel = seconds * motion.panSpeed * directionScale;
        const int panX = int(std::floor(std::fmod(travel * motion.dx,
                                                 double(rainbow_.width))));
        const int panY = int(std::floor(std::fmod(travel * motion.dy,
                                                 double(rainbow_.height))));
        const auto hue = HueMatrix(motion.huePhase + seconds * motion.hueSpeed);
        const auto byte = [](float v) {
            return std::uint32_t(std::clamp(int(std::lround(v)), 0, 255));
        };
        const int w = column.shape.width, h = column.shape.height;
        rainbowPixels_.resize(size_t(w) * size_t(h));
        for (int y = 0; y < h; ++y) {
            const int ry = Wrap(y + panY, rainbow_.height);
            for (int x = 0; x < w; ++x) {
                const size_t at = size_t(y) * size_t(w) + size_t(x);
                const std::uint32_t plate = column.shape.pixels[at];
                const unsigned maskAlpha = plate >> 24;
                if (!maskAlpha || kBayer[(y & 3) * 4 + (x & 3)] >= reveal) {
                    rainbowPixels_[at] = 0u;
                    continue;
                }
                const int rx = Wrap(x + panX, rainbow_.width);
                const std::uint32_t blob = rainbow_.pixels[
                    size_t(ry) * size_t(rainbow_.width) + size_t(rx)];
                const float r = float(blob & 255u),
                            g = float((blob >> 8) & 255u),
                            b = float((blob >> 16) & 255u);
                const std::uint32_t rr = byte(hue[0]*r + hue[1]*g + hue[2]*b);
                const std::uint32_t gg = byte(hue[3]*r + hue[4]*g + hue[5]*b);
                const std::uint32_t bb = byte(hue[6]*r + hue[7]*g + hue[8]*b);
                // Full-alpha rainbow -> 128/255 rainbow, 127/255 plate.
                // Partial-alpha rainbow retains proportionally more plate.
                const unsigned rainbowWeight = ((blob >> 24) + 1u) / 2u;
                const unsigned baseWeight = 255u - rainbowWeight;
                const std::uint32_t mixedR =
                    ((plate & 255u)*baseWeight + rr*rainbowWeight + 127u)/255u;
                const std::uint32_t mixedG =
                    (((plate >> 8)&255u)*baseWeight + gg*rainbowWeight + 127u)/255u;
                const std::uint32_t mixedB =
                    (((plate >> 16)&255u)*baseWeight + bb*rainbowWeight + 127u)/255u;
                rainbowPixels_[at] = (maskAlpha << 24) | (mixedB << 16) |
                                     (mixedG << 8) | mixedR;
            }
        }
        if (!SDL_UpdateTexture(cache.texture, nullptr,
                               rainbowPixels_.data(), w*4)) {
            cache.failed = true;
            std::fprintf(stderr, "XMVSF 87.461 rainbow upload failed: %s\n",
                         SDL_GetError());
            return false;
        }
        cache.ordinal = ordinal;
        cache.reveal = reveal;
        cache.lastUpdate = now;
        return true;
    }

    void DrawColumn(Column &column, std::uint64_t now, int reveal) {
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
            const std::int64_t ordinal = std::int64_t(slot) +
                (column.scrollDown ? -cycles : cycles);
            FxCache &cache = column.fx[size_t(slot + 2)];
            if (UpdateRainbow(column, cache, ordinal, now, reveal)) {
                SDL_RenderTexture(renderer_, cache.texture, nullptr, &dest);
            } else if (column.plain) {
                // No rainbow asset? Keep the old red/blue panels and Bayer.
                if (column.plain) SDL_RenderTexture(renderer_, column.plain, nullptr, &dest);
            }
            if (!column.portraits.empty()) {
                const auto count = std::int64_t(column.portraits.size());
                SDL_Texture *portrait =
                    column.portraits[column.order[size_t((ordinal % count + count) % count)]];
                if (portrait) SDL_RenderTexture(renderer_, portrait, nullptr, &dest);
            }
        }
    }
    static void Clear(Column &column) {
        for (FxCache &cache : column.fx) {
            if (cache.texture) SDL_DestroyTexture(cache.texture);
            cache = {};
        }
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
        rainbowPixels_.clear();
        rainbow_ = {};
        renderer_ = nullptr;
    }
    void Init(SDL_Renderer *renderer) {
        if (!renderer || renderer_ == renderer) return;
        Release();
        renderer_ = renderer;
        rainbow_ = LoadPng("menu/rainbowblobs.png");
        if (!rainbow_.Valid()) {
            std::fprintf(stderr, "XMVSF 87.461: _art/menu/rainbowblobs.png "
                                 "missing; using red/blue film only\n");
        } else {
            std::fprintf(stderr, "XMVSF 87.461: rainbowblobs %dx%d loaded\n",
                         rainbow_.width, rainbow_.height);
        }
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
