# Progear: progress

## How it is made (2026-11-09)

- **The ROM set** `progearu` (USA 010117), from the Internet Archive's VintageArcade item, where it is called `progear`
  (older collections had the USA set as the parent; MAME's `progear` is now the European set, `pgae.03`/`pgae.04`).
  None of the user's machines had it. Every file's CRC32 is MAME's (`cps2.cpp`, `ROM_START( progearu )`), the key's
  too (named `progearu.key` here, as MAME has it). Every region's set is of that one date.
- **Graphics on SIMMs**: eight files of 2 MB, each one byte of the 64-bit words (MAME's `ROM_LOAD64_BYTE`), joined with
  `+` in `game.cfg` as Ghouls'n Ghosts' are; the samples are four SIMM files in a row. Three buttons (`cps2_2p3b`),
  Capcom's sound driver 1.80 (translated, not rewritten). The picture is horizontal (`ROT0`), 384 x 224.
- **Other sets**: `scripts/other_sets_survey.py` (the add-game skill's) found `progeara` alike but for the header
  (0000C2, 0000D0, 0000D2): an `other_set`. It boots with Asia's warning and plays the versus movie. `progearj`
  differs all through (its own text) and is not taken; the European set was not found to compare. `online_words` has
  the header's words and the 101 bytes of the boot's ROM check (00026A: every 4,095th byte of each 512 KB, as on the
  chips) where the keys differ: without them a USA host and an Asian player came apart at frame 60;
  `JOIN_ROM=…/progeara make GAME=progear check-online` now passes.
- **Translated code**: `make explore` twice (no features, then free play and invincibility so that made-up play goes up
  the stages) and `--record-entries` runs (the ladder, versus, made-up play from stages 4 and 5, attract, the test
  menu, the sound test, practice, boss rush, the viewer): 3,885 entries, 89,482 instructions, 89 left to the
  interpreter (6 of them data); the sound program all translated (4,205 instructions).

## What is known of the game (progear.h has the names)

The game is Cave's. Its variables are at fixed addresses (no base register), its objects are records in work RAM, and
its code is strewn with `cmpi.l #$63A1B8D3,d1`, which does nothing (Giga Wing's has another number). Its work is tasks:
slots of 0x40 bytes from FF2912, a task's number times 0x40 in (0092F0 starts one, 0093D4 stops one), their routines in
the table at 005454. Places are in 64ths of a pixel, heights counted up from the picture's foot.

- **Credits**: the byte at FF2389 (up to 9; a coin adds at 0465DC, Start takes at 0464DC; two coins a credit).
- **Free play**: the board's own. The settings routine (011476) sets FF238B when the coin mode (FF2395, the record's
  first byte) is 0x12, the last; Start then takes no credit and the screens say FREE PLAY.
- **Settings**: the record at FF275E: +00 coin mode, +01 continue (0 on), +02 monitor, +03 demo sound (0 off, 1 on, 2
  half), +04 sound, +05 chute, +10 difficulty (0-7, shown 1-8; 1), +11 ship stock (0-4: 3, 4, 5, 1, 2 ships; 0), +12
  extend (0: 3M then every 4M; 1: 4M and 9M; 2: every 1.5M; 3: none), +13 rapid fire on button 3 (0 off). 011476
  copies them into FF2395, FF28EF, FF28F0, FF28ED, FF28EE, FF28F1, FF28EC, FF28F3, FF28F4, FF28F9. Found by stepping
  each option of the test menu (Shot 1 steps a value; Right does nothing in this menu) and reading the variables. The
  test menu works on copies (FF06D6 on), taken from the variables as it opens (001D76) and put back at Save & Exit
  (001CB6); the test menu's flag is FF38BC (046926).
- **The mode** (the word at FF3ECE, the jump table at 00DE28): 00 the warning, 0E the logos, 01 the show's title, 05 a
  demonstration, 0D the title with credits or free play, 04 Start taken there, 0B the choice of pilot and gunner and the
  first stage's take-off, 07 a stage played, 08 continue and game over, 0C the ranking.
- **Players**: the word at FF3ED0, E0 when player 1 starts and D0 for player 2; bit 4 of its low byte set while player
  1 is out, bit 5 player 2.
- **Stage**: the word at FF3FA2, 0-4, with its double and quadruple at FF3FA4 and FF3FA6 (the indexes the stages'
  tables are read with); 07CDCA counts it on, and 5 is the ending, after which the game goes round again from stage 1
  (the ladder movie: stage 2 at frame 12,907, 3 at 33,552, 4 at 47,270, 5 at 66,825, the last boss from about 78,000,
  the second round's stage 1 by 92,000).
- **The take-off**: choosing the gunner sets FF390E (04704E); the first stage's take-off ends it (05C50A) and starts
  player 1's ship, task 2 (05C5BA). While FF390E is set there is no panel, no Start for the other player and no hit test
  for the ships. A change of stage starts the ships' tasks again (06C202).
- **Ships**: words at FF3F72 (player 1, then 2), with the one in play: set at a game's start from the ship stock
  through the table at 00F914, one taken as a ship's blast ends (03A1A4). **Bombs**: ship+22, 3 at each ship's start,
  one taken at 0396FA. **Score**: BCD long a player at FF3DC6 and FF3DCA, shown with a 0 more.
- **Pilot and gunner**: words at FF39E4 (player 1's pilot: 0 Ring, 1 Bolt) and FF39E8 (gunner: 0 Chain, 1 Nail, 2
  Rivet); their cursors keep their own copies at FF5C81 and FF5C85, which Shot takes.
- **The ships**: records of 0x60 at FF42E2 and FF4342: +00 flags (bit 15 in use, bit 12 hit), +02 up, +04 across,
  +10/+12/+14/+16 the hit box (up, down, right, left: 0x80 each, 2 pixels, so a 4-pixel square), +3E frames out of harm
  after a ship's start. The hit test 04DF7A (once a frame, both ships in turn when two play) tests the box against the
  enemies' bodies (records of 0x20 from FF5C50, as many as FF38E4 + FF38E6, the same box layout; made during the frame
  and cleared before it ends) and sets bit 12 (04E03C); the enemies' shots (records of 0x40 from FF7A90) are points,
  tested by each shot against the box FFFF7C holds (040B02), which sets bit 12 too (040B48). 0393DC clears the bit and
  blows the ship up.
- **The boss**: the stage's controller (a record at FF2952) sets FF295A when the music turns and FF295B when the boss
  comes, at places of the stage's scroll the table at 06ABC6 gives for each stage; the stage's change clears both. There
  is no boss clock.
- **A test switch**: bit 6 of FFFFAC keeps a ship out of harm (FF in its +3E; 039418). It was used to find the stages;
  the port's invincibility does not use it.
- **Sound**: the song table is at 0x9006 after a word of their number (0xDE), as Capcom's. Music found by playing
  (music.cpp names them): 0E the title, 03 the choice, 04-08 stages 1-5, 02 a boss, 0C the last boss, 09 a stage
  cleared. The demonstrations play no music; the continue has none either.

## What the port adds

Free play (the game's own flag, put back from the coin mode when switched off), straight to the title (run unseen until
the title or a game), start at stage (00E9C2: the stage put in as Start is taken; as the play begins the ships' tasks
are started and the take-off ended, as a change of stage does, since a game begun past stage 1 has no take-off), the
cheats (invincibility by clearing the hit at 0393DC and going on unhurt; infinite bombs by skipping the subtraction at
0396FA; infinite ships kept at two while a stage is played), the Board page (choices put in the variables before every
frame, the board's own as the test menu opens, so it copies and saves them), the music's names, and the shoot-'em-up
modes of the toolkit's `shmup/` (practice and its aids, the shot's power set at its start and read out (ship+20), score attack, caravan, boss rush, the stage viewer with all
three scroll screens, attract mode). The aids read the enemies' bodies at the hit test (a hook at 04DF7A that only
looks, kept for drawing only), since the game clears that list before a frame ends.

In the toolkit: the Practice page's Power and Readout rows show only for a game that gives them (Progear gives both),
a game may give its own words for the readout and the summary, and a game with no multiplier has its summaries count its score. Giga Wing
is as it was: the same features, and the same hash logs for its ladder movie and for 8,000 frames of practice.

## Checks (2026-11-09)

- `make GAME=progear check FRAMES=40000 MOVIE=games/progear/movies/ladder.txt ARGS="--features free_play,invincible"`:
  translated code as the interpreter, two runs alike.
- `check FRAMES=12000` with the versus movie (two players): the same.
- `check-blocks` with the ladder movie, 40,000 frames: 171 million translated blocks checked against the interpreter,
  and 156 million of the sound driver's.
- `check-release FRAMES=40000`: passes (7,219 frames differ between the interpreted and the translated build by design:
  interrupts between instructions, not blocks).
- `check-rollback` (400 rollbacks, every cheat and feature of the game's on) and `check-online` (2,999 frames, names and
  chat, the match as an offline replay): pass; and with the Asian set joining a USA host.
- `make check` and `tools/audit.sh`: pass.
- By hand: `--stats` over the ladder (100,000 frames), versus, made-up play for one and two players, 20,000 frames of
  attract, the test menu and practice at a boss: no 68000 instruction outside the translation; the sound driver's one
  (the toolkit's, as in the other games; practice restoring its snapshot adds one more). Features off = no sources:
  8,000 frames of the versus movie with `--cpu interp` give byte-identical hash logs with and without `src/`. A save
  state at 3,000 loaded into a fresh run: frames 3,001 to 6,000 alike. Run-ahead 1 and 2: the same hash log as off.
  Every Board choice read back from its variable, and five ships given; with choices made the test menu shows and saves
  the board's own (the settings memory byte-identical without them). Each mode run: practice at stage 1 and at stage
  3's and stage 4's bosses, with power, ships and bombs chosen at stages 1 and 2 (put in once the ship is there:
  the ship's start puts them back, so practice waits for it) (met from a snapshot just before), score attack, caravan (two minutes), boss rush, the stage
  viewer (stages 1 and 4), attract mode's demonstrations only. Start at stage 2 and 5: the ship, the panel and hits as
  in a game begun at stage 1. The launcher's three pages drawn with no window.

## Not checked by a person

Nobody has played it by hand: not the controls' feel, not the modes in a window (headless runs only), not the sound
(never listened to), not online play between two machines (`check-online` plays two copies on one).

## Next

- More for the readout: Progear's scoring (enemy shots turned to gold as their shooter falls) is not found yet; the
  readout has the shot's power and the frames out of harm after a ship's start.
- The second round and the ending after it: the ladder reaches the second round's stage 2; a run through it would name
  the ending's music.
