# Darkstalkers: The Night Warriors: progress

Issue #215. The set is `dstlk`, Darkstalkers: The Night Warriors (Europe 940705), MAME's parent and the set FBNeo's
training mode plays. Every file's CRC32 checked against MAME's `cps2.cpp`. Sound driver 1.05A.

## Made from Night Warriors' program

`tools/portmap.py` from Night Warriors places 48% of its translated instructions here (674 anchors), so Darkstalkers
builds Night Warriors' sources (`games/nwarr/src`, game.cfg's `sources`), as Vampire Hunter 2 builds Vampire
Savior's: those sources name their places as `nwarr::At(0x...)`, and `family.h` here says where each is in this game
(`tools/places.py`, then by hand). Facts that differ between the two are in each game's `family.h`.

## How it differs from Night Warriors (found here)

| What | Night Warriors | Darkstalkers | How found |
| --- | --- | --- | --- |
| Player records | FF8388, 0x500 apart | FF8388, 0x400 apart | the second searched for in a fight's dump |
| Record's end | choice +4A1, colours +4A6, person +49F | +3A1, +3A6, +39F (0x100 less) | 005F98 (the copy of the choice), 005DE2 |
| Edge byte (keep-in) | +6F | +6A | 006204 |
| Health writers | 35 | 21, 4 of them with no counterpart | `lst.py -r` |
| The show | 2 QSound, 4 moon, 6 intro and title, 0E-14 fights, 16 ranking | 2 intro and title (step 08 on), 4-0A QSound, 0C the map, 0E a fight, 10 ranking | watch, pictures every 200 frames |
| Choosing | FF8004 0C (speed, character, guard mode screens) | FF8004 0 (speed on the select screen) | watch |
| Thrown things | three a player | two a player (places.py: FFAA2E, FFAAEE player 2's; FFABAE, FFAC6E player 1's) | places.py |

The same: the board's start (2 to 10, then 0 at frame 447), FF8000 2 in a game, FF8004 04 the match, FF8008 0A
while a round is fought, health +42 (0x90), X +10, Y +14, the frame +1C, the box tables +5C, half width +1C0.
| Player in a fight | +03 is 2 | +00 is 1 (+03 stays 0) | dumps of fights |
| Choosing | colours chosen, then guard mode | the bits at A5+7417, one a player still choosing (02F3A4 clears it) | whole-RAM diff around a choice |
| Colours | 0x500 a character, eight sets | 0x140 a character, two sets: a button, or Start (or the other if the opponent has them) (003F0A, 02F394 on) | listing |
| The Board | ten settings | seven: difficulty, damage, time count speed (slow, normal, fast), game speed (turbo 1-3, free select), max round, continue, demo sound (off on a new board); the same variables (02AB06 on) | the copy; the test menu's one page stepped |
| Stages | 14 | 12, Huitzil's and Pyron's the last two (the ladder reaches them); tunes 08 + the stage | --poke, the soundtrack log |
| Computer's play | inside the player's step | also at 033A8C and 033AA6 for each player; a player handed to it mid-fight is given all of 058606 first, or the play reads a script that isn't one and runs into data (05C890) | --watch, CPS2RECOMP_PC_HISTORY |

`family.h` holds all of it; Night Warriors' sources read it (`family::`), and Night Warriors' own `family.h` has its
values, so its hash logs are what they were before (seven runs compared: ladder, versus, training in widescreen,
endless battle, model viewer, ring out, attract).

## Features (Night Warriors' sources)

Free play, straight to the title, the cheats (the 21 damage sites), the Board page, the music's names, training mode
(hit boxes, frame data and meter, input display, combo readout, the dummy), attract mode (everything, the
introduction, the fights), the stage viewer, the model viewer and colour editor, survival, endless battle, ring out,
widescreen and invisible walls, the camera zoom, Huitzil and Pyron at a switch (Start held choosing Victor or
Demitri: tried, both play). Each tried with pictures.

The release check runs the ladder at Turbo 1 (game.cfg's `release_features`): at the faster speeds the two builds'
ladders part, the game timed to the instruction by its interrupts (as Super Turbo's).

Nobody has played it by hand yet.

## Checks (all pass)

- `check` ladder (30,000 frames) and versus (12,000); `check-blocks` (261,572,214 blocks); `check-release` (44 frames
  of pictures differ, at Turbo 1); `check-rollback` (400 rollbacks, three ways); `check-online` (2,996 frames, names
  and chat, the match = its replay); `check-widescreen` (all 12 stages painted to both ends, invisible walls the
  board's game, nothing interpreted).
- By hand: the Board choices land; no damage holds through hits and throws over made-up ladders; run-ahead 0/1/2
  alike; a save state round-trips; features off = no sources (6,000 frames, Night Warriors' `src/` emptied).
- Night Warriors' checks all pass again on the shared sources, and its hash logs are unchanged.
