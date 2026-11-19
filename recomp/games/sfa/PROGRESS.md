# Street Fighter Alpha: progress

The European arcade release of 1995-07-27 (ROM set `sfa`, the parent set), *Street Fighter Alpha: Warriors' Dreams*,
on Capcom's CPS-2 board. Started 2026-10-17. Its sources are its own (`src/`), not Alpha 2's: see "Alpha 2's
sources" below.

## Where it stands (2026-10-18)

- Boots and plays on the toolkit's own machine: the warning, the QSound logo, the introduction, the title, the
  attract mode's fights and rankings, the character choice, versus fights, the whole one-player ladder to the last
  opponent, an ending, the score ranking and the title again; seen in a window, played and listened to.
- 68000 program: 64,023 instructions translated to C++ (3,826 entries from made-up play, `make explore`). No
  instruction falls back to the interpreter over runs `explore` never made: four ladders to the ending, two
  versus runs, 20,000 frames of attract, the test menu and plain two-player play (checked 2026-10-17); the
  interpreter stays only as the fallback for a place no run has reached. The code ends at 0x054AC6 (`code_end`
  stays 100000).
- Sound: Capcom's sound driver **1.05A** (`version 1.05A /CPS2 1993 / JUNE`, at 0x0F in `sfz.01`): not the
  toolkit's 2.01b, so the Z80 program runs translated, as Alpha 2's 1.05C does: 411 entries, 2,326 instructions,
  all of it (the sound test, `--sound-test`, every song in turn, seeds 1-8). Listened to: right.
- The features every game has: free play (Enhanced), straight to the title, the board's settings (the Board
  page), cheats (no damage for each player, one-hit knockouts, infinite time), save states, run-ahead, arcade
  timing, a custom soundtrack (its songs named by number until found), the in-game menu and the launcher.
