# Dimahoo Recomp

> Part of **[CPS-2 Recomp](../../README.md)**: one download and one launcher for every Capcom arcade game this
> project has ported. This page is about Dimahoo; the [main page](../../README.md) says how to download and play.

An unofficial native PC port of the arcade game *Dimahoo* (*Great Mahou Daisakusen* in Japan; Eighting/Raizing,
published by Capcom, 2000, CPS-2 board) for macOS, Linux (Steam Deck included) and Windows, made by static
recompilation: a vertical shoot-'em-up for one or two players, with a training mode, a score attack, a caravan, a boss
rush, free play, the board's own settings and cheats.

> **Beta.** The game plays through its six stages to the ending in made-up play and its code is checked instruction by
> instruction, but nobody has played it by hand yet, and its sound has not been listened to. Expect rough edges, and
> please [report what you find](#feedback-and-contributing).

**No game files are included.** You need your own copy of the game: the arcade ROM set `dimahoou` (USA, 000121), from
a board you own, or the European or Japanese set of the same date. This project cannot help you get one.

This project is not affiliated with, endorsed by, or sponsored by Capcom, Eighting or Raizing. *Dimahoo* is a
trademark of Capcom.

![The title](../../docs/screenshots/dimahoo/title.png)

![Training mode with the ship's true hit box, the points of the enemies' shots and the enemies' bodies](../../docs/screenshots/dimahoo/practice.png)

## What it is

The game's 68000 program is translated ahead of time into C++ and compiled into the port, and so is its sound
program. What the program expects of the board is code written for this project, not an emulator: the CPS-2's video,
its inputs, its settings memory and QSound.

- **One or two players at once**, each with a keyboard layout or a gamepad of their own, and **online** against
  another player (rollback, as the other games). The buttons are what the game has: **Shot**, **Bomb** and
  **Auto shot** (the shot held down).
- **Upright**: the game was made for a monitor stood on end, and its picture is shown stood up in a tall window
  (3:4), as the cabinet showed it; or on its side, for a screen turned on its side (the Display page's Picture turned).
- An in-game menu (Esc or `F1`; on a gamepad, the left stick's click or Start and Select together) with save states,
  speed (1/8 to 8x: slow motion), pause and frame advance, rapid fire, run-ahead, the slowdown (arcade perfect or none),
  the cheats and every feature.
- **Pure** plays exactly as the board does. **Enhanced** (the default) adds free play (the board's own). Every other
  feature is a switch of its own.
- **Training mode**: any of the six stages, from its start or straight at its boss, with the character, ships and bombs
  you choose. One button (Backspace, or a gamepad's left shoulder) puts it back as it began, at once; another replays
  your last attempt. Its aids: a readout of the magic gauge (the items toward the next magic level), and a summary when
  a boss comes and at each stage's end (the time, the points scored, ships lost, bombs used). The cheats work in
  training mode too. [All about training mode →](../../docs/PRACTICE.md)
- **Hit boxes**, on the Cheats page and in every mode, the board's own game too: each ship's true hit box (6 by 10
  pixels, far smaller than the character), the enemies' bodies and the point of every enemy shot, each a switch of its
  own, drawn with the objects as the board shows them.
- **Alternative modes**, at the head of the in-game menu:
  - **Score attack**: one player and one credit from the first stage, no continue and no cheats; the best score is
    kept.
  - **Caravan**: a score attack against the clock: two, three or five minutes from the first stage, a ship for every one
    lost; the best score in the time is kept for each length.
  - **Boss rush**: every boss in turn, the way between them run unseen; the best score is kept, and the time.
  - **Stage viewer**: a stage scrolling by with nobody on it: no ships, enemies, shots or panel. Its speed is yours to
    choose; up or right scrolls it faster, down or left back the way it came, and the shot button stops it.
  - **Attract mode**: the game's own show on a loop, with its sound on: all of it, or only the demonstrations.
- **Start at stage**: a new game starts at the beginning of any of the six stages.
- **Custom soundtrack**: your own music files in place of the board's music ([how](../../docs/SOUNDTRACK.md)); the
  game names its pieces in `songs.txt`.
- **Cheats**: infinite ships, infinite bombs, and invincibility for each player.
- The board's operator settings on the Board page (difficulty, ships, bombs, extra ships, one loop or two, continue,
  demo sound), each as the board is set unless you choose; or the game's own test menu from the in-game menu.
- **Slowdown** (the Game page): arcade perfect, the default: the processor waits for its memory and the video chip as
  on the board, so the game slows where and as much as the board does ([docs/TIMING.md](../../docs/TIMING.md)); or none:
  the processor overclocked, and the game never slows.
- **Straight to the title**: at a start or a reset, the board's start, its warning, the logos and the story run unseen.
- Your ROM set is only ever read. Settings, save states and the board's settings memory go in your own user folder,
  or beside the launcher in [portable mode](../../docs/LAUNCHER.md#portable-mode).

## What you need

- The ROM set `dimahoou` (*Dimahoo*, USA 000121): `dimahoou.zip` (with `dimahoo.zip` beside it if your collection
  keeps the shared graphics and sound there) or a folder with the set's files, the key file `dimahoou.key` among them.
  The European set (`dimahoo`) and the Japanese *Great Mahou Daisakusen* (`gmahou`) of the same date play too, as the
  board plays in that region. The launcher checks every file by checksum and tells you which is missing or different.
- One of: macOS 13 or later (Apple silicon or Intel); Linux x86_64 or arm64, or SteamOS; Windows 10 or 11, 64-bit.
- For two players: two gamepads, or keys chosen for the second player on the Controllers page.

## Playing

Dimahoo comes with [CPS-2 Recomp](../../README.md#getting-started): open its launcher and show it your ROM set (drop
`dimahoou.zip` or its folder on the window, or choose the folder you keep your sets in), then press **Play** beside
Dimahoo. From a terminal, `cps2recomp-launcher --play dimahoo` starts it straight away; the game's own program
(`dimahoo`, beside the launcher) runs by itself too: `dimahoo --help` lists what it takes.

## Building from source

With your own ROM set in `rom/dimahoo/` (ignored by Git), from the repository's top:

```sh
make GAME=dimahoo            # twice the first time
make GAME=dimahoo explore    # finds more of the game's code by made-up play, if you want it
make GAME=dimahoo run
make GAME=dimahoo check
```

[PROGRESS.md](PROGRESS.md) has what was found of the game; [docs/BUILDING.md](../../docs/BUILDING.md) has every
platform.

## Feedback and contributing

Bug reports and pull requests are welcome: see [CONTRIBUTING.md](../../CONTRIBUTING.md) for the rules (above all:
nothing from the game, ever).

## License and credits

GPL-3.0-only (see [LICENSE](../../LICENSE)); [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md) lists what comes
from other projects. The repository holds no game data and no key: the names header's addresses are numbers.
