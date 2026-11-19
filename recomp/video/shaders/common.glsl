// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
// (shared by the picture's shaders: a byte of a buffer of bytes read as 32-bit words, and a line's record's fields)
#define BYTE(buf, at) ((buf[(at) >> 2] >> (8u * ((at) & 3u))) & 0xFFu)
// A line's record (video/gpu_picture.cpp, kLineWords words): the layers' registers it is drawn with
#define L_MAPS 0u        // 3 words
#define L_ROWS 3u
#define L_SCROLL 4u      // 6 words: across and down for each screen
#define L_ROW_OFFSET 10u
#define L_VIDEO_CONTROL 11u
#define L_LAYER_CONTROL 12u
#define L_PRIORITY 13u   // (the object list's priority control)
#define L_SIDE 14u       // (a wider picture's columns each side: PictureOptions::side)
#define L_WIDTH 15u      // (the picture's width)
#define L_OWN_LEFT 16u   // 3 words: where the board's own columns start, for each screen (moved: the game's sources')
#define L_FILL_HOLES 19u
#define L_OVERRIDES 20u  // (how many tiles the game's sources give beyond the board's columns on the line)
#define L_CPS1 21u       // (a CPS-1's line: its layers' order and priority masks are another board's)
#define L_ENABLE 22u     // (a CPS-1's: the layer control's bits that switch scroll screens 1-3 on, a byte each)
#define L_LAYER_MASK 23u // (PictureOptions::layerMask: the layers drawn, bit 0 the objects, 1-3 the scroll screens)
#define L_OVER 28u       // (the objects alone over another picture: a guest's stage; out, their pixels and nothing else)
#define L_MARKS 29u      // (bit 0: the panel's objects told from the rest, PictureOptions::panelObject; bit 1: the pixels in
                         // front of the fighters marked, markFront; bit 3: the objects alone drawn over another game's
                         // stage in the same pass, colours.frag's `stage`; bits 4-6 frontLevel;
                         // bits 16-23 the stage's line under the line, Machine::guestShift)
#define L_MASKS 24u      // (a CPS-1's: 4 words, its priority masks: the pens of a screen in front of the objects)
#define L_WORDS 32u
// (each line's tiles given beyond the board's columns: kOverrides of them, two words each)
#define OVERRIDES 32u
// (after the 224 lines' records: the graphics ROMs' size, in bytes; then another set's graphics' size, 0 if none)
#define L_GRAPHICS_SIZE (224u * L_WORDS)
#define L_EXTRA_SIZE (224u * L_WORDS + 1u)
// (a CPS-1 whose graphics mapper does not keep the tiles in order: whether it has one, then its table, machine/romset.h's
// GfxTable: for each kind (0 the objects, 1-3 the scroll screens) and each 0x100 units, the base (-1: not drawn), then
// the masks)
#define L_GFX_ON (224u * L_WORDS + 2u)
#define L_GFX_BASE (224u * L_WORDS + 3u)
#define L_GFX_MASK (224u * L_WORDS + 3u + 4u * 0x400u)
// (video RAM's size, 900000-92FFFF: the maps' and row scroll's addresses can be set up to 0x3FFFF, and nothing is read
// beyond a buffer, which Metal does not check)
#define VIDEO_BYTES 0x30000u
