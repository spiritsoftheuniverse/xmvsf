// XMVSF 87.456: 96px-separated X-Men / SF menu films with clean alpha masks.
#pragma once
#include "xmvsf_home.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

namespace port::xmvsf_home {
class MenuFilmStrips {
    static constexpr int kCanvasWidth = 960, kCanvasHeight = 540;
    static constexpr int kMargin = 16, kGap = 96, kPortraitTop = 78;
    static constexpr double kSpeedPixelsPerSecond = 10.0;
    struct Column {
        Image shape;
        SDL_Texture *plain = nullptr;
        std::vector<SDL_Texture *> portraits;
        int x = 0;
        bool scrollDown = false;
    };
    SDL_Renderer *renderer_ = nullptr;
    Column xmen_, streetFighter_;

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

    // Mask only the portrait. Render the original red/blue film underneath
    // as a separate texture so transparent character regions retain color.
    // Film alpha is authoritative: no portrait pixel may escape the trapezoid.
    // Art begins at y=78 in the absolute 260x453 source-image coordinates.
    static Image MaskPortrait(const Image &film, const Image &portrait) {
        Image out;
        if (!film.Valid() || !portrait.Valid() || film.height <= kPortraitTop)
            return out;
        const int w = film.width;
        const int h = film.height;
        const int areaH = h - kPortraitTop;
        out.width = w;
        out.height = h;
        out.pixels.assign(size_t(w) * size_t(h), 0u);
        const double scale = std::max(double(w) / portrait.width,
                                      double(areaH) / portrait.height);
        for (int y = kPortraitTop; y < h; ++y) {
            const int sy = std::clamp(int(std::floor(
                (double(y - kPortraitTop) + 0.5 - double(areaH) * 0.5) / scale +
                double(portrait.height) * 0.5)), 0, portrait.height - 1);
            for (int x = 0; x < w; ++x) {
                const size_t at = size_t(y) * size_t(w) + size_t(x);
                const unsigned maskAlpha = film.pixels[at] >> 24;
                if (maskAlpha == 0u) continue;
                const int sx = std::clamp(int(std::floor(
                    (double(x) + 0.5 - double(w) * 0.5) / scale +
                    double(portrait.width) * 0.5)), 0, portrait.width - 1);
                const uint32_t person = portrait.pixels[
                    size_t(sy) * size_t(portrait.width) + size_t(sx)];
                const unsigned portraitAlpha = person >> 24;
                if (portraitAlpha == 0u) continue;
                const unsigned clippedAlpha =
                    (maskAlpha * portraitAlpha + 127u) / 255u;
                // Image::pixels is packed 0xAABBGGRR. Preserve RGB exactly;
                // manipulating channels as 0xAARRGGBB swaps red and blue.
                out.pixels[at] = (person & 0x00ffffffu) |
                                 (uint32_t(clippedAlpha) << 24);
            }
        }
        return out;
    }

    void LoadColumn(Column &column, const char *shapeFile,
                    const char *prefix, bool down) {
        column.scrollDown = down;
        column.shape = LoadPng(shapeFile);
        if (!column.shape.Valid()) {
            std::fprintf(stderr, "XMVSF 87.455 menu film missing or invalid: %s\n",
                         shapeFile);
            return;
        }
        column.x = down ? kMargin : kCanvasWidth - kMargin - column.shape.width;
        column.plain = Upload(renderer_, column.shape);
        for (int i = 1; i <= 9; ++i) {
            const std::string filename = std::string("menu/characters/") +
                prefix + " (" + std::to_string(i) + ").png";
            const Image person = LoadPng(filename.c_str());
            if (!person.Valid()) {
                std::fprintf(stderr, "XMVSF 87.455 missing portrait: %s\n",
                             filename.c_str());
                continue;
            }
            SDL_Texture *texture = Upload(renderer_, MaskPortrait(column.shape, person));
            if (texture) column.portraits.push_back(texture);
            else std::fprintf(stderr, "XMVSF 87.455 portrait upload failed: %s\n",
                              SDL_GetError());
        }
        std::fprintf(stderr, "XMVSF 87.455 %s film: %zu portraits at x=%d\n",
                     prefix, column.portraits.size(), column.x);
    }

    void DrawColumn(const Column &column, std::uint64_t now) const {
        if (!column.shape.Valid()) return;
        // Positive gap, never an overlap: 453px film + 96px blank = 549px pitch.
        const int step = column.shape.height + kGap;
        const double distance = double(now) * (kSpeedPixelsPerSecond / 1000.0);
        const std::int64_t cycles = std::int64_t(std::floor(distance / double(step)));
        const float phase = float(std::fmod(distance, double(step)));
        for (int slot = -1; slot <= 4; ++slot) {
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
                    column.portraits[size_t((ordinal % count + count) % count)];
                if (portrait) SDL_RenderTexture(renderer_, portrait, nullptr, &dest);
            }
        }
    }
    static void Clear(Column &column) {
        for (SDL_Texture *texture : column.portraits)
            if (texture) SDL_DestroyTexture(texture);
        column.portraits.clear();
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
        renderer_ = nullptr;
    }
    void Init(SDL_Renderer *renderer) {
        if (!renderer || renderer_ == renderer) return;
        Release();
        renderer_ = renderer;
        LoadColumn(xmen_, "menu/menufilmL.png", "xm", true);
        LoadColumn(streetFighter_, "menu/menufilm.png", "sf", false);
    }
    void Draw(SDL_Renderer *renderer, std::uint64_t now) {
        if (!renderer) return;
        Init(renderer);
        DrawColumn(xmen_, now);
        DrawColumn(streetFighter_, now);
    }
};
} // namespace port::xmvsf_home
