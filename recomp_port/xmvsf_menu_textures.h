// 87.441 — source-texture menu compositor (SDL3 equivalent of our SDL2 menu).
// No 960x540 CPU Blit/Cover pass, and no full-screen texture upload per frame.
// Source artwork stays in xmvsf_home::Menu; only layers are uploaded to SDL.
#pragma once
#include "xmvsf_home.h"
#include "xmvsf_options.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace port::xmvsf_home {

class TexturedMenu {
    Menu &menu_;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *background_ = nullptr;
    SDL_Texture *logo_ = nullptr;
    SDL_Texture *logoII_ = nullptr;
    SDL_Texture *largeFont_ = nullptr;
    SDL_Texture *smallFont_ = nullptr;
    int gifW_ = 0, gifH_ = 0;
    int gifFrame_ = -1, gifHue_ = -1;
    bool warned_ = false;

    static SDL_Texture *MakeTexture(SDL_Renderer *r, const Image &img, bool streaming = false) {
        if (!r || !img.Valid()) return nullptr;
        SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888,
            streaming ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC,
            img.width, img.height);
        if (!tex) return nullptr;
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        if (!SDL_UpdateTexture(tex, nullptr, img.pixels.data(), img.width * 4)) {
            SDL_DestroyTexture(tex);
            return nullptr;
        }
        return tex;
    }

    static void Tint(SDL_Texture *texture, int brightness) {
        if (!texture) return;
        const Uint8 c = Uint8(std::clamp(brightness, 0, 255));
        SDL_SetTextureColorMod(texture, c, c, c);
    }

    static void DrawTexture(SDL_Renderer *r, SDL_Texture *texture, Rect src, SDL_FRect dest) {
        if (!r || !texture || src.w < 1 || src.h < 1 || dest.w < 1 || dest.h < 1) return;
        SDL_FRect from{float(src.x), float(src.y), float(src.w), float(src.h)};
        SDL_RenderTexture(r, texture, &from, &dest);
    }

    bool Init(SDL_Renderer *r) {
        if (renderer_ == r) return largeFont_ && smallFont_ && logo_;
        Release();
        renderer_ = r;
        logo_ = MakeTexture(r, menu_.logo_);
        logoII_ = MakeTexture(r, menu_.logoII_);
        largeFont_ = MakeTexture(r, menu_.large_.sheet);
        smallFont_ = MakeTexture(r, menu_.small_.sheet);
        if (!menu_.backdrop_.frames.empty()) {
            const Image &first = menu_.backdrop_.frames.front();
            background_ = MakeTexture(r, first, true);
            gifW_ = first.width;
            gifH_ = first.height;
            gifFrame_ = -1;
            gifHue_ = -1;
            Tint(background_, 77);
        }
        if (!logo_ || !largeFont_ || !smallFont_) {
            if (!warned_) {
                std::fprintf(stderr,
                    "XMVSF 87.441: texture menu unavailable (%s); CPU compatibility fallback\n",
                    SDL_GetError());
                warned_ = true;
            }
            return false;
        }
        std::fprintf(stderr,
            "XMVSF 87.441: hardware menu textures ready (logo, glyph atlases, GIF %dx%d)\n",
            gifW_, gifH_);
        return true;
    }

    void GlyphText(SDL_Renderer *r, const Font &font, SDL_Texture *texture,
                   const char *s, int anchorX, int top, float scale,
                   int brightness, int alignment) {
        if (!r || !texture || !s) return;
        int x = anchorX;
        if (alignment == 1) x -= int(font.Measure(s) * scale / 2 + .5f);
        else if (alignment == 2) x -= int(font.Measure(s) * scale + .5f);
        Tint(texture, brightness);
        int pen = 0;
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; ++p) {
            const unsigned ch = *p < 128 ? *p : 32;
            const Font::Glyph &glyph = font.glyphs[ch];
            if (ch >= 33 && ch <= 126 && glyph.rect.w > 0) {
                const SDL_FRect dest{
                    float(x + int(pen * scale + .5f)), float(top),
                    float(std::max(1, int(glyph.rect.w * scale + .5f))),
                    float(std::max(1, int(glyph.rect.h * scale + .5f)))
                };
                DrawTexture(r, texture, glyph.rect, dest);
            }
            pen += glyph.advance;
        }
        Tint(texture, 255);
    }

    void DrawLogo(SDL_Renderer *r, uint64_t now) {
        const Rect crop = VisibleBounds(menu_.logo_);
        if (crop.w < 1 || crop.h < 1) return;
        // The original Stage14 MainLayout target — no forced new aspect ratio.
        const float scale = std::min(544.0f / crop.w, 289.0f / crop.h);
        const int drawnW = int(crop.w * scale + .5f);
        const int drawnH = int(crop.h * scale + .5f);
        const int x = 208 + (544 - drawnW) / 2;
        const int y = 51 + (289 - drawnH) / 2;
        const int light = std::clamp(246 + int(9.0 * std::sin(
                         double(now % 1600) * 6.28318530718 / 1600.0)), 220, 255);
        Tint(logo_, light);
        DrawTexture(r, logo_, crop,
                    {float(x), float(y), float(drawnW), float(drawnH)});
        if (logoII_ && menu_.logoII_.Valid()) {
            // Shared 400x300 original art canvas, no independent auto-fit.
            const int fullX = x - int(crop.x * scale + .5f);
            const int fullY = y - int(crop.y * scale + .5f);
            Tint(logoII_, light);
            DrawTexture(r, logoII_,
                {0, 0, menu_.logoII_.width, menu_.logoII_.height},
                {float(fullX), float(fullY),
                 float(int(menu_.logoII_.width * scale + .5f)),
                 float(int(menu_.logoII_.height * scale + .5f))});
        }
    }

    void DrawBackground(SDL_Renderer *r, uint64_t now) {
        if (!background_ || menu_.backdrop_.frames.empty()) return;
        const Image &frame = menu_.backdrop_.At(now);
        if (!frame.Valid()) return;
        const int frameNo = menu_.backdrop_.shownFrame;
        const int hueNo = menu_.backdrop_.shownHue;
        if (frameNo != gifFrame_ || hueNo != gifHue_) {
            gifFrame_ = frameNo;
            gifHue_ = hueNo;
            // Only the current original GIF frame/hue is uploaded at 12.5Hz,
            // never the entire 960x540 composed menu.
            SDL_UpdateTexture(background_, nullptr,
                              frame.pixels.data(), frame.width * 4);
        }
        const double cover = std::max(960.0 / frame.width, 540.0 / frame.height);
        const int cropW = std::clamp(int(960.0 / cover + .5), 1, frame.width);
        const int cropH = std::clamp(int(540.0 / cover + .5), 1, frame.height);
        DrawTexture(r, background_,
            {(frame.width - cropW) / 2, (frame.height - cropH) / 2, cropW, cropH},
            {0, 0, 960, 540});
    }

