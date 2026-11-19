// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: widescreen, as Street Fighter Alpha 2 and Vampire Savior have it. A fight is shown 512 columns
// wide where the board shows 384 (16:9 for 4:3, the height and the size of everything as they were): more of the stage
// on each side. The fight is played in that picture too: the players can go to its edges, and the camera stops where
// its edges come to the stage's art's ends (64 nearer the middle at each end, and on some stages a little further in:
// fight/wide_ends.h). With the feature off none of that is there and the game is the board's. It is off in every
// preset: it changes how the game plays.
//
// With "Invisible walls" (widescreen_walls) as well, the fight is shown in the wide picture but played as on the
// board: nothing here touches the rules (the margin is 0), so the players are stopped where the board's picture ends
// and the camera goes as far as the board's does. Only the picture is wider, and near a stage's end the view is kept
// inside the stage's art, so the board's picture is then off the middle of the wide one.
//
// The far background (the third scroll screen, going at half the camera's speed) is drawn moved, so that it comes to
// the end of its art just as the camera comes to its own end (fight/wide_ends.h).
//
// The rules the wide picture changes are written again here as readable C++ (rewrites: lift.h), each with the margin
// as a line in it: the camera's place across and up (its ends 64 nearer the middle; a mode's wanted place: battle.h),
// and keeping the players inside the picture (64 further out on each side; the ring out lets one over an end).
#include "alternative.h"
#include "battle.h"
#include "court.h"
#include "sfa.h"
#include "state.h"
#include "wide_ends.h"
#include "wide_near.h"

#include <algorithm>
#include <cstdlib>

namespace cps2recomp {

int BattleStage(); // fight/battle.cpp

namespace {
    using namespace sfa;

    constexpr int kSide = Video::kWideSide;
    // The camera's record (the stage's own scroll screen, FF8280), and in it
    constexpr uint32_t kCamera = 0xFF8280;
    constexpr uint32_t kCameraPlaceX = 0x20, kCameraShake = 0x50, kCameraBefore = 0x28, kCameraGoing = 0x3B;
    constexpr uint32_t kCameraPlaceY = 0x24, kCameraShakeY = 0x54, kCameraBeforeY = 0x2C, kCameraGoingY = 0x3A;
    constexpr uint32_t kCameraEndTop = 0x40, kCameraEndBottom = 0x42, kCameraEndLeft = 0x44, kCameraEndRight = 0x46;
    constexpr uint32_t kDrawn = 0x03;                       // byte: not zero, the screen is drawn into its map as it goes (its plan's layout)
    constexpr uint32_t kStripAcross = 0x38, kStripUp = 0x39; // bytes: which half of 64 pixels the last strip was drawn at
    constexpr uint32_t kAtEdge = 0x13C; // a player's: 1 at the picture's left edge, 2 at its right
    // Routines the rewrites call
    constexpr uint32_t kOuterEdges = 0x01A9A8; // D0: the players' left edge, D1 their right (the camera follows these)
    constexpr uint32_t kHeights = 0x01AA2A;    // D1: the higher player's height (D0 the lower)
    constexpr uint32_t kStripPlace = 0x01ADA0; // D0 the way it went: D2, D3 where the strip is from the camera
    constexpr uint32_t kStripMap = 0x01ACEA;   // A1: where in the map; D5, D6 the strip's place
    constexpr uint32_t kStripPlan = 0x01AD24;  // A0: where in the stage's plan
    constexpr uint32_t kStripDown = 0x01ADBC;  // a column of the plan drawn into the map
    constexpr uint32_t kStripAcrossRow = 0x01AE44; // a row

    bool g_play = false;  // fights are played in the wide picture
    bool g_shown = false; // fights are shown in the wide picture (played in it, or played as on the board)
    int g_side = 0, g_shift = 0, g_cameraBefore = 0;

