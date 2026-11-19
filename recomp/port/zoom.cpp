// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The camera's zoom (zoom.h).
#include "zoom.h"

#include "../runtime/game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace port {

namespace {
    // How much of the way to what is wanted each picture goes: the size, and the place looked at when it changes
    // fighter (the camera is on its fighter otherwise). The size changes by at most so much a picture, less going in
    // than coming out (1x to 2x in about two seconds; out in under one), so that it never lurches.
    constexpr float kSizeEase = 0.05f, kPlaceEase = 0.2f;
    constexpr float kMostIn = 0.008f, kMostOut = 0.02f;
    // Keeping both in the picture: the place between them eased to (not jumped to each picture), and the camera goes
    // closer only when it could go this much closer (out at once): a fight's every step does not move it
    constexpr float kBothPlaceEase = 0.1f, kBothCloser = 0.15f;
    constexpr int kLabelTicks = 150; // (two and a half seconds)
    constexpr float kPanStep = 2.0f;  // the picture's pixels a step of Pan moves the camera (a step a picture)
    constexpr float kPanMost = 160.0f;
    // Keeping both in the picture: room beyond a fighter's height (the game's focusTall) above it, below its feet for
    // its shadow, and across, as a part of its height, for its arms and what it wears
    constexpr float kRoomTop = 8.0f, kRoomFeet = 14.0f, kAcrossOfTall = 0.62f;
    bool forbidden = false; // (online play: Forbid)

    // The zoom by distance (issue #272): its sizes as a round starts, with the fighters together, and apart (the whole
    // picture: past its edges would be black); and how far apart their places are for each, in the picture's pixels:
    // together, as a round starts (the CPS-2 fighting games start their fighters 176 to 192 pixels apart), and the
    // ends of the picture's width, short of this much room at each (where the game stops them)
    constexpr float kStartSize = 1.0f / 0.9f, kTogetherSize = 1.25f / 0.9f;
    constexpr float kTogetherGap = 56.0f, kStartGap = 180.0f, kEndRoom = 40.0f;
    // ... the whole picture from this part of the way from the start's distance to the furthest on
    constexpr float kOutFrom = 0.8f;
    // ... and by distance with something else kept in (Game::focusAlso: the court's ball), as close as shows them all,
    // to at most this
    constexpr float kAlsoMost = 1.3f;
    float ByDistance(float gap, int width) {
        const float apart = kStartGap + (std::max(width - 2 * kEndRoom, kStartGap + 1) - kStartGap) * kOutFrom;
        if (gap <= kTogetherGap) {
            return kTogetherSize;
        }
        if (gap <= kStartGap) {
            return kTogetherSize + (kStartSize - kTogetherSize) * (gap - kTogetherGap) / (kStartGap - kTogetherGap);
        }
        if (gap >= apart) {
            return 1.0f;
        }
        return kStartSize + (1.0f - kStartSize) * (gap - kStartGap) / (apart - kStartGap);
    }
} // namespace

bool Zoom::Possible() {
    return !forbidden && cps2recomp::TheGame().focus != nullptr;
}

bool Zoom::KeepsPanel() const {
    return m_size > 0 && m_player == kDistance && Possible() && cps2recomp::TheGame().panel;
}

void Zoom::Forbid(bool forbid) {
    forbidden = forbid;
    m_size = 0;
    m_player = 0;
    m_panX = m_panY = 0;
    m_jump = true;
    m_labelTicks = 0;
}

void Zoom::Said() {
    if (m_size == 0) {
        snprintf(m_label, sizeof(m_label), "ZOOM OFF");
    } else if (m_player == kBoth) {
        snprintf(m_label, sizeof(m_label), "ZOOM  BOTH FIGHTERS");
    } else if (m_player == kDistance) {
        snprintf(m_label, sizeof(m_label), "ZOOM  BY DISTANCE");
    } else {
        snprintf(m_label, sizeof(m_label), "ZOOM %gx  PLAYER %d", static_cast<double>(kSizes[m_size]), m_player + 1);
    }
    m_labelTicks = kLabelTicks;
}

void Zoom::Press(int player) {
    if (!Possible()) {
        return;
    }
    if (m_player >= kBoth) {
        // (from both to one: that one at the size it was last looked at, the first if none)
        m_size = std::max(m_size, 1);
    } else if (m_size == 0 || player == m_player) {
        m_size = (m_size + 1) % kSizeCount;
    }
    if (m_size == 0 || player != m_player) {
        m_panX = m_panY = 0; // (framed afresh: the moved camera was for the other fighter)
    }
    m_player = player;
    Said();
}

void Zoom::Both() {
    if (!Possible()) {
        return;
    }
    if (m_player == kBoth && m_size > 0) {
        m_size = 0;
        m_player = 0;
    } else {
        m_player = kBoth;
        m_size = std::max(m_size, 1);
        m_bothSize = m_shown; // (from where it is: closer only as the fighters allow)
    }
    m_panX = m_panY = 0;
    Said();
}

void Zoom::Choose(int choice) {
    if (!Possible() || choice == Choice()) {
        return;
    }
    if (choice == 3) {
        Both();
        return;
    }
    if (choice <= 0) {
        m_size = 0;
        m_player = 0;
    } else if (choice == 4) {
        m_size = std::max(m_size, 1);
        m_player = kDistance;
    } else {
        if (m_size == 0 || m_player >= kBoth) {
            m_size = 2; // (1.5x)
        }
        m_player = choice - 1;
    }
    m_panX = m_panY = 0;
    Said();
}

