# Street Fighter Zero 2 Alpha: progress

The Asian arcade release of 1996-08-26 (ROM set `sfz2al`, a parent set), on Capcom's CPS-2 board: the last revision
of the "Alpha" version of Street Fighter Alpha 2, and the one played online (FBNeo's set of that name, which
Fightcade plays and peon2's FBNeo training mode is made for). Added 2026-10-28 for issue #135. It is made the way
Vampire Savior 2 is: it builds Street Fighter Alpha 2's sources (`games/sfa2/src`) rather than having its own, so
every feature Alpha 2 has, it has, from the same code.

## How it is made

- **The ROM set** `sfz2al`: from the Internet Archive's `VintageArcade` item (`sfz2al.7z`, not on the user's
  devices); every file's CRC32 is MAME's (`cps2.cpp`, `ROM_START( sfz2al )`), the key's included. The graphics, the
  samples and the sound program are Alpha 2's own files (the same CRCs); only the 68000's program differs. The sound
  program is driver 1.05 (translated).
- **Translated code**: the toolkit as for any game. A fresh tree's first entries were Alpha 2's moved over by
  `tools/portmap.py` (36,261 of 40,677 instructions, 89%, placed at once), and `make explore` found the rest (2,462
  entries). No instruction of the 68000's is left to the interpreter in any run tried (below); the sound program's
  sound tests needed 31 more entries (`--record-entries`).
- **The shared sources** name Alpha 2's places as `At(0x...)` (ROM and work RAM) and `AtA5(...)` (a variable by its
  distance from A5), as Vampire Savior's do (`games/sfa2/src/places.h`; the lookup itself is the toolkit's,
  `runtime/places_in.h`, shared with Vampire Savior's family). Alpha 2's own `family.h` has no list: Alpha 2 built from
  the changed sources is byte for byte the program it was.
- **This game's `family.h`** lists where each of the 406 places is here, between the lines `places: begin` and
  `places: end`, made by `tools/places.py` from portmap's map of the two programs (`--sources games/sfa2/src --from
  work/sfa2 --to work/sfz2al`); twelve were placed by hand (the line says how). Then what is this game's own: the
  characters, the hidden forms, the colours' file, the damage levels' names, and the rules that differ (below).

## What differs from Alpha 2 (found in its code)

- **Places.** Most code moved by a few bytes to a few thousand (the character choice, 0628D8 in Alpha 2, is at
  066B6C, a routine of its own called from two places); the tables by character, from 097AEC on, are 32 rows long
  where Alpha 2's are 24, and moved by +6000 to +7500; the colours' tables are at 260504 (sets) and 267D44 (effects).
  Work RAM: A5+0 to +1FFF is Alpha 2's; the hit test's variables (A5-5808 on) are at A5+6E52 on; the settings
  memory's at A5-5E24 on (Alpha 2's -5E2A).
- **Characters** (a player's +82): Alpha 2's 24 (0-17 the eighteen on the screen, then the classic Chun-Li 12, Gen's
  other style 13, Shin Akuma 14, EX Zangief 15, EX Dhalsim 16, Evil Ryu 17), and **EX M. Bison 18, EX Sagat 19, EX Ken
  1A, EX Ryu 1B**. Seen on the name plates and in fights with each number held (the versus movie, `--poke FF8482=N`).
  The tables have 32 rows: 1C gives a Chun-Li and 1D a Sakura in other colours with routines of their own that no code
  on the board was seen to choose (left out of the lists); 1E and 1F give Ryu's.
- **Super Street Fighter II's ways** (no guard in the air, no quick rise, a throw ends the stun's count): Alpha 2 tests
  the numbers of its two (15, 16); this game a byte of the player's, +B1, set as a round begins from a table by
  character (09CFDC: 1 for 15, 16, 18-1B, 1D-1F). `family::Classic` (guard.cpp, throws.cpp, player.cpp).
- **The guard-range test** (023074): the table of the modes' ways (02308A) has two rows; in the other mode it tests
  one record after the players' (C00 from A5), not both (`kGuardBothHelpers`); the rewrite reads the table from the ROM.
- **A stun's length** (0225DA): the character's own routine (0A1C7E) first, then by chance from one row in the code
  (022654), not by character (`kStunByCharacter`).
- **The super combo gauge** grows less in many places (the counts of `moveq #1/#2,d0` before 026902 differ from
  Alpha 2's before 026DB0); in a rewrite, a player knocked down after a hit in the air gets 2, not 3 (`kDownGauge`).
- **The round's end** (RoundFighting, 005C6E) calls a routine of its own after the winner's count (`kRoundWonToo`,
  005D78).
- **Pushing apart** (00EE14): a byte of player 1's, +29D, has the two looked at the other way about, once
  (`kPushOtherWay`).
