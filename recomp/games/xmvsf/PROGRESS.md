# X-Men vs. Street Fighter: progress

The European arcade release of 1996-10-04 (ROM set `xmvsf`, the parent set), *X-Men Vs. Street Fighter*, on
Capcom's CPS-2 board. Started 2026-10-20 (issue #68). Its sources are its own (`src/`). The first tag-team game in
the collection: each side is a team of two characters, swapped in and out during the fight.

## The ROM set

`xmvsf` was chosen because it is the set competitive play uses: FBNeo's (and so Fightcade's) set of that name, the one
the FBNeo training-mode scripts are written for, and the one the 2022 co-op ROM hack (romhacking.net 6890) patches.
Later revisions exist (961023: USA `xmvsfu`, Asia, Brazil; 970113 Japan). It was downloaded from the Internet Archive
(item VintageArcade, `xmvsf.7z`) with the owner's word, the owner's devices having no copy; all 20 files, the key
included, match MAME's CRC32s (`src/mame/capcom/cps2.cpp`, `ROM_START( xmvsf )`).

The graphics are two rows of four 4 MB sockets. Six buttons a player (MAME's `cps2_2p6b`). Sound driver 1.16b.

## Where it stands (2026-10-20)

- Boots and plays on the toolkit's own machine: the warning, the notices, the introduction, the title, the attract
  mode's fights, the character choice, versus fights, the whole one-player ladder to the ending (Apocalypse, then the
  partners' fight), the staff roll, game over and the show again (the ladder movie: the ending by frame 19,000, the
  title again by 28,000). Seen in pictures only: nobody has played it in a window or listened to it yet.
- 68000 program: 110,052 instructions translated to C++ (692 entries, `make explore` twice). No instruction falls
  back to the interpreter over runs `explore` never made: the ladder movie, made-up ladders, versus, two-player play,
  20,000 frames of attract, the test menu, and every mode (training, attract mode, the stage and model viewers, the
  colour editor, survival, endless battle, ring out). The interpreter stays only as the fallback for a place no run
  has reached.
- Sound: driver 1.16b, translated: 370 entries, 4,710 instructions. `--stats` counts one instruction of the sound
  driver with no translation in every run, as the other games' builds do: the toolkit's, not this game's.
- The features every game has: free play (Enhanced; the game's own), straight to the title, the board's settings
  (the Board page), cheats (no damage for each side, one-hit knockouts, infinite time), save states, run-ahead,
  arcade timing, a custom soundtrack (its music named), the in-game menu and the launcher.
- Everything the other fighting games have: a training mode on the toolkit's session (`fight/session.h`: hit boxes,
  frame data, its meter and log, input display, combo readout, health refills, the dummy; and the game's own gauge
  refill and partner's refill), the Alternative modes page (`fight/alternative.h`: attract mode three ways, the stage
  viewer), the fighting modes (`fight/battle.h`: survival, endless battle, ring out), the model viewer (`model/`),
  custom colours and the colour editor (`fight/colours.h`), widescreen and invisible walls, the hidden character with
  no code.
