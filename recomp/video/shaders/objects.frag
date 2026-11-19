#version 450
#extension GL_GOOGLE_include_directive : require
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// An object tile's pixel: its pen, its level and that it is an object's; nothing (and no depth) where it is clear (pen 15)
#include "common.glsl"
layout(location = 0) in vec2 within;
layout(location = 1) flat in uint tile;
layout(location = 2) flat in uint looks;
layout(location = 3) flat in uint graphicsSize;
layout(location = 0) out uint result;
layout(std430, set = 2, binding = 0) readonly buffer Graphics { uint graphics[]; };
layout(std430, set = 2, binding = 1) readonly buffer ExtraGraphics { uint extraGraphics[]; }; // (bit 18 of `looks`: from these)

void main() {
    const uint k = min(uint(within.x), 15u), line = min(uint(within.y), 15u);
    const bool flipX = (looks & 0x10000u) != 0u, flipY = (looks & 0x20000u) != 0u;
    const uint at = tile * 128u + (flipY ? 15u - line : line) * 8u;
    if (at + 8u > graphicsSize) {
        discard;
    }
    const uint p = flipX ? 15u - k : k;
    const uint part = at + (p >> 3) * 4u, s = 7u - (p & 7u);
    uint b0, b1, b2, b3;
    if ((looks & 0x40000u) != 0u) {
        b0 = BYTE(extraGraphics, part); b1 = BYTE(extraGraphics, part + 1u); b2 = BYTE(extraGraphics, part + 2u); b3 = BYTE(extraGraphics, part + 3u);
    } else {
        b0 = BYTE(graphics, part); b1 = BYTE(graphics, part + 1u); b2 = BYTE(graphics, part + 2u); b3 = BYTE(graphics, part + 3u);
    }
    const uint value = ((b0 >> s) & 1u) | (((b1 >> s) & 1u) << 1) | (((b2 >> s) & 1u) << 2) | (((b3 >> s) & 1u) << 3);
    if (value == 15u) {
        discard;
    }
    // (the pen, the level above it, the panel's (bit 19) and in front of the fighters (20), and the top bit: an object's)
    result = ((looks & 0xFFFFu) + value) | (((looks >> 20) & 7u) << 16) | (((looks >> 23) & 3u) << 19) | 0x80000000u;
}
