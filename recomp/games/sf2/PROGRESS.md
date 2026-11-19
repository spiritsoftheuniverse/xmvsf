# Street Fighter II: The World Warrior: progress

The World release of 1991-05-22 (ROM set `sf2`, the parent set and the last revision), on the classic CPS-1 board
with a 10 MHz 68000. Added 2026-11-08 with every feature the other fighting games have, by building Super Street
Fighter II Turbo's sources, as Champion Edition does ([its PROGRESS.md](../sf2ce/PROGRESS.md) says how, and what the
classic board is).

## The base port

- **The ROM set** `sf2`: from the Internet Archive (`ia-cps1/sf2`, the set the CPS-1 games came from); every file's
  CRC32 is MAME's (`cps1.cpp`, `ROM_START( sf2 )`). The program is four pairs of byte-wide files (`pairs=4`).
- **The board** (MAME's `cps1_10MHz` and its table of the chips): a CPS-B-11 (layer control 26, priority masks 28 to
  2E, palette control 30, the layers' bits 08, 10 and 20, its ID register at 32 reading 0401), the graphics mapper STF29
  (Champion Edition's ranges), a 68000 at 10 MHz (`cpu_clock = 10`). The DIP switches are Champion Edition's (MAME's
  `sf2` inputs).
- **The video library's writes** (`video_writes`): every `move` or `clr` to a CPS-A or CPS-B register in the listing,
  with those a census over attract mode and made-up play saw (`CPS2RECOMP_VIDEO_CENSUS`); the same script gives
  Champion Edition's list exactly. Without them the picture was black.

## What is this game's own

Champion Edition was made from this program: `tools/portmap.py` places two thirds of Champion Edition's instructions
here (18,804 of 28,135). Its `family.h` is Champion Edition's list carried through that map and looked at again, with
eleven places found by hand (each line says how):

- **Work RAM** is Champion Edition's moved by a few bytes: the players' records at FF83C6, the stage FF89CA, the clock
  FF8ACE, the camera FF8BD2 (copies at FF8BD8 and FF8CF4), the difficulty A5+2C7, free play A5+2D3. The records are laid
  out as Champion Edition's (every offset compared over the paired instructions); "has chosen" is +28F (set at 0079B0).
- **The players' steps**: a person's at 02AB98 (its dispatch on the state, 02AC1A), the computer's at 0287BA (its fight
  step, 028864); portmap had taken the computer's for the person's. The computer's set-up (0287CC) has no character
  tables and no plan's round, and keeps no plan in the record (it finds it again every step, 02936C): `computer.cpp`
  goes through what a game has, and takes +273 (which the set-up puts at 2) for "set up".
- **The stun** comes at a fixed 0x1E (`cmpi.w #$1E,0x5E(a3)` at 003340).
- **The characters**: eight to choose; the four bosses the computer's, as on the board. Training mode, the model viewer
  and the other modes have all twelve. No mirror match, so one set of colours a character: the common block at 08ACBA
  (001B44). The program is in byte-wide pairs, so there is no PalMod file.
- **The loop with no coin in**: Champion Edition's steps; the board's start ends at frame 874.
- **The sound program** is its own (`sf2_9.12a`), with Champion Edition's table at 1106 (0x8C songs) and the same
  numbers for the music.

## The checks

All with the build of this branch, on the mini.

- `check` over the ladder (46,000 frames, `free_play,no_damage,one_hit`; M. Bison beaten at about frame 36,000, the title again at about 44,000) and the versus movie (12,000 frames): ok
  (translated code is the interpreter's, two runs alike).
- `check-blocks` over the ladder: 389,609,146 translated blocks checked against the interpreter, and 49,535,062 of the sound
  driver's.
- `check-rewrites` (Super Turbo's rewrites of the camera across and up and of keeping the players in, on this game's
  own code): CameraAcross 31,621 calls, CameraUp 35,972, KeepIn 78,591 on the ladder, none differed.
- `check-rollback` (three ways, 400 rollbacks each) and `check-online`: ok.
- `check-widescreen`: every stage painted to both ends (Vega's cage 10 columns short at its right end, under the 20
  allowed); with invisible walls the game's memory is the board's at every 4,000 frames of the ladder.
- `check-release FRAMES=40000`: ok (the picture hash in the pull request).
- By hand: no instruction on the interpreter in the ladder, two made-up games, versus, attract mode and the test menu; features off with no sources (the build without `sources = ssf2t`, `--cpu interp --no-rewrites`)
  the same hash log over the smoke movie; run-ahead 1 and 2 the hash log of 0; a save state loaded at frame 3,000 the
  same from 3,001 on; the launcher's pages; pictures of every mode; the soundtrack's log.

## Next

- Nobody has played it by hand yet.
- Its stages are not guests in the other games yet.
