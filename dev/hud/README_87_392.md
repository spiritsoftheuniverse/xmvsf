# TEST 87.396 — Audio-anchored grouped HUD shock (forest-safe)

Branch: `feature/hud-shock-reveal`. Version 4 only, Version 3 unchanged.

## Evidence and anchor

In the supplied `87.395.zip`, the F9 movie starts at a native checkpoint
around frame **290** (`replays/session_0001/manifest.tsv`) and records 383
frames. This marks the requested transition area, but is not an authoritative
audio-playback timestamp. The native audio engine owns that timestamp.

`xm20_vs_transition.h` calls `xmvsfHost::enterVs()` at the already verified
68000 chooser-to-Versus handoff. The WASAPI mixer later sets
`m.versusPlaying=true` and `m.versus.pos=0` at its first actual
`audio/music/versusscreen.ogg` PCM output. TEST 87.396 now increments a
thread-safe playback generation **at exactly that audio mixer event** and
samples the generation on the next emulated game frame.

That one event arms the 210-emulated-frame hold (3.5 seconds at 60 fps).
Then a 30-emulated-frame Bayer-dither reveal animates:

- The native CPS2 timer, portraits, health bars and status decorations.
- The independently painted stun bars.
- A separate white-hot specular shock sweeping in from both sides with an
  electric-blue trailing glow; white filaments can light real HUD artwork
  in Bayer gaps without ever painting over transparent scenery pixels.

The reveal ends in the original colors. Phase 02, 04, 06, and 08 NEVER arm,
restart, or otherwise shift this clock. No new M68K heuristic is introduced.

## Forest/transition fix

TEST 87.395 applied a per-tile suppression/restoration pass to native
priority-7 objects **even during ROM status 02/04**. Those statuses still
render the selection/Versus handoff atop native forest scenery, so suppressing
overlay pixels prematurely revealed forest/trees at the transition.

TEST 87.396 starts the **clock** from the audio event but restricts actual
HUD pixel suppression/reveal to the fighting-scene statuses **06 and 08**
alone. The Versus scene and its backdrop are never modified. Per-tile native
depth and background restoration remain unchanged within the valid HUD pass.

No modifications to audio mixing, stage selection, CPS ROM/RAM, gameplay
rules, controller input, Stable binaries, or GitHub Actions.

## Local-only checks

Build with existing `BUILD_TEST.bat` and launch `RUN_TEST.bat`, both now
targeting **87.396**. The local standalone `dev/hud/RUN_TEST_HUD_392.bat`
remains under its historical filename but now tests the 87.396 clock and
scene mask, using `dev/hud/tests/test_hud_shock_392.cpp`.

Verify the local logs contain matching:

- `XM87.396 VERSUS_OGG_PLAY audible_start=N`
- `XM87.396 HUD_SHOCK_SONG_START song=N frame=...`
- `XM87.396 HUD_SHOCK_OBJECT phase=06/08 ...`

Look for no forest/tree leakage on the VS screen and the HUD's reveal
3.5 seconds after `versusscreen.ogg` first actually plays.

**Not yet compiled or visually accepted on the user's Windows runtime.**
