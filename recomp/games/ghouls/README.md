# Ghouls'n Ghosts Recomp

> Part of **[CPS-2 Recomp](../../README.md)**: one download and one launcher for every Capcom arcade game this
> project has ported. This page is about Ghouls'n Ghosts; the [main page](../../README.md) says how to download and
> play.

An unofficial native PC port of the arcade game *Ghouls'n Ghosts* (Capcom, 1988, CPS-1 board; *Daimakaimura* in
Japan) for macOS, Linux (Steam Deck included) and Windows, made by static recompilation.

> **Initial classic CPS-1 port, with sound.** The game's picture, controls and arcade music and effects run on the
> toolkit's shared classic CPS-1 board. Board options and cheats are declared in `game.cfg`; there is no training
> mode yet. The YM2151 uses [ymfm](https://github.com/aaronsgiles/ymfm), the BSD-3-Clause library listed in
> [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md). The OKI MSM6295 is implemented in `sound/okim6295.cpp`.

**No game files are included.** You need your own copy of the game: the arcade ROM set `ghouls` (World), from a board
you own. This project cannot help you get one.

This project is not affiliated with, endorsed by, or sponsored by Capcom. *Ghouls'n Ghosts* is a trademark of Capcom
Co., Ltd.

## What you need

- The ROM set `ghouls` (*Ghouls'n Ghosts*, World): `ghouls.zip` or a folder with its files (a merged zip, with
  `ghoulsu`, `daimakai` and `daimakair` in it, works too: files are found by checksum). Other regions and revisions
  are refused.
- One of: macOS 13 or later; Linux x86_64 or SteamOS; Windows 10 or 11, 64-bit.

## Building from source

With your own ROM set in `rom/ghouls/` (ignored by Git), from the repository's top:

```sh
make GAME=ghouls            # twice the first time
make GAME=ghouls explore    # once: finds the game's code by made-up play and translates it
make GAME=ghouls run
```

## Progress

**Where it stands.** On the classic CPS-1 board of [Street Fighter II](../sf2ce/README.md), with four things a board
of 1988 needed and the toolkit had not:

- **The 68000 at 10 MHz** (`cpu_clock` in game.cfg): 640 cycles a line instead of 768, as MAME's `cps1_10MHz`, so the
  game slows down where the board does.
- **Its program in pairs of files**, one with the even bytes and one with the odd (`a+b`), and a file of words high
  byte first (`/be`).
- **Its graphics in groups of eight files** of a byte each (eight joined with `+`), after four files of two bytes.
- **The graphics mapper on the objects too.** The PAL DM620 keeps objects 2000-3FFF in its third bank and scroll 3's
  tiles from 8000 in its second (`gfx_map`); the objects had not gone through the mapper, which Street Fighter II's
  S9263B did not need.

The CPS-B-01 has the same registers as Street Fighter II's CPS-B-21, with no multiplier. The board has no OKI ROMs,
so there are no voices: the YM2151 plays the music and the effects. The DIP switches are as the boards left the
factory (`dip` in game.cfg): 3 lives, difficulty 2, a bonus life at 30K, 60K and every 70K, continues allowed.

**The Board page** (the in-game menu's, F1 or Esc, and the launcher's): the DIP switches by name, from game.cfg's
`board_option_N` lines: difficulty (1 to 8), lives (3 to 6), the bonus life, continue and demo sound, each "As the
board is set" unless chosen. The board reads them only as it starts, so a change takes effect after Reset (the menu's
Reset, or the next time the game opens).

**The Cheats page** (the menu's and the launcher's), from game.cfg's `cheat_N` lines: writes to the work RAM before
every frame while a cheat is on, only while a game is played. Infinite lives, infinite time, the armour (steel or gold,
kept through hits: enemies cannot kill Arthur, a fall still does) and the weapon (lance, dagger, torch, axe, discus).
Each can be switched any time; off, the game goes on as it was. The addresses were found here with RAM dumps, traces and
pokes from a save state; game.cfg says how.

**Next.** Features of its own.

## Reproducible checks

`movies/smoke_play.txt` supplies 40,000 frames of input from reset. It exercises coin/start and gameplay;
it is a smoke route, not a completed game. Ghouls finishes its board tests later than Street Fighter II;
its first coin/start during those tests is ignored and the next pair starts play.

```sh
make GAME=ghouls check FRAMES=40000 MOVIE=games/ghouls/movies/smoke_play.txt
make GAME=ghouls check-rollback FRAMES=12000
make GAME=ghouls check-online FRAMES=6000
```

The online check uses two local peers and simulated packet delay/loss; it does not test Internet play.
The classic sound program runs on the Z80. `check-sound`'s QSound high-level-driver comparison is not applicable.
Windows is the tested platform for this contribution; macOS/Linux builds still need verification.
