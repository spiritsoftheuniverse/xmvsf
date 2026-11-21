# XMVSF native front end — 87.492 (visible native OAM ink ownership and F9)

## 87.492 — source-level visual evidence for all OAM graphics

The 31 new user screenshots (`screenshots(1).zip`) cover native
fight frames 3425–8183. Their source records include **6,464 OAM
entries across 355 distinct source signatures**; appearances of
the same source vary with animation, and raw OAM bounding rectangles
are not evidence of rendered visible pixels. This is a sample of
one fight/stage, **not** proof of semantic names for the full ROM.

**F9 now samples visible OAM ink in the actual renderers.** The
diagnostic-only `VisibleOamInk` ledger starts at the beginning of
each emulated frame when F9 is on; it attributes a native pixel to
its original OAM entry only after it survives transparency, native
scroll priority and HUD concealment. The first renderer to report
a screen pixel claims it so a simultaneous native/board video-check
cannot double-count. This produces, per source, a count of actual
native drawn pixels and a tight on-screen ink bounding box.

F9 HUD, WALLS and ALL OAM modes now outline and label **only entries
with measured visible ink**. Source-only records or objects hidden
behind others no longer acquire misleading floating rectangles.
The previously verified WALLS source-exemption signature is unchanged;
the inspector never alters the game's graphics, ROM, VRAM, HUD
timing, combat logic or F10 recorder. The compact F9 badge shows
the selected source's actual `INK` count, slot and bank-relative
address. The corresponding F6 TSV appends whether the pixels were
measured, visible-ink count and exact min/max coordinates.

This is the first graphics-wide source ownership pass, applicable
to fighters, HUD elements, OAM effects and objects in **any native
stage**. **Scroll 1–3 remain explicitly source-map only**, because
their final per-pixel ownership after OAM overlap has not yet been
measured; F9 must not claim to have proven their visible pixels.
Unidentified assets likewise retain source indices rather than
invented names.

The new SDL-free `VisibleOamInk` C++ smoke executes per-pixel
deduplication, overlapping OAM sources, real tight bounds, zero
visible pixels for hidden candidates, shifted object slots, and
frame reset. The F9 renderer smoke now requires source-derived
visible ink rather than just a source bounding rectangle. Existing
HUD concealment, wall immunity and stage select behavior are not
modified. Windows full build/gameplay remain to be verified locally.

## 87.491 — fix actual missing wall object protection and F9 evidence

The user's `screenshots.zip` contains both the original isolated
blank wall boxes and later captures of a frame with **HUD and walls
together**. A comparison of the native `f9_source` records exposes
the precise regression in 87.490: the *same wall object identity*
migrates from **bank 0, OAM slots 4–39** (frame 2014) to
**bank 1, OAM slots 114–149** (frame 816). Both scenes contain exactly
30 objects with full source tile `1C60A`, attribute `2318`,
priority 7, palette 24, and 4×3 16px cells (64×48).
Thus a slot 4–39 requirement was never a valid identity and
explains why the actual concealed boxes were not protected after
the game rearranged its object list.

The wall exemption now uses **the full source OAM identity, without
an OAM slot or bank gate**. Both renderers therefore exclude the
corresponding *whole objects* from HUD-shock dither clipping,
including while legitimate HUD OAM objects share the same frame.
No graphics ROM data or native game RAM are edited, and no wall
is hidden/removed or protected by a screen-space rectangle.
The C++ smoke verifies source object identity at slots **4, 39,
114, and 149** and rejects variants with different attributes,
priority, code, or dimensions.

F9's earlier "HUD" selection was simply any object rectangle that
intersected the top 64 pixels. That was **not actual HUD identity**
and produced floating boxes. Now the default HUD source group uses
the same **native FBNeo HUD source classifier** as the renderer.
**WALLS** is a separate, explicitly verified OAM source group,
automatically selected when F9 is enabled during a frame containing
the wall objects. F8 cycles HUD SOURCES, WALLS, ALL OAM, MAP 1, MAP 2,
MAP 3. WALLS shows the full family of 30 source bounds in cyan and
the current page's slots in yellow, with labels *touching* their own
source rectangles rather than floating 10px above them.

SCROLL MAP records are **source addresses, not proof that those map
graphics survived priority or OAM occlusion**. F9 no longer draws
scroll-map boxes as though they are visible image elements, and the
compact badge explicitly says MAP ONLY. All actual addresses remain
captured in the matching F6 `.graphics.tsv`; no new color matching,
tile filters, speculative pixel-ownership assertions, or runtime
F10 triggers were added.

**Test scope:** the C++ pre-Ninja smoke checks wall relocation,
HUD/WALL/ALL/MAP mode classification, page navigation, and that the
scroll map view does not draw misleading floating object bounds.
Windows compilation and real in-game protection still require local
validation.

## 87.490 — preserve the captured native wall blocks, minimal F9 badge

The four captured `.graphics.tsv` files identify **30 native OAM
objects**, slots 4–39, with full 20-bit source tile `0x1C60A`,
attributes `0x2318`, level 7, palette 24, and a single source
object extent of **4×3 16px cells (64×48 pixels)**. The repeated
logo/wall artwork is therefore native OAM, not a scroll layer.

These **whole source objects** are now excluded from the HUD-shock
classifier in both native rasterizers, before the per-pixel dither
clipping occurs. They are **not** removed, hidden, blacklisted as
individual ROM tiles, or altered in RAM. The source check uses
slot, full tile code, full attributes, priority and object extent.
OAM bank is intentionally ignored because hardware banks switch
across frames: this signature is the stable observed identity.

F9 **HUD** mode excludes these confirmed walls as non-HUD;
F8 **ALL OAM** still shows them for inspection. The old **91px
full-width diagnostic ledger** is gone. Its replacement is a
**24px-tall, 298px-wide maximum badge** with two lines of crisp
integer-pixel font: source mode/page/frame and a single page
source ID/address. Only the current page's source boxes are drawn.
The complete detailed source records remain in F6's
`.graphics.tsv` companion, so screen captures retain evidence
without covering the gameplay.

A compiled C++ smoke now tests the exact wall identity (positive
and negative variants), removal from HUD-only inspection, continued
visibility under ALL OAM inspection, and confirms the unused
bottom-right viewport is no longer covered by the ledger.
The existing +180f HUD reveal and F10 diagnostic-only operation
have not been changed.

## 87.489 — F9 indexed, data-driven native graphics inspector

**F9 (gameplay):** toggles the on-screen source inspector. Default
**HUD OAM** view identifies native latched sprite objects intersecting
the upper 64 pixels (the HUD band) and draws source-attached bounding
boxes with actual **OAM slot indices**. The source ledger at the
bottom identifies each object's exact bank-local OAM byte offset
(`OAM0+xxxx` or `OAM1+xxxx`), full ROM object tile code (`Txxxxx`),
ROM graphics byte offset (`Gxxxxxxx`), palette, priority, screen
coordinates, and size. OAM offsets are deliberately labelled
bank-relative: the CPS2 alternates physical OAM banks, and printing
a made-up static 68K bus address would be misleading.

**F8:** cycles source categories **HUD OAM → ALL OAM → SCROLL 1 →
SCROLL 2 → SCROLL 3**. **[ / ]** page through native records in
groups of seven, with no wrap or input-repeat traps. Scroll-source
pages show the exact native `0x9xxxxx` VRAM address of the scroll
map entry, map index, graphics code, ROM byte offset, palette and
on-screen tile bounds. A named HUD part is **not** guessed from
the code or the picture; this is a labelable native object candidate.

**F6 while F9 is on:** saves the **annotated** gameplay PNG, with
the same source indices and ledger visible as in the window.
It also saves the complete unmodified `.graphics.tsv` source
inventory beside it. The onscreen inspector is a presentation-copy
operation, before the ordinary SDL3 compositor. It does not modify
the emulator's graphic buffers, graphics ROM, VRAM, OAM, HUD
concealment, or song timing. SDL_GPU's direct-frame shortcut is
temporarily bypassed while F9 is active to guarantee the inspector
is present on screen (and in F6).

