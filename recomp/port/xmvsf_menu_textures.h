// 87.441 — source-texture menu compositor (SDL3 equivalent of our SDL2 menu).
// No 960x540 CPU Blit/Cover pass, and no full-screen texture upload per frame.
// Source artwork stays in xmvsf_home::Menu; only layers are uploaded to SDL.
#pragma once
#include "xmvsf_home.h"
#include "xmvsf_menu_film.h"
#include "xmvsf_options.h"
#include "xmvsf_ii_strikes.h"
#include "xmvsf_palette_fx.h"
#include "xmvsf_intro_timeline.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace port::xmvsf_home {

class TexturedMenu {
    Menu &menu_;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *background_ = nullptr;
    SDL_Texture *logo_ = nullptr;
    SDL_Texture *logoII_ = nullptr;
    SDL_Texture *largeFont_ = nullptr;
    SDL_Texture *smallFont_ = nullptr;
    SDL_Texture *revealLarge_ = nullptr, *revealSmall_ = nullptr;
    XM87235II::Painter iiPainter_;
    MenuFilmStrips films_;
    std::vector<Uint8> originalLogoRGBA_, pulsedLogo_, introLogo_;
    std::vector<Uint8> backgroundOriginalRGBA_, introBackground_;
    std::vector<uint32_t> largeRevealPixels_, smallRevealPixels_;
    std::uint64_t lastLogoTick_ = std::numeric_limits<std::uint64_t>::max();
    int introLogoStep_ = -1, introBackgroundStep_ = -1, menuRevealStep_ = -1;
    bool warnedLogoUpdate_ = false;
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
        logo_ = MakeTexture(r, menu_.logo_, true);
        if (menu_.logo_.Valid()) {
            originalLogoRGBA_.resize(menu_.logo_.pixels.size() * 4u);
            std::memcpy(originalLogoRGBA_.data(), menu_.logo_.pixels.data(), originalLogoRGBA_.size());
        }
        iiPainter_.load(r);
        logoII_ = MakeTexture(r, menu_.logoII_);
        largeFont_ = MakeTexture(r, menu_.large_.sheet);
        smallFont_ = MakeTexture(r, menu_.small_.sheet);
        revealLarge_ = MakeTexture(r, menu_.large_.sheet, true);
        revealSmall_ = MakeTexture(r, menu_.small_.sheet, true);
        if (!menu_.backdrop_.frames.empty()) {
            const Image &first = menu_.backdrop_.frames.front();
            background_ = MakeTexture(r, first, true);
            gifW_ = first.width;
            gifH_ = first.height;
            gifFrame_ = -1;
            gifHue_ = -1;
            Tint(background_, 77);
        }
        films_.Init(r);
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


