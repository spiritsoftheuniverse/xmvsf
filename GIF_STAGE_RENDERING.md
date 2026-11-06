# XMVSF animated-GIF stage rendering contract

**All future GIF-based levels use the Rock Castle rendering pipeline.** This
is a standing implementation rule, not a one-off Rock Castle shortcut.
The source of truth is the full animated GIF under `_art/`, decoded at
runtime. Preserve its original frames, colors, transparency and frame delays;
do not flatten it into an arbitrary static screenshot, rip separate tiles,
recreate motion by hand, or reuse the unrelated Wily Factory sprite manifest.

## Pipeline: one image, one registered camera

1. **Host stage choice:** the game menu resolves the GIF stage explicitly
   before starting a match. Preserve the ordinary random/native stage
   selection and existing Wily Factory path.
2. **Native world donor:** the established Stage86.48 selector chooses native
   CPS2 stage 05 for camera movement, combat geometry, actor positions,
   hazards and effects. Never write the observed stage byte `FF5113`.
3. **Camera:** observe `s16(FF524C)+64` horizontally and `s16(FF5250)`
   vertically, conditional on the verified native actor pointer / stage
   phase. Do **not** derive camera position from fighter coordinates or
   elapsed time.
4. **Single world-image transform:** use the verified Wily registration,
   `worldX = -64 + round(nativeX * 381 / 384)` for native X in [0,384].
   The matching Y camera is native Y in [-14,256]. A 384x224 native view
   travels 381 horizontal and 270 vertical world pixels, hence the
   source GIF represents a virtual **765x494** world footprint.
   Calculate **one uniform source-pixels-per-world-pixel scale**:
   `s = min(gifWidth / 765, gifHeight / 494)`.
   A frame crops `(384*s) x (224*s)` source pixels. Its top-left corner
   is the centered world footprint plus
   `((worldX+64)*s, (clamp(nativeY,-14,256)+14)*s)`.
   Clamp after rounding. This keeps all GIF frames registered against
   fighter coordinates with **both axes of the real camera**, with
   no frame-dependent stretch and no independent aspect-ratio panning.
   Source resolution may vary; all animation frames share one camera.
5. **Animation:** SDL2_image decodes the original animated GIF once.
   Track its supplied per-frame delays using the CPS2 60 Hz game-frame
   clock. Never upload or recreate every animation frame when the
   GIF's frame index has not changed. Do not let wall-clock time desync
   F9 recording, pause, or high-speed replay.
6. **Composition order:** image first; independent dizzy HUD underlay next;
   then the existing two-fill CPS2 OBJ alpha extraction (fighters, native
   super/hit effects); then native white/colored flash. Keep the original
   emulated framebuffer pristine. Do not paint the original forest over
   the custom GIF.
7. **Double Feature (DF):** the camera transform and all scene dimensions
   are **identical** in normal and DF modes. DF applies its separate
   background brightness grading (77/255) to the GIF texture, not the
   native fighter/HUD/FX layers. The stage's original-color artwork is
   restored when DF is off. Do not revive a full-screen GPU readback
   as part of GIF rendering. DF character select instead grades a native-
   size CPU-buffer copy before the SDL texture upload; no GPU readback,
   no change to emulation speed or input processing.
8. **Lifecycle and failure:** release decoded GIF/SDL textures on native
   video exit or renderer/session change. If the source asset is missing
   or fails to load, show the original native stage rather than a blank
   screen. A TEST build failure never overwrites Stable or purges the
   warm compiler object cache.

### Implementation map

- `dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_201_gif_camera.h`:
  pure fixed-point X/Y registration shared by GIF stages.
- `dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_200_rock_castle.h`:
  GIF decoding, frame clock, texture and native overlay composition.
- `dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage86_49_painter.h`:
  route selected custom stage to its painter without changing Wily.
- `dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_49_camera.h`:
  read-only native stage05 camera.
- `dev/stable/buildtree/src/burn/drv/capcom/cps_draw.cpp`:
  native sprite extraction and native palette/FX separation.
- `dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_201_select_fast.h`:
  DF character-select fast path, **not** a stage-specific hack.

### Diagnostics and verification

Verify camera left/center/right, vertical extremes, fighter alignment
during movement and jump, DF on/off mid-session, GIF frame transitions,
pause and replay, return-to-menu cleanup, original Wily stage unchanged,
and the non-DF character-select speed. The
`XM87.201 GIF_CAMERA` and `XM87.201 DF_SELECT_FAST` log markers
document the registered crop and native-resolution selection path.
A successful isolated unit test is **not** proof of Windows native
rendering; record actual Windows build and in-game observations.