**Original shortcuts:** F5 still saves the state; **Shift+F9**
loads the state in XMVSF; other games retain their original F9.
F10 is still diagnostic-only and F6 still takes native screenshots.
The new pre-Ninja F6 preflight compiles and **executes** a C++ test
of overlay toggling, source index preservation, HUD vs scroll modes,
and bounded page navigation. No GitHub Actions.

**Important:** boxes identify actual source records, not confirmed
visible pixels; CPS2 priorities and occlusion can hide some of them.
The next evidence-based step is to compare these indexed captures
across frames and only then attach semantic HUD names or source
suppression. No graphics source is currently denied by F9.

## 87.488 — find native graphic ownership before suppressing art

The F6 capture `xmvsf gameplay 2026-11-20 22.41.12.png`
shows a repeated **64 × 48** X-Men vs Street Fighter logo pattern
over part of the game picture. This exact repetition strongly
suggests a reused graphic source (possibly one of the CPS tilemap
scroll layers), but a raster screenshot alone cannot establish which
CPS2 register, map, or OAM record owns it. Do **not** add logo tile-ID
blacklists or screen-coordinate removals.

On **every new gameplay F6 screenshot**, the recomp now also writes
the **same-basename `.graphics.tsv`** in `screenshots/`. The report
contains the actual latched OAM record slots (priority, 20-bit ROM
tile code, palette, flips/extent and coordinates) and read-only 8px
samples of the three CPS-A/CPS-B-selected scroll maps (layer enable,
scroll offsets, map entry address, source code/attribute/palette).
These records come from the live native renderer and video RAM, not
a filter applied to the screenshot.

The scroll map table lists *candidate source entries*, not verified
visible pixel ownership, because CPS2 priority and mid-raster
register changes can occlude them. Compare the repeated logo
sample locations against the actual entries and scroll/layer control
before selecting the **whole native source** to hide. No gameplay
input, render, HUD shock, timing, native RAM, ROM or graphics has been
changed in this revision. The F6 capture is diagnostic-only; F10
remains separate and does not trigger any visual effect.

## 87.487 — calibrated automatic HUD reveal; stage/input/audio corrections

**Timing, now measured from the attached 87.485 F10 README:** the
same emulator session reports VS PCM start frame **910**, verified HP
frame **916**, and F10 marker at frame **1090**. Therefore the desired
automatic first reveal frame is **VS PCM +180 native frames**. The
previous recomp timer began revealing at +120, 60 frames too early.
The recomps use their independent `CombatHudClock::kSongToShockFrames`
of 180, without modifying the original FBNeo reference header and
without using F10 as a gameplay trigger. Masking remains fully hidden
until the deadline (and until verified native HP is drawable); the
existing 30-frame Bayer/electric-blue/white shock follows. The visual
HUD mask identity is unchanged from the user's working 87.486 build.

**Stage directional correction:** Previous 5-column modulo wrapping
moved from leftmost to rightmost, producing a visual four-panel
movement opposite to the pressed direction. The stage grid now bounds
all four directions at its edges. Its directional and LB/RB/Q/E
events each have an independent stateful, conflict-safe press latch:
simultaneous opposites and diagonals are neutral; releasing one
opposing direction re-arms the remaining input. No fast auto-repeat,
no changes to other menus' input policy. The new SDL-free
`xmvsf_stage_nav.h` is mirrored; a compiled/run smoke exercises
boundary, second-page, diagonal, conflict, release, and new-entry
behavior.

**Fight! tail:** the one-shot WAV is still played on level confirm,
and the existing wait remains. At the exact switch to rendering
character select, its isolated SDL3 audio device is muted, paused,
and its remaining queue cleared. The next valid level-confirm cue
resumes the device with normal gain. This prevents any buffered
trailing syllable from bleeding over character select.

**Build policy:** existing stage installer idempotence check updated
for `stagePressed`; both stage/HUD regressions execute C++ behavior
tests rather than matching operator characters. No GitHub Actions
and no FBNeo Stable/TEST binaries touched.

## 87.485 — F10 is strictly a marker, never a shock trigger

**Correction:** 87.484's F10 -> `PreviewShockAt()` wiring was a
misinterpretation of the requested timing and has been removed.
Pressing F10 only toggles the existing bounded native M68K recorder.

The supplied `latest.zip` begins at emulated frame **1451**, with
`FF5104=0008` and visible priority-7 top HUD OAM, including timer
sprite tile `0x15BA9` at (176,0). The ZIP contains no frame from
the earlier versus PCM or the 04-to-VS handoff. **There is no
measurable relative song-to-F10 offset in that file.** Accordingly
no 1451 absolute-frame magic number or invented extra delay was
put into gameplay.

The source of the partial mask is a state-address conflation:
`FF5104` (the native game step recorded at F10) and `FF5000`
(the ROM fight status) are different. The video now records both,
so `FF5104=0008` authorizes FBNeo's **entire upper native HUD**
family, rather than the restricted 02/04 HP-only transition mask.
This applies in both rasterizers and remains per-object/per-ink;
the stage is never masked as a rectangular region.

The F10 recorder's `README.txt` now writes a **read-only**
`HUD_CALIBRATION` record with the user's pressed emulated frame,
first versus audio frame, elapsed audio frames at the press,
first verified HP frame, `FF5104` and `FF5000`.
A capture made with this revision at the intended instant provides
the missing paired timestamps needed to move the **automatic**
shock timing precisely, without ever triggering it from F10.

**Timing status:** 87.485 deliberately retains the original
FBNeo music/visual timeline; the exact intended additional
offset has not been computed from the insufficient 87.480 ZIP.
Only F10 isolation, HUD coverage and future timestamp diagnostics
are changed/validated by source inspection.

## 87.484 — exact F10 press is the first shock frame

The `latest.zip` recording begins at native frame **1451**. That is
the desired first visible shock frame, **not** the beginning of another
120-frame hold. When the user starts an F10 capture during native
combat (`FF5000=08`), the F10 down-edge now calls
`xmvsf_vs::PreviewShockAt(machine.frame)` and resets the top-HUD
reveal to **step 1 on that very first drawn frame**. Stopping F10
does nothing to the shock. The original Bayer/white/blue 30-frame
reveal is unchanged. The normal match-trigger path remains separate
from the debug F10 preview; F10 is not required to play the game.

The regression where only HP bars disappeared came from passing
`palette >= 0x1A` as `scenery=true` to FBNeo's existing
`objectForPhase` classifier, inadvertently rejecting HUD portrait
graphics before its complete priority-7 HUD grouping was applied.
Both the default native and board raster renderers now use the full
original FBNeo HUD group (plus the ROM-verified timer), preserving
the strict source-only behavior in phases 02/04.

The local smoke verifies frames 1450/1451/1452 have reveal steps
0/1/2, with full reveal at 1480, without a music countdown.

## 87.483 — F10-calibrated native combat-HUD shock gate

The uploaded compact F10 `latest.zip` begins at emulator frame **1451**,
while `FF5104=0008`, both chooser phases are `07`, and object bank 0
contains genuine upper HUD HP strip tiles (e.g. `0x159B5`,
priority 7, palette 2 at native x=144, y=20). The capture starts
**after** the 04-to-VS transition and does not contain the earlier first
combat frame. It cannot establish an exact offset from combat entry to
frame 1451, and the global 1451 frame must never be hard-coded.

TEST 87.483 keeps `versusscreen.ogg` and the original FBNeo music
120-frame hold unchanged but adds a separate visual guard in
`xmvsf_vs_handoff.h`. Real native `FF5000=08` plus **a drawable,
source-verified priority-7 HP pixel** now marks combat HUD availability.
Until that point the HUD stays invisible. The first actual source
frame remains hidden, then the unmodified white/electric-blue,
8x8-Bayer shock materializes for 30 native frames, and only after
both the original audio hold and live-HUD gate permit it.

The gate is called in **both** the board and the default native
software rasterizer *before* hidden-HUD pixels are suppressed,
so it cannot deadlock waiting for an already masked first pixel.
It resets on a new VS handoff, leaving gameplay, and round-ending
0A/0C transitions. There is no F10-dependent trigger and no
fixed session-frame delay. Local preflight checks these invariants
and the original 87.482 behavior before Ninja.

