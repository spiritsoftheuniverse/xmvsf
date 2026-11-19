# Super Street Fighter II: progress

The World arcade release of 1993-10-05 (ROM set `ssf2`, the parent set and the last revision), on Capcom's CPS-2
board: *Super Street Fighter II: The New Challengers*, the board's first game. Added 2026-11-08. Super Turbo was made
from its program, and this port is made the way Street Fighter Zero 2 Alpha is from Alpha 2: it builds Super Turbo's
sources (`games/ssf2t/src`) rather than having its own, so every feature Super Turbo has, it has, from the same code.

## How it is made

- **The ROM set** `ssf2`: from the Internet Archive's `VintageArcade` item (`ssf2.7z`; the user approved the Archive
  for these sets); every file's CRC32 is MAME's (`cps2.cpp`, `ROM_START( ssf2 )`), the key's included. The graphics'
  eight files are Super Turbo's first eight (the same CRCs: Super Turbo adds a third row); the sound program
  (`ssf-01a`) is driver 1.04, translated.
- **Translated code**: the toolkit as for any game. A fresh tree's first entries were Super Turbo's moved over by
  `tools/portmap.py` (28,767 of 49,821 instructions, 58%, placed at once); `make explore` found the rest (2,484
  entries, 74,552 instructions).
- **The shared sources** now name Super Turbo's places as `At(0x...)` (ROM and work RAM), as Alpha 2's and Vampire
  Savior's do (`games/ssf2t/src/places.h`). Super Turbo's own `family.h` has no list: Super Turbo built from the
  changed sources is byte for byte the program it was (below).
- **This game's `family.h`** lists where each of the 84 places is here, made by `tools/places.py` from portmap's map
  (`--sources games/ssf2t/src --from work/ssf2t --to work/ssf2`); eight were placed by hand (the line says how). Then
  what is this game's own (below).

## What differs from Super Turbo (found in its code)