    void UpdateLogo(std::uint64_t now, float introRemaining) {
        if (!logo_ || originalLogoRGBA_.empty()) return;
        const int step = std::clamp(int(introRemaining * 16.0f + 0.5f), 0, 16);
        if (lastLogoTick_ != std::numeric_limits<std::uint64_t>::max() &&
            now >= lastLogoTick_ && now - lastLogoTick_ < 80u &&
            step == introLogoStep_) return;
        XMMenuColor::logoPalettePulse(originalLogoRGBA_, pulsedLogo_,
            menu_.logo_.width, menu_.logo_.height, static_cast<Uint32>(now));
        const std::vector<Uint8> *pixels = &pulsedLogo_;
        if (introRemaining > 0.0f) {
            XMMenuColor::introLogoDither(pulsedLogo_, introLogo_,
                menu_.logo_.width, menu_.logo_.height,
                static_cast<Uint32>(now), introRemaining);
            pixels = &introLogo_;
        }
        if (!SDL_UpdateTexture(logo_, nullptr, pixels->data(), menu_.logo_.width * 4)) {
            if (!warnedLogoUpdate_) {
                std::fprintf(stderr,"XMVSF 87.454 logo FX upload failed: %s\n",SDL_GetError());
                warnedLogoUpdate_ = true;
            }
            return;
        }
        lastLogoTick_ = now;
        introLogoStep_ = step;
    }
    // Base and II artwork share one source canvas and a combined optical center.
    void DrawLogo(SDL_Renderer *r, std::uint64_t now, std::uint32_t elapsed,
                  bool cinematic) {
        const Rect crop = VisibleBounds(menu_.logo_);
        if (crop.w < 1 || crop.h < 1) return;
        const float scale = std::min(544.0f / crop.w, 289.0f / crop.h);
        const int drawnW = int(crop.w * scale + .5f);
        const int drawnH = int(crop.h * scale + .5f);
        int x = 208 + (544 - drawnW) / 2;
        const int y = 51 + (289 - drawnH) / 2;
        const SDL_Rect iiBounds = iiPainter_.FinalBounds();
        if (iiBounds.w > 0 && iiBounds.h > 0) {
            const int left = std::min(crop.x, iiBounds.x);
            const int right = std::max(crop.x + crop.w, iiBounds.x + iiBounds.w);
            const double baseCenter = double(crop.x) + double(crop.w) / 2.0;
            const double combinedCenter = double(left + right) / 2.0;
            x += int(std::round((baseCenter - combinedCenter) * scale));
        }
        const float remaining = cinematic ? XM87235Intro::logoStrength(elapsed) : 0.0f;
        UpdateLogo(now, remaining);
        Tint(logo_, 255);
        DrawTexture(r, logo_, crop,{float(x),float(y),float(drawnW),float(drawnH)});
        const int fullX = x - int(crop.x * scale + .5f);
        const int fullY = y - int(crop.y * scale + .5f);
        const SDL_Rect canvas{fullX,fullY,
            std::max(1,int(menu_.logo_.width * scale + .5f)),
            std::max(1,int(menu_.logo_.height * scale + .5f))};
        if (iiBounds.w > 0 && iiBounds.h > 0) {
            iiPainter_.draw(r, now, elapsed, cinematic, canvas);
        } else if (logoII_ && menu_.logoII_.Valid() &&
                   (!cinematic || elapsed >= XM87235Intro::StrikeStart +
                                              XM87235Intro::StrikeDurationMs)) {
            DrawTexture(r, logoII_,
                {0,0,menu_.logoII_.width,menu_.logoII_.height},
                {float(fullX),float(fullY),
                 float(int(menu_.logoII_.width * scale + .5f)),
                 float(int(menu_.logoII_.height * scale + .5f))});
        }
    }
    void RevealFonts(int level) {
        level = std::clamp(level, 0, 16);
        if (level == menuRevealStep_) return;
        menuRevealStep_ = level;
        constexpr Uint8 bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
        const auto update = [&](const Image &source, SDL_Texture *texture,
                                std::vector<uint32_t> &buffer) {
            if (!texture || !source.Valid()) return;
            buffer = source.pixels;
            for (int y = 0; y < source.height; ++y)
                for (int x = 0; x < source.width; ++x)
                    if (bayer[(y & 3) * 4 + (x & 3)] >= level)
                        buffer[size_t(y) * source.width + x] &= 0x00ffffffu;
            if (!SDL_UpdateTexture(texture,nullptr,buffer.data(),source.width*4))
                std::fprintf(stderr,"XMVSF 87.454 font FX upload failed: %s\n",SDL_GetError());
        };
        update(menu_.large_.sheet,revealLarge_,largeRevealPixels_);
        update(menu_.small_.sheet,revealSmall_,smallRevealPixels_);
    }

    void DrawBackground(SDL_Renderer *r, std::uint64_t now, float introStrength = 0.0f) {
        if (!background_ || menu_.backdrop_.frames.empty()) return;
        const Image &normalFrame = menu_.backdrop_.At(now);
        if (!normalFrame.Valid()) return;
        const int frameNo = menu_.backdrop_.shownFrame;
        const int hueNo = menu_.backdrop_.shownHue;
        const int step = std::clamp(int(introStrength*16.0f+.5f),0,16);
        if (frameNo != gifFrame_ || hueNo != gifHue_ || step != introBackgroundStep_) {
            gifFrame_ = frameNo;
            gifHue_ = hueNo;
            introBackgroundStep_ = step;
            if (introStrength > 0.0f && frameNo >= 0 &&
                size_t(frameNo) < menu_.backdrop_.frames.size()) {
                const Image &original = menu_.backdrop_.frames[size_t(frameNo)];
                backgroundOriginalRGBA_.resize(original.pixels.size()*4u);
                std::memcpy(backgroundOriginalRGBA_.data(),original.pixels.data(),
                            backgroundOriginalRGBA_.size());
                XMMenuColor::backgroundHue(backgroundOriginalRGBA_,introBackground_,
                    static_cast<Uint32>(now),introStrength,original.width);
                SDL_UpdateTexture(background_,nullptr,introBackground_.data(),original.width*4);
            } else {
                SDL_UpdateTexture(background_,nullptr,normalFrame.pixels.data(),
                                  normalFrame.width*4);
            }
        }
        Tint(background_,int(77.0f+178.0f*introStrength+.5f));
        const double cover=std::max(960.0/normalFrame.width,540.0/normalFrame.height);
        const int cropW=std::clamp(int(960.0/cover+.5),1,normalFrame.width);
        const int cropH=std::clamp(int(540.0/cover+.5),1,normalFrame.height);
        DrawTexture(r,background_,
            {(normalFrame.width-cropW)/2,(normalFrame.height-cropH)/2,cropW,cropH},
            {0,0,960,540});
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
        films_.Release();
        iiPainter_.release();
        for (SDL_Texture *texture : {background_, logo_, logoII_, largeFont_, smallFont_, revealLarge_, revealSmall_})
            if (texture) SDL_DestroyTexture(texture);
        background_ = logo_ = logoII_ = largeFont_ = smallFont_ = nullptr;
        revealLarge_ = revealSmall_ = nullptr;
        originalLogoRGBA_.clear(); pulsedLogo_.clear(); introLogo_.clear();
        backgroundOriginalRGBA_.clear(); introBackground_.clear();
        largeRevealPixels_.clear(); smallRevealPixels_.clear();
        lastLogoTick_ = std::numeric_limits<std::uint64_t>::max();
        introLogoStep_ = introBackgroundStep_ = menuRevealStep_ = -1;
        warnedLogoUpdate_ = false;
        renderer_ = nullptr;
        gifW_ = gifH_ = 0;
        gifFrame_ = gifHue_ = -1;
    }

