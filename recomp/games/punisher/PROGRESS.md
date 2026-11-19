# The Punisher: progress

## How it is made (2026-11-01)

- **The ROM set** `punisheru` (USA 930422), whose parent is `punisher` (World). Every file's CRC32 is MAME's. No key
  file; the PAL dumps in the set are not read.
- **The board**: a CPS-1 with QSound, as Cadillacs and Dinosaurs (`board = cps1` in game.cfg; the toolkit's machine,
  video and ROM set have it: [docs/TOOLKIT.md](../../docs/TOOLKIT.md)). Two things this game needed of the toolkit:
  - Its first eight program files are pairs, even bytes then odd (MAME's `ROM_LOAD16_BYTE`), and the last file is
    16-bit words: `board = cps1 pairs=4` (machine/romset.cpp interleaves the pairs).
  - Its CPS-B-21 is set up as MAME's table has it for this game (QS3: layer control at 12, priority masks 14 16 08 0A,
    palette control 0C, layer enable bits 04 02 20), and the chip gives its ID, 0C00, at register 0E. The game reads
    it as it starts (000598: `$80014E` and FC3F must be 0C00) and stops at 0006AA, a loop on itself, when it is not:
    the two numbers after the nine (`Machine::ReadCps1`).
- **The sound program** (`ps_q.5k`): Capcom's QSound driver, its key worked out as for Cadillacs and Dinosaurs
  (`machine/kabuki.cpp`).
- **Translated code**: `make explore` (with free play and the cheats, so that made-up play goes through the stages),
  then each stage, the viewers, attract mode, score attack, the test menu and the sound test run with
  `--record-entries` (5,271 entries): about 127,000 instructions, 31 left to the interpreter (19 of them data); the
  sound program all translated (2,232 instructions). `make check` passes.

## What is known of the game (punisher.h has the names)

- A5 is FF8000; the variables are mostly below it.
- **Credits**: the word at A5-24EC (up to 9; 09BF5A adds, 09C2B4 takes).
- **Free play**: the board's own. The byte at A5-24C4 lets Start through with no credit (09C2A0); the coin set-up
  (006A4E) sets it when the test menu's coin mode (A5-24AF) is 9, FREE PLAY. Two checks of the program's own put it
  on too (003F02, 004B2A: a changed program plays for free), and a game begun with it on takes its stage from a
  switch byte (A5-24D9 & 7, 0 on this board: 007C6A).
- **Settings**: the record the settings memory keeps at A5-7CFA (FF0306), +0C to +14: coin mode, continue, monitor
  flip, demo sound, sound, difficulty (0-7, 3), number of players (lives, 0-3 for 1-4, 1), extend (800,000 /
  1,800,000 / 2,800,000 / none, 1), player select. 006BEA copies them into variables as the board starts (006C14:
  A5-24AF, -24BA, -24BF, -24BB, -24A9, -24AB, -24AC, -24AD, -24A8); the coin set-up then works out the values the game
  plays by (A5-24BD lives, -24BC the difficulty's level...). The test menu edits a copy (A5-7C05 on), copied from the
  variables as it opens (006F9E) and back as it is left (006D10). Found by stepping each option with Right.
  Player select is the players' screen after Start (09BF90), which shows the Punisher for player 1 and Nick Fury for
  player 2: the players are not chosen.
- **A game**: A5-24C0 is set as soon as a credit is in (09C05A, before Start) and cleared at the game over (00AA0A);
  A5-24C2 has the players in a game (bits, from Start: 09C02C). The game's own task (007A8A) keeps its step at A5-2532
  (the table at 007B7C): 0 a game's start (007B90; 007C76 has set the stage), 4 a stage's title, 8 a stage played, 0C a
  while between one part of a stage and the next, 10 a stage's end (its results). The demonstrations run it too, and
  it is not put back to 0 at a game over.
- **Stages**: the byte at A5-2447, 0-5 the six stages and 7 the bonus round (breaking barrels) between stages 3 and 4;
  007E66 takes the next from the table at 007EA2: 0, 1, 2, 7, 3, 4, 5, then the ending. Found by starting a game at
  each: 1 Back to the War, 2 Raid on Pantaberde, 3 Waterfront Warfare, 4 Death on Rails, 5 Kingpin's Wrath, 6 Final
  Punishment.
- **The clock** (the panel's TIME, 30 at each life): its digits as words of twice their value (A5-6752 tens, A5-6754
  units), counted down each time A5-6756 runs out from 0xBF (0AEB3C), and not while A5-1FEB is set. Found from the
  digits' drawing: FF18AD, the units times two, was the one byte that followed the clock in dumps.
- **Players**: two records of 0x100 from FF8E68 (0E68(a5), 0F68(a5)): +00 in play, +02 the player (0, 1), +10 to +1C
  the animation (below), +20 the place across, +28 the floor, +36 health (word, 0x88 full; 0173F4 fills it with a
  life; damage from several routines: 09042A, 0907EE, 090D70, 0912C0, 091D08, 0925B2...). Lives left after this one
  at A5-245C + the player (01FE16 takes one), scores at A5-2456 and A5-2452 (BCD longs).
- **The show** (00827C) calls its parts in turn from 0082D6: the best scores (00A22E), the story (008464), the title
  (008666; 008330 in the second round), the demonstrations (00A0D4), the players' profiles (00A4E4), the FBI's words
  (00A33E). Attract mode returns from the parts not wanted.
- **Sound**: the sound program's table of songs is at 8006 after a word of their number (0x137), as Cadillacs and
  Dinosaurs'; music.cpp names the pieces found by playing each stage, attract mode and a game lost.
- **The camera** (viewer.cpp): an object at A5-CAA (FF7356), its place across at +20 and up and down at +24, its step at
  +08 (4 moving), its limits at +5E to +64. Its routine (0A84CC) moves it at most 4 pixels a frame and draws the new
  columns (0A9006, 0A9032) and rows (0A8FAE, 0A8FDA) from where it was (+40, +42, kept as it moves: 0A858A). A
  stage's parts are set up by the stage's own code (00E540 is stage 1's first: the camera, its limits and scroll 3's
  place written at once).
- **Animations**: a player's are by its number (020824: the tables at 108C30 for the Punisher and 111384 for Nick
  Fury, 163 each; 02083C: 109C10 and 1123FA, 27 each). A frame (as 00117C steps them): a word that says how far on its
  picture's words are, a byte for +11 and its time, whose top bit says the next word is how far on the next frame is.
  The record keeps the next frame at +16, the picture's words at +12 (and four words before them at +1C, +1A, +3C,
  +3E) and the time left at +10. The Punisher's objects are in colour 0, Nick Fury's in 1.

