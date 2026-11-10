# Final Destination — TEST 87.274

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
   texture. Full image displayed within the native viewport using
   uniform source aspect ratio, centered horizontally.
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