- **The hit test** reads the frame's attack box number unsigned (andi.w #FF, 010474; Alpha 2 ext.w)
  (`kBoxNumbersUnsigned`); a helper's hit also marks the victim +258 (010A76, `kHelperHitMarks`).
- **M. Bison's and Chun-Li's other forms**: the checks by number take EX M. Bison (18) with M. Bison (021332,
  021D24) and 1C with Chun-Li and the classic Chun-Li (021200) (`family::Bison`, `family::ChunLi`).
- **A hit landing** (021D24): +256 cleared too (`kReelClearsToo`); a hit of kind 0B ends at once (its animation, the
  colours put back, +250-252 cleared: `kReelElevenEnds`); auto guard's flash steps the colour set by 3, not 4
  (`kAutoGuardSets`).
- **A throw's reach** in the other modes takes the other's +131 for its box turned about, not its facing (025D34,
  `kThrowTurnedInModes`).
- **The board's settings**: the same variables and values as Alpha 2's (A5+A0 on; stepped in the test menu), but the
  damage levels are named Low, Normal, High and Maximum (`kDamageLevels`; the feature is this game's own in the
  collection). A new board's difficulty is Expert, its game speed Free select 2.
- **Stages, music**: Alpha 2's eighteen stages and the same sound program and songs (the custom soundtrack's names are
  Alpha 2's 59).

## Checked (2026-10-28)

- `make GAME=sfz2al check FRAMES=40000 MOVIE=games/sfz2al/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`
  and `check FRAMES=12000 MOVIE=games/sfz2al/movies/versus.txt`: the translated code does what the interpreter does,
  and two runs are alike. (The movies are Alpha 2's: they replay in this game.)
- `check-blocks` over the ladder, 40,000 frames: 367,284,133 translated blocks checked against the interpreter, none
  differing.
- `check-release FRAMES=40000`: the build with no translated code shows the translated build's pictures but for 8
  frames (150 allowed) and ends on the same one.
- `check-rewrites` (`games/sfz2al/check_rewrites.sh`, Alpha 2's check on this game, with this game's other forms held
  as player 1 too): 222 runs over Alpha 2's movies, the attract mode at three levels of the computer and a training
  session, each with the outer rewrites left out in turn; all 51 rewrites called, no call differs.
- `check-widescreen` (Alpha 2's, run on this game): the stage's plan against the map the game drew (137,156 tiles,
  none differ), no stage's ends unpainted on any of the eighteen (with and without invisible walls), invisible walls
  the board's in memory at 7 places of the ladder, nothing left to the interpreter.
- `check-rollback` (every feature that changes play on, 400 rollbacks) and `check-online` (two copies over a bad
  connection made up, then the match replayed from both players' controls): alike.
- By hand: no instruction of the 68000's left to the interpreter (`--stats`) over the ladder, versus, made-up ladders
  and two-player play, 20,000 frames of attract mode, the test menu, a wide training session, every mode, the model
  viewer, and `--sound-test 1..12` (the sound program: none after its 31 entries). Straight to the title on and off:
  the same `--hash-log`. Features off (`--cpu interp --no-rewrites --features -free_play`, an empty settings.ini)
  against a build with no sources (game.cfg's `sources` taken out): byte-identical hash logs over the versus movie.
  A save state saved at 3000 and loaded: the same hashes from 3001 to 6000. Run-ahead 1 and 2: the same hash log as
  0. The Board page: every choice of all nine puts its value in the variable; the settings memory is the same with
  and without choices, and the test menu shows the board's own values. Infinite time holds the clock at 99. The
  launcher's three pages drawn with no window.
- Alpha 2 is as it was after all of it: the state hashes of its versus, ladder, tag team and a wide training session
  with every aid the same frame for frame as before the change; built from the At() sources before the family facts,
  its program was byte for byte the same. Vampire Savior 2, built with the shared place lookup, is byte for byte the
  same program.
- `make check` (the whole repository: audit, the build with no game, the launchers): ok.

Seen in pictures (no window): the boot, the title, the character choice, fights with every one of the 28 characters
(the byte held), the ladder to Ryu's ending, the staff roll and the title again (Alpha 2's ladder movie: by frame
26,000), a training session with every aid in widescreen, attract mode, the stage viewer, the model viewer with each
EX form, survival, the endless battle, ring out, the team battle, the hidden characters (Evil Ryu and EX Ken from the
versus movie), the test menu's Game configuration with board choices set (the board's own values shown).

## Not checked by a person

Nobody has played it with a gamepad or a keyboard, listened to it, or looked at it in a window. Characters 1C and
1D (a Chun-Li and a Sakura no code was seen to choose) are left out; the board's own hidden modes (survival, the
dramatic battle) were not looked at. PalMod's tables for this set have not been tried.
