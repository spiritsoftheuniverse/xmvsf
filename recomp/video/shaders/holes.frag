#version 450
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// Holes in a wider picture's scroll screens (PictureOptions::fillHoles): a pixel nothing was drawn at (the backdrop's pen),
// in a run of such no wider than the added side, takes the line's own picture mirrored at the nearest drawn pixel (or
// that pixel, where the mirror finds nothing). As native_render.cpp's FillHoles; the marks are left. Other lines, and
// other pixels: as they are.
layout(location = 0) out uint result;
layout(set = 2, binding = 0) uniform usampler2D screens;  // the first pass's: pen, marks above
layout(set = 2, binding = 1) uniform usampler2D lines;    // a line's record a row (common.glsl)

uint Pen(int x, int y) {
    return texelFetch(screens, ivec2(x, y), 0).r & 0xFFFFu;
}

void main() {
    const int x = int(gl_FragCoord.x), y = int(gl_FragCoord.y);
    const uint here = texelFetch(screens, ivec2(x, y), 0).r;
    const int side = int(texelFetch(lines, ivec2(14, y), 0).r), width = int(texelFetch(lines, ivec2(15, y), 0).r);
    result = here;
    if (texelFetch(lines, ivec2(19, y), 0).r == 0u || side <= 0 || (here & 0xFFFFu) != 0xBFFu) {
        return;
    }
    // (the run of backdrop this pixel is in: looked for no further than `side` each way, which is all a fill takes)
    int start = x, end = x + 1;
    while (start > 0 && x - start <= side && Pen(start - 1, y) == 0xBFFu) start--;
    while (end < width && end - x <= side && Pen(end, y) == 0xBFFu) end++;
    const bool left = start > 0, right = end < width;
    if (end - start > side || !(left || right)) {
        return;
    }
    const bool fromLeft = left && (!right || x - (start - 1) <= end - x);
    const int edge = fromLeft ? start - 1 : end;
    const int mirror = 2 * edge - x;
    const uint pen = mirror >= 0 && mirror < width && Pen(mirror, y) != 0xBFFu ? Pen(mirror, y) : Pen(edge, y);
    result = pen | (here & 0xFFFF0000u);
}
