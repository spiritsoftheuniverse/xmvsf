# Cyberbots: Fullmetal Madness: progress

Issue #135. The set is `cybots`, Cyberbots: Fullmetal Madness (Europe 950424), MAME's parent and the set FBNeo's
training mode (and so Fightcade) plays; the USA set has the same date. Every file's CRC32 checked against MAME's
`cps2.cpp`. Sound driver 1.05A (runs translated). Four buttons: low attack, high attack, boost (the board's first
three) and weapon (the fourth, the first kick's line: MAME's `cybots` inputs).

Its program is its own: no game in the collection was made from it, so it has sources of its own (`src/`). They were
copied in shape from Night Warriors' and Pocket Fighter's.

## Where things are (each found here; cybots.h has the names)

| What | Where | How found |
| --- | --- | --- |
| A5 | FF8000 | the step words |
| Step words | FF8000/04/08/0C (in fours) | `--watch FF8000-FF8005` over boot and attract, pictures every 250 frames |
| Board start | FF8000 4 to 14 (00A996 on), frames 39-262, then 0 (00146E, frame 453) | watch |
| Show loop | FF8000 4-0C a machine part by part, 10 a fight, 14-18 the ranking, 1C the introduction (FF8004 68: the title, frame 4014), 20-28 another machine, 2C a fight, 30; then 4 | watch, pictures |
| A game | the game's own routine 00C3A6 (Start pressed); FF80BC 1 in a game; FF8000 4, FF8004 0 choosing, 4 the match (8 between); FF8008 04-0C the story, 10 a round's introduction, 14 fought, 18-20 over | watch through a ladder |
| Players in | FF80E5, bit 0 player 1, bit 1 player 2 | dumps before and after each Start |
| Credits | FF80B6 (BCD byte, at most 9; 0012D2 adds a coin's) | dumps after 1, 2, 3 coins |
| Free play | the game's own: coin mode 12 (the test menu's last); 008EE4 keeps the mode doubled at FF80AC, the coin words FF8084/FF808C from the table at 0092B4 (0101), and sets FF80AA (009064), which 004BB2 on tests with the credits | test menu stepped (19 modes), listing |
| Round clock | FFEBA0, BCD seconds, and the fraction under it (FFEBA1) that 00EFCE takes the timer speed's step off (D0, the table at 00EF8C); a second off at 00EFF4 | watch through a fight |
| Players | FF81A0 and FF85A0, 0x400 apart; health word +44 (0x98 full, 001FEC), the bar +46 | watch on the FBNeo script's health byte (FF85E5) |
| Damage | nine places (`lst.py -r 'sub.*0x44\(a'`): 0463E4 (a hit: the one most go through), 002F16/002F9A/003036/0030A2 (throws and the like, A4), 01B81C/01B88E (damage over time), 045E02/045E5A (both at once) | watch through fights, listing |
| Settings | the record at A5-4600 (FF3A00) is the test menu's; the board's start copies the settings memory's into the variables (008EE4, returning at 0090A2), leaving the test menu again (091256, 0912F4) | each option stepped in the test menu with a dump after each step |
| Sound queue | a ring of 16-byte requests at FF9438, the word at FF9434 the next's place, 0xFF0 (0055FA puts one in, 005544 sends one) | listing |
| Camera | an object (+1A X, +1E Y) that A5+6B1C points to, which is the stage's own (FFEC1A, FFECDA, FFED9A...); 017F3A steps it toward the players (at most 6, its ends the words A5+6B1E and 6B20: 0 and 0x280), the other planes' objects follow its steps (01803E on); 017256 copies it to scroll 2's words FF802E/FF8030 (the Y with 0x200 more on stages from the map's second half) | dumps walking to each end on several stages; the fighters' objects against their X |
| Stage ends | players 0x1C to 0x3E4; they start at 0x1A8 and 0x258; 003782 keeps a player inside the picture (half its width +118 from each edge) | watch on X pressed against an end |
| Stage | FFEA2D (copied to FFEA1A as a round is made, 001FCA): the story's table for one player (0020E4 on: +389 the path, +38A the place in it), at random for two (0021D0); 00-09 and 0C-0F are stages, 0A and 0B are not | each held with --poke, pictures |
| Players made | 002188 makes a player from its choices: +370 the pilot from +389, +371 the machine (and +372, +373 its arm and weapon) from +38C | listing |
| Controls | FF806E (player 1), FF8072 (player 2): bits 0-3 the stick, 4-6 the first three buttons, 8 the fourth | dumps holding known buttons |
| Song table | sound ROM 0x9006, 0x400 songs (as Night Warriors) | the word at 0x9000 |

