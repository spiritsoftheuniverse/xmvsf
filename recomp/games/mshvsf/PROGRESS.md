# Marvel Super Heroes vs. Street Fighter: progress

The European arcade release of 1997-06-25 (ROM set `mshvsf`, the parent set), *Marvel Super Heroes Vs. Street
Fighter*, on Capcom's CPS-2 board. Started 2026-10-28 (issue #135). The sequel to X-Men vs. Street Fighter, on the
Marvel games' shared engine (`fight/marvel.h`): each side is a team of two characters, swapped in and out during the
fight.

## The ROM set

`mshvsf` was chosen because it is the set competitive play uses: FBNeo's (and so Fightcade's) set of that name, the one
the FBNeo training-mode scripts are written for. Other sets of the same date (Asia, Brazil, Hispanic, USA 970625) differ
only in their region; later revisions exist (970702, 970707 Japan; 970827 USA, Brazil). The owner's own copy (a merged
`mshvsf.zip` among their MiSTer backups, on the Windows PC) has the same files as the Internet Archive's (item
VintageArcade, `mshvsf.7z`); all 21 files, the key included, match MAME's CRC32s (`src/mame/capcom/cps2.cpp`,
`ROM_START( mshvsf )`).

The graphics are two rows of four 4 MB sockets. Six buttons a player (MAME's `cps2_2p6b`). Sound driver 1.31.

## Where it stands (2026-10-28)

- Boots and plays on the toolkit's own machine: the warning, the notices, the introduction, the title, the attract
  mode's fights, the character choice (two characters, then the mode and the speed), versus fights, the whole
  one-player ladder to the ending (Apocalypse, then Cyber-Akuma for a player who did well), the staff roll, game over
  and the show again (the ladder movie: the ending by frame 17,700, the title again by 25,000). Seen in pictures only:
  nobody has played it in a window or listened to it yet.
- 68000 program: translated to C++ (155,549 instructions from 1,640 entries: `tools/portmap.py` from X-Men vs. Street Fighter's, `make explore`,
  and the runs below with `--record-entries`). No instruction falls back to the interpreter over runs `explore` never
  made: the ladder movie (and in widescreen), made-up ladders, versus, two-player play, 20,000 frames of attract, the
  test menu, the sound test, and every mode (training, attract mode three ways, the stage and model viewers, the colour
  editor, survival, endless battle and its scroll, ring out, tag-team co-op and 2 vs 2). Four places the modes' other
  machines reach by the camera's jump tables were added by hand (they record no entries). The interpreter stays only as
  the fallback for a place no run has reached.
- Sound: driver 1.31, translated. `--stats` counts one instruction of the sound driver with no translation in every
  run, as the other games' builds do: the toolkit's, not this game's.
- The features every game has: free play (Enhanced; the game's own), straight to the title, the board's settings
  (the Board page), cheats (no damage for each side, one-hit knockouts, infinite time), save states, run-ahead,
  arcade timing, a custom soundtrack (its music named), the in-game menu and the launcher.
- Everything the other Marvel games have: a training mode on the toolkit's session (`fight/session.h`: hit boxes,
  frame data, its meter and log, input display, combo readout, health refills, the dummy; and the game's own gauge
  refill and partner's refill), the Alternative modes page (`fight/alternative.h`: attract mode three ways, the stage
  viewer), the fighting modes (`fight/battle.h`: survival, endless battle, ring out), the model viewer (`model/`),
  custom colours and the colour editor (`fight/colours.h`), widescreen and invisible walls, the zoom, the hidden
  characters with no code, and tag-team co-op and 2 vs 2 (`fight/tag.h`).
- Not added, as the user's rules say: the tag team battle (the game has its own tag team), and the CPU coach (the
  game's computer plays every mode itself: survival, the endless battle and the ring out have it fight as on the
  board).
- **On the Marvel games' shared engine** (`fight/marvel.h`): `src/engine.cpp` describes the game; the training aids'
  view of the players and their boxes, the ring out's movements, widescreen and the walls, the model viewer and the zoom
  are the toolkit's. Two things this game does otherwise than X-Men vs. Street Fighter were added to the toolkit (all
  optional; Marvel Super Heroes and X-Men vs. Street Fighter run exactly as before, below):
  - a tag does not swap the records' contents: each character keeps its record and the game keeps which is on point
    (`playerAt` in `marvel::Engine`, `session::Description`, `modes::Description` and `battle::Description`, with
    `partnerAt` for the battle modes);
  - the camera's step works on a copy of its place and keeps it between its ends in two pieces
    (`cameraWorkX`/`cameraWorkY`, `keepCameraDown`, `keptAcross`/`keptDown`; and the model viewer's camera way down at
    a hook of its own).
  The frame copy the two games share (X-Men vs. Street Fighter's 001AE6, this game's 001E08) is the toolkit's
  `marvel::CopyFrameBoxSets`; the model viewer can order the characters (`viewerOrder`: the players' first, the last
  opponents at the end).
- No readable rewrites: what the features change is done in hooks that run the game's own code unless a feature is
  on (the camera's keeping and the walls with widescreen, the ring out's flight), each a few lines.