    // The court's (fight/court.h): the wide picture forced, the camera held in the stage's middle, and as much of the
    // stage to each side of the board's picture as is drawn whole there (each stage's own, the same both ways, judged
    // from the court's picture at Video::kMaxSide), so the court is as wide as the stage is drawn: the fighters walk and
    // the ball goes out at its edges. (Another game's stage is drawn by that game, to 64 a side.)
    // (the Japanese street's fence breaks at its very ends; the Great Wall's mountains end and are mirrored past 128; the
    // train yard's far sheds come round again, 558 apart)
    constexpr int kCourtSides[kStages] = {176, 192, 128, 192, 128, 192, 80, 176, 192, 192, 80, 192, 192};
    int Side() {
        if (!court::On()) {
            return kSide;
        }
        if (const char *forced = getenv("SFA_COURT_SIDE")) { // (for measuring a stage: its court at that side)
            return std::clamp(atoi(forced), 0, static_cast<int>(Video::kMaxSide));
        }
        const unsigned stage = Ram16(kStage) / 2;
        return BattleStage() > kStages || stage >= kStages ? kSide : kCourtSides[stage];
    }

    int Margin() {
        return g_play ? Side() : 0;
    }

    // How much further in than 64 the camera stops at each end of each stage, for the stage's own scroll screen and
    // its floor: where the wide picture would show them past what the board shows (the floor's lines go slower than
    // the camera: Ryu's, Adon's, Guy's, Birdie's...). Measured by tools/stage_ends.sh with the far background left
    // out (it is drawn moved instead: SetWide).
    constexpr wide::Ends kEnds[kStages] = {{7, 7}, {0, 0}, {7, 7},   {0, 0},   {7, 7}, {30, 30}, {0, 0},
                                           {7, 7}, {36, 36}, {36, 36}, {0, 0}, {30, 30}, {30, 30}};
    // (the third scroll screen's speed against the camera's: half, on every stage)
    constexpr int kFarNum = 1, kFarDen = 2;
    wide::Ends StageEnds() {
        const int stage = Ram16(kStage) / 2;
        return stage < kStages && !court::On() ? kEnds[stage] : wide::Ends{}; // (the court's sides are its own)
    }
    // How far short of the board's ends the camera stops, at each end, in the picture played in
    int LeftMargin() {
        return g_play ? Side() + StageEnds().left : 0;
    }
    int RightMargin() {
        return g_play ? Side() + StageEnds().right : 0;
    }

    // --- the camera ---

    // The step's most: what is beyond it is cut to it, in D2
    uint16_t Step(M68K &c, uint16_t by, uint16_t most) {
        if (by > most) {
            c.d[2] = most;
        } else {
            SetLow16(c.d[2], by);
        }
        return static_cast<uint16_t>(c.d[2]);
    }

    // 01A646. The end of the camera's place across: which way it went (0C right or staying, 08 left), its shake added,
    // kept; and as it crosses each 32 pixels, a strip of the stage's plan drawn into the map ahead of it
    void AcrossDone(M68K &c, uint32_t camera) {
        SetRam8(camera + kCameraGoing, static_cast<int16_t>(c.d[0]) < static_cast<int16_t>(Ram16(camera + kCameraBefore)) ? 0x08 : 0x0C);
        const uint16_t shown = static_cast<uint16_t>(c.d[0] + Ram16(camera + kCameraShake));
        SetLow16(c.d[0], shown);
        SetRam16(camera + kCameraPlaceX, shown);
        if (!Ram8(camera + kDrawn)) {
            return;
        }
        SetLow8(c.d[0], (shown & 0x20) ^ Ram8(camera + kStripAcross));
        SetLow8(c.d[1], Ram8(camera + kStripAcross));
        if (c.d[0] & 0xFF) {
            return;
        }
        SetRam8(camera + kStripAcross, Ram8(camera + kStripAcross) ^ 0x20);
        SetLow8(c.d[0], Ram8(camera + kCameraGoing));
        Call(c, kStripPlace);
        Call(c, kStripMap);
        Call(c, kStripPlan);
        Call(c, kStripDown);
    }

