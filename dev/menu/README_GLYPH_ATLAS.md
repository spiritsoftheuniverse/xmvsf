# XMVSF native menu sprite atlas

Work branch: feature/menu-logo-glyph-atlas-selection-dimming.

## Verified asset dimensions and mapping

- _art/logo.png: 400 x 300, X-Men vs. Street Fighter II foreground title.
- _art/logo.webp: retained alternative, unchanged.
- _art/sf2font.png: 190 x 70, nominal 16-column x 6-row ASCII grid, cell stride 12 x 12.
- _art/sf2fontsmall.png: 128 x 48, nominal 16-column x 6-row ASCII grid, cell stride 8 x 8.

Both sheets index printable ASCII directly (columns go left to right):
- Row 0: ASCII 32 through 47 (space through slash).
- Row 1: ASCII 48 through 63 (digits and punctuation).
- Row 2: ASCII 64 through 79 (at sign through O).
- Row 3: ASCII 80 through 95 (P through underscore).
- Row 4: ASCII 96 through 111 (backtick through o).
- Row 5: ASCII 112 through 127 (p through DEL; DEL unused).

The large image is 2 pixels short on its right and bottom edges. The renderer
clips edge cells rather than trying to sample pixels beyond the image.

Implementation: dev/stable/buildtree/src/burner/sdl/xmvsf_menu_glyph_atlas.h

## Rendering policy

The PNG is the foreground title, above the original 30%-brightness animated
menu background. The source is a 400x300 **export canvas**, NOT a 400x300
on-screen target. The renderer measures the alpha-visible art, crops only
the extra transparent source margins, then fits the original painted pixels
proportionally inside the exact reference area. No resaved, repainted, or
warped source PNG is necessary.

### User's screenshot: fixed native-coordinate alignment

The supplied 1811x1358 example is nearly 4:3. Its positions map uniformly
into a 720x540 safe frame centered at x=120 inside the game's 960x540 SDL
logical canvas, so the **artwork and glyphs keep the original aspect ratio**
rather than being distorted to fill the widescreen window.

All values below are **logical SDL coordinates**, not physical display
pixels (window resizing leaves them anchored):

| Element | Native target |
| --- | --- |
| Logo visible artwork | x=235, y=51, maximum w=544, h=289 (fit and center) |
| Menu text centerline | x=489 |
| ARCADE | y=347 |
| TRAINING / VERSUS / CONTROLS / OPTIONS | y=378 / 409 / 440 / 471 |
| A SELECT  B BACK | centered x=489, y=495 |
| ESC TO QUIT | centered x=500, y=522 |
| TEST 87.206 | right edge x=942, y=522 |

These are explicitly stored in XMGlyph::MainLayout, so one presentation
contract controls the initial frame and every subsequent boot/menu frame.
The title block ends near y=340; the selected entry begins at y=347.
The 7-pixel gap matches the reference rather than the previous 400x300
canvas assumption.

Primary menu choices use the **large SF2 sprite font at 1.35x**. The
control hint uses the small sprite font at 1.375x; the exit hint is at 1.20x,
and the version stamp at 1.08x. SDL's nearest-neighbor sampler snaps those
fractionally scaled pixels to the pixel grid without blurring.
The other menus retain their existing layout and small font behavior.

## Proportional spacing normalization

The underlying sheets retain their original 16 x 6 coordinate grids and
colored pixels. A one-time glyph measurement pass scans alpha in each cell
and extracts its leftmost/rightmost painted columns. The renderer uses those
exact source bounds and computes a per-character horizontal advance:
- Large font: painted width + 2 native pixels of tracking.
- Small font: painted width + 1 native pixel of tracking.
- Spaces use a separate half-cell word gap rather than a full sprite cell.
- Centered text is centered by the **sum of measured advances**, not the
  number of characters.
- All spacing measurements happen in source pixels. The native menu
  fits them at the screenshot-derived fractional scale with pixel-snapped
  destination rectangles and nearest-neighbor sampling; auxiliary menus
  retain their existing integer scale.
- A blank large-font slash uses the original small-font slash as fallback.