## The game's memory and routines (found by experiment)

Variables sit below A5 = FF8000; the facts are in `src/mshvsf.h`, each with how it was found. `tools/portmap.py --from`
X-Men vs. Street Fighter's work folder (with its guessed entries) `--to work/mshvsf` placed 59,212 of 109,811 of that
game's instructions here and named 10,586 places, which gave most of what is below a first guess; each was then
confirmed here. In short, X-Men vs. Street Fighter's variables 0x800 lower (FF5000 there is FF4800 here):

- **Where the game is**: FF4800, a byte, 8 while a round is fought; FF4900 set once a game is played, FF4901 who is in;
  FF4904, the game's step (4 choosing, 00932E; 8 a match, 00A5D0), FF4906 the match's. The show runs as a task (its
  record at FF2A78, step byte FF2A7C, 2 while a scene runs); the title's scene places the title at 0A80AE (frame
  2,400 from switching on).
- **Credits** FF4C75; **free play** the game's own coin mode 12 (FF4C81), which sets FF4C77 (0AD9E2), tested where a
  player is let in (0AB54E).
- **The board's settings**: the record at FFFBC0 is read into variables at the board's start (0AD926, called from
  001010; the hook at its return, 001016, puts the choices in). The test menu edits a copy of its own (FF3130 on, the
  record's offsets), from which each option's values were read as it was stepped; the names read from pictures. Mode
  select (+13, FFFEDB: 0 manual only, 1 free select, 2 easy only) takes X-Men vs. Street Fighter's Automatic's place;
  the game speed's board's own is Free select (3).
- **Players**: four records of 0x400 from FF3800 (player 1's first character, player 2's, then their second ones).
  **A tag does not swap the records** (as X-Men vs. Street Fighter's does): the long at FF48C8 is player 1's character
  on point, FF48D0 player 2's (the longs after them the partners'; traced over a tag: FF48C8 went from FF3800 to FF4000,
  the records' characters stayed). A record's +00 is 1 on point, +03 a person plays it (0 the computer), +06 who is in
  charge, +0C/+10 where it stands (longs, y down), +14/+1C/+20 the speeds and the pull down, +34 the animation frame,
  +4B facing, +52 the character (a word, even), +6C the box table, +70-+7C the box numbers, +82/+83 attacks hit / can be
  hit, +85 hit-stop, +A0 the action, +A4 the push box (the box places are the FBNeo hitbox script's for this set), +250
  health (0x90 full; `sub.w d5,0x250(a6)` at 018536 and 018BB2), +252/+254 the gauge's bar and levels (the side's, on
  the point character's record, copied at a tag), +257 the colour choice, +25A what comes back while resting, +260 which
  of the side's picks it is, +261 1 while it rests, +264 the partner's record, +268 its palettes.
- **Characters**: the word +52, each number put in player 1's picks and seen in a fight with its name: 00 Apocalypse
  (the last opponent), 02 Cyclops, 04 Captain America, 06 Hulk, 08 Wolverine, 0A Shuma-Gorath, 0C Omega Red, 0E
  Spider-Man, 10 Blackheart, 12 Ryu, 14 Ken, 16 Chun-Li, 18 Dhalsim, 1A Zangief, 1C M. Bison, 1E Akuma, 20 Sakura, 22
  Norimaro (the Japanese release's), 24 Dan, 26 Cyber-Akuma (the last opponent after Apocalypse for one who did well,
  00B210), 28 Mech-Zangief, 2A Dark Sakura, 2C Shadow, 2E U.S. Agent, 30 Mephisto, 32 Armored Spider-Man; 34 on reset
  the board.
- **Picks**: FF48B0/B2 player 1's two characters, FF48B4/B6 player 2's, then bytes of their colours (FF48C4 on); each
  round's records are made from them (0113D6 on). Choosing: a 0x20 record a player from FF4980, +0C the character under
  the cursor; a character is taken at 00A66E (the hidden characters' hook).
- **Stage**: FF4913 (0-11, named from pictures); taken at 00AF34 from a table (0BD720) by a count that goes on from match
  to match; the last fights are on 0 (00B220); the hook at 00AF62 puts a mode's stage and characters in.
- **Camera**: one of three scroll-layer records by stage (the hitbox script's table: X-Men vs. Street Fighter's 0x800
  lower), +0C across (the picture's left edge less 0x40), +10 down. Its step (0A1C4E across, 0A1ED4 down) copies them to
  +76/+78, moves the copy the way FF4BE8 (across) and FF4BE9 (down) say, keeps it between +68/+6A and +6C/+6E (0A55AC,
  0A55D0) and puts it back (0A55C4, 0A55E8). The floor is the camera's lowest place + 0xD8 (0x1D8, or 0x3E8 in
  Apocalypse's lair). A character is kept inside the picture at 011BB8 (0x55 to 0x1AB on from the plane's camera word,
  the planes' table at 021FAE).
- **Panel and objects**: as X-Men vs. Street Fighter's (colour sets 00-04 the panel, 08 the shadows, 0C-17 the
  fighters, 1A on the stage's pieces); the widescreen pictures keep the panel in the middle.
- **Colours**: a long a character at 1F172E, two sets (punch, kick) of three palettes, 0x60 apart. PalMod's file is
  `mvs.06a`.
- **Animations**: groups of words of offsets from a list a character at 137476 (011762 starts one); frames of 0x10
  bytes, copied into the record by 001E08 (the same instructions as X-Men vs. Street Fighter's 001AE6).
- **Music**: each character's theme is 0C and its number (word / 2); the screen before a match plays player 2's
  character on point's, the round its partner's (traced over the ladder with `CPS2RECOMP_SOUNDTRACK_LOG` and the
  characters' words). Others: 01 the opening, 02 player select, 04 before each stage, 05 the winner, 07 the QSound logo,
  0B before the last fights, 32 after the last, 24 the ending, 20 the staff roll, 0A game over.
- **The round's result**: FF4805. **The sound queue**: FF2BE6, the next place at FF2BE0 (0ABD78). **The random
  number**: 00192C, a long divs then the number kept; a hook at 00193C starts a block there, as X-Men vs. Street
  Fighter's needed.

Facts were confirmed with the help of the FBNeo training mode's scripts (peon2/fbneo-training-mode: `games/mshvsf`,
`hitboxes/marvel-hitboxes.lua`), each found again here; no code was taken.

## The checks

(2026-10-28, all on the Mac.)

- `make GAME=mshvsf check FRAMES=40000 MOVIE=games/mshvsf/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`:
  translated code does what the interpreter does over the whole ladder; two runs alike.
- `make GAME=mshvsf check FRAMES=12000 MOVIE=games/mshvsf/movies/versus.txt`: the same over versus.
- `make GAME=mshvsf check-blocks MOVIE=games/mshvsf/movies/ladder.txt FRAMES=40000`: every translated block checked
  against the interpreter, and the sound driver's (305,621,353 blocks and 140,537,960 of the sound driver's, no
  difference). The translation: 1,640 entries, 155,549 instructions (76,766 of them from 1,112 guessed entries).
- `make GAME=mshvsf check-release FRAMES=40000`: two runs of the build with no translated code alike; its pictures the
  translated build's but for 17 frames (each one frame early), and the same last picture.
- `make GAME=mshvsf check-widescreen`: every one of the twelve stages walked to both ends in widescreen, no unpainted
  columns; invisible walls leave the game's memory the board's at every 4,000 frames of the ladder; the ladder in
  widescreen with no instruction left to the interpreter.
- The tag-team movies (X-Men vs. Street Fighter's, which play the same here):
  `make GAME=mshvsf check FRAMES=6200 MOVIE=games/mshvsf/movies/coop.txt ARGS="--features free_play,tag_team=1 --poke FF3A54=03"`
  and `make GAME=mshvsf check FRAMES=5200 MOVIE=games/mshvsf/movies/two_vs_two.txt ARGS="--features free_play,one_hit,tag_team=2"`:
  translated as interpreted, two runs alike; the label under the bars follows the character on point through the tags
  (pictures every 100 frames; the records' bytes +261 traced).
- `make GAME=mshvsf check-rollback` and `check-online`: pass (3,000 frames, 940 rollbacks; the online match the same as
  both players' controls replayed).
- By hand: `--stats` over the runs above and every mode (no instruction to the interpreter); features off and no
  sources at all give the same `--hash-log` over 6,000 frames of versus (`--cpu interp`); a state saved at frame 3,000
  and loaded into a run of the same input: frames 3,001-6,000 alike; run-ahead 1 and 2 give run-ahead 0's hash log over
  8,000 frames of versus; "Straight to the title" on and off give the same hash log; every Board choice read back from
  its variable, the test menu showing the board's own with choices in, and the settings memory the same with and
  without them; no damage (player 2's team's health stays 0x90 through versus), infinite time (the clock at 99),
  one-hit knockouts (the ladder); the hidden characters (Start held on Dan: Shadow); frame data over a training session
  (`frame_data.csv`); pictures of every mode, the zoom at 2x and on both, and the launcher's three pages.
- Marvel Super Heroes and X-Men vs. Street Fighter after the toolkit's changes: built at origin/main and on this
  branch, the same scripted runs on both (the ladder, versus, training with the aids, widescreen, invisible walls, the
  model viewer standing and in an animation, survival, ring out, the endless battle on one stage and scrolling, attract
  mode, the stage viewer, the zoom; `CPS2RECOMP_BATTLE_SEED=1`): all 28 hash logs alike (the hash takes the picture
  in), and both games' feature lists the same.

## Not checked by a person

Nobody has played it in a window, used a gamepad with it, or listened to it: every check above is headless. The
custom soundtrack files, the dummy's record and replay, and the arcade timing have not been tried by hand. The themes of
Captain America, Omega Red, Ryu, Dhalsim, M. Bison, Akuma and Dan are named by their numbers, not heard.

## Next

- Online play is the toolkit's; this game has not been tried over a network.
