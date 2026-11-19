# Dimahoo: progress

## How it is made (2026-11-09)

- **The ROM set** `dimahoou` (USA 000121), from the Internet Archive's VintageArcade item. None of the user's machines
  that were on had it. Every file's CRC32 is MAME's (`cps2.cpp`, `ROM_START( dimahoou )`), the key's too. MAME's parent,
  `dimahoo`, is the European set of the same date (older lists call it the USA one).
- Four graphics files in one row, three buttons (`cps2_2p3b`), Capcom's sound driver 1.80 (translated, not rewritten).
  The picture is vertical (`ROT270`): the board draws it lying on its side, 384 x 224, its top at the right.
- **Other sets**: `scripts/other_sets_survey.py` (the add-game skill's) found `dimahoo` (Europe) and `gmahou` (Japan)
  alike but for three words of data: two of the vectors' table (000030, 000038) and the region the boot copies to
  A5+8E6 (007F8A). Both are `other_set`s; each boots with its region's warning (Japan's with its own title). The bootleg
  `dimahoud` differs all through and is not taken.
- **Translated code**: `make explore` twice (no features, then free play and invincibility so that made-up play goes up
  the stages) and `--record-entries` runs (the ladder to the ending, versus, made-up play for one and two players and
  from stage 4, attract, the test menu, the sound test, practice at a boss, boss rush, the viewer, the other regions):
  6,501 entries; the sound program's 868.

## What is known of the game (dimahoo.h has the names)

The game is Eighting/Raizing's, not Capcom's own. **Its base register A5 holds 928000, inside the video memory**
(900000-92FFFF): its variables, its stack (A7 92Dxxx) and the test menu's working copy are there, past the scroll
screens and object tables it draws from. Work RAM holds the ships' and enemies' records and the hit tests' grids. The
toolkit's `Ram8` and the like now reach the video memory by address (runtime/game.cpp), as does `--trace`; `--dump-ram`
writes it beside work RAM (`FILE.video`). Its code is strewn with `cmp.w a4,d7 / cmp.w d4,d1 / cmpa.w d5,a3`, which
does nothing. Its work is tasks (008248 starts one, `trap #0`); the show is one long script of calls (038D8A).

- **Credits**: the byte at A5+8CD (up to 9; 013B20 adds as coins come, A5+8CC counting the coins; 013A8E takes at Start).
- **Free play**: the board's own. The settings routine (01120E) sets A5+8CF when the coin mode is 0x12, the last; Start
  then takes no credit (013A6A). Start does nothing during the story, free play or not, until the title.
- **Settings**: the record at A5+8FC: +00 coin mode, +01 continue (0 on) -> A95, +02 monitor -> A96, +03 demo sound
  (0 off, 1 on, 2 half) -> A93, +04 sound mode -> A94, +05 chute type, +10 difficulty (0-7, shown 1-8; 3) -> A92, +11
  player stock (0-4: 1-5 ships; 2) -> A8D, +12 bomber stock (0-4; 3) -> A8E, +13 extend type (0: 2M and 5M; 1: 2M only;
  2: 4M only; 3: every 2M; 4: every 4M; 5: none) -> A8F, +14 stage type (0 one loop, 1 two) -> A90, +15 invincible ->
  A91. Found by stepping each option of the test menu with Right and reading its working copy (A5+7E6C on), which it
  takes from the variables as it opens. The boot calls the settings routine at 00BBAC. In the test menu A5+306 is FF.
- **The step** (word, A5+40): 1 the boot, 3 the title with credits or free play, 5 a game, 6 the show's demonstration;
  the rest of the show leaves 2E36 in it (`move.w $2.w`). **A game is on**: A5+306 = 1 (0139F6). **Who plays**: A5+307,
  bit 0 player 1, bit 1 player 2. **The scene** (word, A5+30A): 04 the choice of character, 0C a stage, 14 the change of
  stage, 18 continue, 1C game over, 20 after it, 24 the ending. **The show's title** begins at 038E7E (A5+42, its time,
  set to 0x1E0; frame 3216 from a cold start).
- **Stages**: six. A5+31A counts the stages played (0-5, 008BDE); A5+31E is the stage it is, looked up at 008B7E in
  the table at 008CCC by the route at A5+320 (eight bytes a route: the first three stages in one of six orders, then 3,
  4, 5). The route stays 0 (only 008B64 touches it, keeping it under 6). After the sixth, a second loop (A5+322) if the
  board says two (008BB4), else the ending. The ladder movie: stage 2 at about 15,450, 3 at 25,670, 4 at 36,190, 5 at
  54,840, 6 at 78,050, the ending at 109,950 and the title again at 114,600.
- **The players**: records of 0x100 at A5+46 and A5+146: +00 state (0 out, 1 playing, FF a ship lost), +02 the player,
  +04 the score (a binary long, shown as it is), +08 ships in stock besides the one in play (from A8D, 02A4F6; 02A2EA
  takes one), +09 bombs (from A8E at each ship's start, 02A194; 029FDE takes one), +0F the character (0 Solo-Bang, 1
  Karte, 2 Miyamoto, 3 Grimlen; the choice's cursor), +F0 the ship's record.
- **The ships**: records in work RAM at FF1000 and FF1C00, their state in their task's +10 (0x22 on: blown up). Bit 5 of
  +48 is a hit this frame; with no barrier (bit 7) and the board's invincibility off, 0292E4 starts the blast. The hit
  tests: 002E60 tests the ship's box (+08 to +0A across, +0C to +0E up: 6 by 10 pixels) against the enemies' shots,
  points (+32 across, +30 up) in a grid of lists at the word FFE076; 0031DE tests it against the bodies' boxes (a grid at
  FFE078, nodes with a kind and a record, its box at +08 to +0E). The grids are made during a frame and gone at its end.
  Places are pixels: across is the picture's row as the board draws it, up its column plus 64.
