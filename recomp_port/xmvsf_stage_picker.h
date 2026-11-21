// XMVSF 87.487: bounded 18-stage picker, no 4-panel horizontal wrap.
// XMVSF_RECOMP_STAGES_87_455: shared runtime/source installer marker.
// Original stage IDs are 0..10 at FF5113. XMVSF2 previews are deliberately
// not passed to the native/guest stage selector until their renderers are ported.
#pragma once
#include "xmvsf_home.h"
#include "xmvsf_stage_nav.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace port::xmvsf_home {

class StagePicker {
public:
    enum class Action { None, Move, Back, Confirm, Unavailable };
    static constexpr int kOriginals = 11;
    static constexpr int kExtras = 7;
    static constexpr int kCount = kOriginals + kExtras;
    static constexpr int kColumns = 5;
    static constexpr int kPageSize = 10;
    static constexpr int kCell = 150;

private:
    struct Entry {
        std::string name, relativePath;
        Image image;
        SDL_Texture *texture = nullptr;
    };
    std::vector<Entry> entries_;
    Font labels_;
    SDL_Texture *labelsTexture_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    bool active_ = false;
    int selected_ = 0;

    static SDL_Texture *Texture(SDL_Renderer *r, const Image &img) {
        if (!r || !img.Valid()) return nullptr;
        SDL_Texture *texture = SDL_CreateTexture(
            r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img.width, img.height);
        if (!texture) return nullptr;
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        if (!SDL_UpdateTexture(texture, nullptr, img.pixels.data(), img.width * 4)) {
            SDL_DestroyTexture(texture);
            return nullptr;
        }
        return texture;
    }

    void Add(const std::string &name, const std::string &path) {
        Entry e;
        e.name = name;
        e.relativePath = path;
        if (!path.empty()) e.image = LoadPng(path.c_str());
        entries_.push_back(std::move(e));
    }

    void Load() {
        entries_.clear();
        const std::array<const char *, kOriginals> names{{
            "Apocalypse's lair", "The garden pond", "The cavern",
            "The waterfall", "The snowy temple", "The great tree",
            "Manhattan at night", "The refinery", "The space base",
            "The TV studio", "The shopping mall"
        }};
        // The user-indexed 0.png..10.png correspond exactly to native
        // stage byte FF5113. No name heuristics or alphabetical reordering.
        for (int i = 0; i < kOriginals; ++i)
            Add(names[size_t(i)], std::string("stagethumbs/") + std::to_string(i) + ".png");

        // Seven XMVSF2 thumbnails from feature/menu-film-strip. Fixed order.
        const std::array<const char *, kExtras> extraFiles{{
            "01_wilystage.png", "02_finaldestination.png", "03_guilestage.png",
            "04_sigmastage.png", "05_akumastage.png", "06_ryustage.png",
            "07_boomerstage.png"
        }};
        const std::array<const char *, kExtras> extraNames{{
            "Wily Stage", "Final Destination", "Guile Stage",
            "Sigma Stage", "Akuma Stage", "Ryu Stage", "Boomer Stage"
        }};
        for (int i = 0; i < kExtras; ++i)
            Add(extraNames[size_t(i)], std::string("stagethumbs/xmvsf2/") + extraFiles[size_t(i)]);
        labels_.Load("sf2fontsmall.png", 8);
        int ready = 0;
        for (int i = 0; i < kCount; ++i) ready += entries_[size_t(i)].image.Valid() ? 1 : 0;
        std::fprintf(stderr, "XMVSF 87.466 stage picker: %d/18 images loaded (11 native + 7 XMVSF2 previews)\n", ready);
    }

    void Text(SDL_Renderer *r, const std::string &str, int anchorX, int y,
              float scale, Uint8 brightness = 190) {
        if (!labelsTexture_) return;
        SDL_SetTextureColorMod(labelsTexture_, brightness, brightness, brightness);
        int x = anchorX - static_cast<int>(labels_.Measure(str.c_str()) * scale / 2 + 0.5f);
        for (unsigned char c : str) {
            const unsigned ch = c < 128 ? c : 32;
            const Font::Glyph &g = labels_.glyphs[ch];
            if (g.rect.w > 0 && g.rect.h > 0) {
                const SDL_FRect src{float(g.rect.x), float(g.rect.y),
                                    float(g.rect.w), float(g.rect.h)};
                const SDL_FRect dst{float(x), float(y),
                                    float(g.rect.w) * scale, float(g.rect.h) * scale};
                SDL_RenderTexture(r, labelsTexture_, &src, &dst);
            }
            x += static_cast<int>(g.advance * scale + 0.5f);
        }
    }

    bool InitTextures(SDL_Renderer *r) {
        if (renderer_ == r) return labelsTexture_ != nullptr;
        Release();
        renderer_ = r;
        labelsTexture_ = Texture(r, labels_.sheet);
        for (auto &e : entries_) e.texture = Texture(r, e.image);
        return labelsTexture_ != nullptr;
    }