public:
    explicit TexturedMenu(Menu &menu) : menu_(menu) {}
    TexturedMenu(const TexturedMenu &) = delete;
    TexturedMenu &operator=(const TexturedMenu &) = delete;
    ~TexturedMenu() { Release(); }
    bool Renderable(SDL_Renderer *r, bool classic) const {
        return classic && r && menu_.Visible() && std::getenv("XMVSF_MENU_CPU_FALLBACK") == nullptr;
    }

    void Release() {
        for (SDL_Texture *texture : {background_, logo_, logoII_, largeFont_, smallFont_})
            if (texture) SDL_DestroyTexture(texture);
        background_ = logo_ = logoII_ = largeFont_ = smallFont_ = nullptr;
        renderer_ = nullptr;
        gifW_ = gifH_ = 0;
        gifFrame_ = gifHue_ = -1;
    }

    // Original OPTIONS page layout; draws source bitmap-font textures,
    // never a CPU-composited frame. Called while the normal home menu hides.
    bool RenderOptions(SDL_Renderer *renderer, uint64_t now, const OptionsPanel &options,
                       const recomp::Bindings &bindings, const BoundTable &table,
                       const PadOwners &owners) {
        if (!renderer || !options.Active() || !Init(renderer)) return false;
        SDL_SetRenderDrawColor(renderer, 8, 16, 36, 255);
        SDL_RenderClear(renderer);
        DrawBackground(renderer, now);

        const auto center = [&](const char *str, int x, int y,
                                float scale, int bright, bool large = false) {
            GlyphText(renderer, large ? menu_.large_ : menu_.small_,
                      large ? largeFont_ : smallFont_, str, x, y,
                      scale, bright, 1);
        };
        const auto left = [&](const char *str, int x, int y, float scale, int bright) {
            GlyphText(renderer, menu_.small_, smallFont_, str, x, y,
                      scale, bright, 0);
        };
        center("OPTIONS", 480, 23, 2.8f, 190, true);
        left("LB/Q", 88, 76, 1.8f, 138);
        static constexpr const char *tabs[] = {"GAME", "GRAPHICS", "CONTROLLER"};
        constexpr int tabX[] = {292, 472, 677};
        for (int i = 0; i < 3; ++i) {
            const int brightness = options.Tab() == i ? 255 : 86;
            center(tabs[i], tabX[i], 76, 2.0f, brightness);
            if (i == options.Tab()) {
                SDL_SetRenderDrawColor(renderer, 149, 111, 69, 255);
                const SDL_FRect underline{float(tabX[i] - 54), 99.0f, 108.0f, 2.0f};
                SDL_RenderFillRect(renderer, &underline);
            }
        }
        center("RB/E", 863, 76, 1.8f, 138);
        if (options.Tab() == 0) {
            for (int i = 0; i < 7; ++i) {
                const std::string label = options.GameLabel(i);
                const bool unavailable = i >= 1 && i <= 3;
                const int brightness = unavailable ? 96 : i == options.GameRow() ? 255 : 92;
                center(label.c_str(), 480, 142 + i * 45, 1.85f, brightness);
            }
            center("BOARD SETTINGS TAKE EFFECT ON RESTART", 480, 480, 1.35f, 131);
        } else if (options.Tab() == 1) {
            for (int i = 0; i < 8; ++i) {
                const std::string label = options.GraphicsLabel(i);
                center(label.c_str(), 480, 140 + i * 40, 1.75f,
                       i == options.GraphicsRow() ? 255 : 90);
            }
            center("CTRL + / - RESIZE   CTRL + BACKSPACE MAXIMIZE", 480, 480, 1.3f, 135);
        } else {
            center("LEFT / RIGHT PAGE", 480, 109, 1.7f, 145);
            const int visible = owners.VisibleActions(options.Player());
            // Direct port of Stage14's dynamic centered input column:
            // 16-pixel device caption, then exactly 12 physical pixels to
            // the binding sheet, then 18 pixels between binding rows.
            const int groupHeight = 16 + 12 + 16 + (visible - 1) * 18;
            const int deviceY = 156 + (296 - groupHeight) / 2;
            const int inputY = deviceY + 16 + 12;
            const std::string device = options.DeviceLabel(owners);
            center(device.substr(0, 55).c_str(), 480, deviceY, 1.65f,
                   options.ControlRow() == 0 ? 255 : 120);
            const float scale = 1.64f;
            int widest = 0;
            std::array<std::string, 14> labels;
            for (int i = 0; i < visible; ++i) {
                labels[size_t(i)] = options.BindingLabel(i, bindings, table, owners);
                widest = std::max(widest, int(menu_.small_.Measure(labels[size_t(i)].c_str()) * scale));
            }
            const int startX = (960 - widest) / 2;
            for (int i = 0; i < visible; ++i) {
                const int intensity = options.ControlRow() == i + 1 ? 255 : 92;
                left(labels[size_t(i)].c_str(), startX, inputY + i * 18,
                     scale, intensity);
            }
            center("<", 275, 490, 1.65f, 138);
            center("PLAYER 1", 375, 490, 1.6f, options.Player() == 0 ? 255 : 105);
            center("PLAYER 2", 585, 490, 1.6f, options.Player() == 1 ? 255 : 105);
            center(">", 685, 490, 1.65f, 138);
        }
        const char *notice = options.Message();
        if (*notice) {
            center(notice, 480, 462, 1.45f, options.Capturing() ? 255 : 173);
        }
        center("A SELECT   B / ESC BACK", 480, 511, 1.5f, 176);
        left("TEST 87.447", 834, 523, 1.05f, 160);
        SDL_RenderPresent(renderer);
        return true;
    }

    bool Render(SDL_Renderer *renderer, uint64_t now) {
        if (!renderer || !menu_.Visible() || !Init(renderer)) return false;
        SDL_SetRenderDrawColor(renderer, 8, 16, 36, 255);
        SDL_RenderClear(renderer);
        DrawBackground(renderer, now);
        DrawLogo(renderer, now);
        static constexpr std::array<const char *, 4> rows{
            "ARCADE", "TRAINING", "VERSUS", "OPTIONS"
        };
        for (int i = 0; i < 4; ++i) {
            GlyphText(renderer, menu_.large_, largeFont_, rows[size_t(i)],
                      489, 347 + i * 31, 1.35f,
                      menu_.selection_ == i ? 255 : 80, 1);
        }
        GlyphText(renderer, menu_.small_, smallFont_,
                  "A SELECT   B / ESC BACK", 489, 495, 1.375f, 235, 1);
        GlyphText(renderer, menu_.small_, smallFont_,
                  "ESC TO RETURN", 18, 522, 1.08f, 185, 0);
        GlyphText(renderer, menu_.small_, smallFont_,
                  "TEST 87.442", 942, 522, 1.08f, 185, 2);
        SDL_RenderPresent(renderer);
        return true;
    }
};

} // namespace port::xmvsf_home