- **Buttons**: 1 shot, 2 bomb (029F9E), 3 the shot held (02A666 sets button 1's bit).
- **Bosses**: their music (A5+79F4, the last asked for at 00CB6C): C9, CA, CC and, last, CB; from the boss's "has
  appeared" to its end.
- **Sound**: the song table at 0x9006 after a word of their number (0xD2), as Capcom's; the numbers asked for are the
  songs: C1 the story, C2 the choice, C3-C8 stages 1-6, C9 a boss (stages 1, 3, 4, 6), CA stage 2's, CC stage 5's and
  stage 6's before the last, CB the last boss, CD a stage cleared, CE the ending, CF the staff roll, D1 the title.
- **The panel** is scroll screen 1 (its bars, the scores, the ships left); the stage is screens 2 and 3.

## What the port adds

Free play (the game's own flag, put back from the coin mode when switched off), straight to the title (run unseen until
the show's title or a game), start at stage (02EDBE: the stage's count put in as the choice of character ends), the
cheats (invincibility by going on unhurt at 0292E4, as the board's own invincibility does; infinite bombs by skipping
the subtraction at 029FDE; infinite ships kept at two while a stage is played), the Board page (choices put in the
variables before every frame, the board's own as the test menu opens, so it shows and saves them), the music's names,
and the shoot-'em-up modes of the toolkit's `shmup/` (practice with the character, ships and bombs; the ship's 6 x 10
box, the shots' points and the bodies' boxes read at the hit tests, a readout of the magic gauge (+0A) by hooks that only look, kept for drawing only; the
summaries counting the score; score attack, caravan, boss rush, the stage viewer with screens 2 and 3, attract mode).

**The picture turned upright** (toolkit): `Game::vertical` (Dimahoo sets it) gives the Display page a "Picture turned"
row, Upright by default: the finished picture, aids and all, is turned a quarter to the left and shown at 3:4 in a
tall window (a third taller than the board's lines times the window size); On its side shows it as the board draws it.
A turned picture is drawn by the CPU (the GPU's own picture steps aside), and the menu stays as it is over it. The
readout's boxes are drawn on the board's picture before it turns, and its text upright after (Port::Shown), sized by
the picture's shorter side. The
controls need nothing: a vertical cabinet's controls are already the turned picture's.

No widescreen: upright, the picture's width is the board's 224 lines, which no setting of the video chip makes more
of; a wider picture would need the game drawn into lines the board does not have. Not attempted.

## Checks (2026-11-09)

All on the code as pushed (the other Mac ran the `check`s, `make check` here):

- `make GAME=dimahoo check FRAMES=40000 MOVIE=games/dimahoo/movies/ladder.txt ARGS="--features free_play,invincible"`:
  translated code as the interpreter, two runs alike.
- `check FRAMES=12000` with the versus movie (two players): the same.
- `check-blocks` with the ladder movie, 40,000 frames: 268 million translated blocks checked against the interpreter,
  and 273 million of the sound driver's.
- `check-release FRAMES=40000`: passes (93 frames differ between the interpreted and the translated build by design:
  interrupts between instructions, not blocks).
- `check-rollback` (400 rollbacks, every cheat and feature of the game's on) and `check-online` (2,992 frames, names and
  chat, the match as an offline replay): pass; and with the Japanese set and the European set each joining a USA host
  (`JOIN_ROM=…`). Those first came apart at frames 60 and then 180: the boot's ROM check reads the program where the
  keys differ, and so does play (the vectors' word at 000002, and words at every 256 bytes and 0xD8 more, found by
  recording every read of the program over the ladder, versus, made-up play, attract and the test menu, in a build
  made for it and thrown away). `online_words` lists all of them: 883 words.
- `make check` and `tools/audit.sh`: pass.
- By hand: `--stats` over the ladder (120,000 frames, to the ending), versus, made-up play for one and two players,
  20,000 frames of attract, the test menu, practice at a boss and boss rush: no 68000 instruction outside the
  translation (the sound driver's one, the toolkit's, as in the other games). Features off = no sources: 8,000 frames of
  the versus movie and of a game begun with coins, `--cpu interp`, byte-identical hash logs with and without `src/`.
  Straight to the title on and off: the same hash log. A save state at 3,000 loaded into a fresh run: frames 3,001 to
  6,000 alike. Run-ahead 1 and 2: the same hash log as off. Every Board choice read back from its variable, five ships
  given as four in stock; with every choice made the test menu shows the board's own and the settings memory comes out
  byte-identical. The cheats: no ship lost with invincibility (three without), no bomb taken with infinite bombs, four
  ships lost and the game going on with infinite ships. Each mode run: practice at stages 1, 2 and 6 and at stage 3's
  and 4's bosses, score attack, caravan, boss rush (through the bosses of stages 1 to 5 with the board's invincibility
  poked on), the stage viewer, attract mode's demonstrations. Start at stage 4: the ship, panel and bombs as in a game
  begun at stage 1. The Japanese and European sets boot with their own warnings. The launcher's three pages drawn with
  no window. The turned picture: the window path run briefly; the window's picture as `--present-every` draws it,
  upright, with the aids on the ship and the text upright.

## Not checked by a person

Nobody has played it by hand: not the controls' feel, not the modes in a window (headless runs only, and one short
windowed run), not the sound (never listened to), not online play between two machines (`check-online` plays two
copies on one).

## Next

- The second loop (board set to two loops) and its ending, not run yet.
