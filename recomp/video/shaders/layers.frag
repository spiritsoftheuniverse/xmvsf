#version 450
#extension GL_GOOGLE_include_directive : require
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// The picture's first pass (docs/NATIVE_VIDEO.md, the GPU renderer): at each pixel the three scroll screens in the layer
// control's order, back to front, from the game's maps and row scroll's table in video RAM and the graphics ROMs;
// out: the pen of the front one drawn there (the backdrop's, 0xBFF, if none) and the marks of those drawn (a bit for
// each place in the order). As native_render.cpp's DrawScroll, a pixel at a time.
#include "common.glsl"
layout(location = 0) out uint result;
layout(std430, set = 2, binding = 0) readonly buffer Graphics { uint graphics[]; };
layout(std430, set = 2, binding = 1) readonly buffer Video { uint video[]; };
layout(std430, set = 2, binding = 2) readonly buffer Lines { uint lines[]; };
// (a line's tiles given beyond the board's columns: the column its left edge is at (16 bits) and the screen (above);
// then the tile and its attributes)
layout(std430, set = 2, binding = 3) readonly buffer Overrides { uint overrides[]; };
// (another set's graphics, and which tiles of the maps are drawn from them: a byte for each four bytes of video RAM)
layout(std430, set = 2, binding = 4) readonly buffer ExtraGraphics { uint extraGraphics[]; };
layout(std430, set = 2, binding = 5) readonly buffer ScrollExtra { uint scrollExtra[]; };

uint Word(uint at) {
    return at + 1u < VIDEO_BYTES ? (BYTE(video, at) << 8) | BYTE(video, at + 1u) : 0u;
}

// Scroll screen `which` (0-2) at the pixel: true and its pen and tile's attributes if it draws there
bool Screen(uint which, uint x, uint line, uint l, out uint pen, out uint penValue, out uint tileAttr) {
    const uint rasterLine = line + 16u, videoControl = lines[l + L_VIDEO_CONTROL];
    const uint base = (lines[l + L_MAPS + which] * 256u) & ~0x3FFFu & 0x3FFFFu;
    const uint shift = 3u + which, size = 1u << shift, mapMask = (64u << shift) - 1u;
    uint scrollX = lines[l + L_SCROLL + which * 2u];
    const uint scrollY = lines[l + L_SCROLL + which * 2u + 1u];
    if (which == 1u && (videoControl & 1u) != 0u) {
        const uint rows = (lines[l + L_ROWS] * 256u) & ~0x7FFu & 0x3FFFFu;
        scrollX += Word(rows + (((rasterLine + lines[l + L_ROW_OFFSET]) & 0x3FFu) * 2u));
    }
    const uint y = (rasterLine + scrollY) & mapMask, row = y >> shift;
    const int ownLeft = int(lines[l + L_OWN_LEFT + which]);
    const uint mx = uint(64 - ownLeft + int(scrollX) + int(x)) & mapMask; // (the map's column of pixels this pixel shows)
    const uint column = mx >> shift, k = mx & (size - 1u);
    const int tileAt = int(x) - int(k); // (the picture's column the tile's left edge is at)
    uint index;
    if (which == 0u) index = (row & 0x1Fu) + (column << 5) + ((row & 0x20u) << 6);
    else if (which == 1u) index = (row & 0x0Fu) + (column << 4) + ((row & 0x30u) << 6);
    else index = (row & 0x07u) + (column << 3) + ((row & 0x38u) << 6);
    uint code = Word(base + index * 4u);
    uint attr = Word(base + index * 4u + 2u);
    if (tileAt < ownLeft || tileAt + int(size) > ownLeft + 384) {
        // (beyond the board's own columns: the game's sources' tile, if they gave one)
        const uint count = min(lines[l + L_OVERRIDES], OVERRIDES), o = line * OVERRIDES * 2u;
        for (uint n = 0u; n < count; n++) {
            const uint w = overrides[o + n * 2u];
            if ((w >> 16) == which && (int(w << 16) >> 16) == tileAt) {
                code = overrides[o + n * 2u + 1u] & 0xFFFFu;
                attr = overrides[o + n * 2u + 1u] >> 16;
                break;
            }
        }
    }
    // (a CPS-2's scroll screens' tiles are in the graphics' second 8 MB; a CPS-1's where the objects' are)
    const uint tiles = lines[l + L_CPS1] != 0u ? 0u : 0x800000u;
    uint at, rowBytes;
    if (which == 0u) { at = tiles + code * 64u + ((column & 1u) != 0u ? 4u : 0u); rowBytes = 8u; }
    else if (which == 1u) { at = tiles + code * 128u; rowBytes = 8u; }
    else { code &= 0x3FFFu; at = tiles + code * 512u; rowBytes = 16u; }
    if (lines[L_GFX_ON] != 0u) {
        // (through the graphics mapper, in 8-pixel units: native_render.cpp's DrawScroll)
        const uint unit = code << (which == 0u ? 0u : which == 1u ? 1u : 3u), entry = unit >> 8;
        if (entry >= 0x400u || int(lines[L_GFX_BASE + (which + 1u) * 0x400u + entry]) < 0) {
            return false;
        }
        at = (lines[L_GFX_BASE + (which + 1u) * 0x400u + entry] + (unit & lines[L_GFX_MASK + (which + 1u) * 0x400u + entry])) * 64u +
             (which == 0u && (column & 1u) != 0u ? 4u : 0u);
    }
    const uint tileLine = y & (size - 1u);
    const bool flipX = (attr & 0x20u) != 0u, flipY = (attr & 0x40u) != 0u;
    at += (flipY ? size - 1u - tileLine : tileLine) * rowBytes;
    const bool extra = lines[L_EXTRA_SIZE] != 0u && base + index * 4u < VIDEO_BYTES && BYTE(scrollExtra, (base + index * 4u) / 4u) != 0u;
    if (at + rowBytes > lines[extra ? L_EXTRA_SIZE : L_GRAPHICS_SIZE]) {
        return false;
    }
    const uint p = flipX ? size - 1u - k : k;   // (the pixel of the tile's row: 8 to each 4 bytes, one plane each)
    const uint part = at + (p >> 3) * 4u, s = 7u - (p & 7u);
    uint b0, b1, b2, b3;
    if (extra) {
        b0 = BYTE(extraGraphics, part); b1 = BYTE(extraGraphics, part + 1u); b2 = BYTE(extraGraphics, part + 2u); b3 = BYTE(extraGraphics, part + 3u);
    } else {
        b0 = BYTE(graphics, part); b1 = BYTE(graphics, part + 1u); b2 = BYTE(graphics, part + 2u); b3 = BYTE(graphics, part + 3u);
    }
    const uint value = ((b0 >> s) & 1u) | (((b1 >> s) & 1u) << 1) | (((b2 >> s) & 1u) << 2) | (((b3 >> s) & 1u) << 3);
    if (value == 15u) {
        return false;
    }
    pen = ((attr & 0x1Fu) + 0x20u * (which + 1u)) * 16u + value;
    penValue = value;
    tileAttr = attr;
    return true;
}

