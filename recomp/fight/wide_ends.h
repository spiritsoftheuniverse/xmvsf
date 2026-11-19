// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Widescreen at a stage's ends (issues #239, #116): the wide picture never shows a scroll screen past what the board
// itself shows of it. On the board a stage's scroll screens are painted for the camera's whole range and the 384
// columns from it; the wide picture shows `side` more on each side, so near an end it could show past that (a gap,
// or art the board never shows).
//
// - The stage's own plane, and its floor (whose lines go at speeds of their own), keep inside it by the camera: it
//   stops `side` short of each of the board's ends, and on some stages further in still (Ends, measured for each stage
//   by tools/stage_ends.sh). Fights played in the wide picture then end that much sooner.
// - A far plane, one going slower than the camera (Street Fighter Alpha's third scroll screen goes at half its speed),
//   would need the camera much further in still. It is drawn moved instead (FarScroll): over the camera's range it
//   goes a little slower than on the board, so that it comes to its own art's ends just as the camera comes to its own.
//   Only the picture changes: nothing the game works out is touched.
//
// With invisible walls the camera goes to the board's ends, and the view is kept on the art (ViewShift): the board's
// picture then to one side of the wide one, never past its edge.
#pragma once

#include <algorithm>
#include <cstdint>

namespace cps2recomp::wide {

// How much further in than `side` the camera stops at each end of a stage, for its own plane and floor
struct Ends {
    int left = 0, right = 0;
};

// How far the whole view is drawn moved (Video::Wide::scroll, the objects with it) for the picture being made from the
// camera at `camera`, between the board's ends end0 and end1: none while the view is on the stage's art; nearer an end
// (invisible walls: the camera goes there) the view stays where it was, up to `side` (so the board's picture is always
// inside the wide one)
inline int ViewShift(int camera, int end0, int end1, int side, Ends ends) {
    const int from = end0 + side + ends.left, to = std::max(from, end1 - side - ends.right);
    return std::clamp(camera - std::clamp(camera, from, to), -side, side);
}

// Video::Wide::scroll for a far plane that goes at `num`/`den` of the camera's speed, for the picture made from the
// camera at `camera`: its view kept inside what the board shows of it (its places with the camera at the board's ends
// end0 and end1, and the 384 from them). The camera goes from `from` to `to` in the picture played in (with invisible
// walls: the board's ends); the whole view is drawn moved by ViewShift.
inline int FarScroll(int num, int den, int camera, int end0, int end1, int from, int to, int side, Ends ends) {
    if (to <= from) {
        return ViewShift(camera, end0, end1, side, ends);
    }
    camera = std::clamp(camera, from, to);
    // (places along the plane from where it is now: its view's left edge where nothing moves it, at a camera place)
    auto view = [&](int at) { return (at - camera) * num / den - side - ViewShift(at, end0, end1, side, ends); };
    const int lo = (end0 - camera) * num / den, hi = (end1 - camera) * num / den; // (its places at the board's ends)
    const int first = std::max(lo, view(from)), last = std::min(hi - 2 * side, view(to));
    // (its view's left edge: from the first to the last as the camera goes from its one end to the other; painted
    // narrower than the wide picture, it stays in the middle of what there is)
    const int left = last < first ? (first + last) / 2 : first + (last - first) * (camera - from) / (to - from);
    return -side - left;
}

// A stage's facts for the wide picture: how much further in the camera stops, and each scroll screen's places across
// with the camera at the board's ends where it is a far plane drawn moved ({0, 0}: it goes with the view)
struct Stage {
    Ends ends;
    int16_t places[3][2];
};

// The far planes' Video::Wide::scroll (the others left as they are: the view's shift), for the picture made from the
// camera at `camera` between the board's ends; `play`: fights played in the wide picture (the camera then stops short
// of its ends), else invisible walls (it goes to them)
inline void FarPlanes(int scroll[3], const int16_t places[3][2], int camera, int end0, int end1, bool play, int side, Ends ends) {
    const int from = play ? end0 + side + ends.left : end0, to = play ? end1 - side - ends.right : end1;
    for (int n = 0; n < 3; n++) {
        if (places[n][1] > places[n][0]) {
            scroll[n] = FarScroll(places[n][1] - places[n][0], end1 - end0, camera, end0, end1, from, to, side, ends);
        }
    }
}

} // namespace cps2recomp::wide