## 87.482 — original FBNeo versus and HUD shock handoff

Uses FBNeo's existing `xm20_vs_early_phase.h` transition gate and
`xmvsf_stage87_392_hud_shock.h` timeline, ROM-HUD source classifier,
8x8 Bayer dither and original white/electric-blue traveling shine.
The native FF5104 chooser-step transition from 04 to 05/06/07
(fallback directly to 08 while not yet fighting) is the **only** VS cue.
The last character/speed confirmation alone cannot trigger it.

The original song 02 (`audio/music/characterselect.ogg`) is enabled
for Training, Arcade and Versus when no custom soundtrack override was
configured. Native stage/VS song request 04 is consumed on the XMVSF
soundtrack path, leaving the selection music and sound effects running.
At the actual M68K handoff, `versusscreen.ogg` plays ONCE through
the existing SDL3 Ogg decoder; the select-song audio device is muted.
The first queued versus PCM starts FBNeo's 120-native-frame hidden
interval and 30-native-frame top-HUD materialization, with the original
first-source late-availability guard. Combat status FF5000=08 stops
the VS one-shot.

Both the default **native-video rasterizer** and the CPS2 board-reference
renderer apply that same FBNeo classifier to verified ROM objects and the
same reveal mask and shine **per actual HUD ink pixel**. The native renderer
retains each original OAM entry's identity even after expanding it into
16-pixel tiles. Hidden ink leaves the stage and fighter colors underneath
intact. No screen-space blackout, generalized palette filtering,
renderer fallback, ROM/RAM change, or GPU/SDL2 dependency.
The subsystem remains scoped to native XMVSF's SDL3 front end.

The `recomp_port/tests/test_vs_hud_shock.py` preflight compiles the
CPS2 raster renderer, XMVSF source hook and SDL3 helper, and executes
the original 120+30 timing and handoff smoke before local Ninja build.
Runtime audiovisual and device-start accuracy require the local
Windows executable to be tested after `RECOMP_BUILD.bat`.

## 87.480 — compact F10 diagnostics, no directory explosion

F10 uses one reusable directory: `screenshots/f10_m68k/latest/` by
default (or `XMVSF_M68K_CAPTURE_DIR/latest/` when configured). A new F10
recording **replaces** the five prior files in `latest`; it never creates
another timestamped session. Older timestamped folders from previous versions
are left untouched so nothing is silently deleted.

The capture ends at **900 emulated frames or 512 KiB total output**,
whichever happens first. Selection frames are sampled every eight native
frames and immediately on native state changes. OAM is sampled every six
frames/state change, with **separate snapshots for each of the two object
banks**. Unused zero-tile slots and duplicate bank snapshots are suppressed.
Per-sample, per-file and overall caps stop object-delta and selection-write
bursts. Repeated controller-panel writes are not stored.

Full object-bus writes are **off by default**. To record a bounded raw
object-write trace, launch the game with `XMVSF_F10_RAW_OAM=1` in the
environment. Normal F10 captures still provide the smaller
`frames.tsv`, `hooks.tsv`, `oam_delta.tsv` and
`selection_ram_writes.tsv` observations. F10 never modifies native game
memory or speed. This change is diagnostic-only and does not fix the
Apocalypse stage itself.

## 87.479 — native selection output must never be skipped

## 87.479 — native selection output must never be skipped

87.477 turned on fast-forward as soon as P1's verified native
`FF5183` count reached two. The frontend then called
`port.machine->audio.clear()` and `continue` every UI frame
while `session::CompletingP1Selection()` was true. This created
the reported second-character freeze/last-frame jump and the
silent character-select music interval. The ROM kept advancing,
but the intermediate native audio/pictures were never presented.

87.479 keeps the FBNeo-proven FF5183 input-owner handoff while
removing second-selection fast-forward and its frontend AV gate.
P1's native mode/speed steps still auto-confirm at normal speed.
Training *startup* FF and Fight voice remain unchanged.

The 87.476 palette-27 OAM filter hid selector frames as well.
Remove the filter from both CPU/native renderers and disable
`hideP2Selector` in Training. Show the ROM's native flicker and
animations intact rather than indiscriminately hiding palette 27.
Both native cursors may be visible before handoff; future
P2-specific masking requires actual object identity rather than
palette alone.



## 87.478 — compile repair from local Windows build log

The 87.477 helper incorrectly created `cps2recomp::xmvsf`, shadowing
XMVSF's original GLOBAL `::xmvsf` namespace when compiling
`games/xmvsf/src/training.cpp`. Consequently `Player`, `kPlayer1`,
`kChoosing`, and dozens of other existing global XMVSF symbols became
unresolvable; the new callback was not itself the underlying failure.

Keep the FBNeo-derived `FF5183` accepted-count handoff unchanged. The
helper now extends the global `namespace xmvsf`, matching `xmvsf.h`.
The compiled selector smoke test includes a same-scope namespace lookup
regression to prevent silently reintroducing this exact C++ collision.
No ROM, gameplay state, or selection routing modifications.



## 87.477 — use the already-working FBNeo Training handoff

**Canonical source:** `dev/stable/buildtree/src/burner/sdl/xmvsf_stage17_auto.h`
on `main` (`XM87.68 ROM_P1_ACCEPTED`). It reads byte `0xFF5183`
and moves physical P1 control to P2 on accepted count **2**, without
waiting for a released button or inferred phase. The matching
`xmvsf_stage14_mode.h` routes SDL device 0 to game slot 1 and
does not duplicate the input into slot 0. Only already-held
face-button confirmations are blocked across the owner switch;
directions are never globally latched.

The uploaded recomp F10 session `xmvsf_f10_1795187918523_0`
independently confirms that exact FF5183 byte: **0 to 1 on
native frame 706**, **1 to 2 on native frame 763**. Both increments
coincide with the ROM advancing P1's selection record. 87.476's
phase-04/06 approximation is superseded. The on-screen F10 capture
and P2 OAM display suppression remain unchanged.

Recomp Training now reads `Ram8(kChoosing + 3u)` (compile-time
verified to be FF5183) for the early handoff; it does not rely
on two calls to `00B1E8`. Native auto-confirm/fast-forward
and the existing 240-frame safety fallback remain. Do not
claim runtime parity with FBNeo until a local Windows gameplay
test passes; the sources use different host input plumbing.



## 87.476 — Training handoff and cursor identified by native capture

The uploaded F10 session `xmvsf_f10_1795187918523_0`
establishes the actual P1 native selection transitions:
frame 705 P1 `01->02` (00B1E8 first-pick hook);
frame 737 `02->03`; **frame 762 `03->04` (second pick)**;
frame 803 `04->06`; frame 848 `06->07`.
The previous input transfer did not begin until frame 849,
**87 emulated frames after the actual second confirmation**.
The old counter remained at 1 because 00B1E8 did not run
for the second team pick.

Training now routes input to P2 at the ROM-observed P1 phase
04/06, fast-forwards the intervening native tail, and stops at
native P1 phase 07. Original auto-confirm and its 240-native-frame
fallback remain. No ROM or game RAM selection state is forced.

In the same capture, the native P2 cursor group uses CPS-2
OAM attribute palette `0x1B`. It moves as the P2 cursor moves
(frame 850-851) and vanishes at P2 acceptance (903-904).
CPU/native renderers suppress those OAM entries during Training
selection instead of guessing planar pixel pens. P1 objects and
other game modes are unaffected.

The selection RAM write-specific TSV was empty: translated 68000
code can bypass Machine::Write8/Write16. The full before/after
native record trace supports the transition timing, **not**
exact writer instruction provenance.



## 87.475 — F10 source assertion follows actual call site

87.474 refactored the F10 handler to cache `Capture::Get()` in a local
reference `f10`, then invoke `f10.Toggle(...)`. Both the old capture
preflight and the new status preflight still asserted that the previous
direct call expression existed. That was a false failure: the handler
still invokes Toggle. Both tests now check the local reference and
its actual Toggle invocation. No game logic or capture behavior changed.



## 87.474 — in-window recording indicator for F10

