// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: widescreen, as Night Warriors, Pocket Fighter, the Alpha games and Vampire Savior have it. A fight is
// shown 512 columns wide where the board shows 384 (16:9 for 4:3, the height and the size of everything as they were):
// more of the stage on each side. The fight is played in that picture too: the players can go to its edges, and the
// camera stops where its edges come to the stage's ends (64 nearer the middle at each end). With the feature off none of
// that is there and the game is the board's. It is off in every preset: it changes how the game plays.
//
// With "Invisible walls" (widescreen_walls) as well, the fight is shown in the wide picture but played as on the
// board: nothing here touches the rules (the margin is 0), and near a stage's end the view is kept inside the stage.
//
// The rules the wide picture changes, each with the margin in it, are the game's own code done here when the margin is
// not 0 (with it 0 the game's code runs): the camera's step across (017FCC to the right and 01800E to the left keep it
// inside its ends: words at A5+6B1E and A5+6B20, 0 and 0x280) and keeping a player inside the picture (003782, which
// also lets the ring out's player over an end: battle.h).
#include "battle.h"
#include "alternative.h"
#include "cybots.h"
#include "state.h"
#include "wide_ends.h"

#include <algorithm>
#include <iterator>

namespace cps2recomp {

namespace {
    using namespace cybots;

    constexpr int kSide = Video::kWideSide;
    constexpr uint32_t kCameraLeftEnd = 0xFFEB1E, kCameraRightEnd = 0xFFEB20;
    // The camera's object (A6 at 017F88; A5+6B1C points to it): its place across (+1A), the step it last took (+50)
    // and which way (+44: 0 left, 4 right), which the far planes' objects follow (01803E on)
    constexpr uint32_t kCameraStep = 0x50, kCameraWay = 0x44;
    // A player's: a byte 1 at the picture's left edge and 2 at its right (+116), the other player's record (+374, a long)
    constexpr uint32_t kAtEdge = 0x116, kOpponent = 0x374, kFlags = 0x126;

    bool g_play = false;  // fights are played in the wide picture
    bool g_shown = false; // fights are shown in the wide picture (played in it, or played as on the board)
    int g_side = 0, g_shift = 0, g_cameraBefore = 0;

    int Margin() {
        return g_play ? kSide : 0;
    }
    wide::Ends StageEnds() {
        return wide::Ends{};
    }
    int LeftMargin() {
        return g_play ? kSide + StageEnds().left : 0;
    }
    int RightMargin() {
        return g_play ? kSide + StageEnds().right : 0;
    }

    // --- the camera ---

    int CameraLeftEndNow() {
        return static_cast<int16_t>(Ram16(kCameraLeftEnd));
    }
    int CameraRightEndNow() {
        return static_cast<int16_t>(Ram16(kCameraRightEnd));
    }

    // 017FCC: the camera (A6) steps D1 to the right, no further than its right end (here 64 nearer the middle)
    bool CameraRight(M68K &c) {
        if (!Margin()) {
            return false;
        }
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        const uint16_t was = Ram16(camera + kX), step = static_cast<uint16_t>(c.d[1]);
        const uint16_t end = static_cast<uint16_t>(Ram16(kCameraRightEnd) - RightMargin());
        const uint16_t now = static_cast<uint16_t>(was + step);
        if (static_cast<int16_t>(end) <= static_cast<int16_t>(now)) {
            SetRam16(camera + kX, end);
            SetRam16(camera + kCameraStep, static_cast<uint16_t>(end - was));
        } else {
            SetRam16(camera + kX, now);
            SetRam16(camera + kCameraStep, step);
        }
        SetRam8(camera + kCameraWay, 4);
        Return(c);
        return true;
    }
    // 01800E: the camera steps D0 (below nothing) to the left, no further than its left end (64 nearer the middle)
    bool CameraLeft(M68K &c) {
        if (!Margin()) {
            return false;
        }
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        const uint16_t was = Ram16(camera + kX), step = static_cast<uint16_t>(c.d[0]);
        const uint16_t end = static_cast<uint16_t>(Ram16(kCameraLeftEnd) + LeftMargin());
        const uint16_t now = static_cast<uint16_t>(was + step);
        if (static_cast<int16_t>(end) >= static_cast<int16_t>(now)) {
            SetRam16(camera + kX, end);
            SetRam16(camera + kCameraStep, static_cast<uint16_t>(end - was));
        } else {
            SetRam16(camera + kX, now);
            SetRam16(camera + kCameraStep, step);
        }
        SetRam8(camera + kCameraWay, 0);
        Return(c);
        return true;
    }

    // 017F3A: the camera (A6) steps towards the players. A mode that wants it somewhere of its own (battle.h) has it
    // there, as far as its ends let it (in the wide picture, 64 nearer the middle), the step taken kept for the far
    // planes to follow (+50, +44)
    bool CameraAcross(M68K &c) {
        const int wanted = modes::CameraAt().x;
        if (wanted < 0) {
            return false;
        }
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        const int was = static_cast<int16_t>(Ram16(camera + kX));
        const int now = std::clamp(wanted, CameraLeftEndNow() + LeftMargin(), CameraRightEndNow() - RightMargin());
        SetRam16(camera + kX, static_cast<uint16_t>(now));
        SetRam16(camera + kCameraStep, static_cast<uint16_t>(now - was));
        SetRam8(camera + kCameraWay, now < was ? 0 : 4);
        Return(c);
        return true;
    }

