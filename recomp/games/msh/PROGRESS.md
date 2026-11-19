# Marvel Super Heroes: progress

The European arcade release of 1995-10-24 (ROM set `msh`, the parent set), *Marvel Super Heroes*, on Capcom's CPS-2
board. Started 2026-10-23. One-on-one fights with the Infinity Gems; the engine X-Men vs. Street Fighter was made from.

## The ROM set

`msh` was chosen because it is the set competitive play uses: FBNeo's (and so Fightcade's) set of that name, the one
the FBNeo training-mode scripts are written for. The USA set (`mshu`) has the same date and program but its region;
a later revision (951117: Japan, Brazil, Hispanic) exists. It was downloaded from the Internet Archive (item
VintageArcade, `msh.7z`) with the owner's word, the owner's devices having no copy; all 21 files, the key included,
match MAME's CRC32s (`src/mame/capcom/cps2.cpp`, `ROM_START( msh )`).

The graphics are two rows of four 4 MB sockets. Six buttons a player (MAME's `cps2_2p6b`). Sound driver 1.05.

## Where it stands (2026-10-23)

- Boots and plays on the toolkit's own machine: the warning, the notices, the introduction, the title, the attract
  mode's fights, the character choice, versus fights, the whole one-player ladder to the ending (seven opponents, Dr.
  Doom, Thanos), the staff credits, game over and the show again (the ladder movie: the ending by frame 30,800, the
  title again by 32,000). Seen in pictures only: nobody has played it in a window or listened to it yet.
- 68000 program: 113,841 instructions translated to C++ (1,274 entries, `make explore` twice). No instruction falls
  back to the interpreter over runs `explore` never made: the ladder movie, made-up ladders, versus, two-player play,
  attract mode with its demo sound, the test menu, and every mode (training, the stage and model viewers, the colour
  editor, survival, endless battle, ring out), and the ladder in widescreen. The interpreter stays only as the
  fallback for a place no run has reached.
- Sound: driver 1.05, translated: 402 entries, 2,289 instructions (the show with its demo sound on and the sound test
  added the last 22 entries).
- The features every game has: free play (Enhanced; the game's own), straight to the title, the board's settings
  (the Board page), cheats (no damage for each player, one-hit knockouts, infinite time), save states, run-ahead,
  arcade timing, a custom soundtrack (its music named), the in-game menu and the launcher.
- Everything the other fighting games have: a training mode on the toolkit's session (`fight/session.h`: hit boxes,
  frame data, its meter and log, input display, combo readout, health refills, the dummy; and the game's own gauge
  refill and all six Infinity Gems), the Alternative modes page (`fight/alternative.h`: attract mode three ways, the
  stage viewer), the fighting modes (`fight/battle.h`: survival, endless battle, ring out), the model viewer
  (`model/`), custom colours and the colour editor (`fight/colours.h`), widescreen and invisible walls, the hidden
  characters with no code (Start held as a character is chosen: a punch Dr. Doom, a kick Thanos).
- **Shared with X-Men vs. Street Fighter** (`fight/marvel.h`): the two games' engine is one, at other places, so what
  their sources would write alike is the toolkit's, from a description of each game (`src/engine.cpp`): the training
  aids' view of the players (frame data, a hit landing) and their boxes, the ring out's own movements, widescreen
  (the camera kept between its ends, a character kept inside the picture, the wide picture), and the model viewer
  (the animations, the second player put out of the way, the camera after player 1). X-Men vs. Street Fighter was
  moved onto it in the same work, and its runs are the same as before (below).
- No readable rewrites: what the features change is done in hooks that run the game's own code unless a feature is
  on (the camera's clamp and the walls with widescreen, the ring out's flight), each a few lines.

## The game's memory and routines (found by experiment)

Variables sit below A5 = FF8000 (A5-3800 on); the facts are in `src/msh.h`, each with how it was found. The layout is
X-Men vs. Street Fighter's moved down (FF5000 there is FF4800 here): `tools/portmap.py --from` X-Men vs. Street
Fighter's work folder `--to work/msh` placed 46% of that game's instructions here and 2,636 places it names, which
gave most of the variables below a first guess; each was then confirmed here.