    void Outline(SDL_Renderer *r, int x, int y, uint64_t now) const {
        // Two-pixel pixel-box outline: the alternating pink/blue pixels flicker.
        const unsigned phase = static_cast<unsigned>(now / 110u);
        for (int n = 0; n < kCell; n += 5) {
            const bool pink = ((n / 5 + phase) & 1u) == 0;
            SDL_SetRenderDrawColor(r, pink ? 255 : 42, pink ? 54 : 196,
                                   pink ? 207 : 255, 255);
            const float pos = float(n);
            const float len = float(std::min(5, kCell - n));
            SDL_FRect top{float(x) + pos, float(y) - 3, len, 3.0f};
            SDL_FRect bottom{float(x) + pos, float(y + kCell), len, 3.0f};
            SDL_FRect left{float(x) - 3, float(y) + pos, 3.0f, len};
            SDL_FRect right{float(x + kCell), float(y) + pos, 3.0f, len};
            SDL_RenderFillRect(r, &top);
            SDL_RenderFillRect(r, &bottom);
            SDL_RenderFillRect(r, &left);
            SDL_RenderFillRect(r, &right);
        }
    }

public:
    StagePicker() = default;
    StagePicker(const StagePicker &) = delete;
    StagePicker &operator=(const StagePicker &) = delete;
    ~StagePicker() { Release(); }

    bool Active() const { return active_; }
    int Selected() const { return selected_; }
    bool Original() const { return selected_ >= 0 && selected_ < kOriginals; }
    void Open() {
        if (entries_.empty()) Load();
        selected_ = 0;
        active_ = true;
    }
    void Close() { active_ = false; }

    // Input already edge-filtered by frontend_sdl.cpp. Shoulder edges are separate.
    Action Press(unsigned input, bool pageLeft, bool pageRight) {
        if (!active_) return Action::None;
        if (input & recomp::kMenuBack) {
            Close();
            return Action::Back;
        }
        const int pageTarget = stage_nav::Page(
            selected_, pageLeft, pageRight, kCount, kPageSize);
        if (pageTarget != selected_) {
            selected_ = pageTarget;
            return Action::Move;
        }
        // An ambiguous/both-shoulders press is neutral. It cannot also
        // cause an accidental directional step or confirm.
        if (pageLeft || pageRight) return Action::None;
        unsigned direction = 0u;
        if (input & recomp::kMenuLeft) direction |= stage_nav::Left;
        if (input & recomp::kMenuRight) direction |= stage_nav::Right;
        if (input & recomp::kMenuUp) direction |= stage_nav::Up;
        if (input & recomp::kMenuDown) direction |= stage_nav::Down;
        const int next = stage_nav::Move(
            selected_, direction, kCount, kPageSize, kColumns);
        if (next != selected_) {
            selected_ = next;
            return Action::Move;
        }
        if (direction) return Action::None; // edge doesn't wrap or confirm
        if (input & recomp::kMenuConfirm)
            return Original() ? Action::Confirm : Action::Unavailable;
        return Action::None;
    }

    bool Draw(SDL_Renderer *r, uint64_t now) {
        if (!active_ || !r || !InitTextures(r)) return false;
        Text(r, "SELECT TRAINING STAGE", 480, 28, 2.65f, 240);
        const int page = selected_ / kPageSize;
        const int start = page * kPageSize;
        constexpr int gapX = 14, gapY = 26;
        constexpr int x0 = (960 - (kColumns * kCell + (kColumns - 1) * gapX)) / 2;
        constexpr int y0 = 118;
        for (int slot = 0; slot < kPageSize && start + slot < kCount; ++slot) {
            const auto &e = entries_[size_t(start + slot)];
            const int x = x0 + (slot % kColumns) * (kCell + gapX);
            const int y = y0 + (slot / kColumns) * (kCell + gapY);
            const SDL_FRect rect{float(x), float(y), float(kCell), float(kCell)};
            if (e.texture) {
                SDL_RenderTexture(r, e.texture, nullptr, &rect);
                if (start + slot >= kOriginals) {
                    SDL_SetRenderDrawColor(r, 4, 8, 16, 100);
                    SDL_RenderFillRect(r, &rect);
                }
            } else {
                SDL_SetRenderDrawColor(r, 18, 24, 35, 240);
                SDL_RenderFillRect(r, &rect);
                Text(r, "NO ART", x + kCell / 2, y + kCell / 2, 1.55f, 100);
            }
            if (start + slot == selected_) Outline(r, x, y, now);
        }
        std::string title = entries_[size_t(selected_)].name;
        if (title.size() > 38) title.resize(38);
        Text(r, title, 480, 457, 1.6f, 255);
        if (Original())
            Text(r, "A / ENTER TRAIN    B / ESC BACK", 480, 484, 1.5f, 175);
        else
            Text(r, "XMVSF2 STAGE NOT PORTED TO RECOMP", 480, 484, 1.45f, 155);
        Text(r, "LB / Q PREV       RB / E NEXT", 480, 506, 1.6f, 185);
        Text(r, std::string("PAGE ") + std::to_string(page + 1) + " / 2",
             480, 525, 1.2f, 130);
        return true;
    }

    void Release() {
        if (labelsTexture_) SDL_DestroyTexture(labelsTexture_);
        labelsTexture_ = nullptr;
        for (auto &e : entries_) {
            if (e.texture) SDL_DestroyTexture(e.texture);
            e.texture = nullptr;
        }
        renderer_ = nullptr;
    }
};

} // namespace port::xmvsf_home