    // --- the players kept in ---

    // 003782: the player (A6) kept inside the picture, half its width (+118) in from each edge (the camera's X and
    // 0x180 on); which edge held it at +116; with bit 5 of its flags (+126, in D2: 003762) the other player is pushed
    // along by as much. Played wide, the edges are the wide picture's. (A5+6B9B set: not kept, 00378A.)
    bool KeepIn(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (RingOutLetsOut(player)) {
            Return(c);
            return true;
        }
        if (!Margin()) {
            return false;
        }
        const uint32_t other = Ram32(player + kOpponent) & 0xFFFFFF;
        c.a[4] = Ram32(player + kOpponent);
        SetRam8(player + kAtEdge, 0);
        if (Ram8(0xFFEB9B)) {
            Return(c);
            return true;
        }
        const bool pushes = (c.d[2] & 0x20) != 0;
        const int32_t camera = CameraX();
        const int32_t half = static_cast<int16_t>(Ram16(player + kHalfWidth));
        const int32_t x = static_cast<int32_t>(Ram32(player + kX));
        const int32_t left = (camera + half - Margin()) * 0x10000;
        if (left >= x) {
            SetRam8(player + kAtEdge, 1);
            SetRam32(player + kX, static_cast<uint32_t>(left));
            if (pushes) {
                SetRam32(other + kX, Ram32(other + kX) + static_cast<uint32_t>(left - x));
            }
        }
        const int32_t x2 = static_cast<int32_t>(Ram32(player + kX));
        const int32_t right = (camera + 0x180 - half + Margin()) * 0x10000;
        if (right <= x2) {
            SetRam8(player + kAtEdge, 2);
            SetRam32(player + kX, static_cast<uint32_t>(right));
            if (pushes) {
                SetRam32(other + kX, Ram32(other + kX) + static_cast<uint32_t>(right - x2));
            }
        }
        Return(c);
        return true;
    }

    // --- the picture ---

    int CameraLeftEnd() {
        return static_cast<int16_t>(Ram16(kCameraLeftEnd));
    }
    int CameraRightEnd() {
        return static_cast<int16_t>(Ram16(kCameraRightEnd));
    }
    bool InFight() {
        return Ram8(kInGame) != 0 && Ram16(kMode) == kGameRunning && Ram16(kStep) == kGameMatch;
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
        if (g_shown && OnStage(camera) && OnStage(shown)) {
            g_side = kSide;
            // (the view's left edge is 64 left of the camera, and no further left than the camera's own end; with
            // invisible walls the camera goes nearer, and the board's picture is then to one side)
            g_shift = wide::ViewShift(shown, CameraLeftEnd(), CameraRightEnd(), kSide, StageEnds());
        }
        Video::Wide &wide = machine.video.wide;
        wide.side = g_side;
        wide.scroll[0] = wide.scroll[1] = wide.scroll[2] = g_shift;
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

// The camera's place a frame ago, which the picture is drawn from, in a save state: a frame put back (a save state,
// online play's rollback) is drawn, and its players kept in, as it was the first time
void WidescreenState(Archive &ar) {
    ar.Mark(0x57494445); // WIDE
    ar.Pod(g_cameraBefore);
}

void WidescreenBeforeFrame(Machine &machine) {
    const int camera = CameraX(), shown = g_cameraBefore;
    g_cameraBefore = camera;
    // (fights only; the stage viewer keeps the board's picture. With invisible walls the rules stay the board's)
    g_shown = FeatureOn("widescreen") && InFight() && !modes::StageViewer();
    g_play = g_shown && !FeatureOn("widescreen_walls");
    SetWide(machine, camera, shown);
}

// A spare machine's frame (the modes' other machines) is drawn as wide as the port's picture is, so that the two can
// be put together: from the spare's own camera (where it is now, for want of where it was)
void WideForSpare(Machine &machine) {
    g_kept = {g_side, g_shift};
    const int camera = CameraX();
    SetWide(machine, camera, camera);
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
    // Where a player's fighter stands in the picture, for the window's zoom (fight/alternative.h, modes::Focus): the
    // middle of its feet, the wide picture's columns counted (the floor at line 192 with the camera at its lowest)
    void FocusPlace(int player, int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(player) + kX)) - CameraX() + WideStage();
        y = 192 - (static_cast<int16_t>(Ram16(Player(player) + kY)) - static_cast<int16_t>(kGround) - CameraY());
    }
} // namespace

void RegisterWidescreen() {
    modes::Focus(FocusPlace, 40, 136);
    TheGame().widestPicture = [] { return Video::kWidth + (FeatureOn("widescreen") ? 2 * kSide : 0); };
    AddHook(HookAt(0x017F3A), "CameraAcross", CameraAcross);
    AddHook(HookAt(0x017FCC), "CameraRight", CameraRight);
    AddHook(HookAt(0x01800E), "CameraLeft", CameraLeft);
    AddHook(HookAt(kKeptInPicture), "KeepIn", KeepIn);
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