F10 had only a console log, leaving no way to see whether capture
was running during gameplay. The SDL3 2D compositor now draws a
high-contrast **red F10 REC + native frame count** in the upper-left
corner *after* the existing touch/noise overlay, without affecting
rendered native pixels or the ROM. Press F10 again to show a green
**F10 SAVED** notification for 3.5 seconds; a failed start shows
**F10 FAILED**. The same messages appear in the native window title,
even if a menu obscures the gameplay picture. Auto-stop at the
original 3,600 native frame limit also produces the SAVED notice.

The native capture files and game hooks from 87.472 are unchanged.
The status drawing executes only in the SDL3 two-dimensional
presenter (the default XMVSF classic renderer); alternate GPU/
OpenGL renderers still show the state in the window title.



## 87.473 — source-only F10 smoke test warning policy

The standalone g++ preflight previously used blanket `-Werror`, which
made unrelated unused-parameter warnings in the original Z80/M68K bus
headers fatal on MSYS2 UCRT64. It now adds
`-Wno-error=unused-parameter` after `-Werror`: warnings are visible, but
pre-existing stub parameters no longer stop the build. Actual C++ errors
and other warning categories are still fatal. The M68K capture, ROMs,
selection behavior, film strip, and original headers are untouched.


## 87.472 — F10 native M68K selection/OAM capture

Press **F10** on the character-select screen to START a capture, choose
both P1 characters and try the dummy selector, then press **F10** again
to STOP. Also auto-stops after 3,600 native frames (60 seconds at 60fps).
The console prints the full capture folder. Files are written to

`screenshots/f10_m68k/xmvsf_f10_<timestamp>_<sequence>/`

(the same screenshots root used by F6, or the folder named in
`XMVSF_M68K_CAPTURE_DIR`). One folder has `frames.tsv`,
`hooks.tsv`, `oam_delta.tsv`, `oam_writes.tsv`,
`selection_ram_writes.tsv` and `README.txt`.
Zip/upload that one folder for native phase and cursor ownership analysis.

- `frames.tsv`: **every real emulated frame**, including invisible
  fast-forward. P1/P2 selection phases and cursors before/after; both
  raw 0x20-byte selection records; four chosen-character words; input
  before Training redirects, immediately after its redirect, and
  as presented to the CPU; game phase, panel words, M68K PC/SR/cycles;
  the current Training handoff/auto-confirm flags and OAM bank.
- `hooks.tsv`: genuine entry to the original 68000 `00B1E8` hook,
  including PC, A4, D0, D1, cursor, phase and cycles. An *entry*
  does not itself prove that a character has been accepted.
- `oam_writes.tsv`: M68K object RAM writes, real bus address, bank,
  object slot, value, observed PC/A0/A4 and CPU cycles. For translated
  native code the observed PC is **not necessarily the exact writing
  instruction**, so writer ownership must be established from the
  sequence instead of invented.
- `selection_ram_writes.tsv`: actual M68K bus writes to both native
  selection records (`FF5180`/`FF51A0`), character picks, selection
  game steps and the raw P1/P2 panel words. Each write includes the
  observed CPU PC and register context; native translated-code PCs are
  explicitly labelled observational, not inferred writer addresses.
- `oam_delta.tsv`: initial raw next-frame CPS2 object list plus
  slot changes per native frame (X/Y/tile/attributes/palette and the
  object register offsets). No guesswork about palette-27 filtering;
  the trace shows actual objects emitted by the ROM.

F10 does not modify M68K behavior, hide sprites, inject input, or alter
Training selection. Observations in speculative run-ahead frames are
excluded from the capture; emulated fast-forward frames are included.
Existing 87.471 logic remains unchanged to make failures reproducible.

## 87.471 — actual hidden P2 cursor and zero-gap native handoff

87.470 handed the controls to P2 too early, before the ROM finished
the P1 mode/speed selection, and it never suppressed P2's visible
selector. 87.471 corrects both.

On native P1 character acceptance two (`00B1E8`) the remaining
P1 mode/speed selection runs in existing native FastForward slices,
with intermediate frames and audio unpresented. The source image
stays on screen until P1's choosing record really reaches
`kChoosingDone`; P1 then controls Dummy 1 immediately. A 240-frame
fallback avoids getting stuck in a differing ROM. P1's held
confirmation remains suppressed on the first dummy frame.

The **actual native P2 cursor**, and only its palette 27 outline
tiles, is suppressed in Training. A shared exact four-planar CPS2
graphics decoder checks the cursor's verified native pens 10-12
and rejects any tiles with other opaque ink. The same predicate
applies to the board CPU, native CPU and GPU renderers so graphics
are not guessed or hidden with screen rectangles, color-keying,
or unstable screen-pixel filters. P1's selector, portraits, stages
and gameplay are untouched. Normal Arcade/Versus are unaffected.
The source test also compiles and exercises the classifier locally.

## 87.470 — immediate native P1-to-dummy P2 takeover

Native XMVSF `00B1E8` confirms each character pick. After P1's
**second confirmed character**, Training hands directional/attack input to
dummy P2 immediately on the next emulated frame, with no wait for P1's
later mode/speed confirmation steps (native state `FF5180 >= 7`).
The held second-P1 confirmation is not repeated on Dummy 1.

Before P1 has accepted both characters, the P2 dummy input is gated to
prevent early picks. After handoff, the native P1 mode/speed confirmations
are sent as short real button pulses without overwriting RAM or skipping
native selection states, alongside the user's live P2 selection inputs.
A bounded 240-frame fallback restores manual P1 control if those native
steps do not advance, rather than trapping the selection process.

Only the input ownership and Training native choice hook change. This
does not yet add a new P2 cursor drawing overlay or modify original OAM
graphics. The 87.469 voice handoff, 87.468 select dither and film-strip
effects remain intact. The new local source preflight runs before Ninja.

## 87.469 — move "Fight!" voice to level confirmation

The one-second existing `audio/fight!.wav` now plays **once at the
Training stage-confirm edge**, instead of being triggered when the ROM
character-select screen is revealed. This uses its own SDL3 audio stream
rather than the menu music/movement/selection mixer: that stream clears
queued menu audio whenever the menu disappears, which previously made
long cues prone to an abrupt truncation.

The confirmed stage image stays on screen during the existing hidden
fast-forward boot. If native character select is ready before the voice
finishes, the game waits on the existing stage image for the remaining
WAV duration plus a short output-device tail. It then resumes game frames,
starts the character-selection reveal, and lets song 0x02 use the Ogg
mapping added in 87.468. No second "Fight!" event is fired at select.

If `fight!.wav` is unavailable or audio is muted, the hold is skipped
and the normal menu confirmation feedback remains available. A source
preflight verifies stage-confirm-only playback, stream separation, and
handoff ordering. Neither ROM files nor build caches are modified.

## 87.468 — native character selection noise-pixel reveal

Choosing a native Training stage captures the **actual last 960×540 stage-picker
frame** from the SDL3 texture renderer, including the moving character film
and selected pink/blue outline. The ROM still auto-boots invisibly (87.467).
When native character selection becomes active, the complete live game
picture is revealed **pixel by pixel over 580 ms**, by replacing opaque
pixels of that captured stage frame with new game pixels, using the
existing curl-noise and Bayer vocabulary. There is no black interstitial
screen, alpha blend, ROM tile filtering, or character-select input freeze.

The already-authored `audio/music/characterselect.ogg` is associated with
the ROM's existing player-select song `0x02` through
`audio/music/soundtrack.ini`, using the recomp's own Ogg decoder and
soundtrack playback. When a Training session starts, the local track is
enabled only if no other custom soundtrack was configured. Normal
Arcade/Versus restores the previous settings; unmatched songs play from
the board's normal QSound program.

The overlay is full physical-window size and keeps the stage menu's 16:9
letterbox in ultrawide, while native game rendering happens underneath.
Only the source and SDL3 visuals are changed. ROM folders, original music,
film-strip implementation, and local build cache are left unchanged.
## 87.467 — hide ROM disclaimer on Training entry

