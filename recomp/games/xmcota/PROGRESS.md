# X-Men: Children of the Atom: progress

The European arcade release of 1995-03-31 (ROM set `xmcota`, the parent set), *X-Men: Children of the Atom*, on
Capcom's CPS-2 board. Started 2026-10-29 (issue #135). Capcom's first Marvel fighting game: the engine Marvel Super
Heroes was made from, and so the first of the games on `fight/marvel.h`.

## The ROM set

`xmcota` was chosen because it is the last revision (950331) and the set competitive play uses: FBNeo's (and so
Fightcade's) set of that name, the one the FBNeo training-mode scripts are written for. The USA, Japan and Asia sets
are 950105 or older. It was downloaded from the Internet Archive (item VintageArcade, `xmcota.7z`) with the owner's
word, the owner's devices being off; all 21 files, the key included, match MAME's CRC32s (`ROM_START( xmcota )`).

The graphics are two rows of four 4 MB sockets. Six buttons a player (`cps2_2p6b`). Sound driver 1.05A.

**Other regions' sets**: of the clones, only the Hispanic and Brazil sets (`xmcotah`, `xmcotab`) are 950331. Their
deciphered programs differ from this set's in three words, all data after a `jmp` at 000D1E: the region (000D26, read
at 000BA8: 4 Europe, 8 Hispanic, 0A Brazil) and the program's checksum (000D36, read at 082644). Both are `other_set`
lines in `game.cfg`; each boots with its own region's warning and plays the versus movie, and this set's hash log is
the same with and without the lines.

## Where it stands (2026-10-29)

- Boots and plays on the toolkit's own machine: the warning, the notices, the introduction, the title, the show's
  fights and rankings, the character choice, versus fights, the whole one-player ladder to the ending (eight
  opponents, Juggernaut and Magneto last), the staff roll, game over and the show again (the ladder movie: Magneto
  beaten by frame 25,700, the staff roll by 31,700, game over by 35,500). Seen in pictures only: nobody has played it in
  a window or listened to it yet.
- 68000 program: 108,617 instructions translated to C++ (1,080 entries, `make explore` twice, then the entries of the
  runs below). No instruction falls back to the interpreter over runs `explore` never made: the ladder movie, two
  made-up ladders, versus, two made-up two-player games, 20,000 frames of the show with its demo sound, the test menu,
  and every mode (training on Avalon, the stage and model viewers with every animation, survival, endless battle,
  ring out, attract mode), and the ladder in widescreen.
- Sound: driver 1.05A, translated: 430 entries, 2,329 instructions (the sound test added 23).
- The features every game has: free play (Enhanced; the game's own), straight to the title, the board's settings
  (the Board page), cheats (no damage for each player, one-hit knockouts, infinite time), save states, run-ahead,
  arcade timing, a custom soundtrack (its music named), the in-game menu and the launcher.
- Everything the other fighting games have, as Marvel Super Heroes has it: a training mode on the toolkit's session
  (hit boxes, frame data, its meter and log, input display, combo readout, health refills, the dummy, and the X-Power
  gauge kept full), the Alternative modes page (attract mode three ways, the stage viewer), the fighting modes
  (survival, endless battle, ring out), the model viewer, custom colours and the colour editor, widescreen and invisible
  walls, the camera's zoom (the engine's), the hidden characters with no code (Start held as a character is chosen:
  light punch Juggernaut, medium punch Magneto, hard punch Akuma).
