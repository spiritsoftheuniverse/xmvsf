// XMVSF native 384x224 character-select composition.
// The nine 384x224 PNGs are coordinate maps, not manually rebuilt UI.
// Canvas is composed at CPS-2 resolution and presented through the existing
// Presenter::Show path (including F6, F10, reveal overlay and aspect scaling).
#pragma once
#include "xmvsf_home.h"
#include "xmvsf_character_select_backdrop.h"
#include "xmvsf_character_select_layout.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace port::xmvsf_home {
class CharacterSelectScreen {
    CharacterSelectBackdrop background_;
    CharacterSelectPortraitBackdrop portraitVideo_{"menu/characterselect/portraitbackground.fdb"};
    Image characters_, grid_, p1Cursor_, p2Cursor_, maskL_, maskR_, stageBrace_;
    std::array<Image, 18> portraits_{};
    std::array<Image, 18> stageThumbs_{};
    std::vector<std::string> stageNames_;
    Image canvas_;
    bool loaded_ = false;
    bool valid_ = false;

    static Image Source(const char *name) {
        return LoadPng(name);
    }
    static Image Layer(const char *name) {
        return LoadPng((std::string("menu/characterselect/screencomposition/") + name).c_str());
    }
    static bool NativeLayer(const Image &image) {
        return image.Valid() && image.width == 384 && image.height == 224;
    }
    static bool NativePortrait(const Image &image) {
        return image.Valid() && image.width == 160 && image.height == 120;
    }
    // The two mask PNGs define the actual transparent panel silhouettes.
    // Ignore the opposite-side white blocking areas of those masks.
    void AnimatedPanel(const Image &video, const Image &mask, bool left) {
        if (!video.Valid() || !mask.Valid() || !canvas_.Valid()) return;
        const int atX = left ? 6 : 218;
        constexpr int atY = 6;
        constexpr int cropY = 20; // 160x160 animation, 160x120 visible portrait canvas
        Image masked;
        masked.width = 160;
        masked.height = 120;
        masked.pixels.resize(160u * 120u, 0u);
        for (int y = 0; y < 120; ++y) {
            for (int x = 0; x < 160; ++x) {
                const std::uint32_t coverage =
                    mask.pixels[static_cast<size_t>(atY + y) * 384u + static_cast<size_t>(atX + x)] >> 24;
                if (coverage == 0u) continue;
                const std::uint32_t src =
                    video.pixels[static_cast<size_t>(y + cropY) * 160u + static_cast<size_t>(x)];
                const unsigned a = (src >> 24) * coverage / 255u;
                const unsigned red = src & 255u;
                const unsigned green = (src >> 8) & 255u;
                const unsigned blue = (src >> 16) & 255u;
                // Blue animation turns into red on P1. P2 keeps native blue.
                const unsigned outR = left ? blue : red;
                const unsigned outG = left ? green / 2u : green;
                const unsigned outB = left ? red : blue;
                masked.pixels[static_cast<size_t>(y) * 160u + static_cast<size_t>(x)] =
                    (a << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
        Blit(canvas_.pixels, 384, 224, masked, {0,0,160,120}, {atX,atY,160,120});
        // The supplied red/blue mask artwork retains its frame/detail without
        // covering the animation; its opposite-side blockers are cropped out.
        Blit(canvas_.pixels, 384, 224, mask, {atX,atY,160,120},
             {atX,atY,160,120}, 255, 58);
    }
    void Cursor(const Image &source, int sourceIndex, int targetIndex) {
        const int dx = xmvsf_select::X(targetIndex) - xmvsf_select::X(sourceIndex);
        const int dy = xmvsf_select::Y(targetIndex) - xmvsf_select::Y(sourceIndex);
        Blit(canvas_.pixels, 384, 224, source, {0,0,384,224},
             {dx,dy,384,224});
    }
    bool LoadArt() {
        if (loaded_) return valid_;
        loaded_ = true;
        characters_ = Layer("Characters.png");
        grid_ = Layer("character grid.png");
        p1Cursor_ = Layer("1p Selector.png");
        p2Cursor_ = Layer("2p Selector.png");
        maskL_ = Layer("Portrait Mask L.png");
        maskR_ = Layer("Portrait Mask R.png");
        stageBrace_ = Layer("Stage Brace.png");
        valid_ = NativeLayer(characters_) && NativeLayer(grid_) &&
            NativeLayer(p1Cursor_) && NativeLayer(p2Cursor_) &&
            NativeLayer(maskL_) && NativeLayer(maskR_) &&
            NativeLayer(stageBrace_);
        for (unsigned i = 0; i < portraits_.size(); ++i) {
            const std::string file = std::string("menu/characterselect/portraits/") +
                xmvsf_select::kSlots[i].portrait;
            portraits_[i] = Source(file.c_str());
            valid_ = NativePortrait(portraits_[i]) && valid_;
            // Extracted portrait sheets retain an opaque hot-magenta key
            // behind Ryu/Akuma/etc. Clear that key at runtime so the new
            // portrait FDB remains visible; never rewrite the source PNG.
            if (portraits_[i].Valid())
                for (auto &pixel : portraits_[i].pixels) {
                    const unsigned red = pixel & 255u;
                    const unsigned green = (pixel >> 8) & 255u;
                    const unsigned blue = (pixel >> 16) & 255u;
                    if (red >= 240u && green <= 25u && blue >= 240u)
                        pixel &= 0x00ffffffu;
                }
        }
        for (int i = 0; i < 11; ++i)
            stageThumbs_[static_cast<size_t>(i)] =
                Source((std::string("stagethumbs/") + std::to_string(i) + ".png").c_str());
        constexpr const char *extra[] = {
            "01_wilystage.png", "02_finaldestination.png", "03_guilestage.png",
            "04_sigmastage.png", "05_akumastage.png", "06_ryustage.png",
            "07_boomerstage.png"
        };
        for (int i = 0; i < 7; ++i)
            stageThumbs_[static_cast<size_t>(i + 11)] =
                Source((std::string("stagethumbs/xmvsf2/") + extra[i]).c_str());
        if (const auto file = FindAsset("stagethumbs/levelnames.csv"); !file.empty()) {
            std::ifstream stream(file);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) stageNames_.push_back(line);
            }
        }
        if (!valid_)
            std::fprintf(stderr, "XMVSF character select: incomplete native composition/portraits\n");
        return valid_;
    }
public:
    ~CharacterSelectScreen() { Release(); }
    // Kick asynchronous decode while the main menu is still showing.
    // Both FDBs are ready before most users enter selection, avoiding
    // a one-frame flash of the original ROM chooser while decoding.
    bool Preload(SDL_Renderer *renderer) {
        return renderer && LoadArt() && background_.Open(renderer) &&
               portraitVideo_.Open(renderer);
    }
    void Release() {
        background_.Close();
        portraitVideo_.Close();
        canvas_ = {};
    }
    // Character IDs come from the ROM choosing records, not image order.
    // stageVisible is false until an observed ROM stage-announcement signal;
    // never display the brace from a guessed frame count.
    const Image *Build(SDL_Renderer *renderer, std::uint64_t nativeFrames,
                       std::uint16_t p1, std::uint16_t p2, bool showP1,
                       bool showP2, bool cursorP1, bool cursorP2,
                       const Font &font, bool stageVisible = false,
                       int stageIndex = -1) {
        if (!renderer || !LoadArt() || !background_.Open(renderer) ||
            !portraitVideo_.Open(renderer)) return nullptr;
        const Image *base = background_.Frame(nativeFrames);
        if (!base) return nullptr;
        canvas_ = *base;
        const Image *portraitFrame = portraitVideo_.Frame(nativeFrames);
        if (portraitFrame) {
            AnimatedPanel(*portraitFrame, maskL_, true);
            AnimatedPanel(*portraitFrame, maskR_, false);
        }
        const int a = xmvsf_select::Find(p1);
        const int b = xmvsf_select::Find(p2);
        if (showP1 && a >= 0) {
            const Image &portrait = portraits_[static_cast<size_t>(a)];
            Blit(canvas_.pixels, 384, 224, portrait, {0,0,160,120}, {6,6,160,120});
        }
        if (showP2 && b >= 0) {
            const Image &portrait = portraits_[static_cast<size_t>(b)];
            Blit(canvas_.pixels, 384, 224, portrait, {0,0,160,120}, {218,6,160,120});
        }
        Blit(canvas_.pixels, 384, 224, characters_, {0,0,384,224}, {0,0,384,224});
        // This overlay is intentionally ABOVE the character sprites.
        Blit(canvas_.pixels, 384, 224, grid_, {0,0,384,224}, {0,0,384,224});
        const bool ink = (nativeFrames / 4u) % 2u == 0u;
        if (ink && showP1 && cursorP1 && a >= 0) Cursor(p1Cursor_, 9, a);
        if (ink && showP2 && cursorP2 && b >= 0) Cursor(p2Cursor_, 8, b);
        if (showP1 && a >= 0)
            font.Text(canvas_.pixels, 384, 224,
                      xmvsf_select::kSlots[static_cast<size_t>(a)].name,
                      86, 115, 0.78f, 255, 1);
        if (showP2 && b >= 0)
            font.Text(canvas_.pixels, 384, 224,
                      xmvsf_select::kSlots[static_cast<size_t>(b)].name,
                      298, 115, 0.78f, 255, 1);
        if (stageVisible && stageIndex >= 0 &&
            stageIndex < static_cast<int>(stageThumbs_.size()) &&
            stageThumbs_[static_cast<size_t>(stageIndex)].Valid()) {
            const Image &thumb = stageThumbs_[static_cast<size_t>(stageIndex)];
            Blit(canvas_.pixels, 384, 224, thumb,
                 {0,0,thumb.width,thumb.height}, {154,36,76,75});
            Blit(canvas_.pixels, 384, 224, stageBrace_, {0,0,384,224}, {0,0,384,224});
            if (stageIndex < static_cast<int>(stageNames_.size())) {
                const auto &name = stageNames_[static_cast<size_t>(stageIndex)];
                const int measured = std::max(1, font.Measure(name.c_str()));
                const float size = std::min(0.59f, 76.0f / measured);
                font.Text(canvas_.pixels, 384, 224, name.c_str(),
                          192, 151, size, 255, 1);
            }
        }
        return &canvas_;
    }
};
} // namespace port::xmvsf_home
