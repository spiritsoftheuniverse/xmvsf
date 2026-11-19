// XMVSF 87.455: cached, alpha-masked X-Men / Street Fighter menu film columns.
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
    static constexpr int kMargin = 16, kOverlap = 96, kPortraitTop = 78;
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

    // The film itself is opaque within the trapezoid. Keep its pixels behind
    // transparent portrait cutouts and preserve its alpha outside the shape.
    // Portrait art begins at y=78 on the ORIGINAL 260x453 panel image.
    static Image Compose(const Image &film, const Image &portrait) {
        Image out = film;
        if (!film.Valid() || !portrait.Valid() || film.height <= kPortraitTop)
            return out;
        const int w = film.width, h = film.height, areaH = h - kPortraitTop;
        const double scale = std::max(double(w) / portrait.width,
                                      double(areaH) / portrait.height);
        for (int y = kPortraitTop; y < h; ++y) {
            const int sy = std::clamp(int(std::floor(
                (double(y - kPortraitTop) + 0.5 - double(areaH) * 0.5) / scale +
                double(portrait.height) * 0.5)), 0, portrait.height - 1);
            for (int x = 0; x < w; ++x) {
                const size_t at = size_t(y) * size_t(w) + size_t(x);
                const uint32_t backing = film.pixels[at];
                const unsigned filmAlpha = backing >> 24;
                if (!filmAlpha) continue;
                const int sx = std::clamp(int(std::floor(
                    (double(x) + 0.5 - double(w) * 0.5) / scale +
                    double(portrait.width) * 0.5)), 0, portrait.width - 1);
                const uint32_t person = portrait.pixels[
                    size_t(sy) * size_t(portrait.width) + size_t(sx)];
                const unsigned a = person >> 24;
                if (!a) continue;
                uint32_t result = uint32_t(filmAlpha) << 24;
                for (int shift : {0, 8, 16}) {
                    const unsigned b = (backing >> shift) & 255u;
                    const unsigned p = (person >> shift) & 255u;
                    result |= uint32_t((b * (255u - a) + p * a + 127u) / 255u) << shift;
                }
                out.pixels[at] = result;
            }
        }
        return out;
    }

    void LoadColumn(Column &column, const char *shapeFile,
                    const char *prefix, bool down) {
        column.scrollDown = down;
        column.shape = LoadPng(shapeFile);
        if (!column.shape.Valid() || column.shape.height <= kOverlap) {
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
            SDL_Texture *texture = Upload(renderer_, Compose(column.shape, person));
            if (texture) column.portraits.push_back(texture);
            else std::fprintf(stderr, "XMVSF 87.455 portrait upload failed: %s\n",
                              SDL_GetError());
        }
        std::fprintf(stderr, "XMVSF 87.455 %s film: %zu portraits at x=%d\n",
                     prefix, column.portraits.size(), column.x);
    }

    void DrawColumn(const Column &column, std::uint64_t now) const {
        if (!column.shape.Valid()) return;
        const int step = column.shape.height - kOverlap;
        if (step <= 0) return;
        const double distance = double(now) * (kSpeedPixelsPerSecond / 1000.0);
        const std::int64_t cycles = std::int64_t(std::floor(distance / step));
        const float phase = float(std::fmod(distance, double(step)));
        for (int slot = -1; slot <= 5; ++slot) {
            const int y = int(std::floor(float((slot - 1) * step) +
                                  (column.scrollDown ? phase : -phase)));
            if (y >= kCanvasHeight || y + column.shape.height <= 0) continue;
            SDL_Texture *texture = column.plain;
            if (!column.portraits.empty()) {
                const std::int64_t ordinal = std::int64_t(slot) +
                    (column.scrollDown ? -cycles : cycles);
                const auto count = std::int64_t(column.portraits.size());
                texture = column.portraits[size_t((ordinal % count + count) % count)];
            }
            if (!texture) continue;
            const SDL_FRect dest{float(column.x), float(y),
                                 float(column.shape.width), float(column.shape.height)};
            SDL_RenderTexture(renderer_, texture, nullptr, &dest);
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