void Zoom::Pan(int across, int down) {
    if (m_size == 0) {
        return;
    }
    m_panX = std::clamp(m_panX + across * kPanStep, -kPanMost, kPanMost);
    m_panY = std::clamp(m_panY + down * kPanStep, -kPanMost, kPanMost);
}

void Zoom::Set(int size, int player) {
    if (forbidden) {
        return;
    }
    m_size = player >= kBoth ? std::max(size, 1) : std::clamp(size, 0, kSizeCount - 1);
    m_player = player;
    m_jump = true;
    Said();
}

void Zoom::Follow(int width, int height) {
    if (m_labelTicks > 0) {
        m_labelTicks--;
    }
    int x = 0, y = 0;
    float wanted = 1.0f, wantedX = width / 2.0f, wantedY = height / 2.0f;
    bool seen = false;
    if (m_size > 0 && Possible() && m_player >= kBoth) {
        // Both: the rectangle they are in, and as close as shows all of it (by distance: as close as their distance
        // says, if that shows all of it)
        int x2 = 0, y2 = 0;
        seen = cps2recomp::TheGame().focus(0, x, y) && cps2recomp::TheGame().focus(1, x2, y2);
        if (seen) {
            // (the places are focusLift above the feet; a fighter reaches focusTall up from them)
            const cps2recomp::Game &game = cps2recomp::TheGame();
            const float across = game.focusTall * kAcrossOfTall;
            const float above = static_cast<float>(game.focusTall - game.focusLift) + kRoomTop;
            const float below = static_cast<float>(game.focusLift) + kRoomFeet;
            float left = std::min(x, x2) - across, right = std::max(x, x2) + across;
            float top = std::min(y, y2) - above, bottom = std::max(y, y2) + below;
            // (and what else the game wants kept in the picture: a mode's ball, counted in their distance)
            float gap = static_cast<float>(std::abs(x2 - x));
            int ax = 0, ay = 0, reach = 0;
            const bool also = game.focusAlso && game.focusAlso(ax, ay, reach);
            if (also) {
                left = std::min(left, static_cast<float>(ax - reach));
                right = std::max(right, static_cast<float>(ax + reach));
                top = std::min(top, static_cast<float>(ay - reach));
                bottom = std::max(bottom, static_cast<float>(ay + reach));
                gap = static_cast<float>(std::max({x, x2, ax}) - std::min({x, x2, ax}));
            }
            const float fits = std::clamp(std::min(width / (right - left), height / (bottom - top)), 1.0f, kSizes[kSizeCount - 1]);
            if (m_player == kDistance && also) {
                // (the fighters and something else, a ball: as close as shows them all, in as far as kAlsoMost)
                wanted = std::min(fits, kAlsoMost);
            } else if (m_player == kDistance) {
                wanted = std::min(ByDistance(gap, width), fits);
            } else {
                if (fits < m_bothSize || fits > m_bothSize + kBothCloser || fits == kSizes[kSizeCount - 1]) {
                    m_bothSize = fits;
                }
                wanted = m_bothSize;
            }
            wantedX = (left + right) / 2 + m_panX;
            wantedY = (top + bottom) / 2 + m_panY;
        }
    } else if (m_size > 0 && Possible()) {
        seen = cps2recomp::TheGame().focus(m_player, x, y);
        if (seen) {
            wanted = kSizes[m_size];
            wantedX = x + m_panX;
            wantedY = y + m_panY;
        }
    }
    if (m_jump || !m_settled) {
        // (from nothing, the place is where the size starts from, and the size eases; asked for, both are there)
        m_offX = m_offY = 0;
        m_x = wantedX;
        m_y = wantedY;
        m_settled = seen;
        m_wasSeen = seen;
        m_followed = m_player;
        if (m_jump && seen) {
            m_shown = wanted;
            m_jump = false;
        }
    }
    m_shown += std::clamp((wanted - m_shown) * kSizeEase, -kMostOut, kMostIn);
    if (std::fabs(wanted - m_shown) < 0.002f) {
        m_shown = wanted;
    }
    // The camera is on the fighter, but for a gap left where it was when it changed fighter (or went back to the
    // middle), which closes as the size eases
    if (seen != m_wasSeen || m_player != m_followed) {
        m_offX = m_x - wantedX;
        m_offY = m_y - wantedY;
    }
    m_wasSeen = seen;
    m_followed = m_player;
    m_offX *= 1 - kPlaceEase;
    m_offY *= 1 - kPlaceEase;
    if (m_player >= kBoth && seen) {
        m_x += (wantedX + m_offX - m_x) * kBothPlaceEase;
        m_y += (wantedY + m_offY - m_y) * kBothPlaceEase;
    } else {
        m_x = wantedX + m_offX;
        m_y = wantedY + m_offY;
    }
    if (m_shown == 1.0f && !seen) {
        m_settled = false; // (out again: the next zoom starts at its fighter)
    }
}

bool Zoom::View(int width, int height, SDL_FRect &view) const {
    if (m_shown <= 1.0f) {
        return false;
    }
    // (the place looked at in the middle, always: past the picture's edges is black; but both fighters are kept
    // inside the picture, which holds them)
    view.w = width / m_shown;
    view.h = height / m_shown;
    view.x = m_x - view.w / 2;
    view.y = m_y - view.h / 2;
    if (m_player >= kBoth) {
        view.x = std::clamp(view.x, 0.0f, width - view.w);
        view.y = std::clamp(view.y, 0.0f, height - view.h);
    }
    return true;
}

} // namespace port