- **Shared** (`fight/marvel.h`): the training aids' view of the players and their boxes, the ring out's movements,
  widescreen and the model viewer are the engine's code, described by `src/engine.cpp`. Four things of this game's were
  new to the engine and are fields of its description now (the other games give none of them, and run as before:
  Marvel Super Heroes' ladder in widescreen, model viewer, training and ring out have the same hash logs as before):
  a frame of eight bytes (the picture's long first, the time at +6: `frameTimeAt`, `framePictureAt`,
  `framePictureMask`), the attack's phase in the top bit of the frame's flags (`attackPhaseMask`), a camera each stage's
  routine keeps between its ends itself (twelve places, `keepCameraPlaces`, two of them going on after the keeping), and
  the camera's step down going by a byte of its own (`cameraWayDown`).
- **Avalon** (Magneto's stage) waits for Magneto's entrance before a round begins; the game never puts anyone else
  there, but a mode's stage choice can. With nobody as Magneto, `modes.cpp` sets the byte his entrance sets
  (FF4854), as the game does itself when a challenger joins there (013120).
- No readable rewrites: what the features change is done in hooks that run the game's own code unless a feature is on.

## The game's memory and routines (found by experiment)

Variables sit below A5 = FF8000; the facts are in `src/xmcota.h`, each with how it was found. Many are at Marvel Super
Heroes' places: `tools/portmap.py --from` that game's work folder `--to work/xmcota` placed 29% of its instructions here
and 1,754 places it names, which gave first guesses (credits, free play, the settings record, the sound queue, the
animation table, the frame routine); each was confirmed here, and many were not where it put them.

- **Where the game is**: FF4800, a byte, 4 while a round is fought (Marvel Super Heroes' 8), 6 once it is decided
  (the winner in FF4804: 0 player 1, 1 player 2); FF4880 set once a game is played, FF4881 who is in; FF4884, the
  game's step (4 choosing, 8 a match: set at 0047AE), FF4886 the match's. The clock: FF4808 (BCD) and its frames
  FF4809, taken at 06CBB4.
- **Credits** FF4BDD; **free play** the game's own coin mode 12 (FF4BE7), which sets FF4BDE (085A92); the title says
  FREE PLAY (074118).
- **The board's settings**: the record at FFEB8C is read into variables at the board's start (0859D6, called from
  000C92; the hook at its return, 000C98). The test menu edits a copy of its own (FF2E1C on, loaded at 080F5C), so each
  option was stepped there with the copy dumped after each press, and the copy's loads give the variable of each. The
  table is in `board.cpp`. The event is two flags too (FFEDBE one match, FFEDBF sudden death, from a table at 085B38).
- **The show**: its scenes one after another (073FBE): the introduction's five (075388-076084, each marking FF2600),
  then the title, a routine of its own (074712, called at 07409A; frame 2,076).
- **Players**: two records of 0x400 from FF4000, laid out as Marvel Super Heroes' but for a few: +04 state (2
  fighting), +0C/+10 place, +14/+1C/+20 speeds and pull, +34 the frame, +4D facing, +50 the character (0 Wolverine to
  16 Magneto, 18 Akuma: the FBNeo training script's list), +64 the frame's flags (top bit set through an attack),
  +70/+72 attacks, +74-+7A hurt boxes, +7C throwable (seen to change through a jab), +82 the hit's freeze (+89 there),
  +83/+84 can hit / can be hit, +88 the box table, +A0 the action (0 stand, 2 walk, 4 jump, 8 crouch, 20 attack), +A2
  the push box (its table 0C1618), +A9 at the picture's edge, +AF the colour set, +190 health (0x8F full), +194 the
  X-Power gauge (0x8E full). The box places are the FBNeo hitbox script's for this game, confirmed by the boxes drawn.
- **Damage**: 00D36C and 00D8AE (`sub.w d5,0x190(a6)`); 00E28A takes what a move costs its own player.
- **Choosing**: the cursors FF4900 (player 1) and FF4902 (player 2), places in the table at 050AF0 (ten the cursor
  reaches, then Juggernaut, Magneto and Akuma); FF4912 a bit per player still to choose. A button (or Start) takes the
  character at 05064E (the hidden characters' hook).
- **Stage**: FF488F. Stage n is character n's own (tables at 00461E and 00462C; Akuma's 0F); a one-player game fights
  each opponent on its stage (0045CC), a two-player one stops a roulette on one (00468A). Twelve stages (0-0B).
- **Camera**: the record FF4980 on stages 0 and 0B, FF4A00 elsewhere (the hitbox script's stage bases): place +0C/+10,
  ends +70/+72 across and +74/+76 down; the floor 0xD8 below the lowest place (0xCC on stage 1). The step across
  (06EB74, by FF4B9A) and down (06EC7A, by FF4B9B); each stage's routine keeps it between its ends (06F7A8-0739EC).
  A player is kept inside the picture at 00A302 (0x55 to 0x1AB on from its plane's camera, every character 0x15 wide).
- **Colours**: a long a character at 003362 (its set, the other 0x80 on, four palettes), copied at a round's start
  (003148) to colour sets 0C (player 1) and 10 (player 2). Objects: sets 00-03 the panel (and tiles 16000-16FFF), 06
  the shadows, 0C-13 the fighters, 14 on the stage.
- **Animations**: a long a character at 0C14EA (009ED8 reads it), frames of 8 bytes put in the record at 001806.
- **Sound**: the queue at FF4C00 (next place FF4BFA, 07A530); the song table at 0x9006 of the sound ROM, 0x315 of them.
  A round plays its stage's character's theme (07 + the stage).
- **Random number**: 001332, a hook after its `divs` (001342) as in Marvel Super Heroes.

## The checks

- `make GAME=xmcota check FRAMES=40000 MOVIE=games/xmcota/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`:
  translated code does what the interpreter does, two runs alike (40,000 frames).
- `check` with the versus movie: the same (12,000 frames).
- `check-blocks` over the ladder: 302,611,792 translated blocks checked against the interpreter, and 261,054,882 of the
  sound driver's.
- `check-rollback`: the same over 3,000 frames with 400 rollbacks three ways (the game's own features; all the play
  features with widescreen; the frames run again not drawn). A frame 0.50 ms, its state 328,899 bytes.
- `check-online`: the two sides the same over 2,997 frames, and the match the same as both players' controls replayed
  without the network.
- `check-widescreen`: no unpainted columns at either end of any of the twelve stages; invisible walls keep the game's
  memory the board's at every 4,000 frames of the ladder; the ladder in widescreen leaves nothing to the interpreter.
- `check-release`: the build with no translated code makes the translated build's pictures but for 383 single frames
  (allowed 400: the blinking FREE PLAY on the second player's side is drawn a frame apart each time it changes, 347 of
  them in the Magneto fight), and ends on the same picture.
- By hand: free play (Start with no coin, FREE PLAY on the title); straight to the title on and off give the same hash
  log; one-hit knockouts and no damage take the ladder to the ending; every Board choice puts its value in its
  variable, the test menu shows the board's own with choices set, and the settings memory is the same; a save state
  loaded at frame 3,000 replays the next 3,000 frames alike; run-ahead 1 and 2 give the same hash log as none; features
  off and a build with no sources give byte-identical hash logs (`--cpu interp`, 6,000 frames); the hidden characters
  (Juggernaut, Magneto, Akuma) chosen by either player; the launcher's three pages.

## Not checked by a person

Nobody has played it in a window, with a controller, or listened to it: everything above is from pictures, hashes and
logs. Online play between two machines, the colour editor's panel, and PalMod files are untried.

## Next

- Akuma's stage (0F) is not offered by the stage viewer or training: only the twelve characters' stages are.
- The songs never heard in a run (03, 06, 13-16, 1A, 1C, 1D) are not named yet.