After selecting a native stage, the SDL3 frontend retains the last displayed
thumbnail selection frame while the existing native Training auto-boot runs in
fast-forward (up to 120 emulated frames per Port::Tick). The ROM's disclaimer,
title and credit screens are *not presented*. Training only becomes visible
once `modes::Choosing()` detects the actual character-selection state, not
merely the two-player join state. Once both players have joined, the boot
helper sends no extra Start inputs. Its existing 4,000-frame fallback still
prevents a stuck transition.

The guard does not affect regular Arcade/Versus, film strips, or stage assets.
Leaving Training cancels any unfinished auto-boot. Source-only regression
checks run before Ninja; the ROM directory and build cache are never modified
by this patch.

## 87.466 — film-strip and Training stage selector unified

This is the integration branch: all 87.465 film-strip work remains (animated
panels, rainbow backing, curl-noise top/bottom dither, audible song entry,
SDL3/F6 native captures, controller fixes and their local tests).

Selecting TRAINING now darkens the *same animated film background*, without
a backplate, and displays 150×150 nearest-neighbor thumbnails in a 5×2 grid.
The flickering pink/blue pixel outline follows D-pad/arrows; A/Enter selects;
B/Esc returns; LB/RB or Q/E changes the 10-item pages.

All original stage thumbnails are mapped explicitly from indexed
`_art/stagethumbs/0.png` through `10.png`, followed by the 7 named
`_art/stagethumbs/xmvsf2/` files (Wily, Final Destination, Guile, Sigma,
Akuma, Ryu, Boomer). They were already present on this branch and all
18 are required by the local build preflight. No image re-export occurs.

Native stage choices set `session::kStageFeature` temporarily, then call
`session::StartSession`; the existing M68K hook at `0x00BAEA` selects native
stage IDs 0..10 via `0xFF5113`. Leaving Training for Arcade or Versus
restores the prior stage setting. The seven XMVSF2 thumbnails display but
are intentionally non-playable until their stage renderers are ported to
recomp; this avoids incorrectly selecting guest stages.

F6 captures also support the selector in the existing film/capture pipeline.
The original tests plus stage-picker source, image, hook and idempotence
checks run locally through `RECOMP_BUILD.bat`. No Actions or ROM changes;
`main` is left untouched until the user merges this branch.

## 87.464 — animated entrance dither at both screen edges

The scrolling character panels emerge from the **top and bottom edges**
with about **20% visible ordered-dither pixels at the first screen row**.
The mask gradually opens across each outer **108px** (20% of the
960×540 canvas height) and is fully open across the middle.

The effect uses the perpendicular gradient (curl) of an advected,
two-octave smoothly interpolated noise field. Its motion follows
each film column's scrolling direction, with a stable per-panel
ordinal seed: no random per-frame jumps or reset when panels recycle.
The moving 8×8 Bayer pattern is applied to **both** the colored
rainbow backing and the full-opacity character silhouettes; no
extra opacity multiplier is applied to the characters.

Each slot reuses cached SDL3 textures, now including a dynamic masked
portrait. F6's same-timestamp offscreen capture and visible rendering
reuse the cached mask; all textures are released at menu shutdown.
The standalone C++ edge helper is regression-tested during the local
build preflight, and the installer syncs it to `recomp/port`.
Existing rainbow color mix, 60% film alpha, 2x blob tiles, scrolling,
menu text reveal, and one-time music entrance remain unchanged.

## 87.463 — soft menu music entrance, never on song loops

## 87.463 — soft menu music entrance, never on song loops

The menu theme `xmvsfMenu.wav` still enters at the existing original
menu music cue. Its initial gain is **34%** (audible immediately),
then increases with a smoothstep curve to the existing **72%**
target over **280ms**. The envelope is evaluated from actual PCM sample
counts, avoiding irregularities from window redraws and F6 captures.

The envelope saturates after one initial entrance. Subsequent song
loops and reentering the menu after gameplay immediately retain the
normal 72% music gain. The menu movement/select cues, initial
crash/voice cues, and audio queue limits are unaffected.

## 87.462 — translucent film, stronger team colors, larger rainbow blobs

## 87.462 — translucent film, stronger team colors, larger rainbow blobs

Only the **red/blue panel backing** is made translucent: it renders at
**60% opacity**, allowing the MVC meteor animation to remain visible
through the film. Each **character portrait stays at its original
opacity** on top, with its existing trapezoid clipping mask and
post-text Bayer reveal preserved.

The background color blend now favors the team color at **75% red/blue**
and **25% rainbow** (at most, where rainbow pixels are fully opaque).
The `rainbowblobs.png` tiles sample at **2× their previous size**, with
the wrap period doubled in X and Y, preserving seamless directional
panning and independent per-panel hue cycling. The fallback red/blue
backdrop uses the same 60% opacity if the rainbow image is missing.

Previous 389-pixel panel spacing, 20px/sec scrolling, character shuffle,
and native F6 screenshot destination remain unchanged.

## 87.461 — living rainbow film strip backing

## 87.461 — living rainbow film strip backing

The home film columns now load `_art/menu/rainbowblobs.png`.
For each visible 260×453 trapezoid, the original red/blue backing
contributes approximately **50%** of its color and an independently
moving, hue-rotating tile of `rainbowblobs` contributes the other
**50%**. The original backing's alpha is the final mask: the rainbow
never draws outside the trapezoid. Portraits are still fully visible
above the background blend.

Each physically scrolling panel receives a stable hashed motion
direction, pan speed, hue speed, and starting hue. RGB hues cycle
continuously. The tiles wrap on both axes. Animated panel textures are
cached and refreshed at a 75ms cadence (or on Bayer reveal changes),
without creating or destroying textures each frame. F6's offscreen and
visible draws reuse each panel texture. The previously established
80px-closer spacing, 2x scroll speed, random character order, and
post-text dither remain unchanged.

The file is a required authored runtime image, checked by the film
preflight. If it cannot be loaded at runtime, the original red/blue
backing is rendered instead. No ROM data or generated translation is
changed.

## 87.460 — film strips: tighter, faster, and sequenced after text

## 87.460 — film strips: tighter, faster, and sequenced after text

Both 260×453 film columns are spaced **80 source pixels closer**
than before. Their new 389px pitch overlaps tapered panels by 64px.
The original 16px horizontal edge margin is unchanged. Scrolling is
**20px/sec** instead of 10px/sec; X-Men continue downward and
Street Fighter upward.

At load time, each column shuffles the nine characters into a random
non-repeating order, which stays stable during frames and cycles.
The film backing and portraits remain completely hidden until the main
menu text finishes its 500ms Bayer reveal. They then dither in with
the **same 4×4 source-pixel Bayer pattern** over a subsequent 500ms.
The normal renderer updates texture pixels only when the dither step
changes and restores original alpha after the intro. No changes to
native F6 screenshot capture or screenshot destination.

## 87.459 — single F6 screenshot destination

## 87.459 — single F6 screenshot destination

Run `RECOMP_RUN.bat`: it immediately creates
`F:\\xmvsf-recomp\\screenshots\\` and supplies that absolute folder to
the game as `XMVSF_SCREENSHOT_DIR`. Press **F6** on the home menu,
Options, the in-game menu, or during gameplay. All F6 PNGs go into
this **one repository-root folder**, never the player's
`recomp/work/xmvsf/` folder.

The program prints the capture folder at startup and logs an F6
request and the saved PNG path (or a failure). Gameplay F6 takes the
native pixels directly from `Port::Shown()` and saves through the same
writer as every menu. F12 retains its original screenshot destination.
When launching manually without the batch file, F6 defaults to
`../screenshots/` relative to the game's working directory.

## 87.458 — F6 native-resolution screenshot capture

## 87.458 — F6 capture from every screen