For example, the large capital I is only 5 painted pixels wide (versus
10-11 for most capitals), so its advance is now **7** instead of **12**.
The small lowercase i is 3 pixels wide and advances **4** instead of **8**.
The original PNG sheets are left unmodified; these layout metrics are
derived at load time, which keeps changes compatible with source artwork.

Selected choices show at full brightness. All other choices are darkened.
Do not draw selection arrows, surrounding boxes, or highlight stripes.
Only selection presentation changes; the input and menu navigation routes
remain intact. Colored mode identity bars and ordinary panel boundaries are
not selection indicators.

The source glyph pixels are rendered with nearest-neighbor sampling,
preserving original multicolor sprite pixels. SDL2_image loads the original
file without a regenerated duplicate. Missing source images log errors and
retain the old bitmap text fallback so the native menu remains operable.

For exported title PNGs with opaque white canvas, a flood fill makes only
border-connected white transparent; enclosed white highlights are preserved.
Texture atlases are released before renderer teardown or menu/game handoff.

## Animated menu background and color-separated logo effects (TEST 87.206)

The native boot menu now loads **_art/menuanimation.gif** instead of the
previous Marvel vs. Capcom super-effect GIF. The source is an authentic GIF87a
384 x 224 image with **8 frames**, each **50 ms** long. It loops at the
original 400 ms cycle, cover-scaled proportionally to 960 x 540 and
center-cropped as needed. RGB colors undergo a continuous 36-second full HSV
hue rotation at 30% menu-background brightness. Alpha is untouched.
A streaming texture avoids allocating a new GPU texture per GIF frame.
If SDL_image cannot decode animation, the first frame remains available as
a static, hue-shifting fallback.

The foreground **_art/logo.png** remains the original source artwork. Its
alpha-visible rectangle and menu alignment are unchanged. A second streaming
texture (separate from the unmodified fallback logo) applies per-pixel
effects by original hue category:

- **Yellow/orange:** two independent slow pulse envelopes with a small
  lift toward white, so fully saturated bright colors visibly glow.
- **Green:** its own brightness pulse envelope and saturation-safe lightening.
- **Blue/purple:** intersecting moving sine-wave value fields plus a
  source-locked 4 x 4 Bayer dither. This gives a gaseous brightness
  undulation without repainting or blurring the logo.
- **Red and grayscale:** retained without modification.
- **Source alpha:** always retained; texture updates do not compound over
  previous frames. The logo effect updates at most every 80 ms.

The original large/small SF2 font atlases and their proportional tracking are
unaffected. These are TEST-only SDL frontend changes; no CPS2 ROM input,
hitbox, DHC, stage capture or other game simulation code is touched.
The code resides in xmvsf_menu_color_fx.h,
xmvsf_stage87_195_menu_gif.h and xmvsf_menu_glyph_atlas.h.

The native builder checks that **menuanimation.gif** is present and nonempty,
and includes its SHA256 in test/build_logs/BUILD_TEST.log. The binary is
**runtime/stable/XMVSF_Test_87_206.exe**; no fallback to 87.204 or Stable.
Build/run/replay launchers now all target the new effects version.

## TEST 87.206: purple vibration, music handoff and FIGHT crash evidence

The lowest Street Fighter extruded purple shadow and central purple VS core
now vibrate in discrete visual intervals: a 0.55-second undulation every
2.65 seconds, with independently phased brightness oscillations at
approximately 7.4 and 9.2 Hz. These are strictly original-source RGB
modulations of dark-purple pixels in two source-coordinate regions. Glyph
silhouettes, letter faces, texture coordinates and alpha are not moved.

The existing single Windows WASAPI device now crossfades the menu theme
from its current audible level to silence using a smoothstep envelope over
**500 milliseconds** at the first VS/early-combat host signal. The combat
fallback now sends that signal immediately rather than waiting for a
secondary poll. Repeat combat status observations cannot restart the fade.

The left DHC Super Chat panel is closed by default and on every new gameplay
session. Shift+G opens/closes it explicitly; F10 input probes, chat history
and log recording remain available even while hidden.