    // Original OPTIONS page layout; draws source bitmap-font textures,
    // never a CPU-composited frame. Called while the normal home menu hides.
    bool RenderOptions(SDL_Renderer *renderer, uint64_t now, const OptionsPanel &options,
                       const recomp::Bindings &bindings, const BoundTable &table,
                       const PadOwners &owners, bool present = true) {
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
        left("TEST 87.464", 834, 523, 1.05f, 160);
        if (present) SDL_RenderPresent(renderer);
        return true;
    }

    bool Render(SDL_Renderer *renderer, uint64_t now, bool present = true) {
        if (!renderer || !menu_.Visible() || !Init(renderer)) return false;
        const auto &clock = XM87235Intro::State();
        const std::uint32_t elapsed = clock.Elapsed(now);
        const bool cinematic = clock.started && elapsed < XM87235Intro::TotalMs;
        const float backgroundStrength = clock.started ?
            XM87235Intro::backgroundStrength(elapsed) : 0.0f;
        const auto channel = [](float v) -> Uint8 {
            return Uint8(std::clamp(int(v+.5f),0,255));
        };
        SDL_SetRenderDrawColor(renderer,
            channel(8.0f+247.0f*backgroundStrength),
            channel(16.0f+239.0f*backgroundStrength),
            channel(36.0f+219.0f*backgroundStrength),255);
        SDL_RenderClear(renderer);
        DrawBackground(renderer,now,backgroundStrength);
        // The text finishes its 500ms dither at TotalMs. Only then
        // reveal the panels, using the same 4x4 source-pixel Bayer mask.
        const int filmReveal = !clock.started ? 16 :
            std::clamp(int(XM87235Intro::progress(
                elapsed, XM87235Intro::TotalMs, XM87235Intro::MenuMs) * 16.0f), 0, 16);
        films_.Draw(renderer, now, filmReveal);
        if (elapsed >= XM87235Intro::LogoStart) DrawLogo(renderer,now,elapsed,cinematic);
        if (elapsed < XM87235Intro::MenuStart) {
            if (present) SDL_RenderPresent(renderer);
            return true;
        }
        const int reveal = cinematic ?
            std::clamp(int(XM87235Intro::menuReveal(elapsed)*16.0f),0,16) : 16;
        if (cinematic) RevealFonts(reveal);
        SDL_Texture *mainFont = cinematic && revealLarge_ ? revealLarge_ : largeFont_;
        SDL_Texture *captionFont = cinematic && revealSmall_ ? revealSmall_ : smallFont_;
        static constexpr std::array<const char *, 4> rows{
            "ARCADE", "TRAINING", "VERSUS", "OPTIONS"
        };
        for (int i = 0; i < 4; ++i) {
            GlyphText(renderer, menu_.large_, mainFont, rows[size_t(i)],
                      489, 347 + i * 31, 1.35f,
                      menu_.selection_ == i ? 255 : 80, 1);
        }
        GlyphText(renderer, menu_.small_, captionFont,
                  "A SELECT   B / ESC BACK", 489, 495, 1.375f, 235, 1);
        GlyphText(renderer, menu_.small_, captionFont,
                  "ESC TO RETURN", 18, 522, 1.08f, 185, 0);
        GlyphText(renderer, menu_.small_, captionFont,
                  "TEST 87.464", 942, 522, 1.08f, 185, 2);
        if (present) SDL_RenderPresent(renderer);
        return true;
    }
};

} // namespace port::xmvsf_home