    // 01A57A. The camera's place across: it follows the players' middle when they are far apart (C0 or more), and
    // otherwise moves only when one of them comes near the picture's edge; at most 5 a frame, and not past its ends (in
    // the wide picture, 64 nearer the middle, or more: kEnds). Then its shake, which way it went, and the strip of the map.
    void CameraAcross(M68K &c) {
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        Call(c, kOuterEdges);
        if (modes::CameraAt().x >= 0) {
            // (a mode wants the camera at a place of its own: it goes there, as far as its ends let it)
            const int end0 = Ram16(camera + kCameraEndLeft) + LeftMargin(), end1 = Ram16(camera + kCameraEndRight) - RightMargin();
            // (the court's: the stage's middle, its ends there or crossed)
            SetLow16(c.d[0], static_cast<uint16_t>(court::On() ? modes::CameraAt().x : std::clamp(modes::CameraAt().x, end0, end1)));
            SetLow16(c.d[2], 0);
            AcrossDone(c, camera);
            return;
        }
        const uint16_t left = static_cast<uint16_t>(c.d[0]), right = static_cast<uint16_t>(c.d[1]);
        const uint16_t place = Ram16(camera + kCameraPlaceX);
        int16_t by = 0;
        int way = 0; // 1 right, -1 left, 0 stays
        SetLow16(c.d[2], static_cast<uint16_t>(right - left));
        if (static_cast<uint16_t>(right - left) >= 0xC0) {
            // (far apart: their middle in the middle)
            by = static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint16_t>(right + left) >> 1) - place - 0xC0);
            way = by < 0 ? -1 : 1;
        } else if ((by = static_cast<int16_t>(right - place - 0x120)) >= 0) {
            way = 1;
        } else if ((by = static_cast<int16_t>(left - place - 0x60)) <= 0) {
            way = -1;
        }
        SetLow16(c.d[2], static_cast<uint16_t>(by));
        if (way == 0) {
            SetLow16(c.d[0], place);
        } else if (way > 0) {
            const uint16_t moved = static_cast<uint16_t>(place + Step(c, static_cast<uint16_t>(by), 5));
            SetRam16(camera + kCameraPlaceX, moved);
            // (the right end: in the wide picture 64 nearer the middle, or more)
            const uint16_t end = static_cast<uint16_t>(Ram16(camera + kCameraEndRight) - RightMargin());
            SetLow16(c.d[0], static_cast<int16_t>(end) <= static_cast<int16_t>(moved) ? end : moved);
        } else {
            const uint16_t moved = static_cast<uint16_t>(place - Step(c, static_cast<uint16_t>(-by), 5));
            SetRam16(camera + kCameraPlaceX, moved);
            const uint16_t end = static_cast<uint16_t>(Ram16(camera + kCameraEndLeft) + LeftMargin());
            SetLow16(c.d[0], static_cast<int16_t>(end) >= static_cast<int16_t>(moved) ? end : moved);
        }
        AcrossDone(c, camera);
    }

    // 01A696. The end of the camera's place up and down: which way it went, its shake added, kept, and a row of the
    // stage's plan drawn into the map as it crosses each 32 pixels
    void UpDone(M68K &c, uint32_t camera) {
        SetRam8(camera + kCameraGoingY, static_cast<int16_t>(c.d[0]) < static_cast<int16_t>(Ram16(camera + kCameraBeforeY)) ? 0x04 : 0x00);
        const uint16_t shown = static_cast<uint16_t>(c.d[0] + Ram16(camera + kCameraShakeY));
        SetLow16(c.d[0], shown);
        SetRam16(camera + kCameraPlaceY, shown);
        if (!Ram8(camera + kDrawn)) {
            return;
        }
        SetLow8(c.d[0], (shown & 0x20) ^ Ram8(camera + kStripUp));
        SetLow8(c.d[1], Ram8(camera + kStripUp));
        if (c.d[0] & 0xFF) {
            return;
        }
        SetRam8(camera + kStripUp, Ram8(camera + kStripUp) ^ 0x20);
        SetLow8(c.d[0], Ram8(camera + kCameraGoingY));
        Call(c, kStripPlace);
        Call(c, kStripMap);
        Call(c, kStripPlan);
        Call(c, kStripAcrossRow);
    }

    // 01A5F2. The camera's place up and down: it follows the higher player when that one is more than 96 above its
    // bottom (or below), at most 1 a frame, and not past its ends; then which way it went, its shake, and the row of the
    // map. (A mode can want it at a place of its own.)
    void CameraUp(M68K &c) {
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        Call(c, kHeights);
        if (modes::CameraAt().y >= 0) {
            const CameraWanted &wanted = modes::CameraAt();
            SetLow16(c.d[0], static_cast<uint16_t>(wanted.beyondEnds ? wanted.y :
                                                   std::clamp<int>(wanted.y, Ram16(camera + kCameraEndBottom), Ram16(camera + kCameraEndTop))));
            SetLow16(c.d[2], 0);
            UpDone(c, camera);
            return;
        }
        const uint16_t high = static_cast<uint16_t>(c.d[1]);
        const uint16_t place = Ram16(camera + kCameraPlaceY);
        const int16_t by = static_cast<int16_t>(high - place - 0x60);
        SetLow16(c.d[2], static_cast<uint16_t>(by));
        if (by > 0) {
            const uint16_t moved = static_cast<uint16_t>(place + Step(c, static_cast<uint16_t>(by), 1));
            SetRam16(camera + kCameraPlaceY, moved);
            const uint16_t end = Ram16(camera + kCameraEndTop);
            SetLow16(c.d[0], static_cast<int16_t>(end) <= static_cast<int16_t>(moved) ? end : moved);
        } else if (by < 0) {
            const uint16_t moved = static_cast<uint16_t>(place - Step(c, static_cast<uint16_t>(-by), 1));
            SetRam16(camera + kCameraPlaceY, moved);
            const uint16_t end = Ram16(camera + kCameraEndBottom);
            SetLow16(c.d[0], static_cast<int16_t>(end) >= static_cast<int16_t>(moved) ? end : moved);
        } else {
            SetLow16(c.d[0], place);
        }
        UpDone(c, camera);
    }

    // --- the players inside the picture ---

    // 01EEEA. The player at A6 kept inside the picture (in the wide picture, its edges); a player pressed against an
    // edge while reeling passes the push back to the other (if they are near enough).
    void KeepIn(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        SetRam8(player + kAtEdge, 0);
        c.a[4] = static_cast<uint32_t>(static_cast<int16_t>(Ram16(player + kOpponent)));
        // (the ring out: a player knocked about at an end of the stage goes over it)
        if (RingOutLetsOut(player)) {
            SetLow16(c.d[1], 0);
            return;
        }
        const uint32_t other = c.a[4] & 0xFFFFFF;
        uint16_t apart = static_cast<uint16_t>(Ram16(other + kX) - Ram16(player + kX));
        if (apart & 0x8000) {
            apart = static_cast<uint16_t>(0u - apart);
        }
        SetLow16(c.d[1], apart);
        const uint16_t half = Ram16(player + kHalfWidth);
        SetLow16(c.d[2], half);
        const uint16_t shown = static_cast<uint16_t>(Ram16(kCameraX) - Margin());
        const uint16_t x = Ram16(player + kX);
        uint16_t over = static_cast<uint16_t>(x - half - shown);
        int edge = 0;
        if (static_cast<uint16_t>(x - half) <= shown) {
            edge = 1; // (left: over is how far past, nothing or below)
        } else {
            SetLow16(c.d[2], static_cast<uint16_t>(2 * half));
            over = static_cast<uint16_t>(over + 2 * half - (0x180 + 2 * Margin()));
            if (over & 0x8000) {
                SetLow16(c.d[0], over);
                return;
            }
            edge = 2;
        }
        SetLow16(c.d[0], over);
        SetRam16(player + kX, static_cast<uint16_t>(x - over));
        SetRam8(player + kAtEdge, static_cast<uint8_t>(edge));
        if (Ram16(player + kState) != 0x0202 || Ram8(player + 0x239)) {
            return;
        }
        if (Ram8(player + kInAir)) {
            if (!Ram8(player + 0x23F)) {
                return;
            }
            over = static_cast<uint16_t>(over - Ram16(player + kSpeedX));
            SetLow16(c.d[0], over);
        }
        if (apart >= 0x100 || Ram8(player + 0x13F) || Ram8(other + kInAir)) {
            return;
        }
        SetRam16(other + kX, static_cast<uint16_t>(Ram16(other + kX) - over));
    }

    // 01EFA2. Both players (one holding the other: a throw) kept inside the picture together
    void KeepBothIn(M68K &c) {
        const uint32_t player = c.a[6];
        c.a[4] = c.a[0] = static_cast<uint32_t>(static_cast<int16_t>(Ram16(player + kOpponent)));
        c.a[1] = player;
        const uint32_t other = c.a[4] & 0xFFFFFF;
        SetRam8(other + kAtEdge, 0);
        SetLow16(c.d[1], Ram16(other + kX));
        if (!(Ram16(other + kX) < Ram16((player & 0xFFFFFF) + kX))) {
            c.a[0] = player;
            c.a[1] = c.a[4];
        }
        const uint32_t leftOne = c.a[0] & 0xFFFFFF, rightOne = c.a[1] & 0xFFFFFF;
        const uint16_t shown = static_cast<uint16_t>(Ram16(kCameraX) - Margin());
        uint16_t half = Ram16(leftOne + kHalfWidth);
        SetLow16(c.d[2], half);
        uint16_t over = static_cast<uint16_t>(Ram16(leftOne + kX) - half - shown);
        SetLow16(c.d[0], over);
        if (over & 0x8000) {
            SetRam16((player & 0xFFFFFF) + kX, static_cast<uint16_t>(Ram16((player & 0xFFFFFF) + kX) - over));
            SetRam16(other + kX, static_cast<uint16_t>(Ram16(other + kX) - over));
            SetRam8(other + kAtEdge, 1);
            return;
        }
        half = Ram16(rightOne + kHalfWidth);
        SetLow16(c.d[2], half);
        over = static_cast<uint16_t>(Ram16(rightOne + kX) - shown + half - (0x180 + 2 * Margin()));
        SetLow16(c.d[0], over);
        if (over & 0x8000) {
            return;
        }
        SetRam16((player & 0xFFFFFF) + kX, static_cast<uint16_t>(Ram16((player & 0xFFFFFF) + kX) - over));
        SetRam16(other + kX, static_cast<uint16_t>(Ram16(other + kX) - over));
        SetRam8(other + kAtEdge, 2);
    }

    // --- the picture ---

    int CameraLeftEnd() {
        return Ram16(kCamera + kCameraEndLeft);
    }
    int CameraRightEnd() {
        return Ram16(kCamera + kCameraEndRight);
    }
    bool InFight() {
        return Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchRound;
    }
    bool OnStage(int camera) {
        return CameraRightEnd() >= CameraLeftEnd() + 2 * kSide && camera >= CameraLeftEnd() && camera <= CameraRightEnd();
    }

    // An object's place: the panel's stay in the middle, the stage's and the fighters' go with the view
    int MoveObject(uint32_t tile, uint16_t attributes, int) {
        return ThePanel(tile, attributes) ? 0 : g_shift;
    }

    // The wide picture's switches for the frame about to run (the port's, or a spare's): the camera's place now and
    // where it was for the picture being made (a frame behind)
    void SetWide(Machine &machine, int camera, int shown) {
        g_side = g_shift = 0;
        int far = 0;
        if (g_shown && OnStage(camera) && OnStage(shown)) {
            g_side = Side();
            // (the view's left edge is 64 left of the camera, and no nearer the camera's own end than the stage's art
            // lets it: nearer the end than that, the stage is drawn to the side by what is missing. Played in the wide
            // picture the camera never goes there; with invisible walls it does, and the board's picture is then to one
            // side)
            const int end0 = CameraLeftEnd(), end1 = CameraRightEnd();
            const wide::Ends ends = StageEnds();
            g_shift = wide::ViewShift(shown, end0, end1, g_side, ends);
            // (the far background kept inside its art over the camera's range: fight/wide_ends.h)
            const int from = g_play ? end0 + g_side + ends.left : end0, to = g_play ? end1 - g_side - ends.right : end1;
            far = wide::FarScroll(kFarNum, kFarDen, shown, end0, end1, from, to, g_side, ends);
        }
        Video::Wide &wide = machine.video.wide;
        wide.side = g_side;
        // (the first scroll screen is the panel's bars: it stays in the middle, as the panel's objects do)
        wide.scroll[0] = 0;
        wide.scroll[1] = g_shift;
        wide.scroll[2] = far;
        wide.object = g_side ? MoveObject : nullptr;
        wide.tile = nullptr;
        wide.fillHoles = g_side != 0;
    }

    struct Kept {
        int side, shift;
    } g_kept;
} // namespace