**Observed user issue:** the executable unexpectedly **exits/crashes at
"FIGHT!"**. It must not be confused with focus loss, a pause modal or a
mere rendering stall. A confirmed root cause has NOT been established
without a native exception report. 87.206 therefore adds a Windows
unhandled-exception handler that records the actual exception code,
instruction address, DLL/EXE offset and last gameplay/video checkpoint
in `test/87.206/crash_report.log`, and attempts to write
`test/87.206/crash.dmp` using the system DbgHelp DLL. It also guards
null SDL video resources during frame rendering. This is a diagnostic and
defensive pass, **not a claim that the crash is already fixed**. If an
abrupt exit persists, supply the crash_report.log, optional crash.dmp
and the current native RUN_TEST output for exact triage.

BUILD_TEST.bat now checks required 87.206 headers and regenerates just
the affected `main.cpp`, `run.cpp`, `vid_sdl2.cpp`, and `aud_sdl.cpp`
objects when this executable is missing or the related sources change.
All other warm objects, Stable, and older capture/replay files remain.

## Native stun bar replacement (TEST 87.207)

Source and screenshot reference now exist on the same branch:

- `_art/stunbar_base.png`: **61 x 6**, the ORIGINAL RIGHT-side image.
- `_art/examples/example_stunbar.png`: **61 x 8** composition reference.
  It is not loaded in-game or substituted for the source base.

The base's highest original pixel is anchored at native CPS-2 **y=45**,
which matches the preceding stun HUD; P1 starts at x=61 and P2 ends at
x=322 (P2 starts at x=262 for the 61-pixel source). The P1 source pixels
are horizontally mirrored. Do not rotate or invert the bar vertically.

The reference composition places the six-pixel source base at its rows
2..7, while the active original orange region is at rows 0..5. At full
stun, the active region spans exactly **44 pixels**, with each subsequent
row shifted right one pixel. The render code preserves that shape, but
replaces orange with moving six-pixel-wide diagonal rainbow bands. Stun
usage from the original ROM continues to control the active width.
One pixel of dark inset is drawn along the diagonal growth cut and across
the bottom of the active region at reference row 6 (inside the base).
The rest of the source PNG, existing shadows and 1-pixel diagonal grooves
remain untouched.

All drawing remains in the **existing native SDL HUD/UI order**, after
the game scene and before Training/Versus text panels. Wily's existing
compositor draws the same routine once; normal stages get it exactly once.
No new rendering layers, gameplay RAM changes, stun timing, or collision
logic. The original total/per-hit counter readouts remain.

The PNG is decoded into RGBA bytes once and then reused across frames.
Missing or wrongly sized PNGs produce a named log entry and a readable
fallback bar; BUILD_TEST.bat requires the original PNG to prevent quietly
shipping the fallback. TEST 87.207 recompiles native UI/HUD source
(including `cps_obj.cpp`) and publishes only
`runtime/stable/XMVSF_Test_87_207.exe`, never Stable.

## TEST 87.208 — Seam-free purple vibration and corrected stun fill

The two visible horizontal limits in the logo arose from hard rectangular
`vsCore` / `bottomShadow` thresholds, together with a low-brightness
chroma cutoff. Those rectangles are removed. Purple membership now follows
the original PNG's **actual purple RGB pixels**, including dark gradients;
two overlapping vibration frequencies crossfade smoothly over the logo's
height. The moving Bayer gas remains source-pixel-locked. There is no
rectangular crop, alpha change, blur or pixel displacement.

The provided `_art/examples/example_stunbar.png` depicts the active
**44-pixel partial** fill, not the bar's maximum capacity. The native right
stun base (`_art/stunbar_base.png`, 61x6) determines its own opaque span
per source row. At **100% stun**, every original visible source span is
filled, through both ends. The P1 copy still mirrors P2 horizontally.
At partial stun, the growing diagonal leading cut is one source pixel
per row; the exact ROM stun value determines its width. The 1-pixel
shadow sits **inside** the filled span and is also intersected with
the original PNG's base alpha, preventing the former two-pixel overrun.