- Tag-team co-op and 2 vs 2 (issue #69, 2026-10-21): the toolkit's input router (`fight/tag.h`), told by `src/tag.cpp`
  where the point character and the choosing are. Four players' controls (game.cfg `players = 4`); the board still
  reads two.
- **The Marvel games' shared code** (2026-10-23, with Marvel Super Heroes): the training aids' view of the players and
  their boxes, the ring out's movements, widescreen and the walls, and the model viewer are now the toolkit's
  `fight/marvel.h`, from this game's description of itself (`src/engine.cpp`); `training.cpp`, `battle.cpp` keep only
  what is the game's own, and `widescreen.cpp` and `model.cpp` are gone. The game's hash logs over the ladder (25,000
  frames), training with the aids, widescreen, invisible walls, the model viewer, survival, the ring out, the endless
  battle, attract mode and the stage viewer were the same before and after, and so was its feature list.
- No readable rewrites: what the features change is done in hooks that run the game's own code unless a feature is
  on (the camera's clamp and the walls with widescreen, the ring out's flight), each a few lines.
- The toolkit's fighting modules were made to take a game built otherwise than the Alpha games (all optional; the
  other games' runs are the same as before): steps that are not four words, a sound queue of another size, a camera
  whose record moves with the stage and whose ends differ by stage, a camera word that is not the picture's left edge
  and heights that grow downwards, a computer byte that is 0 for the computer, and a tag team's partner.

## The game's memory and routines (found by experiment)

Variables sit below A5 = FF8000 (A5-3000 on); the facts are in `src/xmvsf.h`, each with how it was found. In short:

- **Where the game is**: FF5000, a byte, 8 while a round is fought (0A when a side is beaten, then 0C-12 and back to 0
  for the next match: a match is one round); FF5100 set once a game is played, FF5101 who is in (bit 0 player 1, bit
  1 player 2); FF5104, the game's step (4 choosing, 8 a match), FF5106 the match's. The show has no step word: it
  runs as a task (its record at FF24DC, step byte FF24E0, 2 in the introduction); the title is placed at 075BF2.
- **Credits** FF545F (nine at most); **free play** the game's own coin mode 12 (FF5469), which sets FF5460, tested
  where a player is let in (078016).
- **The board's settings**: the record at FFFAA4 is read into variables at the board's start (079ECA on, called from
  000E20; the hook at its return, 000E26, puts the choices in). The test menu edits a copy of its own (FF2D90). The
  table is in `board.cpp`, each option changed in the test menu, saved and read back.
- **Players**: four records of 0x400 from FF4000 (player 1's first character, player 2's, then their second ones).
  **A tag or a knockout swaps the records' contents**, so FF4000 and FF4400 always hold the character on point; +220
  says which of the side's picks it is. Fields: +03 a person plays it (0 the computer), +06 who is in charge (0 the
  character itself), +0C/+10 where it stands (longs, y down, 1D8 the floor), +14/+1C/+20 the speeds and the pull
  down, +34 the animation frame, +4B facing, +52 the character (a word, even: 0 Wolverine to 24 Alpha Chun-Li), +6C
  the box table, +70-+7C the box numbers, +85 hit-stop, +A0 the action (0 stand, 2 walk, 6 jump, 0C crouch, 24/26
  guard, 28 attack, 36 hit), +A4 the push box, +210 health (0x90 full), +21A what comes back while resting, +212/+214
  the hyper combo gauge, +217 the colour choice, +224 the partner's record, +228 its palettes in the buffer.