- Everything the other fighting games have (2026-10-19): a training mode on the toolkit's session (fight/session.h:
  hit boxes, frame data, its meter and log, input display, combo readout, health refills, the dummy; and the game's
  own stun readout and gauge refill), the Alternative modes page on the toolkit's (fight/alternative.h: attract mode
  three ways, the stage viewer), the fighting modes (fight/battle.h: survival, endless battle, ring out), the model
  viewer (model/), custom colours and the colour editor (fight/colours.h), widescreen and invisible walls, the hidden
  characters with no code, and the music's names. All on the shared modules, described from this game's own facts
  (below); the one thing the modules lacked (this game's panel is a scroll screen, not objects) is a field of
  fight/battle.h's (`panelLayers`), not a copy here.
- Readable rewrites: four, for widescreen (`widescreen.cpp`): the camera's place across (01A57A) and up (01A5F2),
  keeping a player inside the picture (01EEEA) and both together (01EFA2); `check_rewrites.sh` checks every call of
  each against the game's own code.

## What the modes and the training mode use (found by experiment, 2026-10-19)

Alpha 1's player records are laid out much as Alpha 2's: the same steps, boxes, half width and edge byte, stick and
stun bytes at the same places; the fields from +40 to +5F are 0x10 before Alpha 2's, those from +80 on 0x20 after. Each
fact below was found or checked here, in this game (names in `sfa.h`).

- **The steps of a game**: a round is fought with FF8000 2, FF8004 4, FF8008 4, FF800C **2** (0 its introduction).
  **Who is in**: FF818D, bit 0 player 1, bit 1 player 2 (copied from the title's FF81A8 as a game begins, 0040D6);
  **who has chosen**: FF81AC, the same bits (0191E0 adds a player's in).
- **The characters**: a player's byte **+A2** (the fight's tables are by it: 01CE2A); 0 Ryu, 1 Ken, 2 Akuma, 3 Charlie,
  4 Chun-Li, 5 Adon, 6 Sodom, 7 Guy, 8 Birdie, 9 Rose, 10 M. Bison, 11 Sagat, 12 Dan (each seen in a fight with +A2
  held at it; Alpha 2's first thirteen). The choice is made at 0191AC (a punch or a kick newly pressed, +BA and +BC:
  the colours +C8, 0 a punch, 1 a kick; +39 set once chosen). The cursor's table of characters is at 019DBE (player 1)
  and 019DDC (player 2); the two "?" boxes are Akuma and M. Bison only in the table at 019E54, which the game uses when
  A5+1BF is set (by nothing found).
- **The stages**: a word at **FFAE00**, the stage doubled; stage n is character n's (each seen with the word held). The
  match is made at 006F6E, reached from 018E1E and by a jump from 018DF2; its last step, **006FD0**, takes the stage
  from the second player's character (A1): the hook there puts in what a mode wants. Each stage's tune from the
  soundtrack log over a fight on each: Ryu's 01, Ken's 02, Akuma's 0B, Charlie's 09, Chun-Li's 03, Adon's 06, Sodom's
  05, Guy's 08, Birdie's 07, Rose's 0A, M. Bison's 0C, Sagat's 04, Dan's 32; player select 0E, versus 0F.
- **The sound queue**: a ring of sixteen-byte requests at A5-59C2 (FFA63E), the end word at A5-49C2 (012BEE puts one
  in; 012B3E sends the oldest), Alpha 2's layout; FF05 stops the music.
- **The camera**: its record at FF8280, the place at FF8290 (copies FF802E, FF82A0, FF82A8); 256 to 640 across on
  every stage (the record's +44 and +46); at most 5 a frame across, 1 up. Its routine (01A4AE) draws a 16-pixel column
  of the stage's plan into the map as it crosses each 32 pixels, 64 to the left or 432 to the right of the camera
  (01ADA0's table), so the map holds what the wide picture shows on every stage (seen at both ends of all thirteen).
  The players start at 544 and 736, the walls at 280 and 1000. The camera does not follow a player whose +4E has its
  top bit set (01A9A8).
- **The attract loop** (FF8000 with no coin in): 0C and 0E the notices, **10** the introduction, in which FF8004 goes
  2 a demonstration fight, 4 pictures, 6 another fight, 8 and 0A pictures, 0C the title; 12-16 the score ranking, 1C-20
  the notices again, **28** the introduction again with two other fights, 2C the versus ranking, then 0 and on. The
  fights are chosen by FF0CEA (pairs 0, 2, 4, 6, stepped on as each introduction begins: 001CFE) and FF0CEB (the first
  or second of the pair, turned over after each fight: 002964), read at 002A52.
- **A player's record**: +04-07 the steps (in a fight 2; +05 0 the player in charge, 2 reeling, 0E the round's
  introduction; +06 00 standing, 02 crouching, 04 walking, 06 jumping, 0A attacking, 0C guarding), +0B facing right,
  +0D its first colour set (player 1 0, player 2 8), +10 X, +14 height (28 the floor), +20 the animation frame, +24 its
  time, +2A the opponent (a word), +2F in the air (01 a jump, FF knocked up), +30/+34 the speeds and +38/+3C their pulls
  (longs), +40 health (+42 the bar), +44 the reeling's kind (4 knocked into the air), +45 the hit's kind (FF guarded:
  00F792), +4F the hit stop, +57 the combo the other is landing, +BE the super combo gauge (a word, 0x30 a level, 0x90
  full: 0217A8), +C5 played by the game (the fight step goes to 021DBA for it: 01CCD6), +C8 the colours, +109 the
  stick, +12A half its width, +13C at the picture's edge, +134 knocked down, +136/+137 the stun's timer and what has
  built up, +13A the stun that stuns (00F922).
- **A player's step**: a person's goes through 01CCF4's state table to 01CE40 and on by +05 at **01CE48**; the
  computer's through 021DCA's to 021DE6 and on by +05 at **021DF4**. Both are hooked (frame data, the ring out).
- **Hits**: the hit test 00F4C0 (A6 the attacker, A1 the other, A3 the attack); 00F646 makes it a hit or a guarded one;
  **00F638**, after it, tells the session. Damage at 00FA06 (a hit: chip on a guarded one), 00FA20 (one point) and
  021148 (a throw).
- **Boxes** (the hit test 00F4C0, the boxes' test 0104B8, the push 00F304): the animation frame names three it can be
  hit in (+08 head, +09 body, +0A legs, into the tables at the record's +110, +114, +118; four bytes a box), its attack
  (+0B into +11C, sixteen bytes an attack, the box first) and its push box (+0C into +120). A box: its middle's offset
  across (turned about when facing right) and up, half its width and half its height, one byte each. Drawn in the
  right places on the picture (seen).
- **Animations**: three tables of a long a character (01FEBE, 01FEF2, 01FF5A; started by 01E8F8, 01E91C, 01E94A with
  the number in D0; the second has another table, 01FF26, for the computer, with the same lists here), each a list of
  word offsets; a frame is 0x18 bytes, as in Alpha 2 (a word of time, a word whose top bit says the next is a jump to
  the long after the frame, the picture, the boxes' numbers); stepped at 01E96E; 01EA5E flies a step (the speeds by
  their pulls, the place by the speeds). A knock-down's reelings are in the third list, by the hit's kind (the table at
  01F41A): 13 thrown into the air.
- **The round's result**: FFAE0C, bit 0 player 2 knocked out, bit 1 player 1 (00F4A6).
- **Thrown things**: a pool of 8 records of 0x80 from FF9000, +00 1 in use (01B848 steps them; Ryu's fireball the
  first). Their hit test (01018A) takes the attack from the frame's +0B in a table the record's long +50 names, from
  its word +2 on: training draws those boxes too (orange; Ryu's fireball's seen on it). The random number: a word at FF81AE (016460).
- **Objects**: sets 00-07 player 1's, 08-0F player 2's (the fighter and what it throws), 10-17 the sparks and dust,
  18-19 the stage's pieces, 1A-1E the panel's gauges and words, the shadows in 1F (tiles 1B7A0-1B7AF; found from the
  object lists of fights on every stage). The bars, names and clock at the top are the **first scroll screen**, not
  objects: the stage viewer and the model viewer leave it out (layerMask), and the ring out lays it over the stage
  machine's picture with the fighters (`panelLayers`).
- **Colours**: the characters' sets at 1E7960, two of eight palettes each (a punch's and a kick's), copied to the
  palette buffer at 914000 (player 1) and 914100 (player 2) as a round starts (011090); in sfz.06, PalMod's file.
- **The super combo gauge**, the stun and the combo counter were named in the FBNeo training script for this set
  (peon2/fbneo-training-mode, games/sfa); each was checked here.
- **The music**: each ending (Ryu's 15, Ken's 16, Chun-Li's 17, Sagat's 18, Sodom's 19, Adon's 1A, Birdie's 1B, Guy's
  1C, Charlie's 1D, Rose's 1E, Akuma's 1F, M. Bison's 20, Dan's 33), two staff rolls (21 and 31), the winner 10, game
  over 13 and the ranking 24: the soundtrack log over the ladder with every character, with pictures where each began;
  the QSound notice 22 and the introduction 0D (attract mode), a new challenger 12, continue 11 (a lost game).
  `--music-test` lists the board's music (00-22, 24, 25, 31-34) and which ends: three are not placed yet (14, once, 13
  seconds; 25 and 34, over and over).

## Alpha 2's sources

Capcom made Vampire Savior 2 and Vampire Hunter 2 from Vampire Savior's program, and their ports build its sources
(docs/TOOLKIT.md). Alpha 1 and Alpha 2 were compared for the same before anything was written (2026-10-17):

- `tools/portmap.py` from Alpha 2 to Alpha 1: 254 anchors, 27.9% of the instructions it reaches placed (4,838 of
  17,311), 5% of all Alpha 2 has translated. Vampire Savior to Vampire Savior 2 placed 76%.
- What the instructions do, the places they name left out: 38% of Alpha 2's code and 50% of Alpha 1's is in a run of
  eight found somewhere in the other. The two share much of Capcom's common code, in other places.
- The routines Alpha 2's sources hook: of the 200 code addresses they name, the run of 12 instructions that starts
  there is **nowhere in Alpha 1 for 161** (126 even for a run of 6), every one of hit.cpp's, damage.cpp's,
  machines.cpp's and training.cpp's among them. Alpha 2 rewrote the fight's code.
- And Alpha 2's sources name their places as plain addresses: to be shared, about 309 would become `At()`, with
  Alpha 2 proved unchanged.

So this game has its own sources. Where a routine has the same shape in both (the settings memory's, the settings
read, a game's beginning, the damage taken off health), Alpha 2's sources are a guide to reading it.

## The game's memory and routines (found by experiment)

Variables are reached from A5 = FF8000, as in Alpha 2; many of Alpha 2's are here 0x100 further on.

- **Where the game is** (`sfa.h`): words at FF8000, FF8004, FF8008, FF800C, each a step of the one before (2 in a
  game; 2 choosing, 4 the match; 4 a round on; 2 fought, 0 its introduction), as in Alpha 2. Traced with `--watch`:
  the board's start goes 2, 4 (the warning, frames 108-289), 6, 8, 0 (frame 475); the show then 2, 4, ..., 10 the
  introduction (frame 948), whose step 0C is the title fading in from white (frame 2122); 12 on the attract fights.
  With a coin in, from that 0 to 2 and 4 (frame 486): the title asking for Start. FF8002 counts down each frame.
- **Credits**: a byte at FF8170 (RAM dumps after 0, 1, 2 and 3 coins; one coin a credit as a new board has it).
  Alpha 2's is FF8076.
- **The players**: records at FF8400 and FF8800 (0x400 long, as Alpha 2's). Health a word at **+40** (Alpha 2's
  +50), full 0x90; +42 the bar shown, which follows it; +14A a copy. Found by RAM dumps through a fight.
- **Damage**: a hit's comes off at **00FA06** (`sub.w d3,0x40(a1)`, the victim at A1), a throw's at **021148**
  (`sub.w d3,0x40(a4)`); the hit routine also takes one point off without D3 (00FA20-00FA26, from 00F8E2 and
  00F906), its two ways meeting again at 00FA2A. Found with `--watch` on the health. The cheats are hooks there
  (`cheats.cpp`).
- **The round's clock**: a byte at **FFAE09** (A5+2E09), seconds left: 99 (0x63) at a round's start (004378), a
  second off at **004B3A** (`subq.b #1,0x2E09(a5)`; below nothing, time is up: A5+2E0D set, 005736); A5+2E0A counts
  a second's frames (0058F6 sets them). FF365D is the picture's copy, in BCD. Found by RAM dumps
  against the clock shown (91, 87, 83, 79) and `--watch`.
- **The board's settings**: read once as the board starts, at **00B2BA** (called at 001102, returning to 001108),
  from the settings record at A5-5E7E into variables at A5+198 on: Alpha 2's routine at 00ACF0, each variable 0x100
  further. A game begins at **004076** (Alpha 2's 004B6C), which copies the difficulty to A5+1BE. The values and
  their names, from the test menu's Configuration pages, each stepped through with the variable read after each
  step (the board's own in brackets):

  | | A5+ | values |
  | --- | --- | --- |
  | Continue | 198 | 0 off, 1 on (on) |
  | Demo sound | 199 | 0 off, 1 on (on) |
  | Difficulty | 1A0 | 0 easy, 1 normal, 2-6 hard 1-5, 7 expert (normal) |
  | Damage level | 1A1 | 0 low, 1 normal, 2 high, 3 maximum (normal) |
  | Timer speed | 1A2 | 0 slow, 1 normal, 2 fast, 3 fastest (normal) |
  | Game speed | 1A3 | 0 normal, 1-2 turbo 1-2, 3 free select (free select) |
  | Max round | 1A4 | 1, 2, 3: one, three, five rounds (three) |
  | Event | 1A5 | 0 off, 1 one match, which clears continue (off) |
  | Automatic | 1BB | 0 off, 1 free select (off) |

  The test menu edits these variables themselves. The collection's launcher shares a feature between games by name
  and choices: Difficulty, Timer speed, Event and Automatic are shared with Alpha 2's, which mean the same. Game
  speed has choices of its own (four; Alpha 2 has five). Damage level has Super Puzzle Fighter II Turbo's choices
  (Low, Normal, High, Maximum), where it means the counter gems an attack sends, so it is marked as this game's own
  (`OwnFeatures`, game.cpp).
- **The settings memory**: read and written at **00B942** and **00BA0A**, variables A5-5D7E (buffer), -5D78
  (count), -5D74 (start), +1CC (busy). Instruction for instruction Alpha 2's 00B444 and 00B50C (28 and 49 of them).
  The toolkit's shortcut (`runtime/board.h`) is **not used**: the routine they call to drive the chip's lines
  (00BB28) also sets bit 0x10 of the board's output port 804040 from A5+63 bit 2, where Alpha 2's (00B62A) puts in a
  copy kept at A5+88, and the shortcut never writes the port. It only saves time, so the game's own routines run.
- **The music**: the sound program's table of songs at 0x9006, four bytes a song, the word at 0x9000 the count
  (0x300; 498 entries used), read the same way as Alpha 2's (Z80 code at 0206-021D).

## Checked

- `make GAME=sfa check` (translated code against the interpreter, block by block; two runs alike): the ladder movie
  (40,000 frames, with and without its features), the versus movie (12,000), made-up play with the features on
  (40,000), a 70,000-frame ladder with one-hit knockouts. All ok.
- `make GAME=sfa check-blocks`: the ladder movie (361,817,107 68000 blocks, 349,459,882 of the sound program's),
  versus (105,755,806 and 82,756,919), a whole made-up ladder of 130,000 frames (1,171,002,117 and 837,912,573),
  40,000 frames with the features on, 30,000 of the test menu with a board choice. No difference.
- `make GAME=sfa check-release` (40,000 frames): two runs of the build with no translated code alike; its pictures
  the translated build's but for 3 frames, ending on the same.
- **With every feature off the game runs as with no sources**: the interpreter (`--cpu interp`, which takes no
  notice of where blocks start) gives the same per-frame hashes with `src/` and without it (6,000 frames of
  made-up play). The translated build's hashes move when entries are added (docs/BUILDING.md), which is why the
  interpreter is the measure here.
- Straight to the title: the per-frame hashes are the same with it on and off; the picture comes back at frame
  2124 (the title's white), 485 with free play.
- The board's settings: each of the 31 choices puts the right value in its variable as the board starts; Expert's
  copy (A5+1BE) is 7 when a game begins; the test switch puts the board's own values back; the settings memory file
  is the same with and without a choice.
- Save states: a state saved at frame 3,000 of made-up play, loaded at 3,000 into a run that had played otherwise:
  the 3,000 frames after it the same as the first run's. Run-ahead 1 and 2: the state the same as without it, all
  6,000 frames; the picture shown at N the plain game's at N+1 (N+2) but where the input changes.
- The custom soundtrack (a test tone as a playlist, 29 pieces of music over the ladder): the pictures the same as
  without it, all 27,000 frames, and the 68000's work RAM at the end. (`--hash-log` and `--hash-game` differ from
  the first piece played from a file, in Alpha 2 too: the sound side is told "the music stops" instead.)
- Arcade timing: the ladder plays to its end, nothing to the interpreter.
- In a window (the contributor, macOS, 2026-10-18): the picture, the sound and music, the launcher, every feature
  above, a remapped control, save states, the test menu.

**On macOS, SDL 3.4.14 leaves the window black** (found here; asking the build for 3.4.16 or later is a toolkit
change of its own). With Homebrew's 3.4.14 every game's window stays black while the sound plays: its Metal GPU
device takes a frame's fence still being worked on for one done, so after the first frames every swapchain picture
is skipped (SDL 3.4.16: "Fixed the SDL_QueryGPUFence() return value on Apple platforms"). A release build compiles
SDL 3.4.16 itself.

The modes and the training mode (2026-10-19), all headless, by pictures, RAM and logs:

- Training: the session reached from the menu, player 1 choosing for both, the aids drawn (hit boxes over the
  fighters, frame data: Ryu's jab startup 4, active 4, recovery 5; the meter; the input display; the combo and stun
  readouts; the gauge full); the dummy guarding ("+2 on guard").
- Attract mode (all three ways: the fights only goes through all four demonstration pairs), the stage viewer and its
  infinite stage (no panel), survival (29 beaten in one run), endless battle, the ring out (Dan thrown over the right
  end and falling; Ryu jumping off and landing on the next stage; the player knocked over the left end ends it), the
  model viewer (Akuma on white, M. Bison on Adon's stage), the colour editor, the hidden characters (Start on Ryu:
  Akuma; on Ken: Dan; on Sagat: M. Bison).
- No instruction to the interpreter in any of them (training, attract, the viewer, survival, endless battle on three
  stage choices, the ring out, the model viewer, the editor), nor in fights of the hidden three; the endless battle's
  34 new entries recorded into the entries file first.
- `make GAME=sfa check-rewrites`: every call of the four rewrites the game's own (four movies, 0 differed:
  CameraAcross 16,333 calls over the ladder, KeepIn 28,931, KeepBothIn 369 over versus).
- `make GAME=sfa check-widescreen`: no unpainted columns at either end of any of the thirteen stages; with invisible
  walls the game's memory the same as with widescreen off every 4,000 frames of the ladder; nothing to the interpreter.
- `make GAME=sfa check` (ladder 40,000 frames with its features, versus 12,000), `check-blocks` (the ladder:
  365,551,096 blocks and 141,418,072 of the sound driver's, no difference), `check-release` (2 frames early, the same
  last picture), `make check` (the repository): all ok, with all of it in.
- With every feature off the game is as with no sources: `--cpu interp --no-rewrites`, 6,000 frames of made-up play,
  the same hashes as a build with `src/` empty (the rewrites are checked by check-rewrites; the hash takes the cycles
  in, which a rewrite charges on average).
- Save states (made-up play, saved and loaded at 3,000: frames 3,001-6,000 alike) and run-ahead 1 and 2 (the versus
  movie, 6,000 frames alike with 0).
- The toolkit's new `panelLayers` (fight/battle.h) and `Video::markLayers`: Vampire Savior's and Alpha 2's scripted
  runs the same before and after, hash log for hash log (attract three ways, the viewer, a training session with every
  aid and its frame data log, the model viewer); survival run on both (its opponents are random, so not compared).

**Not checked by a person yet**: nobody has played the modes in a window or tried them with a gamepad; the dummy's
recording and replay and its answers have not been tried by hand.

## The movies

Scripted, as Alpha 2's: `ladder.txt` (with `free_play,no_damage,one_hit`: Start, choose, then forward and a hard
punch over and over) takes Ryu to Sagat, his ending, the score ranking and the title by frame 26,400; `versus.txt`
(with `free_play`) is Ryu against Ken, then the winner on against the computer.

## The computer player: scripts (2026-10-27)

The CPU is Alpha 2's design, and Vampire Savior's before it (games/sfa2/PROGRESS.md, "The computer player: scripts"):
an interpreter (`022810`, called each step from the computer's player routine at `021EF2` while `+24E` and A5+`2E0E`
are clear) running scripts from the ROM, four banks of them for each character. `tools/aiscripts.py sfa` prints them
all (32,309 instructions in 13 characters' scripts; `--character NAME` for one). Game data: nothing decoded is kept in
the repository.

- The computer's part of a player's record is Alpha 2's, `+200` to `+24F`; the level is `+C7`. The banks' tables are
  found by `lea $X(pc),a0` (Alpha 2's have `movea.l #`): plans `023E2A`, tactics `023E5E`, reactions `023E92`, answers
  `023EC6`. The answers' table has four entries (what follows it is other data). Chance tables (128 bytes each): a
  tactic or back to the plan at a script's end `11FB18`, a broken-off script dropped `11FB98`, a go to a tactic
  `120498`, the Alpha counter by character `11FC18`.
- Less than Alpha 2's: one if (0C; the second, 14, is not there), so one way of stepping over instructions
  (`023CD4`, with each skipped word read into D1 by runs of `move.w (a0)+,d1`); waits 00 to 20 (no 22 or 24; 20 is the
  "open" wait with the other not just up, waiting `1A`); the countdown waits look at no zero first (a count at 0
  goes round to FFFF, as the game's does); one jump kind (00). No clamp on the instructions done in a frame.
- The think time (`0239C6`): the character's table by level, read through the program counter (deciphered space:
  `CodeLong`, `0239FC`), and one of 16 signed words at random (`023EFA`), not below 0.
- Tests: up to 34; 30 and 32 name the projectile classes table `0231C4`, as Alpha 2's 30/32 do theirs. Test 22 has
  Charlie's own rule (character 3: his projectile out, `+238`). The projectile gap reads the boxes' bytes. A reaction
  is chosen by the other's projectile word (`+124`: entry 2 if one is out) or else its frame's `+17`.
- Checked: every script decodes, nothing left over, and the places the game kept against them
  (`aiscripts.py sfa --against`): the attract mode 772 of 772, the endless battle 3,242 of 3,242, none elsewhere
  (`--cpu blocks --watch FF8630-FF8633 --watch FF8A30-FF8A33`, `CPS2RECOMP_WATCH_REPORTS=200000`).
- As C++ (`src/computer.cpp`): ComputerThinks (`022810`), AnswerNow (`023CC6`), BrokenOff (`02348A`), Reactions
  (`023B78`) and Chance (`0227F6`). What they call outside stays the game's: the attack coming (`0226B2`), the normal at
  the other's attack (`02191A`), the characters' specials (`023A3E`), the random number (`016462`). D0, D1 and the flags
  are free for the first three (both callers of BrokenOff, `021DF4` and AnswerNow, set D0 and D1 before reading them),
  D0 and D1 for Reactions; Chance gives back everything.
  - Checked with `--check-rewrites` (check_rewrites.sh's computer part: the attract mode at levels 0, 0C and 1F; the
    endless battle CPU against CPU in seven pairs of characters; level, tier and plan held with `--poke`, the gauge
    full): about 336,000 calls of ComputerThinks, 543,000 of Reactions, 30,000 of Chance, 1,300 of BrokenOff and
    450 of AnswerNow, none differing. Every operation, wait, walk, test and the four answers were reached (counted
    while checking, then taken out).
- The ring out is left to the game's own play. Alpha 2's tactics for it, written for this game (out of a corner,
  knocking the other over its end), made no difference CPU against CPU (the side with them won 54% and 44% of 329
  and 297 ring outs; 55% and 45% with nobody playing the mode), and the toolkit's coach did worse (44% and 36%).
- A hang found on the way, the toolkit's (fight/battle.cpp `ToRound`): a match begun from the character choice was
  given a word at random as the game's random number. The game's routine (three times itself, the top byte of that
  its new top byte and added to its bottom one: Alpha 1-3's and Vampire Savior's) keeps a number in one round of
  them (the longest 43,534 of the 65,536), and never comes to the 130 it keeps going round between two, or one, from
  any other (`4054` and `C014`, `4040` and `C000`...). With one of those every choice at random came out the same, and a reaction (bank
  4) broke in on itself without end (`023B6A`'s `bra 022810`), a step that never came back (1 run in 36 of the ring
  out). The number is now the game's own stepped on so many times at random (`Description::randomNext`).

## Tech debt

From the review of the pull request that brought this game in (tmcleroy/cps2recomp#34), left for a change of its own
because it touches other games' sources:

- **`board.cpp` and `title.cpp` are near copies of Alpha 2's** (and `board.cpp` of Vampire Savior's): `PutChoices`,
  the two hooks, `BoardBeforeFrame`, the flags; `TitleFrame` with its steps. Only the table of settings, the hook
  addresses, the mode/step/credit addresses and the labels differ. Toolkit helpers taking those (a settings table and
  two hook addresses; the mode, step and credit addresses and the title's steps) would replace the copies, so a fix is
  made once.
- **Alpha 2's copies have the two bugs fixed here**: its Board flags are not in the game's state (run-ahead can lose a
  choice put back after the test menu; a loaded state can let the test menu save the choices), and its skip to the
  title does not start again at a reset within it. The helpers would fix them there too; Vampire Savior's `board.cpp`
  is to be looked at for the first.
- **`SetLow16`** is a third copy (`sfa2.h`, `vsav.h`), and `PlayerOf` a copy of Alpha 2's: `SetLow16` belongs in
  `runtime/lift.h`.
- **The settings memory's shortcut** (`runtime/board.h`) could learn the line routine's other output bit (here bit
  0x10 of 804040, from A5+63), so that this game can use it too.

The helpers can be checked here with Alpha 1's and Alpha 2's sets (each game's hashes the same before and after,
the Board page's and the title's tests above); Vampire Savior's and the games built from its sources need their sets.

## Next

- Hand-play: the training mode (the dummy's recording and replay, its answers), the modes, widescreen, the colour
  editor, in a window.
- The game's own extras for the dummy, as Alpha 2's port has them (the Alpha counter while guarding), once their
  commands are checked here.
- The Dramatic Battle (the hidden three-player fight) needs a code made-up play never enters: a movie of it, played by
  hand, so that its code is translated and checked too.