- **Places.** The players' records are at FF83CE (0x400 apart; Super Turbo's FF844E), and most variables from A5+400
  on are 0x80 or 0x100 lower. The code is earlier in the ROM (the camera's routines at 09BC80 and 09BD98 where Super
  Turbo's are at 0CC4E4 and 0CC630).
- **The player's record** is Super Turbo's to +3B1: every offset the sources name was compared over the instructions
  portmap paired (the character +391, the side +381, the person +38E, the other player +39E, the colours +3B1, the
  stun +5E, the bar +1BC...). Super Turbo's own bytes, the old version +3B6, Akuma +3BD, the super combo gauge +2B4
  and the times stunned +2B1, are never used here: the sources read 0 there.
- **What Super Turbo has and this game has not** (`family::kTurbo`): the super combo gauge (no gauge refill), the
  hidden characters (Akuma, the old versions: no "Hidden characters" switch), the four speeds (no Game speed on the
  Board page: its variable is the place 0, none), and a fighter's reach out pushing the camera and the edge (the
  rewrites of the camera across and of keeping the players in leave D2-D5 alone, as this game's code does).
- **The camera up** (09BD98) has no flag of its own holding it first (Super Turbo's FF8F90): the place is 0, none.
- **A stun** comes at a fixed 0x1F (`cmpi.w #$1F` at 0711CE, and the word at 0711BC that the throws' stun reads),
  not by how many times the player has been stunned.
- **The panel's names and faces** are one routine (00E5D2); Super Turbo makes the faces' palettes apart (0104B2:
  the place 0 here, and the tag style's swap goes to the names alone).
- **Colours**: eight sets a character (the six buttons, Start, a button held), each of one, three or eight palettes
  (the table at 093CDC: three for Ryu, Guile, Ken, Chun-Li, Sagat and Dee Jay, eight for M. Bison, one for the rest).
  The colour editor takes a count for each character now (`colours::Description::palettesOf`). The copy to the board
  (093C76) is Super Turbo's.
- **The sound program's table of songs** is at 7C16 (the count at 7C10: the program's `ld hl,$7C10` at 01E0), and its
  entries carry a bank byte of 10 above the 128 KB ROM (108411 for 8411: 0A71 makes the bank from it, which goes
  round), so the toolkit masks them (`Game::songAddresses`). 63 songs are music; they are Super Turbo's but its own
  title, player select and final fight (D2-D6). Found with `CPS2RECOMP_SOUNDTRACK_LOG` over the ladder and attract
  mode with its sound on: the introduction and title 33, player select 34, the bonus stages 3B; the last fight plays
  M. Bison's stage's tune (0C).
- **The board's settings** are Super Turbo's variables (difficulty A5+2D3, continue +2DA, demo sound +2DD, copied
  from the settings memory at 007A16) with no speed; read where the board's start returns (008380, after `jsr $798A`;
  Super Turbo's 008FFC after `jsr $823A`). A new board: difficulty 4 (medium), continue on, demo sound off.

## Found on the way: Super Turbo's tag team battle

In the tag style, a fighter tagged in had its health put back above what its bar showed (+1BC). The game only steps the
bar down to the health (00E33A here, 01006A in Super Turbo: stepped by one until the two are alike), so the bar went on
down past nothing and was drawn empty for the rest of the round (Super Turbo's bars at 0xF76A and 0xFD30 in the
team movie at frame 3,600). The live tag's frame now puts the bar at the health when it is below it. Both games.

## Other games' stages in it

It hosts the other fighting games' stages as Super Turbo does (games/ssf2t/src/stages.cpp, game.cfg's `guests`, Super
Turbo's own left out: they are this game's): training on Darkstalkers' and Gem Fighter's seen, and check-rollback and
check-online pass with `other_stages`.

## Its stages in the other games

Its stages are Super Turbo's: the same graphics files, the same stage code (portmap pairs it), the same sixteen. The
host games show them already as Super Turbo's (`games/ssf2t/guest.h`), so it has no guest entry of its own (as Hyper
Street Fighter II has none).

## Other regions

`ssf2a` (Asia 931005) and `ssf2j` (Japan 931005) are this release in other regions, but their sound program is
another (`ssf.01`, not `ssf-01a`): an `other_set` line takes only the program and the key, so they are not taken. The
930911 sets (`ssf2u`, `ssf2h`, `ssf2r1`...) are an earlier revision.

## The checks

All with the build of this branch, on the mini.

- `make GAME=ssf2 check FRAMES=55000 MOVIE=games/ssf2/movies/ladder.txt ARGS="--features free_play,no_damage,one_hit"`:
  ok (translated code is the interpreter's, two runs alike). The ladder movie (Super Turbo's, made longer for the bonus
  stages) beats M. Bison at about frame 45,000; the ending, the staff roll and the title again at about 54,000.
- `check` over the versus movie (12,000 frames): ok.
- `check-blocks` over the ladder (55,000 frames): 460,198,970 translated blocks checked against the interpreter, and
  358,131,775 of the sound driver's.
- `check-rewrites`: CameraAcross and CameraUp 10,497 calls, KeepIn 19,809, none differed (on each movie and two
  made-up two-player games).
- `check-rollback`: ok, three ways (400 rollbacks each). `check-online`: ok (the two sides the same over 2,992 frames
  with every Game feature, the match the same replayed without the network).
- `check-widescreen`: every stage painted to both ends; with invisible walls the game's memory is the board's at every
  4,000 frames of the ladder.
- `check-release FRAMES=40000`: ok (picture hash in the pull request).
- By hand: no instruction left to the interpreter in the ladder, two made-up games, versus, 20,000 frames of attract
  mode and of the test menu; features off with no sources (the build without `sources = ssf2t`) the same hash log, frame
  for frame; run-ahead 1 and 2 the hash log of 0; a save state loaded at frame 3,000 the same from 3,001 on; each
  Board choice in its variable; the launcher's three pages.
- **Super Turbo is as it was**: its ladder (40,000 frames) and a widescreen training session with every aid (6,000
  frames) give the same hash logs as the build from before the change; its rewrites check passes; its feature list and
  song list are the same.

## Next

- Nobody has played it by hand yet.