bool WidePlay() {
    return g_play;
}
int WideMargin() {
    return Margin();
}
void WideEnds(int &left, int &right) {
    left = LeftMargin();
    right = RightMargin();
}
int WideSide() {
    return g_side;
}
int WideStage() {
    return g_side + g_shift;
}

// The camera's place a frame ago, which the picture is drawn from (it is a frame behind), in a save state: a frame
// put back (a save state, online play's rollback) is drawn, and its players kept in, as it was the first time
void WidescreenState(Archive &ar) {
    if (ar.version == 0 || ar.version >= 8) { // (a save state from before has none)
        ar.Pod(g_cameraBefore);
    }
}

void WidescreenBeforeFrame(Machine &machine) {
    const int camera = static_cast<int16_t>(Ram16(kCameraX)), shown = g_cameraBefore;
    g_cameraBefore = camera;
    // (fights only; the stage viewer keeps the board's picture. With invisible walls the rules stay the board's)
    // (the court, fight/court.h: always the wide picture, played in it, for the room)
    const bool court = court::On();
    g_shown = (FeatureOn("widescreen") || court) && InFight() && !modes::StageViewer();
    g_play = g_shown && (!FeatureOn("widescreen_walls") || court);
    SetWide(machine, camera, shown);
}

// A spare machine's frame (the modes' other machines) is drawn as wide as the port's picture is, so that the two can
// be put together: from the spare's own camera (where it is now, for want of where it was)
void WideForSpare(Machine &machine) {
    g_kept = {g_side, g_shift};
    const int camera = static_cast<int16_t>(Ram16(kCameraX));
    SetWide(machine, camera, camera);
    // (exactly as wide as the port's picture, even where the spare is not on its stage yet)
    if (g_side != g_kept.side) {
        Video::Wide &wide = machine.video.wide;
        g_side = wide.side = g_kept.side;
        g_shift = 0;
        wide.scroll[0] = wide.scroll[1] = wide.scroll[2] = 0;
        wide.object = g_side ? MoveObject : nullptr;
    }
}
void WideForPort(Machine &) {
    g_side = g_kept.side;
    g_shift = g_kept.shift;
}