// A CPS-1's pixel: its layer control's four places back to front, the objects' (0) among them. Out: the pen of the
// screens behind the objects (bits 0-11), whether the one just behind them has a pen there its tile's mask puts in front
// of them (bit 12), that the objects are drawn (bit 13); and the pen of the screens in front of them (bits 16-27), if
// any drew (bit 31)
uint Cps1(uint x, uint line, uint l) {
    const uint control = lines[l + L_LAYER_CONTROL], videoControl = lines[l + L_VIDEO_CONTROL];
    const uint enable = lines[l + L_ENABLE];
    uint below = 0xBFFu, above = 0u, high = 0u;
    bool passed = false, objects = false, anyAbove = false;
    for (uint n = 0u; n < 4u; n++) {
        const uint layer = (control >> (6u + 2u * n)) & 3u;
        if (layer == 0u) {
            passed = true;
            objects = (lines[l + L_LAYER_MASK] & 1u) != 0u;
            continue;
        }
        const bool on = (lines[l + L_LAYER_MASK] & (1u << layer)) != 0u && (control & ((enable >> (8u * (layer - 1u))) & 0xFFu)) != 0u &&
                        (layer == 1u || (videoControl & (layer == 2u ? 4u : 8u)) != 0u);
        if (!on) {
            continue;
        }
        const bool belowObjects = n < 3u && ((control >> (6u + 2u * (n + 1u))) & 3u) == 0u;
        uint pen, value, attr;
        const bool drawn = Screen(layer - 1u, x, line, l, pen, value, attr);
        if (belowObjects) {
            high = drawn ? (lines[l + L_MASKS + ((attr >> 7) & 3u)] >> value) & 1u : 0u;
        }
        if (drawn) {
            if (passed) {
                above = pen;
                anyAbove = true;
            } else {
                below = pen;
            }
        }
    }
    return below | (high << 12) | (objects ? 1u << 13 : 0u) | (above << 16) | (anyAbove ? 0x80000000u : 0u);
}

void main() {
    const uint x = uint(gl_FragCoord.x), line = min(uint(gl_FragCoord.y), 223u);
    const uint l = line * L_WORDS;
    if (lines[l + L_CPS1] != 0u) {
        result = Cps1(x, line, l);
        return;
    }
    const uint control = lines[l + L_LAYER_CONTROL], videoControl = lines[l + L_VIDEO_CONTROL];
    uint order[3] = uint[3]((control >> 6) & 3u, (control >> 8) & 3u, (control >> 10) & 3u);
    const uint l3 = (control >> 12) & 3u;
    if (order[0] == 0u) { order[0] = order[1]; order[1] = 0u; }
    if (order[1] == 0u) { order[1] = order[2]; order[2] = 0u; }
    if (order[2] == 0u) { order[2] = l3; }
    uint pen = 0xBFFu, marks = 0u;
    for (uint n = 0u; n < 3u; n++) {
        const uint layer = order[n];
        bool on = (lines[l + L_LAYER_MASK] & (1u << layer)) == 0u ? false
                : layer == 1u ? (control & 2u) != 0u
                : layer == 2u ? (control & 4u) != 0u && (videoControl & 4u) != 0u
                : layer == 3u ? (control & 8u) != 0u && (videoControl & 8u) != 0u : false;
        uint p, value, attr;
        if (on && Screen(layer - 1u, x, line, l, p, value, attr)) {
            pen = p;
            marks |= 1u << n;
        }
    }
    result = pen | (marks << 16);
}
