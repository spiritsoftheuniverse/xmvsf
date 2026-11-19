#version 450
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// The picture with the readouts over it (port/gpu_picture.cpp, Shown): the readouts' colours are already times their
// alpha, so over the picture is theirs plus the picture's times what they leave; the rows of a picture stacked two high
// (zoomed: the game's above, with the readouts below) are each the picture's
layout(location = 0) out vec4 colour;
layout(set = 2, binding = 0) uniform sampler2D picture;
layout(set = 2, binding = 1) uniform sampler2D readouts;

void main() {
    const ivec2 at = ivec2(int(gl_FragCoord.x), int(gl_FragCoord.y) % 224);
    const vec4 p = texelFetch(picture, at, 0), r = texelFetch(readouts, at, 0);
    colour = vec4(r.rgb + p.rgb * (1.0 - r.a), 1.0);
}