The board's settings, the variable each goes to (from A5), and the test menu's names (the board's own in brackets):
difficulty A8 (0-7: 1 easiest to 8 hardest; 4 medium), damage level 100 (1 minimum to 8 maximum; 4 medium), timer
speed 101 (1 slow to 8 fast; 4 normal), event 102 (off), join in 103 (0 on, 1 off; on; the event sets it off,
00909C), continue AF (on), demo sound A7 (on). The test menu's System page: coin (19 modes, 12 free play), continue,
monitor, demo sound, sound mode, chute type, language.

### The player record (RAM dumps of players standing, crouching, jumping, attacking, hit and guarding; the FBNeo
training script for this set, and a script by jedpossum it took values from)

| Offset | What |
| --- | --- |
| +00 | 1 while the record is in use |
| +03, +04 | 0 free (+04: 0 standing or walking, 4 crouching, 8 in the air, 0C landing), 4 busy (+04: 0 attacking, 4 a special move, 8 knocked down, 0C thrown, 10 reeling); 019D70 dispatches on +03 |
| +09 | 1 facing right (a box's offset across is kept as if facing left) |
| +1A, +1E | X and Y, longs with 16 bits of fraction (Y 0x30 on the floor, upwards); +22, +26 the same as the frame before left them |
| +32 | the box tables (a word at the start of each kind: +0 attacks of 16 bytes, +2 head, +4 body, +6 legs, +8 push of 8): across, up, half width, half height |
| +3A, +3C | the animation frame's time left, and the frame (a long in the ROM: 12, 24 or 34 bytes by its byte +0B) |
| +44, +46 | health, and its bar |
| +62-+66 | the box numbers now (attack, head, body, legs, push) |
| +116 | 1 or 2: the picture's left or right edge held the player |
| +118 | half its width |
| +130 | not zero: it guarded the hit being taken |
| +200 | the computer's script (020484 starts it) |
| +253 | 1 while the computer plays it this round (0196E6 copies +380, the match's, as a round is set up) |
| +370-+373 | pilot, machine, arm, weapon |
| +374 | the other player's record |
| +381, +382, +383 | choosing: 1 while choosing, 1 once chosen, the step (0 pilot, 4 on the machine) |
| +388, +389, +38B, +38C | the machine cursor (0-3), the pilot chosen, the colours (bit 0), the machine chosen |

The whole picture stands still while a hit lands with FFEBA4 set (FBNeo's script). A5+6B9A set stops the clock and
the damage (the round over, the game's own pauses).

### Hooks

| Where | What for |
| --- | --- |
| 0090A2, 0912F4 | the Board page's choices put in the variables |
| 00C3A6 | a game begins (the Board page's choices back after the test menu) |
| 00EFCE | infinite time (the step off the second's fraction made 0) |
| nine damage sites | no damage, one-hit knockouts, the training session's no knockouts |
| 019D70 | each player's step (frame data; the ring out's own movements). It runs from 019CF8 once a frame for each (and again for one with +1AA), and its own states jump back to it: a step is counted only when the return address is 019CF8's |
| 0463B8 | a hit taken, guarded or not (the training session) |
| 0197AE | the computer's play of a player: one handed to the computer mid-fight (the modes) is given a script (020484) |
| 002188 | a player made from its choices: the modes' machines and stage, the hidden machines |
| 003782 | a player kept inside the picture: the wide picture's edges; the ring out lets one over an end |
| 017F3A, 017FCC, 01800E | the camera: where a mode wants it; its ends in the wide picture |

## Features

- Free play (the game's own coin mode), straight to the title, the Board page, the cheats, a custom soundtrack with
  the music's names (a two-player fight's tune is its stage's, 12-1C; the one-player story has tunes of its own).
- Training mode (fight/session.h): hit boxes from the box tables, frame data and the meter (the step hook), the input
  display with the game's button names (LA, HA, B, W: `fight::SetButtonNames`, new in the toolkit), the combo
  readout, the dummy, health refills, start over, the stage.
- Alternative modes (fight/alternative.h, fight/battle.h): attract mode (everything, the introduction, the fights),
  the stage viewer, survival, endless battle, ring out. The ring out's flung player flies by the port's own steps (its
  speeds kept in the state); the winner coming down onto the next stage is its record's moving part (+03 to +36F, not
  its place or the computer's script) handed over.
- Widescreen and invisible walls (widescreen.cpp): the stages' art is drawn past the board's picture on all fourteen;
  the camera's ends and the players' edges 64 further out.
- The model viewer (model.cpp): 16 machines, the four hidden ones too; the animations by the game's groups (a
  machine's table at 01F40E, read through the opcodes' cipher: its sixteen longs are in the source).
- Custom colours and the colour editor (colours.cpp): each machine's two sets of seven palettes, each palette from a
  table of its own (`colours::Description::lineAddress`, new in the toolkit); the ice stages' own colours (0154EE)
  are the game's.
- Hidden machines (choosing.cpp): Start held as a machine is chosen.
- The camera's zoom (`modes::Focus`: lift 40, tall 136, from frames rendered at 2x).

## Checks (2026-11-08, all passed)

- `make GAME=cybots check` with the ladder (40,000 frames, free_play,no_damage,one_hit: the whole one-player game to
  the staff roll, frame 31,000, and the title again) and versus (12,000 frames, with and without free play): translated
  code does what the interpreter does, two runs alike. (A first run found the computer's-script hook acting in the
  game's own show, frame 3,355 of attract mode: it is now only in the modes.)
- `check-blocks` (the ladder): 392,180,882 translated blocks and 161,451,191 of the sound driver's checked, no difference.
- `check-release` (40,000 frames): the interpreted build's pictures are the translated build's.
- `check-rollback`: the same over 3,000 frames with 400 rollbacks, with the game's features and with every play
  feature on; a frame 0.56 ms, the state 350,583 bytes.
- `check-online`: the two sides the same over 2,998 frames (pings of about 120 ms, 8 frames run again at most).
- `check-widescreen`: no unpainted columns at either end of the fourteen stages; invisible walls the board's game
  (memory compared every 4,000 frames of the ladder); nothing to the interpreter.
- No interpreter fallback: 36 runs (the movies, made-up ladders and two-player play, 20,000 frames of attract, the test
  menu with made-up input, sound tests 1-12, training, every mode, the model viewer and colour editor, the hidden
  machines, widescreen) with their entries recorded (10,292): 0 instructions to the interpreter in each.
- Features off = no sources: 20,000 frames of `--cpu interp` with and without `src/`, byte-identical hash logs.
- A save state at frame 3,000 loaded and the same input replayed: frames 3,001-6,000 the same. Run-ahead 1 and 2: the
  same hash log as 0 (versus, 8,000 frames).
- The Board page: each choice in its variable at the board's start (first and last choice of each); the settings
  memory the same with and without choices after the test menu.
- The launcher drawn with no window: the Game page (the set's name), Features, Controllers (Low attack, High attack,
  Boost, Weapon).

## Not checked by a person

Nobody has played it in a window, heard its sound or music, or tried it with a gamepad. The hidden machines were tried
in training only (Super-8). The model viewer's animation groups are the game's lists, not checked one by one (52 shown:
one machine has 56). The menu's words new with this game are in English only (not yet in the Portuguese and Spanish
tables).

## Its stages in the other games (guest.h)

Each stage from its own stage code alone (fight/guest_stages.h): the game's own loop (00C3A6, begun by a credit and
Start) with its step (00C3EE) passed over; a stage built by 001B9A, the stage's flags (A5+6A24 cleared, bit 0 of
A5+6B9B set), 001FBA, 014586, 0142B8, 015610 (ready at its first call) and 04D6A6; each frame 015610 and 04E01E. Its
camera is stepped there (017F3A across, 0181A6 up and down: the step at +50 and +52, the way at +44 and +45, which the
far planes follow). After the build the palettes' base (A5+28) is put at 9000 as a fought round has it (the character
choice leaves it at 9040, which drew nothing) and the palettes at full brightness (the round's fade is not run).
Checked with `--stage-check` in Alpha 2 (all fourteen stages, 4:3 and widescreen), Alpha, Vampire Savior and Vampire
Savior Definitive (three each): the camera where wanted, never a blank picture.
