// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The readout over the picture: the lines the game's sources set (runtime/game.h, SetHud) as small panels of
// text.
#pragma once

#include <SDL3/SDL_rect.h>

#include <cstdint>
#include <vector>

namespace port {

struct HudPanel {
    int x, y, width, height; // where on the picture, in its pixels
    std::vector<uint32_t> pixels;
};

// The panels for a picture of this size
std::vector<HudPanel> HudPanels(int width, int height);
// Straight onto a picture (32-bit pixels, red in the low byte); and where the panels went (the boxes are not
// kept: they are drawn over the game's picture, and zoomed with it)
// `parts`: the boxes (1), the panels of text (2), or both (a vertical game's picture turned upright has its boxes drawn
// on the board's picture and its text on the turned one: Port::Shown)
enum : unsigned { kHudBoxes = 1, kHudText = 2 };
void DrawHud(uint32_t *pixels, int width, int height, std::vector<SDL_Rect> *panels = nullptr, unsigned parts = kHudBoxes | kHudText);

} // namespace port