Press **F6** once to save the currently visible menu or game state as a
PNG in the **repository-root screenshots/** folder (different from F12). The exact path and pixel dimensions are printed in the console.

- **Home / film strip menu:** captures the actual SDL3 compositor to a
  960×540 offscreen target, including the scrolling character panels,
  logo, background, and selections, without window scaling.
- **Game / Graphics / Controller Options:** captures the real options
  renderer at 960×540, including the selected row and active remapping UI.
- **In-game game-settings menu:** saves its original `menuPicture`
  pixel buffer at its generated dimensions (commonly 896×672), with no
  postprocessing or desktop pixels.
- **Gameplay:** uses the native `Port::Shown()` pixels and the same
  root-folder writer as the menus (`ShownWidth() × 224`, ordinarily 384×224).
- **F6** is no longer previous save slot. **F7** is next slot and
  **Shift+F7** is previous; **F12** keeps its gameplay-only capture.

Screenshots do not affect ROMs, game saves, or existing build caches. Home
and Options captures render once into a native offscreen SDL texture on
the requested frame, then display the usual frame normally. The local
`test_native_capture.py` verifies all state hooks before Ninja.

Windows SDL3 rendering still needs a local runtime check; these are source
preflight assertions, not evidence of an executable test pass.

Python installers, version checks, and local build requirements are
specified in [PYTHON_PATCHING_RULES.md](PYTHON_PATCHING_RULES.md).

## 87.457 — 16-pixel gaps and head-safe portrait fitting

Panel spacing now matches the 16-pixel menu side margin. The 260×453
trapezoids advance every 469 pixels. Every character is fitted from its
visible alpha bounds into the film's full-alpha safe interior with eight
pixels of inner padding. Contain-fit avoids the previous cover-crop
that hid character heads against the sloped film edges. The original
film alpha still clips the resulting portrait. Both columns share the
same path; scroll directions are unchanged.

## 87.456 — separate film and portrait layers; 96-pixel gaps

The 260×453 film pieces now repeat every 549 vertical pixels, leaving a
full 96-pixel clear margin between consecutive shapes. The red and blue
trapezoid pixels render first; portraits then render as separately masked,
nearest-neighbor SDL textures. The mask is sourced directly from each film
PNG's alpha, and portraits begin 78 pixels below the top of the complete
source panel. Original character RGBA channel order is preserved. X-Men
scroll downward on the left and Street Fighter upward on the right.
Only the local renderer and film preflight changed; no ROM or build-cache
changes are required.

## 87.455 — initial looping menu film

The left X-Men and right Street Fighter trapezoid films are prepared
from `_art/menu` once per SDL renderer, then scrolled independently.
Feature preflights run before Ninja; a passing source test is not a
substitute for an actual Windows build and in-game inspection.

The real XMVSF front end now renders in CPS-2 Recomp's **SDL3 native
window and pixel presenter**, not an additional SDL launcher. The preserved
FBNeo reference in `dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h`
is the source of the composition: a 960x540 logical canvas, original
logo alpha-crop, proportional native SF2 glyphs (large and small),
selected white / unselected dim, centered rows and unchanged menu positions.

**Source art is NOT exported again:** `_art/logo.png`, `_art/logo_II.png`,
`_art/sf2font.png`, `_art/sf2fontsmall.png` and `_art/menuanimation.gif` are
loaded from the repository's existing `_art/` folder at runtime. The
animated GIF retains its frame timing, center-cover crop, 30% brightness,
and 36-second hue rotation. Source images stay untouched; only in-memory
pictures are animated. Fallback text exists if image assets cannot be found.

The menu is shown in the recomp's **existing window**, on startup and via
Esc / gamepad menu input; while shown, no game ticks are executed.

## Current actions

- **ARCADE:** native V2 arcade mode, with its original rules (reset).
- **TRAINING:** starts the existing recomp training session; custom training
  configuration still awaits porting.
- **VERSUS:** initial *local arcade two-player* mode with free play (both
  players join with Start); our complete dedicated versus setup is not yet
  restored.
- **OPTIONS:** opens our native Game / Graphics / Controller tabs, drawn
  with the 87.441 source-texture renderer. Supported settings and controls
  update real recomp features, windows and input bindings. Unported legacy
  features are clearly inactive, not fake toggles.

Navigation uses both keyboard and gamepad: arrows / D-pad, Enter or Space / A.
Esc returns to the home menu, and F1 there opens our Options. Shift+F1 opens the original recomp diagnostics.
Use `XMVSF_RECOMP_HOME_OFF=1` when comparing against untouched upstream.

## 87.454 — Roman II uses the main logo's source-pixel pulse
The final `_art/logo_II.png` is now recolored using the same
`XMMenuColor::logoPalettePulse` routine and SDL tick clock as the main
title. It keeps immutable original RGBA pixels, updates the streaming
texture at most once per 80 ms, and preserves the already-established
nearest-neighbor sampling. Stroke A/B/C/D reveal geometry, masks and
spark particles are unchanged. The source preflight guards the new
palette call, renderer clock, texture upload and cleanup.

## 87.453 — nearest-neighbor filtering for all II textures
The final II numeral and all four stroke textures now explicitly use
`SDL_SCALEMODE_NEAREST` in their shared SDL3 upload helper, matching the
main logo pixel filter. The logo preflight checks this exact helper and both
callers, so a future regression fails before compilation.

## 87.452 — original Stage 87.235 logo reveal and ongoing palette pulsation

The native recomp startup now uses the **original SDL2/FBNeo source timing**,
adapted to existing SDL3 textures and the current one-stream menu mixer. All
artwork remains in `_art/` and both sound cues remain in `audio/`; no
ROMs, generated M68k files, or upstream submodule commits are changed.

| Cold-boot time | Source behavior |
| --- | --- |
| 0 ms | White/spectral background strobe; play `crash.wav` and `xmvsf_welcome.wav` together |
| 1,000 ms | Base title appears and resolves from white/hue dither over 1,000 ms |
| 2,000 ms | Four source-aligned `logo_II_A/B/C/D.png` strokes, 10 frames each at 60 Hz, with tip sparks |
| 2,667 ms | Finished II logo, original menu music starts, SF2 font menu reveals over 500 ms |
| 3,167 ms | All menu inputs unlocked; steady animated backdrop and logo effects remain |

After the reveal, the **original color-selective pulses persist**: orange,
yellow, and green breathing; purple source-pixel vapor/dither and short
vibration bursts affecting the VS center and extended shadow. This replaces
the recomp's former uniform brightness sine wave. All color transformations
start from original decoded PNGs; they never accumulate over prior effects.

The reveal runs **once on cold boot**, not every time the player returns from
combat. The original authored II stroke exports share the 400×300 pixel
canvas and retain their direction and alignment. Recomp GPU textures and
the original Settings/Controller page, gameplay logic, and input
configuration remain independent. The CPU fallback remains available as
before, but the complete effects run through the normal accelerated renderer.

Source ports: `xmvsf_intro_timeline.h`, `xmvsf_palette_fx.h`,
`xmvsf_ii_strikes.h`, plus `xmvsf_menu_audio.h` and
`xmvsf_menu_textures.h`. `apply_menu.py` copies the new headers to
the local `recomp/port` during normal builds. The build script runs
`tests/test_logo_intro.py` **before Ninja**, checking assets, SDL3 source
hooks, compiled C++ timing assertions, and helper installation. This is
a local preflight, **not evidence that the Windows SDL3 binary or audio
has already been tested**. Continue using `RECOMP_BUILD.bat` (warm
incremental Ninja; no `refresh` needed for these source-only edits)
then `RECOMP_RUN.bat`.

## 87.451 Correct the capture activator/deactivator state

Selecting a Controller binding with A or Enter used to set capture=true
and ignore EVERY input for a fixed 180 ms. A legitimate button pressed
quickly was discarded forever, with no timeout, no controller cancellation
and no interruption recovery. The old SDL2 Stage14 version instead used
a release gate between selection and capture.

87.451 adds a small, separately C++-tested capture lifecycle:
- Capture records the actual held Enter, Space, mapped gamepad A, or raw
  joystick confirm button that opened assignment, and ignores only that
  physical initiating press. A DIFFERENT button/key can be assigned
  immediately; no 180 ms discard window.
- Release of the activating key/button makes A itself bindable on the
  NEXT press. Residual analog/hat movement is ignored until A releases.
- Escape, or controller Guide when SDL delivers it, cancels capture.
  Gamepad B remains assignable as an attack button; it is NOT stolen
  as a cancel control.
- Capture times out after 12 seconds. Loss of window focus or physical
  device removal cancels capture instead of leaving it stranded.
- Save success, save failure and cancellation transition to a menu
  rearm phase. Confirm/Back must be released to navigate again; even
  an SDL driver reporting Confirm held forever cannot lock the page
  longer than 1.5 seconds. The original menu one-press edge gate stays.
- Additional SDL joystick and semantic gamepad events for a single
  physical press are suppressed while rearming. Unsupported semantic
  buttons report a message rather than silently appearing stuck.

`tests/test_capture_lifecycle.py` executes a real tiny C++ test using
the MSYS2 UCRT64 g++ compiler before the controller patch or Ninja build,
with checks for quick presses, release/repress, timeout, cancel,
stuck-confirm recovery and resetting on tab/menu transitions.
The new lifecycle header is installed by the same idempotent Options
installer; no extra source-translator run is needed.

The 87.441 GPU menu renderer and 87.447 audio path are untouched.

## 87.450 Repair Python regex and execute the installer before writing

The 87.449 idempotence repair introduced an unbalanced Python regex:
`r"gamepads(\\s*\\)\\s*;"`. It opened a capture group and never
closed it, so `re.compile` raised `unterminated subpattern at position
177` before Ninja could start. The exact fixed capture is
`r"gamepads(\\s*\\)\\s*;)"`. It preserves group 1 (the call's
argument prefix) and group 2 (the closing parenthesis/semicolon).

The real regex has been executed with Python against old/new
`homeTextures.RenderOptions` calls, including repeat matching and
unrelated call tails.

**Build change:** `tests/test_pad_owners_preflight.py` now runs
before `apply_pad_owners.py` and reads the already generated
`recomp/port/frontend_sdl.cpp`. It executes the real controller
patch in memory, then executes it again against its own output.
Both calls must succeed and the second must be identical.
No controller-file writes occur until this preflight passes.

The 87.441 source-texture renderer, 87.447 audio fix and Stage13
controller input behavior are unchanged. The first Windows compilation
still needs to occur locally; Python regex/preflight verification
does not constitute a successful C++ build.

## 87.449 Make the controller installer safe on repeat builds

The 87.448 build successfully installed the final Stage13 input capture
frontend, but tests/test_pad_owners.py then called the installer a
second time to verify idempotence. The original 87.443 gameplay-loop
replacement no longer appeared because the later 87.448 physical
routing upgrade had already transformed it. The installer incorrectly
rejected this fully installed state with "route native game actions by
owned controller ... found 0", stopping before Ninja.

The installer now validates the **final** 87.448 hooks first and
returns the source unchanged when already installed. Only the uniquely
identified Options RenderOptions call can be reconciled in that state.
The historical patch path remains available for first installations and
older frontend upgrades. Incomplete or ambiguous source fails closed.

The regression test now includes a complete-final-source fixture,
a second invocation, a missing-marker fixture, and the existing
duplicate-render-tail fixture. This is a source-only change, retaining
the 87.441 renderer, 87.447 audio fix, saved controller profiles, and
warm incremental compilation. No submodule reset or cache clearing.

## 87.448 Restore the original SDL2 Stage13 controller assignment UX

The temporary 87.442/87.443 device-cycling and EDIT INPUT switch are
removed. Source of truth:
`dev/stable/buildtree/src/burner/sdl/xmvsf_stage13_controls.h`
and `xmvsf_stage14_boot.h` (Controller tab, capture, graphics/layout).

**Device row (row zero):** press A/Enter to arm capture; press a key
to assign the original P1/P2 keyboard profile, or press a physical
button on any connected stick/gamepad to assign that exact SDL instance.
If the device was assigned to the other player, the whole player
configuration and its customized physical binding profile are swapped.
No cycling between KEYBOARD and GAMEPAD modes.

**Action rows:** after assignment press A/Enter on LP, MP, HP, LK,
MK, HK, 3P, 3K, COIN or START to capture the next key, physical
button or positive joystick axis. Keyboard mode exposes 14 actions,
including LEFT, RIGHT, UP and DOWN; controller mode exposes the first
10, with directional input taken from its stick/hat. The raw physical
index is displayed as BUTTON N, AXIS N or KEY name, not a semantic
Xbox button label. The original control column is dynamically centered,
with 12 native pixels between the device caption and first input.

Keyboard defaults are copied from Stage13:
P1 = J K L / U I O / H Y / 5 Enter / A D W S.
P2 = KP1 KP2 KP3 / KP4 KP5 KP6 / KP7 KP8 / KP0 KP Enter /
Left Right Up Down.

Both mapped SDL3 gamepads and unmapped raw sticks use physical button
bindings (SDL_GetGamepadBindings resolves a mapped pad to joystick
indices); existing 87.443 semantic profiles migrate when available.
Persisted GUID, name, ordinal, serial (if exposed) and saved per-device
action profiles remain in the existing recomp settings.ini. Reconnection
uses live SDL instance first and GUID/name fallback. The SDL3 game input
loop now avoids mixing the generic gamepad/keyboard mappings with an
explicitly assigned controller's physical profile.

The stable 87.441 SDL source-texture renderer, 87.447 single menu audio
stream, and 87.443 one-press menu gating are unchanged.

## 87.447 Fix buzzing on entry into Options

The historical installers could leave **both** audio visibility updates in
the same frontend frame:

- 87.437: `homeAudio.Step(home.Visible())`, false when Options is open
- 87.442: `homeAudio.Step(home.Visible() || options.Active())`, true in Options

Since `MenuAudio::Step` clears its SDL audio queue and rewinds playback on
a visibility transition, the simultaneous false/true calls repeatedly
restart the music and produce an audible buzz when opening Options.

The 87.447 Options installer now removes only the old **stamped** 87.437
statement, collapses duplicate 87.442 stream updates, and preserves exactly
one 87.442 update in the active menu loop. It also handles an existing
87.443 controller patch without rebuilding the pinned submodule from scratch.
The pre-build test verifies both synthetic conflicting hook scenarios and
the actual generated frontend contains one 87.442 update and zero 87.437
updates. The 87.441 source-texture renderer, menu WAV files and 87.443
controller assignment logic are unchanged.

Expected repair diagnostic (when historical hooks are present):
`XMVSF 87.447: removed N conflicting legacy audio steps ...`

No translation refresh or GitHub Actions; incremental Ninja compilation
follows the existing source-only checks.

## 87.446 recover repeated Options/audio hooks

Repeated failed 87.443–87.445 builds exposed an installer lifecycle bug:
the 87.437 upgrade kept reinserting its earlier audio-step hook after
87.442 had already replaced that hook. The 87.442 installer then inserted
another complete native Options draw block before the unchanged home
rendering block. Repeated runs could accumulate six copies.

The 87.446 build now:
- Suppresses the old 87.437 upgrade when 87.442 Options already exists.
- Makes the 87.442 Options installer skip reinsertion when its installed
  include, controller and drawing hooks are already present.
- Recovers *only* complete identical 87.442-generated audio/draw blocks
  to one, with a safe alternative for interleaved generated audio hooks.
- Refuses cleanup if any duplicate Options block differs from the known
  emitted source. No Git reset, submodule rewrite, cache deletion, or
  change to the confirmed 87.441 source-texture presentation.
- Validates six-copy recovery, interleaved audio, source protection, and
  second-run idempotence via tests/test_options.py before local Ninja.

The 87.443 ownership installer should now find exactly one active
Options rendering invocation. The actual Windows compilation and hardware
capture still require verification on the user's system.

## 87.443 strict input edges and physical controller profiles

No hold-repeat in the home menu, Options, or upstream diagnostics. One
key/button press moves one row, then requires release before the next
move. The native gameplay button-hold and charge logic are unchanged.

The Controller tab uses press-to-capture on the single DEVICE row, then
10 physical actions (or 14 Keyboard actions). Each P1/P2 slot can select
a particular connected controller by SDL instance ID and save its GUID,
serial when available, ordinal, and the actual Windows/SDL joystick
name. Reconnecting restores that controller's saved action profile
without silently shifting another controller into its player slot.

The name displayed is obtained from SDL_GetJoystickName first, as in
our working SDL2 setup (so SDL's reported Fighter PI appears as such).
Unmapped raw joysticks can be explicitly assigned and their raw
buttons, hats and axes captured. Standard mapped gamepads retain the
recomp's per-button and trigger handling. Unclaimed devices still
support additional P3/P4 recomp co-op roles.

The device identity and mapping profiles are persisted through
recomp::Ini in the existing work/xmvsf/settings.ini; existing keyboard
and gamepad mappings stay valid. Identical GUID devices are protected
by retaining both live player instances before reconnect matching.

See PAD_OWNERS_87_443.md for implementation, exact profile rules and
limits. The source patch and regression checks are committed but the
Windows build and physical Fighter PI detection must still be tested.

## 87.442 native Game / Graphics / Controller Options

The old generic recomp menu has been removed from the **normal Options
navigation**. The in-game screen now uses the original tab organization,
navigation conventions and SF2 bitmap fonts on the existing accelerated
960x540 renderer. Press F1 or choose OPTIONS; use Q/E, Tab/Shift+Tab
or LB/RB to cycle tabs. Esc/B returns to the home menu.

- **GAME:** GAME VERSION displays V2; Tournament, Double Feature and
  Dizzy Bar are visibly NOT PORTED and have no fake implementation.
  Free Play uses the real SetFeature hook. Difficulty and game speed
  use the native board features and persist until next board reset.
- **GRAPHICS:** six original size presets, fullscreen fit and maximize
  operate on the actual SDL3 window; Ctrl sizing shortcuts remain.
- **CONTROLLER:** P1/P2 selection, keyboard/gamepad editing, original
  LP/MP/HP/LK/MK/HK/3P/3K/COIN/START/LEFT/RIGHT/UP/DOWN order, direct
  SDL scancode/gamepad button/trigger capture, live g_bindings updates,
  conflict cleanup and persistent settings.ini through recomp::Ini.
  The connected SDL gamepad names appear where available. Physical pad
  ownership by durable GUID is not yet implemented; the recomp still
  assigns pads by connected order.

Only Shift+F1 from the home menu retains the upstream settings/diagnostic
view for engineering purposes. See OPTIONS_87_442.md for exact hooks,
limits and controls. No changes to 87.441's hardware renderer or original
Stable and main branches.

## 87.441 true SDL source-texture menu

Our older 87.435-87.440 recomp menu was CPU-composited: a 960x540
software buffer was rebuilt every presentation frame, including the
GIF, logo, and font glyphs, then uploaded as one image. The working
SDL2 menu instead draws separate textures directly through an accelerated
renderer. Version 87.441 now follows that original behavior through SDL3,
using the same PNG/GIF assets and 960x540 logical canvas.

The new compositor creates a texture for the logo, II, two font atlases,
and GIF backdrop; only the GIF's changed source frame/hue is uploaded.
No full-menu CPU compositor is used normally. On Windows, Direct3D11
is preferred for the SDL 2D backend, with a fallback to SDL default.
The native gameplay video path is unchanged.

On shutdown, menu textures and the menu WAV mixer are explicitly
released before the renderer/window and SDL_Quit, fixing the old
resource-lifetime mismatch.

Diagnostic switches: XMVSF_MENU_CPU_FALLBACK=1 for old CPU menu, or
XMVSF_RECOMP_CLASSIC_OFF=1 for upstream GPU output. Neither needs a
recompile. See MENU_TEXTURES_87_441.md.

## 87.440 SDL2-style renderer port

The XMVSF game and native menu now use SDL3's accelerated 2D
SDL_Renderer path with the old SDL2 window/rendering policy: nearest
texture sampling, SDL-managed centered 960x540 logical menu, and one
persistent OS window. The custom recomp GPU shader and its linear
edge-blending pass are bypassed for interactive XMVSF.

This does NOT force square pixels or change existing Game/Display
settings, menu layout, artwork, M68000 translation, or window presets.
The native game is still shown with its original FitPicture rectangle.

The port is installed by recomp_port/apply_classic_renderer.py, after
the existing menu installer and before incremental Ninja, and has a
source-only regression under recomp_port/tests/test_classic_renderer.py.
See CLASSIC_RENDERER_87_440.md for details. Set
XMVSF_RECOMP_CLASSIC_OFF=1 at runtime to disable the bridge and restore
upstream behavior for diagnostics.

## 87.439 restoration of original display behavior

87.438 was retracted because its integer-scale/menu downsample changes
altered the intended appearance. **87.439 restores the complete 87.437
drawing/window/presentation source** and changes only the visible version
number. This keeps:

- Original 87.437 physical scaling presets, SDL3 presenter defaults,
  fractional menu fitting, and SDL/VSync behavior.
- The source artwork, PCM menu music, menu movement/selection sounds,
  and controller input.
- The pinned recomp engine, warm Ninja cache, local build scripts, and
  the original FBNeo Stable/TEST builds.

Since `recomp/` is a pinned submodule with locally applied generated
patches, `RECOMP_BUILD.bat` automatically detects and **reverses exactly
the six 87.438 frontend changes** already present in a local checkout
before reapplying the known 87.437 menu integration. An unknown/partly
modified source refuses the rollback without writing a partial file;
there is no `git reset --hard` or removal of ROMs or object caches.

**The rapid compilation is a Ninja/build-cache improvement and is unaffected
by this presentation rollback.** The actual cause of sluggish OS dragging
under SDL3 remains unproven and requires separate profiling rather than
guessing from the size of the source image.

## Window controls (87.437)

While the game or our menu has focus:

- **Ctrl++ / Ctrl+= / Ctrl+keypad plus** advances to the next window-size preset.
- **Ctrl+- / Ctrl+keypad minus** steps to the preceding size.
- **Ctrl+Backspace** toggles maximize/restore without switching to fullscreen.

The six original Stage29 presets remain 384x224 through 2304x1344. Only
the SDL3 window's physical size changes; the game stays in its original
native resolution and the authored 960x540 menu stays fitted/letterboxed.
Scaling keys are ignored while already fullscreen.

## Original menu audio (87.437)

Load WAVs directly from `FBNeo/assets/xmvsf/`, once when the normal native
menu initializes (or the folder named by `XMVSF_MENU_AUDIO`):

- `xmvsfMenu.wav` — loop continuously **only while our home menu is visible**.
- `menu_movement1.wav` — each changed navigation row.
- `menu_selection1.wav` — confirmed menu choice.

One independent SDL3 audio stream converts sources to stereo 48kHz S16
once and mixes with bounded buffering. When a menu selection enters
gameplay, music and queued buffers stop immediately; the selection effect
can finish naturally. No FFNeo sound code and no SDL_mixer dependency.
Missing WAVs are logged and don't prevent gameplay.

## Building and storage

Upstream source is still pinned under the `recomp/` Git submodule. This repo
owns **only** `recomp_port/xmvsf_home.h`, the pinned image decoder under
`recomp_port/vendor/stb_image.h`, and the deterministic
`recomp_port/apply_menu.py` installer. The installer copies those two
headers into the submodule working tree and updates only the known menu
hooks in `recomp/port/frontend_sdl.cpp`; it is idempotent and fails closed
when upstream anchors differ. The upgraded 87.435 menu is handled without
resetting the submodule, so local translated code, ROMs, and build caches remain.

Image decoding uses stb_image v2.32 (upstream nothings/stb, public domain/MIT)
for **PNG and animated GIF**; it is compiled into one translation unit.
No SDL3_image or new runtime DLL is required.

Pull only this branch into your separate worktree:

```powershell
git -C F:\xmvsf-recomp fetch origin feature/recomp-v2-foundation
git -C F:\xmvsf-recomp merge --ff-only FETCH_HEAD
F:\xmvsf-recomp\RECOMP_BUILD.bat
F:\xmvsf-recomp\RECOMP_RUN.bat
```

For a fast patch sanity test (no Windows compilation required):
`python3 recomp_port/tests/check_patch.py`. The actual SDL3 executable must
still be built locally; this commit has not been verified by a Windows build.

No GitHub Actions. No edits to `main`, Stable, FBNeo builds, or ROM binaries.

## Remaining presentation work

Source-authored full intro/reveal timing;
durable pad-identity assignment, unported mode toggles, and our dedicated
Versus/Training lifecycle. These should be ported as **separate functional
modules**, not more version-specific hooks.
