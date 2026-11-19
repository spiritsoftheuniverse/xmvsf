# Street Fighter II: Champion Edition: progress

The World release of 1992-05-13 (ROM set `sf2ce`, the parent set), on the classic CPS-1 board (no QSound). Added as a
first port in PR #298 (the game as the board plays it); brought up to the other fighting games on 2026-11-08 by building
Super Street Fighter II Turbo's sources (`games/ssf2t/src`), as Super Street Fighter II and Hyper Fighting do.

## The board (from the first port)

`board = cps1-classic` (machine/romset.h): a CPS-1 with no QSound. Three things the classic board needed and Cadillacs
and Dinosaurs had not:

- **The graphics mapper.** The PAL S9263B keeps the three scroll screens' tiles in its third bank (`gfx_map` in
  game.cfg, `GfxRange`); without it the backgrounds and the text were other tiles.
- **The CPS-B-21's multiplier** (800140 and 800142 multiplied, the product at 800144 and 800146): the game looks up
  the jump at the start of a fight with it, and jumped to nowhere and restarted without it.
- **The kicks** at 800176: player 1's in bits 0 to 2, player 2's in bits 4 to 6, light to hard.

**The sound** (`machine/cps1_sound.cpp`): the board's Z80 at 3.579545 MHz on its own map (ROM, a bank of it at 8000,
RAM at D000, the YM2151 at F000, the OKI at F002, the bank at F004, the OKI's pin 7 at F006), reading the 68000's
requests from the two latches at 800181 and 800189 (F008 and F00A); its interrupt the YM2151's line. The YM2151 is
ymfm's; the OKI MSM6295 (`sound/okim6295.cpp`) was written for the port from its data sheet.

**The DIP switches** are as the boards left the factory (`dip`, `FF FC 9F`): difficulty 3 (normal), continues
allowed, the demonstrations' sound. The Board page is the switches by name (`board_option_N`): difficulty, continue,
demo sound, two coins to start. The board reads them all the time, so a change applies at once.

## How the features came: Super Turbo's sources

Super Turbo's program grew out of this one (through Super Street Fighter II). `tools/portmap.py` places a quarter of
Super Turbo's instructions here at once (12,980 of 49,821), and the players' records, the camera's record and the step
words are laid out alike, so the game builds Super Turbo's sources (`sources = ssf2t`) with a `family.h` of its own:

- **The places** (`tools/places.py`, 57 of 80 at once; the rest by hand, each line says how). Work RAM from A5 is
  Super Turbo's with most variables lower (the players' records at FF83BE, 0x300 apart; the round's clock FF8ABE; the
  stage FF89BE; the camera's record FF8BBE).
- **The players' records**: every offset to +1FF the sources use is Super Turbo's (compared over the instructions
  portmap paired); the bytes after it are Super Turbo's 0x100 lower (the character +291, the side +281, the person
  +28E, the other player +29E, the buttons +292 and +294, the other player's character +2A2) but the colours (+2B4: 0
  or 1, the other colours when both players have chosen the same character: 00865E, 00868A) and "has chosen" (+2B5, set
  at 0085E6). A round's start clears the records to +280 (0028C2).
- **What Super Turbo has and this game has not** (`family::kTurbo`): super combos, the hidden characters, the speeds,
  a fighter's reach out pushing the camera. Neither a "seen as the other player" byte nor a "not kept in" one: keeping
  a player in the picture (003224) has no tests first. One thing thrown at a time (no second slot, +2AE).
- **Hits**: a player's attack's damage at 09502A (`sub.w d6,0x2A(a2)`), its two ways meeting at 095042; a thrown
  thing's at 0951D2 and 0951E6; a throw's at 003512 and 003810 at once, put by at 0035AC.
- **The clock**: the second's frames counted down at 0094A6. **The stun** comes at a fixed 0x1F (`cmpi.w #$1F` at
  03D0C2: its word stands for Super Turbo's table).
- **The loop with no coin in** (`--watch FF8000-FF8007`, pictures every 250 frames): the board's start ends at frame
  857; 2 the introduction and the title (the title from FF8004 = 8); 0A the demonstration; 0C to 12 the rankings.
