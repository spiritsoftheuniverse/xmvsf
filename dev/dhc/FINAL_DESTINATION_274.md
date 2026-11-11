# Final Destination — TEST 87.274–87.275

Branch: `final-destination` created from main commit
`14889e013ca0b2dadaefc31eecfc110d1e2407e6`, which already contains
`_art/finaldestination.png` (source image 600 × 368 RGBA).

## In-game selection

The shared Training/Versus stage picker includes:
RANDOM, WILY FACTORY, FINAL DESTINATION.
The random pool preserves Wily and includes Final Destination as one
additional custom-stage option. The native game decides all other stages.

The two custom stages use the previously established stage-05 native
fighter/camera path. Final Destination changes *scenery only*, leaving
fighters, FX, physics, character positions, training relocator, and DHC
untouched. The original Wily picker and scene painter remain independent.

## Scene composition (back to front)

1. Procedural native-resolution 384×224, 32-bit RGBA32 space.
   Deterministically seeded stars in three camera-relative parallax
   depths, blue-biased and red-biased twinkling with varied periods.
   Star pixels are generated directly in 32-bit RGBA memory and uploaded
   to a reused SDL texture; no static space artwork required.
2. `_art/finaldestination.png`, loaded **without editing its RGBA source
   pixels or transparency** through SDL_image into a dedicated platform
   texture. The source 600x368 PNG is scaled proportionally into a 768px
   native world layer (not squeezed into a single 384px viewport).
   The native game camera pans across the world art with SDL clipping to
   the ordinary 384x224 viewport. The first substantial opaque source row
   is measured once and aligned to the native ground deck at Y=194.
   The picture's decorative underside extends naturally below the screen.
   Platform and all stage textures explicitly use nearest-neighbor
   sampling; none use SDL_ScaleModeLinear.
3. The preexisting native stun-bar underlay, then the extracted live
   native fighter/FX sprite layer, followed by native full-screen flashes.
4. Existing game UI, frame-data labels, and overlays (outside the stage
   painter) retain their normal order.

The asset locator checks the working directory and executable-relative
`_art` paths. If the image is missing, the code logs
`FINAL_DESTINATION_ASSET_MISSING` and leaves the original native stage
available, rather than displaying a broken partially initialized stage.
Procedural texture creation and native fighter texture upload failures
also fail to native presentation.

## Local TEST use

Pull `final-destination` and run the existing `BUILD_TEST.bat` then
`RUN_TEST.bat`, choosing **FINAL DESTINATION** in Training or Versus.
Build output: `runtime/stable/XMVSF_Test_87_274.exe`.
Persistent build log: `test/build_logs/BUILD_TEST.log`.
No new test suites, build gates, GitHub Actions, or full rebuilds were
introduced. Stable and previous TEST executables are preserved.

Native Windows compilation and gameplay have not yet been executed in
the connector environment. Source/picker/renderer integration has been
verified by inspection.

## TEST 87.275 — native world camera and pixel filtering

Replaced the 87.274 fullscreen platform projection with a 768-pixel
world-space foreground. The 384-pixel wide native camera moves from 0
to 384 and the platform destination shifts from 0 to -384 native
screen pixels. The actual supplied 600x368 artwork is now approximately
768x471 in native coordinates (1.28x source scale); at left/center/right
camera X 0/192/384 the center camera sees the middle of the level.
This is the same world/camera width as the existing stage-05 geometry,
so the platform is no longer crammed into one screen. Native fighter
sprites, HUD order, and procedural blue/red stars are preserved.

New image alignment is based on a one-time scan of the original PNG's
alpha mask near the center; the first opaque deck line is placed at
native screen Y 194. Original RGBA values are never edited.
Explicit SDL_ScaleModeNearest is set on platform, space, fighter and
flash textures; it is not left to renderer defaults. The existing
staged frame draw automatically clips outside the viewport.

The stage reports `XM87.275 FINAL_DESTINATION_READY ... world=768x...`
and `XM87.275 FINAL_DESTINATION_CAMERA cam=... dst=...` to confirm
camera movement and world scaling from the actual in-game render path.

Local build and run versions now use `XMVSF_Test_87_275.exe`; no
additional build checks, GitHub Actions, DHC changes, or Stable edits.
The branch/source change is committed and statically verified;
native gameplay/build verification is not claimed.