namespace {
    using namespace sfa;
    // Where a player's fighter stands in the picture, for the window's zoom (fight/alternative.h, modes::Focus): the
    // middle of its feet, the wide picture's columns counted
    void FocusPlace(int player, int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(player) + kX)) - static_cast<int16_t>(Ram16(kCameraX)) + WideStage();
        y = 239 - (static_cast<int16_t>(Ram16(Player(player) + kY)) - static_cast<int16_t>(Ram16(kCameraY)));
    }
} // namespace

void RegisterWidescreen() {
    modes::Focus(FocusPlace, 44, 112);
    TheGame().widestPicture = [] { return Video::kWidth + (FeatureOn("widescreen") ? 2 * kSide : 0); };
    AddRewrite(0x01A57A, "CameraAcross", CameraAcross, kFreeFlags, 256);
    AddRewrite(0x01A5F2, "CameraUp", CameraUp, kFreeFlags, 222);
    AddRewrite(0x01EEEA, "KeepIn", KeepIn, kFreeFlags, 148);
    AddRewrite(0x01EFA2, "KeepBothIn", KeepBothIn, kFreeFlags, 200);
    // (a stage's pieces in the wide picture's added columns: the game draws an object only near the board's picture,
    // and played wide that counts the added columns too: fight/wide_near.h)
    // (015BA6: D0 the object's place less its scroll screen's; drawn at 015BC4, not at 015BE4)
    AddHook(0x015BBA, "NearPicture", [](M68K &c) {
        return Margin() && wide::NearPicture(c, static_cast<int16_t>(c.d[0]), 0x40, 0x200, Margin(), 0x015BC4, 0x015BE4);
    });
    // (not in the Enhanced preset: it shows more of the stage than the board does, and a player who wants the game as
    // it is played in competition must not get that without asking)
    TheGame().features.push_back({"widescreen", "Display", "Widescreen",
                                  "Fights are shown and played in 16:9, with more of the stage on each side: the players can go to "
                                  "the wide picture's edges, so they can stand further apart than on the board. Off unless switched "
                                  "on here, in every preset: it changes how the game plays.",
                                  kFeatureFirstPage});
    TheGame().features.push_back({"widescreen_walls", "Display", "Invisible walls",
                                  "With widescreen: fights are still shown in 16:9, but played exactly as on the board. The players "
                                  "are stopped where the board's picture ends, by walls that are not seen, and the camera goes as far "
                                  "as the board's; near a stage's end the view stays on the stage, so the play area is then to one "
                                  "side of the picture. Nothing without widescreen.",
                                  0});
}

} // namespace cps2recomp
