# TEST 87.397 — Faster audio-anchored HUD shock primer

Branch: `feature/hud-shock-reveal`. Version 4 only; Version 3 unchanged.

## Evidence in 87.396.zip

The file includes F9 capture records and a separate live 68000 diagnostic
trace. The latest F9 session is `replays/session_0002`, whose manifest
contains a 127-frame recording beginning at a session checkpoint.
That F9 manifest does not provide a synchronized first-audible VS song
sample timestamp, and the M68K and F9 clocks should not be conflated.

The 68000 trace `xm87139_m68k_0001.tsv` shows the first native
chooser-to-match setup change (`FF5104 WORD 0004 -> 0008`) at
**frame 2126**. The combat probe `xm87139_probe_0001.tsv` first
captures a native phase-08 combat frame at **2305**. Their separation is
**179 emulated frames**, approximately three seconds.

The previous 210-frame (3.5 s) delay begins on the first actual
`versusscreen.ogg` audio PCM packet; while the audio packet may follow
the chooser handoff by a few frames, the 210-frame hold can miss this
observed pre-fight window and makes the materialization feel late.

## TEST 87.397 behavior

- **Primer clock starts on the actual Versus track playback event**,
  not on ROM phases 02/06/08, nor on the user's F9 input.
- **Delay reduced to 180 emulated frames (3.0 s)**, a 30-frame shift
  earlier than 87.396.
- On the **first reveal frame** the two white-hot shine filaments
  originate *within* the left/right portrait HUD artwork at x~23 and
  x~360. They immediately light source HUD ink, including pixels
  not yet selected by the Bayer threshold, creating a visible primer.
- The existing 30-game-frame ordered dither and white-to-blue-to-native
  shock sweep remain otherwise unchanged.
- Phase 02/04 selector/Versus artwork is NEVER masked. Native HUD
  pixel manipulation remains restricted to phase 06/08. The 87.395
  exposed-forest regression remains fixed.
- No added recorder controls, visual scene masks, gameplay edits,
  sound-device restarts, GitHub Actions, or Stable overwrites.

## Local verification

Run `BUILD_TEST.bat` and `RUN_TEST.bat`: both target **87.397**.
The standalone regression runner
`dev/hud/RUN_TEST_HUD_392.bat` retains its historical filename,
but now checks the new **180+30** schedule and first-frame glint.

Inspect `XM87.397 HUD_SHOCK_SONG_START` alongside the native
`VERSUS_OGG_PLAY` diagnostic, then confirm the first visible
white primer occurs around the chosen match-screen timing, and
that the forest/Versus transition never exposes background.
A later adjustment may be required if the F9 reference was in a
different game-session clock domain.

Source committed; a Windows native build and visual acceptance are
still required locally.
