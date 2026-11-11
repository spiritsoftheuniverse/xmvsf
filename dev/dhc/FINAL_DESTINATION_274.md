# Final Destination — TEST 87.274–87.280

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

## TEST 87.276 — bottom-up platform anchoring

The full unmodified PNG now anchors its **bottom source row** to the
bottom of the 384x224 native gameplay viewport, at Y=224, regardless of
image height. With the world image at width 768 and scaled proportional
height `worldH`, the placement is `worldTop=224-worldH`.
Only the *top* of over-tall artwork is clipped. This replaces the prior
deck-top alpha scanner and `DECK_SCREEN_Y=194`, which clipped the
platform underside on the bottom. It does not affect horizontal world
camera scrolling (0..384), nearest-neighbor filtering, native fighters,
starfield, other stages or combat.

The `FINAL_DESTINATION_CAMERA` diagnostic now prints the rendered
destination bottom coordinate as `bottom=`, allowing direct
confirmation against the viewport bottom. TEST local build/launcher is
87.276. No new build checks or GitHub Actions. Native play is untested.

**Asset state when changed:** main and final-destination currently
reported the identical original `_art/finaldestination.png` blob SHA
`1404ecedb43ba828ebd29b18da0bf9a7e842f04a`. The newly edited
PNG was not yet visible on GitHub in this check. The renderer uses
the PNG from the working tree at runtime, so the bottom-anchor applies
to any later asset dimensions without a code change.

## TEST 87.277 — native vertical camera, stationary world floor

**Required first read for all levels:** root `README.md` (native camera
contract), then `dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_49_camera.h`.
That existing observer reads the *ROM-owned camera* on native stage 05:
X = signed `0xFF524C` + 64, Y = signed `0xFF5250`. In the verified
super-jump range, camera Y travels from grounded ~256 toward 0.
This is the same camera state used by Wily's stage painter; no new
motion detector, guessed character midpoint, or separate stage camera
is introduced.

For the fixed Final Destination world floor, the vertical draw offset
is now:

```
platformWorldTop = 224 - scaledImageHeight + (256 - nativeCameraY)
```

At grounded Y=256, the **bottom source pixel is anchored at native Y=224**.
As the native camera rises through Y=128 to Y=0, the bottom of the world
platform correctly moves from 224 to 352 to 480 in screen coordinates
(out of the viewport), while the game sprites follow their original
native projection. When the camera descends on landing, the fixed
world floor returns to Y=224. This is the standard game-camera
behavior—not a floor glued to the jumping fighter.

The platform continues to use verified native horizontal camera X and
nearest-neighbor scaling. The procedural starfield already uses
native camera Y for slower parallax; it remains separate behind the
platform and gameplay sprites. `FINAL_DESTINATION_CAMERA` now reports
both native camera X and Y, viewport, draw coordinates and bottom
location when either camera axis changes. There are no changes to
combat code, collisions, DHC, menus, controller input or native
camera registers. Stage-specific camera geometry must be verified
before adapting this transform for other native stages.

TEST 87.277 is source-verified, not Windows gameplay-verified. Stable
and the warm local build cache remain untouched.

## TEST 87.278 — original pink rails masked and trapezoidal light sweeps

The stage now loads the supplied unmodified
`_art/finaldestination.png` and converts a *temporary copy* to
`SDL_PIXELFORMAT_RGBA32`. Pixel classification identifies bright pink
as source pixels where R and B are each at least 95 and at least
40 values above G, with antialias-weighted extraction. The exact
source-alpha portion classified as pink is cut out of that temporary
platform copy and retained in a separate light-mask list. The
repository PNG itself is never rewritten.

Seven path-direction references follow the visible narrow neon rails:
rising left upper bevel, falling right upper bevel, upper front
horizontal, left/right lower perspective edges, distant inner strip,
and lower front horizontal. Each pink pixel is attached to the nearest
rail in source-image-normalized coordinates and receives a
time-varying trapezoidal sweep along that rail's tangent. Leading
and trailing boundaries skew as a function of lateral distance
across the path to form a trapezoid rather than a straight rectangular
band. Lane clocks vary in period, phase and speed; low continuous
channel emission remains visible between bright moving streaks.

**Z-order:** 32-bit RGB starfield, platform with pink holes, masked
animated pink light, the existing native HUD underlay, native
fighter/FX sprites, native white/colored flashes, then game UI.
All platform and light textures use nearest-neighbor sampling.
The light and platform use the *same* 768px native world
rectangle calculated from the native horizontal and vertical
camera; effects cannot drift off the sloped rails when the camera
scrolls or the fighters super-jump. The effect advances by emulated
frame count, not window repaints. No camera RAM, fight physics, DHC,
roster, inputs or other stages were changed.

The opening logger `XM87.278 FD_PINK_MASK image=... pixels=...
lanes=7 effect=world_trapezoids filter=nearest` confirms a
nonzero number of masked source pixels, and a missing-asset error
preserves native stage fallback. This is **source-reviewed** but
not yet exercised in a real Windows game frame.

Local TEST scripts and in-game version label advance to
`87.278` without additional tests, build gates, GitHub Actions
or Stable modification.

## TEST 87.279 — source-color pink light and silent MP4 backdrop