The ordinary accumulating fill now uses a yellow-to-orange graded band.
Only when the displayed bar reaches **100%** (including the existing
dizzy-trigger flash) does it switch to a vivid seven-color rainbow,
in 4px diagonals moving **3 pixels every emulated frame**. The UI render
order and 45 native y-coordinate are unchanged; no gameplay mechanics,
native CPS2 stun counters or Stable build are modified.

The updated builder and launchers target only
`runtime/stable/XMVSF_Test_87_208.exe`. These are source changes, not
a claim of Windows runtime or visual verification.

## TEST 87.209 — Single stun HUD, moving tip mask, Cyclops release restored

**Duplicate stun meter root cause:** `cps_draw.cpp::DoDraw` still called
`XM8733::Draw` into a 384×224 native software overlay **without the new PNG
asset**. That invoked the obsolete 76px fallback. Then
`vid_sdl2.cpp::xm8733Overlay` drew the new 61px PNG stun meter again. The
old native pass has been removed. The existing SDL gameplay HUD/UI pass is
now the **one authoritative stun-bar renderer**; Wily's original caller
shares that pass and avoids a second submission.

**Moving tip mask:** The diagonal leading edge and the one-pixel inset
shadow now share `XM8733::FillBounds`, calculated from each source row's
actual alpha span **and current ROM-derived stun value**. The tip advances
as stun accumulates; its slope shrinks safely near full capacity. The
last shadow scanline reuses the current row-5 mask, clipped against source
alpha, so there is no stationary tip or two-pixel overrun. The fallback
76px bar has been retired even when a PNG fails to load (build preflight
still requires `_art/stunbar_base.png`).

**Cyclops air-throw recovery regression:** Stage **87.121**'s previously
working `XM87121::state().frame` implementation is intact, including
the exact preceding airborne controller and the `200C → 200E` first
flip after victim landed in `1C34`. Its `XM87121_CYCLOPS_RELEASE`
environment flag had silently gone missing from the regular TEST launcher,
leaving it off. It is enabled by default and explicitly in normal
`RUN_TEST.bat` and `RUN_REPLAY.bat`; set the flag to `0` for
controlled A/B comparison. Donor MVC1 launches still force it off.
No new superjump, no opponent writes, no forced follow-up or DHC change.
The CPS2 frame-hook now explicitly checks the driver name begins
with `xmvsf` before invoking the recovered controller logic; MVC1 cannot
inherit the restored XMVSF behavior through a direct ROM launch.
Native event log: `test/87.209/observer/cyclops_air_throw_release.tsv`.

**Regression coverage:** `dev/dhc/tests/test_stun_and_cyclops_209.cpp`
is a standalone C++11 synthetic RAM/pixel-mask test. It checks movement
of both mirrored bar tips, full-width rainbow, warm partial fill,
one-pixel shadow clipping, no old PNG-less fallback, exact Cyclops
state restoration only when the victim grounded, unchanged world X/Y
and victim record, plus explicit disabled recovery. BUILD_TEST.bat runs
this ROM-free test before the native incremental build. The builder
refreshes the changed `cps_draw.cpp`, `cps_run.cpp` and SDL frontend
translation units without touching Stable or older recorded replays.

**Target:** `runtime/stable/XMVSF_Test_87_209.exe`. Source inspection and
unit-test wiring do not constitute a verified Windows gameplay run.

## TEST 87.210 — Original 61x8 stencil, combat opening, 5 animation frames

The 87.209 stun mask incorrectly inferred its fill from the alpha of the
**61×6** base. The artist's reference `_art/examples/example_stunbar.png`
has a precise **54-pixel inner face**, NOT a 56/61-pixel active area:
the active rows have a constant diagonal slope and right endpoints
x=53,54,55,56,57,58. The reference's orange example is 44/54 filled,
with left edges x=10,11,12,13,14,15. A 100% fill is therefore 54
pixels on each row and retains the intended two-pixel gray outer bevel.

`XM8733::SourceSpan` and `XM8733::FillBounds` now use these exact
pixel coordinates, independent of the base transparency silhouette.
Partial yellow/orange and full rapid diagonal rainbow share that
single moving span. On composite rows 2–5, the 1px dark tip is at
`first-1`, matching the PNG example. The final bottom shadow uses
row-5's current fill mask intersected with the original source PNG
alpha. No legacy bar and no fixed-width tip.

