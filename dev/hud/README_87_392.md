# TEST 87.398 — 2-second song primer, invisible HUD startup restored

Branch: `feature/hud-shock-reveal`. Only Game Version 4. No Stable overwrite.

## User feedback and root cause

TEST 87.397 placed the white/blue primer closer to the desired replay mark.
However, the HP/stun/HUD bars could be **already fully visible when the
fight screen first appeared**. This was not a missing alpha mask: the clock
was started correctly by the first actual WASAPI
`versusscreen.ogg` PCM packet, but the 180-frame reveal delay elapsed
during earlier Versus-screen phases. For safety after the 87.395 tree leak,
source HUD masking is correctly disabled during phases **02/04**.
That allowed the 30-frame reveal to finish before any valid fight HUD
object could be concealed.

## Changes

- Start time **unchanged**: the first actual Versus-song audio PCM event.
- Delay reduced by **one second**, from 180 to **120 emulated frames**.
- Dither/shock remains 30 game frames, white-hot side filaments through
  source-HUD ink followed by blue and the exact original artwork.
- New **fight-HUD first-frame guard**: when native status first becomes
  **06/08** for a song-triggered match, remember that frame. The first six
  fight-HUD frames are fully hidden. If the song's 120-frame delay has
  *not* finished, the song clock remains authoritative. If it already
  finished while the VS screen was showing, reveal immediately after
  those six invisible combat-screen frames rather than popping in
  already fully visible.
- Native source-tile filtering remains strictly OFF in phase 02/04;
  this preserves the forest/tree transition fix. No full-screen masks,
  extra graphics over the VS song, or ROM writes.
- The SDL stun bar follows the same computed timeline.

### Clock rules

`firstRevealFrame = max(versusSongFrame + 120, firstFightHudFrame + 6)`

If no fight HUD is present yet, the timeline holds at invisible but only
real phase-06/08 HUD pixels may be suppressed, so VS transition graphics
stay unmodified.

The local regression `dev/hud/RUN_TEST_HUD_392.bat` checks both cases:
when the song delay expires before the fight HUD appears, and when the
fight HUD appears before the audio delay expires. It also tests that
phase 02/04 never participate in native masking and that the first
primer shine remains independent of the Bayer dither.

## Verification

Use `BUILD_TEST.bat` and `RUN_TEST.bat`, targeting **TEST 87.398**.
Look for `XM87.398 HUD_SHOCK_SONG_START` and
`XM87.398 HUD_SHOCK_FIRST_FIGHT_HUD` in the local diagnostics to
see the actual offset. Verify one complete hidden startup, the primer
one second sooner, and no tree pixels during the Versus transition.

Not compiled or visually accepted on Windows in this environment.