- **Where the game is**: FF4800, a byte, 8 while a round is fought, 0A once it is decided (the winner in FF4804: 0
  player 1, 1 player 2, FF neither; 00CB76 on); FF4880 set once a game is played, FF4881 who is in (bit 0 player 1,
  bit 1 player 2); FF4884, the game's step (4 choosing, 8 a match), FF4886 the match's. The show runs its scenes as
  tasks (087132 on, each marking the record at FF2698, its step byte FF269C); the title is the last, 0884A6.
- **Credits** FF4C09 (nine at most); coins counted at FF4C08; **free play** the game's own coin mode 12 (FF4C13), which
  sets FF4C0A, tested where a player is let in (08CE34) and where credits are counted. The same code as X-Men vs.
  Street Fighter's.
- **The board's settings**: the record at FFF912 is read into variables at the board's start (08E9C6 on, called from
  000C66; the hook at its return, 000C6C, puts the choices in). The test menu edits a copy of its own (FF2C24). The
  table is in `board.cpp`, each option changed in the test menu, saved and read back after a start; the names read
  from pictures. The mode is two variables (FFFB4D, and FFFB4A for turbo alone); the event sets FFFB49 too.
- **Players**: two records of 0x400 from FF4000. Fields: +01 in use, +03 a person plays it (0 the computer), +04 its
  state (2 fighting), +06 who is in charge (0 the character itself, 0C reeling), +0C/+10 where it stands (longs, y
  down), +14/+1C/+20 the speeds and the pull down, +34 the animation frame, +4D facing, +50 the character (a word,
  even: 0 Spider-Man to 16 Thanos, each put in a fight and seen; 18 is Anita, not a fighter), +6A the attack's phase
  (80 out, C0 recovering), +74-+80 the box numbers, +86/+87 attacks hit / can be hit, +89 hit-stop, +90 the box table,
  +A0 the action (0 stand, 2 walk, 6 jump, 0C crouch, 24/26 guard, 28 attack, 36 hit), +A2 the push box, +AF the colour
  set (1 player 2's in a mirror match: 008CAC), +190 health (0x90 full), +194 the gauge's bar, +198 its levels, +1B6
  the six Infinity Gems. The box fields are the FBNeo hitbox script's for this game, confirmed by the boxes drawn.
- **Choosing**: a 0x20 record a player from FF4920, its first byte 1 while choosing and 3 once chosen, +0C the
  character under the cursor (written to the player's +50 as it moves, 008FA8); a character is taken at 008F1C (the
  hidden characters' hook).
- **Stage**: FF4893 (0-11). Each stage is a character's own (in the characters' order): a one-player game fights each
  opponent on its stage (008CB4: from the list of opponents), a two-player one takes a stage at random (008D60) or by
  the buttons held (008CD8). Every way meets at 008C4E (after the board's own stage choice for its tests, 00839C),
  where a mode's stage and characters go in. A round plays the theme of the character whose stage it is (song 09 +
  the character's number).
- **Camera**: the record by stage (FF4980 on the mountain lake, FF4B00 on the burning field, FF4A00 on the others: the
  hitbox script's table), +0C across (the picture's left edge less 0x40), +10 down (its top), +72/+74 and +76/+78 its
  ends. It moves at 082440 by the way FF4B9C says (0 after both players, 4 after player 1) and is kept between its
  ends at 083A46 (the same instructions as X-Men vs. Street Fighter's 074488). A character is kept inside the picture
  at 00DFF2 (0x55 to 0x1AB on from the plane's camera word, the planes' table at 01A752). The floor is the camera's
  lowest place + 0xD8: 0x1D8 on most stages, 0x2D8 at Stark Enterprises, 0xD8 on the mountain lake and in the
  neon city, 0x3D8 on the burning field.
- **Panel and objects**: colour sets 00-03 the panel, 08 the shadows, 0B the gems' marks, 0C-0E player 1, 10-12 player
  2, 13 on the stage's pieces; the panel's pieces in other sets are tiles 16000-16FFF (the faces, the clock's mark, the
  gems' marks).
- **Colours**: a long a character at 160D2E, its own set then the other 0x80 on, three palettes each (Thanos's two),
  copied at a round's start (007AEC: player 1's to set 0C, player 2's to 10). PalMod's file is `msh.05`.
- **Animations**: groups of words of offsets from a list a character at 09E6FE (00DBC8 starts one); frames of 0x20
  bytes (the boxes' numbers in the frame itself, not a table of sets as in X-Men vs. Street Fighter), copied into the
  record by 001840. Up to 30 groups a character (the model viewer names the ones after Z AA, AB...).
- **Damage**: `sub.w d5,0x190(a6)` at 011F7A (hits) and 01251E, the player hit in A6.
- **Clock**: FF4808 (BCD seconds), FF4809 frames of the second, counted down at 00D4BC.
- **The sound queue**: 16-byte requests in a ring of 0x400 at FF4C2C, the next place at FF4C26 (08D58E).
- **The random number**: 00134E, a long divs then the number kept; a hook at 00135E starts a block there, as X-Men vs.
  Street Fighter's needed.

Facts were confirmed with the help of the FBNeo training mode's scripts (peon2/fbneo-training-mode: `games/msh`,
`hitboxes/marvel-hitboxes.lua`), each found again here; no code was taken.

## The checks

(2026-10-23, all on the Mac; the numbers are from the last runs.)

- `make GAME=msh check FRAMES=40000 MOVIE=games/msh/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`:
  translated code does what the interpreter does over the whole ladder; two runs alike.
- `make GAME=msh check FRAMES=12000 MOVIE=games/msh/movies/versus.txt`: the same over versus.
- `make GAME=msh check-blocks MOVIE=games/msh/movies/ladder.txt FRAMES=40000`: every translated block checked against
  the interpreter, and the sound driver's (305,946,146 blocks and 260,905,733 of the sound driver's, no difference).
- `make GAME=msh check-release FRAMES=40000`: two runs of the build with no translated code alike; its pictures the
  translated build's but for 36 frames (each one frame early), and the same last picture.
- `make GAME=msh check-widescreen`: every stage walked to both ends in widescreen, no unpainted columns; invisible
  walls leave the game's memory the board's at every 4,000 frames of the ladder; the ladder in widescreen with no
  instruction left to the interpreter.
- By hand: `--stats` over the runs above (no instruction to the interpreter); features off and no sources at all give
  the same `--hash-log` over 6,000 frames of versus (`--cpu interp`); a save state loaded at frame 3,000 goes on as the
  run that saved it (3,000 frames); run-ahead 1 and 2 give run-ahead 0's hash log; "Straight to the title" on and off
  give the same hash log; every Board choice read back from its variable, the test menu showing the board's own with
  choices in, and the settings memory the same with and without them; no damage (player 2's health stays 0x90 through
  hits), infinite time (the clock at 99), one-hit knockouts (the ladder); the hidden characters (Start and a punch: Dr.
  Doom; a kick: Thanos); pictures of every mode.
- X-Men vs. Street Fighter on the shared code (`fight/marvel.h`): its hash logs over the ladder (25,000 frames),
  training with the aids, widescreen, invisible walls, the model viewer, survival, the ring out, the endless battle,
  attract mode and the stage viewer are the same as before the move (`CPS2RECOMP_BATTLE_SEED` gives the fighting modes
  the same choices every run), and its feature list is the same.

## Not checked by a person

- Nobody has played it in a window, with a controller, or listened to it. The pictures and the checks are all.
- The launcher's pages have been seen in a no-window screenshot only.
- Dr. Doom or Thanos as player 1 meets themselves in the ladder's last fights: the game gives player 2 the other
  colours only in a two-player match, so such a fight may have both in the same colours.

## Next

- The 951117 revision (Japan, Brazil, Hispanic) as another game made from this program, if wanted.
- X-Men: Children of the Atom, Marvel Super Heroes vs. Street Fighter and Marvel vs. Capcom on `fight/marvel.h`.