**Visibility:** Original game status `0x06` is the combat stage opening,
before `0x08` FIGHT. The new stun base now appears during `0x06`
already at its final HUD location (native y=45), empty and without
reading uninitialized character stun counters. During `0x08`, the
original live game RAM supplies stun values, per-hit amounts and
rainbow-on-full behavior as before. Selection/Versus statuses `0x02`
and `0x04` do not show the stun HUD.

**Cyclops:** Restore Stage87.121's saved pre-throw airborne controller
**only after five animation-frame changes** at native fighter word
`+0x42` following the observed `200C -> 200E` grab-end flip. This
is NOT a five-game-frame timer. The victim must have landed in native
state `1C34` and Cyclops must still be airborne and in his throw
recovery `200E`. On premature natural recovery/landing, cancel
instead of writing stale controller values. Local logs now expose
`anim42`, `anim_advances`, `GRAB_ENDED`, `ANIM_ADVANCE`,
`RELEASED`, and cancellations so we can verify the actual
game's animation cadence independently of emulation FPS.

The new C++11 ROM-free test at
`dev/dhc/tests/test_stun_and_cyclops_210.cpp` asserts all six
reference fill rows, the two-pixel bevel, mirrored P1/P2 geometry,
empty pre-FIGHT HUD, no appearance in Versus, five non-uniform-timed
sprite-frame advances, unchanged world position and opponent RAM,
and the previous opt-out. BUILD_TEST runs this regression before
the incremental native build. No Stable, ROM, DHC or F9 replay changes.

## TEST 87.211 — Cyclops air-throw timing: every native update

**87.210 regression confirmed in source:** The air-throw release threshold
was coupled to five **changes** in RAM word `+0x42`. A held animation
pose can keep this word unchanged across many real emulation frames, so
recovery could remain stuck at zero or one observed changes even after
Cyclops spent more than five frames in `200E`.

87.211 preserves the original 87.121 native grab-end transition
(`200C -> 200E`), Cyclops-only fighter gate, saved preceding airborne
controller and opponent-grounded `1C34` requirement. It starts the
`recoveryTicks` counter at zero at grab-end, then increments **once on
each distinct native combat-frame callback** while Cyclops remains in
`200E`. At five ticks, it restores the saved airborne state as soon
as the opponent is grounded. Duplicate callbacks with the same frame
ID do not count; the `+0x42` value and changes are still recorded,
but no longer gate recovery. Natural action exit or Cyclops landing
still cancels pending restoration to avoid overwriting another move.

The named TSV log includes `recovery_ticks` and separate
`anim_advances` diagnostics, with `RECOVERY_TICK`, `RELEASED`,
`WAIT_VICTIM`, `FLIP_REJECTED`, and `RELEASE_CANCEL` decisions.
If the real ROM skips the strict 87.121 first-flip signature, its
mismatch is logged rather than silently disappearing.

A new ROM-free C++11 regression at
`dev/dhc/tests/test_stun_and_cyclops_211.cpp` holds the *same* +0x42
pose for five separate native updates, verifies the release on the
fifth update (not on an animation-word change), prevents duplicate
frame counts, checks delayed victim landing, opt-out, natural action
exit, and retains all reference-image stun/HUD assertions.
It replaces the 87.210 synthetic test in BUILD_TEST.bat.

This corrects a demonstrated logical gate. Native Windows gameplay
verification is still required to prove the observed Cyclops recovery.

## TEST 87.212 — Cyclops fixed 100-frame post-grab recovery

**Request:** The previous five-frame release was too early when it
succeeded and intermittently failed to trigger. Replace the delay with
**100 native combat-frame updates after the actual grab-end 200C→200E
transition**, regardless of whether the sprite animation word at +42
changes. The counter does not begin until the grab ends. There is no
early release at frame 5 and no wait for five unique animation poses.

