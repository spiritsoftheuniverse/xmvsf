# Final Fight: progress

## How it is made

- **The ROM set**: `ffight` (World, set 1, parent set). No key file. `ff-32m.8h` is a 16-bit little-endian ROM (no `/be`).
- **The board**: classic CPS-1 (`board = cps1-classic pairs=2`, `cpu_clock = 10`, YM2151 + OKI MSM6295). CPS-B-04
  (`cps_b = 2E 26 30 28 32 2A 02 04 08 20 0004`), graphics mapper (`gfx_map = 8000 | objects:0-43ff:0 scroll1:4400-4bff:0 scroll3:4c00-5fff:0 scroll2:6000-7fff:0`),
  default DIPs `FF F4 9F`.
- **Translated code**: `make explore` found 619 entries (57,278 instructions; 77 left to the interpreter, 57 data) and
  71 Z80 entries (15,399 instructions; 13 left to the interpreter). `make check`, `check-blocks`, `check-release`,
  `check-rollback`, `check-online`, and `--video gpu-check` all pass.

## What is known of the game (`src/ffight.h`)

- `A5 = 0xFF8000`.
- **Game step** (`FF8000` word): `0` title/attract, `2` character select, `4` stage setup, `6` gameplay, `8` area clear,
  `0xA` stage clear, `0xC` ending, `0xE` bonus stage.
- **Attract mode**: `000E64` is the region warning screen loop (`0x258` frames); `017092` dispatches the attract loop via
  `kShow = FF1288` (`-0x6D78(a5)`, `0x00..0x14`) and jump table `0x0170CC`.
- **Stage & progression**: `kStage = FF80BE` (`0..5`, `6..7` bonus), `kArea = FF80BF`, `kProgression = FF80C1` (index into
  `0x0054FC`), `kBonusFlag = FF8122`. Hook `0x004C54` (`kGameBegins`) sets them for `start_stage` and Stage Viewer.
- **Players**: two `0xC0`-byte records from `FF8568`: `+00` in play, `+14` character (`0..2`), `+18` health, `+1A` red bar,
  `+1C` max health (`0x90`), `+80` lives, `+84` BCD score.
- **Timer**: `FF80AF` (BCD), `FF80B0` frame counter.
- **Sound**: Z80 song table at `0x1003` (`0x60` entries of 2 bytes big-endian); 19 music tracks mapped in `music.cpp`.
- **Widescreen**: not implemented yet (beat-'em-up widescreen tracked in #92).