Pink rails: the same seven source-color mask/path directions are retained.
Unlike 87.278, the *base platform keeps its original RGBA unchanged*,
including pink pixels; the separate light mask now uses
`SDL_BLENDMODE_ADD` with **0..30% of each source RGB channel**
at peak streak intensity. Outside a streak the additive contribution is
exactly zero. No replacement neon tint is synthesized. Travel changed
from **2–3 source pixels per emulated frame** to **one pixel every 5–6
emulated frames** without moving the rail mask or changing the
camera transform.

Background source files on the `final-destination` branch:

- `_art/video/fd01.mp4`
- `_art/video/fd02.mp4`
- `_art/video/fd03.mp4`

The game uses native Windows **Media Foundation** Source Reader to
decode original MP4s at their actual frame presentation timestamps
and media-reported durations. The library is resolved at runtime from
Windows `mfplat.dll` and `mfreadwrite.dll`, without adding a project
link dependency, external FFmpeg player, or altering the Makefile and
its warm-cache signature. MP4 audio is explicitly deselected; it cannot
interfere with the existing WASAPI battle audio. If a file, codec,
video format, or Windows MF runtime is unavailable, the log states the
precise failure and the existing starfield/platform stays playable.

Timeline uses the **first native status 08 match frame**, not the early
stage-05 loading frame, and only advances on real emulated frames
(pauses and replay clock naturally pause it):

- Match T+0..10s: no video, original opaque blue/red starfield
- At T+10s: play `fd01.mp4`, one-second fade-in, actual full clip
  duration, final one-second fade-out
- Then precisely **five seconds** without video
- Play `fd02.mp4` with identical fade and own recorded duration
- Five-second gap, then `fd03.mp4` and identical fades
- Five-second gap, then repeat `fd01` → `fd02` → `fd03`, with no
  second ten-second startup pause

Z-order: existing dark SDL clear → native-decoded MP4 frame (alpha
fade) → independent native-resolution parallax stars (slightly
transparent sky pixels only when video is present) → original-color
stage platform → 30%-max pink light mask → native HUD/fighters/FX →
existing game overlays. MP4 source is center-cover cropped without
aspect-ratio deformation. Star and platform pixel art stay
nearest-neighbor while video content uses smooth filtering.

Playback is sequential, uses one streaming decoder/texture at a
time rather than preloading the 60MB+ files, and limits media decoding
per game paint. Match/stage transitions reset the timeline; temporary
fighter sprite capture gaps do not. Video startup/metadata, missing
files, codec, decode and clip transitions are logged under
`XM87.279 FD_VIDEO_...`.

Local TEST launchers/build artifacts and menu version have been
advanced to `87.279`. No additional build checks or GitHub Actions,
no changes to Stable/DHC/combat, and no native Windows verification
has been performed in the connector environment.

## TEST 87.280 — actual GIF files, optimized incremental Windows playback

The user replaced the three MP4s with GIFs on the `final-destination`
branch. Confirmed source filenames and compressed sizes:

- `_art/video/fd01.gif`: 40,015,081 bytes
- `_art/video/fd02.gif`: 49,520,398 bytes
- `_art/video/fd03.gif`: 73,540,654 bytes

The `xmvsf_stage87_274_final_destination.h` painter now includes
`xmvsf_stage87_280_fd_gif.h` instead of the 87.279 Media Foundation
MP4 decoder header. The original MP4 source module is no longer
compiled into the renderer.

**Optimization:** On Windows, the new decoder uses the already
available built-in Windows Imaging Component (WIC), which supports
GIF89a natively, and the game's existing SDL renderer. It *streams*
and composites up to two GIF frames per paint call, avoiding
`IMG_LoadAnimation`'s full-resolution all-at-once expansion of
40–74 MB GIFs. Only the current GIF is held at once; its logical
canvas/decoded patch and **one 384×224 SDL streaming texture**
are reused. GIF frame metadata (delay, disposal, rectangle) is
indexed in bounded batches **during the 10-second initial wait
and five-second between-clip gaps**. Source transparency, incremental
patches, restore-background and restore-previous disposal are
handled by the small software compositor. Display frames are
downsampled before GPU upload. There is no new DLL dependency or
linker flag: WIC is part of Windows and the existing Makefile
already links `-lole32 -luuid`.

If built without the Windows WIC SDK, the header includes a
single-active-GIF SDL_image `IMG_LoadAnimation` fallback;
the Windows runtime intentionally chooses the streaming path
because full predecode is an unacceptable memory/performance
risk with these source files.

**Timing preserved:** 10 seconds after the native match starts,
play GIF 01 with a one-second fade-in and a one-second fade-out
over its last second, then five seconds of no video; repeat
for 02 and 03 and loop back to 01 with the same five-second gap.
Each GIF's duration and frame updates come from its **source
per-frame centisecond delays**, not an arbitrary playback rate.
The existing semi-transparent dark sky and independent blinking
parallax stars render in front of the animated GIF; the stage,
pink streaks, native fighters/FX and HUD remain on top and do
not receive GIF effects.

Missing GIFs / decode failures leave the native match playable and
advance the playlist, without a blocking media player, audio or
new build tasks. On the versioned TEST, look for:
`XM87.280 FD_GIF_OPEN`, `FD_GIF_INDEXED`,
`FD_GIF_MATCH_START`, and `FD_GIF_END` in the launch log.

Local build and runner versions move to 87.280; all modifications
remain on `final-destination`, not Stable/main. The GitHub source
and GIF paths have been verified, but native Windows compilation
and real playback still require running the existing local scripts.
