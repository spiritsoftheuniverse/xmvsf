# Night Warriors Recomp

> Part of **[CPS-2 Recomp](../../README.md)**: one download and one launcher for every CPS-2 game this
> project has ported. This page is about Night Warriors; the [main page](../../README.md) says how to
> download and play.

An unofficial native PC port of the arcade game *Night Warriors: Darkstalkers' Revenge* (Capcom, 1995, CPS-2 board)
for macOS, Linux (Steam Deck included) and Windows, made by static recompilation, with a full training mode.

Night Warriors is the second Darkstalkers game, two years before Vampire Savior: Donovan, Hsien-Ko and the bosses
Huitzil and Pyron join the first game's cast. Its program is its own, not Vampire Savior's, so this port has sources of
its own ([how it works](PROGRESS.md)).

> **Beta.** The game plays and its code is checked instruction by instruction, but nobody has played it by hand
> yet. Expect rough edges, and please [report what you find](#feedback-and-contributing).

**No game files are included.** You need your own copy of the game: the arcade ROM set `nwarr` (Europe 950316, the
set FBNeo's training mode plays) with its key file, from a board you own. This project cannot help you get one.

This project is not affiliated with, endorsed by, or sponsored by Capcom. *Night Warriors* and *Darkstalkers* are
trademarks of Capcom Co., Ltd.

![The title screen](../../docs/screenshots/nwarr/title.png)

![Lord Raptor and Donovan on Lord Raptor's stage, in widescreen](../../docs/screenshots/nwarr/fight.png)

## What it is

The game's 68000 program is translated ahead of time into C++ and compiled into the port; what the program expects of
the board is code written for this project, not an emulator.

- Plays with a keyboard, a gamepad or an arcade stick; every key and button can be changed, for each player, in the
  launcher. In the collection it can have a layout of its own, apart from the other games'.
- An in-game menu (Esc or `F1`; on a gamepad, the left stick's click or Start and Select together) with the training
  mode and the other modes, save states, speed (1/8 to 8x), the cheats, every feature, and a Reset all options row.
- **Zoom** on either fighter or both, as in the other fighting games ([the main page](../../README.md) has the buttons).
- **Pure** plays exactly as the board does. **Enhanced** (the default) adds free play (the board has none of its own,
  so the port keeps it in credits) and your own character colours. Every other feature is a switch of its own.
- The board's operator settings (difficulty, damage level, time count speed, turbo, max round, mode select, join-in,
  event, continue, demo sound) on the Board page, each as the board is set unless you choose; or the game's own test
  menu from the in-game menu.
- **A custom soundtrack**: your own music in place of the board's, with the board's sound effects and voices; the
  `songs.txt` the game writes names its pieces (each stage's, the select and versus screens, the ending).
  [How →](../../docs/SOUNDTRACK.md)
- **Hidden characters** (Game page): Huitzil and Pyron, the bosses, to play: hold Start as you choose Victor for
  Huitzil, or Demitri for Pyron.
- Your ROM set is only ever read. Settings, save states and the board's settings memory go in your own user folder, or
  beside the launcher in [portable mode](../../docs/LAUNCHER.md#portable-mode).

## Training mode

"Alternative modes" at the top of the in-game menu has a Training mode page: Start goes straight to the character
choice with two players (player 1 chooses for both: speed, character, then normal or automatic guard) and into a
fight with no clock that nobody loses.

| Feature | What it does |
| --- | --- |
| Hit boxes | The boxes the game itself tests: where a player can be hit (blue), attacks (red; orange for things thrown) |
| Frame data and frame meter | Startup, active and recovery of each move and the advantage after it, counted in the game's own steps; a row of coloured cells for each player, one a step |
| Input display | What each player presses and for how long, the stick as a keypad number |
| Combo readout | Hits, damage, the round's best |
| Health refills | A player's health comes back when they have not been hit for a moment |
| The dummy | Stands, crouches, jumps, guards; records and replays, and can answer with a recording |
| Start over | The round as it began, in the middle or a corner, sides swapped or not |
| Stage | Any of the fourteen stages, or the game's own choice |

Also: save states, free play, and the cheats (no damage for either player, one-hit knockouts, infinite time).

## Alternative modes

| Mode | What it does |
| --- | --- |
| Training mode | A fight with no clock that nobody loses, and the aids above |
| Attract mode | The game's own show on a loop: everything, the introduction only, or one fight after another |
| Stage viewer | Any of the fourteen stages with nobody on it and no panel, its music playing |
| Model viewer | One character by itself, on a stage or on plain white or black, at any size, standing and moving with your controls or going through any of its animations |
| Survival | One opponent after another on the health you have left |
| Endless battle | Two fighters the game plays, for as long as you like; touch the controls to take one over |
| Ring out | No health: knock your opponent over the end of the stage, then jump down to the next |
| Colour editor | Change a character's colours while you watch: custom colours, below |

## Widescreen

A switch on the launcher's first page, under "Full screen" (and on the in-game menu's Display page). Fights are shown
and played in 16:9: the picture is 512 columns wide where the board's is 384, so there is a third more of the stage in
view. Off unless you switch it on, because it changes how the game plays (the players can stand further apart; the
camera stops 64 short of where it did). The bars and the clock stay in the middle.

**Invisible walls** (Display page) keeps the 16:9 picture and plays the game exactly as the board does.

## Custom colours

Give any character colours of your own, with the colour editor (the character in front of you, its eight colour sets,
16 levels each of red, green and blue) or with text files, one a character, in the `palettes` folder of the game's
user folder. To play in them, choose the character with the button of the set you changed. A `vphe.09b` changed with
PalMod is read too (not yet tried with PalMod itself).

## What you need

- The ROM set `nwarr` (*Night Warriors: Darkstalkers' Revenge*, Europe 950316): `nwarr.zip` or a folder with the set's
  files, including `nwarr.key`. The launcher checks every file by checksum and tells you which is missing or
  different.
- One of: macOS 13 or later (Apple silicon or Intel); Linux x86_64 or SteamOS; Windows 10 or 11, 64-bit.
- A gamepad or stick with six buttons is recommended.

## Playing

Night Warriors comes with [CPS-2 Recomp](../../README.md#getting-started): open its launcher and show it your ROM set
(drop `nwarr.zip` or its folder on the window, or choose the folder you keep your sets in), then press **Play** beside
Night Warriors. From a terminal, `cps2recomp-launcher --play nwarr` starts it straight away; the game's own program
(`nwarr`, beside the launcher) runs by itself too: `nwarr --help` lists what it takes.

## Building from source

With your own ROM set in `rom/nwarr/` (ignored by Git), from the repository's top:

```sh
make GAME=nwarr            # twice the first time
make GAME=nwarr explore    # once: finds the game's code by made-up play and translates it
make GAME=nwarr run
make GAME=nwarr check-widescreen
```

[PROGRESS.md](PROGRESS.md) has what was found and how; [docs/BUILDING.md](../../docs/BUILDING.md) every platform.

## Feedback and contributing

Bug reports and pull requests are welcome: see [CONTRIBUTING.md](../../CONTRIBUTING.md) for the rules (above all:
nothing from the game, ever).

## License and credits

GPL-3.0-only (see [LICENSE](../../LICENSE)); [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md) lists what comes
from other projects. The repository holds no game data.