- **The computer**: its set-up (03A548) is Super Turbo's (character tables 03BD90, its plan 03C714, the plan's round
  03A5A8, two words a character from 0C6698).
- **Free play** is DIP switch C's bit 2, which the game reads into A5+2CF (00171A): the feature clears it before every
  frame, after the Board page's choices (`CoinsBeforeFrame`).
- **The sound queue** is a ring of 64 words at A5+19E, A5+22 the place of the next (006342 puts one in; 006302 gives
  the oldest to the latches). The modes' sounds go in as words (`modes::Description::soundEntry`).
- **Objects** (dumps through fights): 01 player 1, 02 player 2 by default, 0A the shadows, 0C the bars, 0D the score,
  clock and words, 0B, 0E and 0F the sparks of hits and what is thrown, 1A-1F the stage's pieces.
- **Colours**: a fighter is drawn in set character + 1 + its colours byte. The normal colours are the common block of
  object palettes the game copies as a round is set up (0A4268, 001B66); the other colours a table by character (096804:
  a count and the palettes), copied over the next set (object dumps of E. Honda against E. Honda: 2 and 3, set 3 the
  palette at 096858).
- **The music**: the sound program's table of songs is at 1106, two bytes a song, the place high byte first (01EC and
  002D read it so), 0x8E of them (the byte at 1100); a song whose data starts with 0 is music (01FA: the others are
  effects by priority, and the OKI's samples). The custom soundtrack reads the 68000's request at the latch as it is
  written (`Soundtrack::RequestClassic`): a replaced tune becomes F0 (everything stops: no command found stops only the
  music). Its names from `CPS2RECOMP_SOUNDTRACK_LOG` over attract mode with its sound and the ladder: each stage's tune
  (the game's table at 0063BE) and its faster version at low health (79 on, by stage), the title 16, player select
  0E, versus 0F, a match won 10, a bonus stage 0D.

## Code the game runs in RAM

The game calls small routines it writes into video RAM: `jsr` to 0x244240 × 4 (910900, at 0026FA) and 0x123120 × 8
(918900, at 00397E). No translation made from the ROM can have them; they run on the interpreter (33 instructions'
places, about 0.1% of a run), and `--stats` counts them apart (`N of them the game's own code in RAM`). The checks
require none of the ROM's on the interpreter.

## The checks

All with the build of this branch, on the mini.

- `check` over the ladder (52,000 frames, `free_play,no_damage,one_hit`; M. Bison beaten at about frame 42,000, the introduction again at about 50,000) and the versus movie (12,000 frames): ok
  (translated code is the interpreter's, two runs alike).
- `check-blocks` over the ladder: 570,635,151 translated blocks checked against the interpreter, and 56,000,112 of the sound
  driver's.
- `check-rewrites` (Super Turbo's rewrites of the camera across and up and of keeping the players in, on this game's
  own code): CameraAcross and CameraUp 11,073 calls each, KeepIn 22,167 on the ladder, none differed.
- `check-rollback` (three ways, 400 rollbacks each) and `check-online`: ok.
- `check-widescreen`: every stage painted to both ends (Vega's cage 10 columns short at its right end, under the 20
  allowed); with invisible walls the game's memory is the board's at every 4,000 frames of the ladder.
- `check-release FRAMES=40000`: ok (the picture hash in the pull request).
- By hand: no instruction of the ROM's on the interpreter in the ladder, two made-up games, versus, 20,000 frames of attract mode and of the test menu (the video-RAM routines only; one place in attract mode, 03CD70, added to the entries); features off with no sources (the build without `sources = ssf2t`, `--cpu interp --no-rewrites`)
  the same hash log over the smoke movie; run-ahead 1 and 2 the hash log of 0; a save state loaded at frame 3,000 the
  same from 3,001 on; the launcher's pages; pictures of every mode; the soundtrack's log.

## Next

- Nobody has played it by hand yet.
- Its stages are not guests in the other games yet (they would need the classic board run beside a CPS-2 game's).