- **Picks**: FF50B0/B2 player 1's two characters, FF50B4/B6 player 2's; each round's records are made from them
  (00FFC0). Choosing: a 0x20 record a player from FF5180, its first byte 7 once all is chosen; the cursor's character
  is taken at 00B1E8 (the hidden character's hook), and the game gives Alpha Chun-Li for Chun-Li with Start held
  (00B1DA).
- **Tag team** (traced for the router): a point record's +220 is 0 or 1, the pick it is; a tag (HP+HK) swaps the
  records' contents 14 frames after the press, a knock-out as the partner comes in, a team super as the partner stays
  in after it. A choosing record's first byte is its step (0 not choosing, 1 entering, 2 the first pick, 7 all chosen)
  and its byte +03 the picks taken (0-2; the speed is chosen at 2).
- **Damage**: `sub.w d5,0x210(a6)` at 0155A2 (hits) and 015B6A, the character hit in A6.
- **Clock**: FF5008 (BCD seconds), FF5009 frames of the second, counted down at 00FB14.
- **Stage**: FF5113 (0 Apocalypse's lair, 1-10 the others); taken at 00BABC from a table (088BFA) by a count that goes
  on from match to match; the hook at 00BAEA puts a mode's stage and characters in. The music is player 2's point
  character's theme (song 0B + its number / 2), not the stage's.
- **Camera**: one of three scroll-layer records by stage (FF5240, FF52C0, FF5340: the hitbox script's table), +0C
  across (the picture's left edge less 0x40), +10 down (its top), +72/+74 and +76/+78 its ends. It moves at 070222 by
  the way FF53E8 says (0 after both players, 4 after player 1 alone) and is kept between its ends at 074488. A
  character is kept inside the picture at 010806 (0x55 to 0x1AB on from the camera word).
- **Panel and objects**: colour sets 00-04 the panel, 08 the shadows, 0C-17 the fighters (three sets each: player 1's
  point 0C, player 2's 0F, partners 12 and 15), 1A-1E the stage's pieces; the round's words are tiles 1C000-1D1FF.
- **Colours**: a long a character at 1145A8, two sets (punch, kick) of three palettes, 0x60 apart (009F64 copies
  them at a round's start). PalMod's file is `xvs.05a`.
- **Animations**: groups of words of offsets from a list a character at 08A918 (010380 starts one); frames of 0x10
  bytes, copied into the record by 001AE6.
- **The round's result**: FF5005, 1 player 1's side won, 2 player 2's.
- **The sound queue**: 16-byte requests in a ring of 0x400 at FF2648, the next place at FF2642 (0787E0).

Facts were confirmed with the help of the FBNeo training mode's scripts (peon2/fbneo-training-mode: `games/xmvsf`,
`hitboxes/marvel-hitboxes.lua`), each found again here; no code was taken.

## The checks

(2026-10-20, all on the Mac; the numbers are from the last runs.)

- `make GAME=xmvsf check FRAMES=40000 MOVIE=games/xmvsf/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`:
  translated code does what the interpreter does over the whole ladder; two runs alike.
- `make GAME=xmvsf check FRAMES=12000 MOVIE=games/xmvsf/movies/versus.txt`: the same over versus.
- `make GAME=xmvsf check-blocks MOVIE=games/xmvsf/movies/ladder.txt FRAMES=40000`: every translated block checked
  against the interpreter, and the sound driver's (304,680,526 blocks and 143,852,626 of the sound driver's, no
  difference).
- `make GAME=xmvsf check-release FRAMES=40000`: two runs of the build with no translated code alike; its pictures the
  translated build's but for 35 frames (each one frame early), and the same last picture. Before the hook at 001618 they parted
  for good at frame 5,227: the game's random routine (001608) has a long divs, and the frame's interrupt, which asks for
  a random number of its own (00069A), often comes during it; the board (and the interpreter) take it before the new
  number is kept, translated code at the next block's start, after. A block now starts after the divs.
- `make GAME=xmvsf check-widescreen`: every stage walked to both ends in widescreen, no unpainted columns; invisible
  walls leave the game's memory the board's at every 4,000 frames of the ladder; the ladder in widescreen with no
  instruction left to the interpreter.
- `make GAME=xmvsf check FRAMES=6200 MOVIE=games/xmvsf/movies/coop.txt ARGS="--features free_play,tag_team=1 --poke FF4214=03"`
  and `make GAME=xmvsf check FRAMES=5200 MOVIE=games/xmvsf/movies/two_vs_two.txt ARGS="--features free_play,one_hit,tag_team=2"`:
  the tag-team movies (four players' input: a Start by the second person, each pick by its own person with the other's
  presses ignored, a tag, a tag back, knock-outs and the partner coming in, a team super) translated as interpreted;
  two runs alike. Seen in pictures every 25-100 frames; the point bytes traced. The two vs two movie with player 1's
  presses while off point taken out gives the same hashes (they never reach the board). Run-ahead 1 and 2 give the
  co-op movie's hashes; a state saved at 2,500 of two vs two and loaded gives the same frames after. With the mode off,
  the ladder (40,000 frames) and versus (12,000) give the same hashes as the build before it.
- By hand: save states (a state saved at frame 3,000 and loaded into a run of the same input: frames 3,001-6,000
  alike); run-ahead (1 and 2 frames: the same hashes as without over 8,000 frames of versus and a training session);
  features off and no sources (`--cpu interp --features -free_play` against a build with `src/` empty: 8,000 frames
  alike); the board's settings (each choice read back from its variable).
- The other games after the toolkit's changes: Super Gem Fighter Mini Mix and Street Fighter Alpha 2 built at
  origin/main and on this branch, the same scripted runs on both (the ladder, training with start over, attract mode's
  fights, the stage viewer and its infinite stage, the model viewer, endless battle three ways, survival, ring out;
  the fighting modes with their random seed held for the test): every frame's hash alike, and the composed pictures
  of the endless battle, ring out, model viewer and training in widescreen alike.

## Not checked by a person

Nobody has played it in a window, used a gamepad with it, or listened to it: every check above is headless. The
custom soundtrack files, the dummy's record and replay, and the arcade timing have not been tried by hand.

## Next

- Online play (issue #70).
