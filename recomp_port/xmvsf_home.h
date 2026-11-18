// XMVSF recomp native menu — first migration slice.
// This draws into CPS-2 Recomp's own 32-bit picture buffer; no SDL2 or FBNeo references.
// Source: XMVSF original Stage14 menu design (ARCADE / TRAINING / VERSUS / OPTIONS).
#pragma once
#include "../recomp-common/menu.h"
#include "../recomp-common/text.h"
#include <algorithm>
#include <cstdlib>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace port::xmvsf_home {

enum class Action { None, Arcade, Training, Versus, Options };

class Menu {
    bool enabled_ = false;
    bool visible_ = false;
    int selection_ = 0;

public:
    explicit Menu(bool enabled) : enabled_(enabled), visible_(enabled) {}
    bool Enabled() const { return enabled_; }
    bool Visible() const { return visible_; }
    void Open() { if (enabled_) visible_ = true; }
    void Hide() { visible_ = false; }
    void Toggle() { if (enabled_) visible_ = !visible_; }

    Action Press(unsigned input) {
        if (!visible_) return Action::None;
        if (input & recomp::kMenuUp) selection_ = (selection_ + 3) % 4;
        if (input & recomp::kMenuDown) selection_ = (selection_ + 1) % 4;
        if (input & recomp::kMenuBack) { Hide(); return Action::None; }
        if (!(input & (recomp::kMenuConfirm | recomp::kMenuRight))) return Action::None;
        switch (selection_) {
        case 0: return Action::Arcade;
        case 1: return Action::Training;
        case 2: return Action::Versus;
        case 3: return Action::Options;
        default: return Action::None;
        }
    }

    void Draw(std::vector<uint32_t> &pixels, int width = 384, int height = 224) const {
        pixels.assign(static_cast<size_t>(width) * height, 0xFF080C16u);
        const recomp::text::Canvas c{pixels.data(), width, height};
        using recomp::text::Color;
        const Color shade{13, 21, 37, 255}, blue{23, 38, 62, 255};
        const Color line{83, 94, 105, 255}, gold{211, 180, 120, 255};
        const Color white{238, 235, 223, 255}, dim{117, 130, 151, 255};
        c.Fill(0, 0, width, height, shade);
        for (int y = 0; y < height; y += 4) {
            const int band = (y / 4) % 3;
            if (band == 0) c.Fill(0, static_cast<float>(y), width, 1, blue);
        }
        c.Fill(21, 16, width - 42, height - 32, {6, 9, 19, 235});
        c.Fill(25, 19, width - 50, 1, line);
        c.Fill(25, height - 20, width - 50, 1, line);
        auto center = [&](const char *s, float y, float dot, Color color) {
            const float w = recomp::text::TextWidth(s, dot);
            c.Text((width - w) * 0.5f, y, dot, 0, color, s);
        };
        center("X-MEN VS. STREET FIGHTER", 28, 2.0f, gold);
        center("NATIVE RECOMP", 51, 1.0f, dim);

        static constexpr std::array<const char *, 4> entries = {"ARCADE", "TRAINING", "VERSUS", "OPTIONS"};
        for (int i = 0; i < 4; ++i) {
            const float y = static_cast<float>(80 + i * 27);
            const bool chosen = i == selection_;
            if (chosen) {
                c.Fill(87, y - 4, width - 174, 22, {36, 55, 78, 255});
                c.Fill(87, y - 4, 3, 22, gold);
                c.Fill(width - 90, y - 4, 3, 22, gold);
            }
            center(entries[i], y, 1.65f, chosen ? white : dim);
        }
        center("ARROWS / DPAD    A / ENTER SELECT", height - 29, 0.86f, gold);
        // This is intentionally an honest functional first port; logo animation,
        // sound and old Graphics/Controller tabs will follow separately.
    }
};

} // namespace port::xmvsf_home