The earlier 87.121 first-flip recognition was overly restrictive
(exact step06/step08/mode/flag values), and its airborne snapshot
was required to be at most two updates old. This pass identifies the
Cyclops grab-end by action transition while still airborne, accepts
the most recent saved airborne snapshot up to 24 native updates old,
and starts a one-shot timer. It continues through throw-family
substates 2000..201F. Normal unrelated actions or Cyclops landing
cancel pending recovery rather than rewinding a new animation.

The victim's landing is remembered as soon as native Y reaches 472
during the countdown. The fleeting 1C34 landing animation does not
have to remain active precisely on update 100. On update 100, if
Cyclops remains airborne/in throw recovery and the victim landed,
restore Cyclops's original pre-throw airborne control. If the victim
has not landed yet, release on the first later update where ground
contact is observed. No opponent RAM, world position, HP, meter,
ROM, DHC or Stable edits.

The diagnostic TSV logs `GRAB_ENDED`, `RECOVERY_TICK` (first/100th),
`RELEASED`, `WAIT_VICTIM` and `RELEASE_CANCEL` with
`recovery_ticks`, plus independent pose-change observations.
`dev/dhc/tests/test_stun_and_cyclops_212.cpp` simulates a held pose
for exactly 100 frames, ensures no release on frames 1..99, verifies
the victim landing latch after 1C34 ends, tests late landing, validates
an intervening 200E→2010 throw substate, opt-out, natural action exit
and duplicate-frame protection. The existing stun/HUD tests remain.

The TEST build/launchers now target only
`runtime/stable/XMVSF_Test_87_212.exe`. Source checks do not replace
a local Windows gameplay verification.

## TEST 87.213 — Cyclops recovery event scope and repeated throws

User observation: a previous release succeeded **on one attempt**, then
failed on later attempts; it did NOT succeed for only one visual frame.
That points to event lifetime/rearming, not necessarily an immediate
ROM overwrite. This pass keeps the requested **100 native frames**.

**Concrete frame-hook defect:** `cps_run.cpp` invoked
`XM87121::state().frame()` without `!bBurnRunAheadFrame`, even though
nearby native observers explicitly exclude speculative emulation.
The Cyclops tracker is process-static host state, not serialized as part
of CPS2 savestates. A speculative run-ahead invocation could update
`lastFrame`, capture the throw event, or advance the countdown while
the associated `CpsRamFF` state is later discarded. The real committed
frame with the same frame ID could then be skipped as a duplicate.
The hook now runs **only on committed, non-run-ahead CPS2 frames**.

**Per-throw scope:** Every fresh 200C entry increments `throw_id`
and clears the prior release's `pending`, `waiting`,
`victimLanded`, and `recoveryTicks` before capturing a new airborne
baseline. The saved airborne controller can now be recognized from
native airborne actions `0200/0202` and altitude even when transient
mode/AA flags differ. The countdown can continue if Cyclops's throw
action leaves 2000..201F but his native mode remains `32` (still in
throw recovery). Repeated combat phase exits and fighter-ID changes
log `SCOPE_END` with a named reason.

**Verification target:** `dev/dhc/tests/test_stun_and_cyclops_213.cpp`
adds two successful grabs in one synthetic match, with the second
starting during the first's 16-frame follow-up window. Both must
independently reach their 100-frame release, and the second event must
not inherit the first's timer. A separate synthetic recovery sequence
allows action 3400 while native throw mode 32 remains active. The
previous stun-bar reference geometry tests and victim-grounded,
opt-out and natural-exit checks remain.

These source changes address real lifecycle hazards but **do not prove**
which one occurred in the user's native run without a Cyclops TSV.
The native launcher logs to
`test/87.213/observer/cyclops_air_throw_release.tsv`.
`throw_id`, `THROW_CAPTURE`, `GRAB_ENDED`, `SCOPE_END`,
`RELEASE_CANCEL` and `RELEASED` identify how each attempt progressed.
No Stable, ROM, stun HUD, or DHC logic changes.

## TEST 87.214 — Cyclops release at 0.6 seconds and SF2 floating frame data