## What the port adds

Free play (the board's own flag, put back from the coin mode when it is switched off), straight to the title (run
unseen to the show's title), start at stage (007C76), the cheats (health and lives filled while a stage is played,
step 8 only: the results count the health left; the clock's frame count held), the Board page (choices put in the
variables before every frame but while the test switch is held, and in at 006848 before the board's start works out
the rest; the coin set-up run again as a game begins when any is chosen), the alternative modes (attract mode, score
attack, the stage and model viewers: the toolkit's `beat/`), and the music's names.

The stage viewer shows each stage's first stretch: the stage's own code moves the camera from one part to the next,
and the viewer cannot begin a game at a later part. The model viewer's game is begun with the character's own Start
(the Punisher player 1's, Nick Fury player 2's, with two credits).

## Checks (2026-11-01)

The ladder movie (`movies/ladder.txt`: Start, then made-up walking on and attacking) is played with free play and the
three cheats, which take it through stage 1 and into stage 2 (at frame 22,556) by frame 40,000.

- `make GAME=punisher check FRAMES=40000 MOVIE=games/punisher/movies/ladder.txt ARGS="--features
  free_play,infinite_health,infinite_time,infinite_lives"`: translated code does what the interpreter does, two runs
  alike.
- `check-blocks` over the same: 225 million translated blocks and 84 million of the sound driver's, no difference.
- `check-release FRAMES=40000`: two runs of the build with no translated code alike; its pictures the translated
  build's but for 10 frames, ending on the same one.
- `check-rollback` (3,000 frames, and again 12,000: 1,471 rollbacks) the same three ways; `check-online` the two sides
  the same over 2,998 frames, the names and the chat across, the match the same as its replay.
- No interpreter: `--stats` over each stage from its start (40,000 frames), 20,000 frames of attract, made-up play,
  the test menu, the stage and model viewers: no instruction in the interpreter, the sound driver's none either.
- Features off = no sources: 9,000 frames of a coin and a game with `--cpu interp --features -free_play`, against a
  build with no `src/`: the hash logs byte for byte the same.
- Straight to the title on and off, and run-ahead 1 and 2 against 0: the same hash logs (6,000 frames). A save state
  at frame 3000 loaded into a second run: the same from there on.
- The Board page: each choice in its variable (and the lives worked out from it at the board's start); with choices,
  the test menu shows the board's own, and the settings memory is the same with and without them.
- The cheats: the clock stays at 30, health at 0x88 and lives at 2 through a game standing still.
- Cadillacs and Dinosaurs, after the toolkit's two changes: 8,000 frames of a game the same as before them.
- The launcher's three pages, with no window.

## Not done yet

- The later parts of each stage in the stage viewer.
- The ending not reached, and its music not named; pieces not heard are listed by number only.
- Nobody has played it by hand; the sound has not been listened to.
