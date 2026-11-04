# XMVSF Stage 87.139 — F10 diagnostic TEST

**Purpose:** Reproduce the unexpected ordinary tag-in (Ken entering from the wrong side, corner kick without expected horizontal travel, and half-tile color artifacts in Double Feature) while tracing **actual ROM behavior without modifying it**.

## Root cause isolated in 87.138

The old root `RUN_TEST.bat` automatically set `XM87138_TAG_LAND_TEST=1`. That enabled `XM87138Tag::state().frame` on **any native point/reserve identity swap**, including normal tags. Without an active-super/DHC-command gate, the hook rewrote the incoming fighter's X and facing bytes. The Stage 5 painter also applied an experimental darkening overlay while that modified incoming actor was airborne. Therefore the observed wrong-corner movement is consistent with the TEST prototype interfering with native tags; **full DHC super dispatch was not connected at all**. The unusual DF tile colors still need comparison against unmodified actor/OAM captures.

Stage 87.139 removes BOTH gameplay-affecting hooks from `cps_run.cpp` and the Stage 5 painter. The 87.138 prototype header remains archived in source, but **is no longer included or called** by native gameplay. The test launcher also explicitly sets `XM87138_TAG_LAND_TEST=0`.

## Build

Use GitHub Desktop on `feature/dhc-mvc2-stage87`, **Fetch origin → Pull origin**, then use only the original root launchers:

```bat
F:\xmvsf\BUILD_TEST.bat
F:\xmvsf\RUN_TEST.bat
```

The build uses the live `dev/stable/buildtree` with the warm object cache; it compiles the updated `cps_run.cpp`, `run.cpp`, `inp_sdl2.cpp`, and `vid_sdl2.cpp` and publishes **`runtime/stable/XMVSF_Test_87_139.exe`**. It refuses to start 87.138, 87.132.4 or Stable if the current binary is absent. Stable and existing binaries remain untouched.

## Recorder controls / status

**F10** while the game is active starts a numbered recording. Press **F10 again** to stop and flush it. No automatic capture files are written before F10 is pressed.

Bottom-right overlay: **PROBE OFF F10 START** (grey), **PROBE ON [frame count] F10 STOP** (green), or **PROBE ERROR F10 RETRY** (red). The label is drawn after the entire native/DF/Wily composition; it cannot change sprite palettes.

**F9** retains the original full-state input movie, and **Shift+F10** retains the original quick-load action.

## Files to send

Under `F:\xmvsf\test\87.139\`, a capture produces two files with matching session numbers:

- `xm87139_probe_0001.tsv` — one native record for **each of the four fighter slots on every combat frame**, with both CPS-2 player attack bits, three raw controller ports, mapped direction/held/pressed readings, stocks, fighter/partner IDs, action and mode, animation, X/Y, facing, horizontal/vertical velocity, hitbox IDs, and native script addresses. An `IDENTITY` event and console `TAG_OBSERVED` flag mark genuine ROM-side role switches.
- `xm87139_oam_0001.tsv` — bounded raw CPS-2 sprite-object words (priority, palette bank, tile ID, raw screen position, width/height, attributes) during up to 90 frames after a native tag swap. This is sampled **before** Double Feature recoloring and does not change the renderer.

The recorder reads actual packed CPS-2 inputs even when the SDL mapped-controller history is empty (for example, keyboard paths). Raw CPS-2 values plus delivered mapped inputs distinguish controller routing from native action response.

## Reproduction

1. Enter Training or Versus with Ken and another fighter, preferably on the same Stage 5 background where the anomaly occurred.
2. Press F10. Confirm the lower-right status turns **PROBE ON**.
3. Perform a **normal tag** with no super. Repeat near a corner, then do regular moves and a genuine super for comparison.
4. Press F10 to stop. Send the matching **two TSVs**.
5. If the half-tile colors recur, include a screenshot and state whether Double Feature was enabled. Match the visual frame to the native/OAM trace.

Do not use the old Stage 87.138 placement test to collect this data—it was the source of normal-tag interference.

**Validation scope:** Isolated C++11 tests and launcher checks run in GitHub Actions; the full Windows/MSYS2 compile, SDL rendering, and game runtime still require user testing. This is a read-only probe, **not a playable DHC**.