**Native evidence from attached `87.213.zip`:** The Cyclops observer
recorded grab-end frames 3954 and 4851, then cancellations at 4006
and 4904. The first counted 51 native recovery updates and the second
52. The ROM had already naturally landed Cyclops by then
(`RELEASE_CANCEL/CYCLOPS_LANDED`). This was not a 100-frame timer
overflow or a mysterious one-frame release. A 36-frame delay
(approximately 0.6s on a 60fps combat clock) fits inside both observed
recovery windows.

**Change:** Stage87.121's event scope/rearming and committed-real-frame
guard remain as in 87.213. `ReleaseDelayFrames=36` now triggers the
saved pre-throw airborne controller precisely on the 36th committed
update after the `200C -> 200E` grab-end event, as long as Cyclops is
still in the native throw recovery and airborne. The opponent's transient
`1C34` landing code is no longer a release veto: the supplied logs show
the victim already grounded at grab end, and release is intentionally
earlier than Cyclops's own landing. No source ROM/HP/meter/opponent writes,
DHC logic, Stable executable, or pre-existing HUD presentation changes.

**Floating frame data:** The existing Training `S:##F` popup above
fighters now uses the *actual* `_art/sf2fontsmall.png` glyph atlas rather
than the fallback 5×7 procedural font. The popup still rises 40 screen
pixels across its prior 120 native ticks. After the apex, it rises
another **16px (one glyph height)** during exactly **20 native frames**,
shrinking only its horizontal width to zero and fading out. The glyphs'
vertical height never compresses. All rendering remains in the existing
SDL UI pass, no new compositor or game-state readbacks.

The ROM-free C++11 regression
`dev/dhc/tests/test_stun_and_cyclops_214.cpp` covers the release
on update 36, not before; consecutive grabs, disabled mode, natural
action exit, original 61px stun composition, and popup age 119,120,129,
and 139 including 50% width and the full 16px extra ascent.

Native BUILD_TEST.bat and RUN_TEST.bat publish TEST 87.214 only;
Windows gameplay confirmation remains outstanding.

## Native TEST version 87.214 and local verification

**Pulling only main does not include this branch.** The source modifications
are on feature/menu-logo-glyph-atlas-selection-dimming. In the Windows
project repository:

    git fetch origin
    git switch feature/menu-logo-glyph-atlas-selection-dimming
    git pull --ff-only origin feature/menu-logo-glyph-atlas-selection-dimming

Preserve any uncommitted local work before switching branches. Keep the
original _art PNG files; do not resample or replace them.

1. Run BUILD_TEST.bat. It now publishes ONLY
   runtime/stable/XMVSF_Test_87_214.exe after compiling current source.
   A missing executable or modified menu/video/audio/crash header refreshes
   only main.cpp, run.cpp, vid_sdl2.cpp, aud_sdl.cpp, cps_obj.cpp, cps_draw.cpp
   and cps_run.cpp without deleting warm build objects, older binaries, or Stable.
2. Read test/build_logs/BUILD_TEST.log for the 87.214 success line and
   the SHA256 hashes of the compiled executable, four source PNGs and GIF.
   If an asset or the glyph-header include is missing, the builder gives
   a precise error instead of silently building a legacy-looking menu.
3. Run RUN_TEST.bat option 1. The menu window title and bottom-right
   main-menu label both say TEST 87.214. If they do not, you are not
   looking at the newly compiled native XMVSF build.
4. The gameplay path is still the native XMVSF app's zero-ROM-argument
   boot selector, NOT the separate MVC1 capture/ROM playback mode.
   Inspect the title artwork, font tracking, Arcade, Training, Versus,
   Controls, Options, scoreboard and return-to-menu behavior.
5. Verify the unselected text is dim, selected text is bright, and that
   I, i, l, 1, punctuation and word gaps no longer waste full cell widths.
6. New F9 replays, observer/chat diagnostics and replay verification live
   under test/87.214. Legacy test/87.203 capture data is not modified;
   original replay files remain accessible through RUN_REPLAY.bat.

This source audit includes inspection of the tracked code, unchanged font
PNG blob hashes, and explicit version paths. It is not a claim that this
GitHub-only session compiled or ran the native Windows executable.
GitHub Actions are not used.
