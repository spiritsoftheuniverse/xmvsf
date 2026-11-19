# Street Fighter II: Hyper Fighting: progress

The World release of 1992-12-09 (ROM set `sf2hf`, the parent set), on the classic CPS-1 board. Added as a first port in
PR #298; brought up to the other fighting games on 2026-11-08 by building Super Street Fighter II Turbo's sources, as
Champion Edition does ([its PROGRESS.md](../sf2ce/PROGRESS.md) says how, and what the classic board is).

## What is this game's own

Hyper Fighting is Champion Edition's program changed: `tools/portmap.py` places 92% of Champion Edition's instructions
here (25,908 of 28,135), and its work RAM is Champion Edition's, variable for variable. So its `family.h` is Champion
Edition's list carried through that map (each line says which of Champion Edition's places it came from), every code
place looked at again in the listing: much of the code moved (a game begins at 0B99D0, the panel's names at 004E40, the
player's routine at 081178, a person's step 013A32), and the stun's word is at 084A16. Its graphics and sound program are
Champion Edition's own files (the same CRCs), so the music and its names are too.

- **Colours**: Hyper Fighting recoloured the characters. The common block of object palettes is at 0ADA80 (its copy
  at 0015D8), the other colours' table at 09E676 (the routine at 09E632, Champion Edition's 0967C0); PalMod's file is
  `s2te_22.7f`.
- **Translated code**: a fresh tree's entries were Champion Edition's moved over by portmap (2,800), with what the
  first port had found; `make explore` with the shared sources found the rest.
- The game calls the same routines in video RAM as Champion Edition (its PROGRESS.md): those only run on the interpreter.

## The checks

All with the build of this branch, on the mini.

- `check` over the ladder (44,000 frames, `free_play,no_damage,one_hit`; M. Bison beaten at about frame 32,000, the title again at about 42,000) and the versus movie (12,000 frames): ok
  (translated code is the interpreter's, two runs alike).
- `check-blocks` over the ladder: 406,646,243 translated blocks checked against the interpreter, and 47,393,590 of the sound
  driver's.
- `check-rewrites` (Super Turbo's rewrites of the camera across and up and of keeping the players in, on this game's
  own code): CameraAcross 46,540 calls, CameraUp 50,892, KeepIn 108,494 on the ladder, none differed.
- `check-rollback` (three ways, 400 rollbacks each) and `check-online`: ok.
- `check-widescreen`: every stage painted to both ends (Vega's cage 10 columns short at its right end, under the 20
  allowed); with invisible walls the game's memory is the board's at every 4,000 frames of the ladder.
- `check-release FRAMES=40000`: ok (the picture hash in the pull request).
- By hand: no instruction on the interpreter in the ladder, two made-up games, versus, attract mode and the test menu (this game calls no routine in RAM); features off with no sources (the build without `sources = ssf2t`, `--cpu interp --no-rewrites`)
  the same hash log over the smoke movie; run-ahead 1 and 2 the hash log of 0; a save state loaded at frame 3,000 the
  same from 3,001 on; the launcher's pages; pictures of every mode; the soundtrack's log.

## Next

- Nobody has played it by hand yet.
- Its stages are not guests in the other games yet.
