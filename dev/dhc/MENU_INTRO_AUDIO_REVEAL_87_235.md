# TEST 87.235: Cold-boot reveal and three authored WAVs

Branch: `feature/menu-intro-reveal-audio-87-235`, descended from 87.234. No main merge, GitHub Actions, source deletions, or Stable changes.

## First launch timeline

- 0.0–3.0 seconds: start PURE WHITE (including the navy canvas behind the GIF). Its native animated `_art/menuanimation.gif` pixels flash white, then fast hue-strobe with source-space Bayer dither over **three seconds**, settling to the exact original GIF colors and normal 30% brightness/36-second hue cycle. This visual fade **overlaps the title reveal from t=1.0 through t=2.0**. `audio/crash.wav` and `audio/xmvsf_welcome.wav` both begin at boot (t=0), giving the spoken WAV a one-second head start before the logo enters.
- 1.0–2.0 seconds: logo (original `_art/logo.png`) appears PURE WHITE. It hue-strobes downward while fixed source-pixel Bayer dither progressively releases the original colors and existing authored logo palette animation. The already-playing `audio/xmvsf_welcome.wav` accompanies the logo entrance at t=1.0; its natural spoken lead-in is not delayed until after the logo appears. The logo reveal lasts **1.0 second**; the WAV is never sped up or truncated if its natural duration differs.
- **2.000–2.667 seconds:** independently animate `_art/logo_II_A.png`, `_art/logo_II_B.png`, `_art/logo_II_C.png`, and `_art/logo_II_D.png` in that exact sequence at 60 Hz / ten source-mask frames each (40 frames, 667 ms total). **A up-right, B up, C down, D up-right**. The leading edge of each authored shape is computed from its nontransparent pixels, Bayer-dithered at the boundary, and emits a small colored/white particle spray that flies off the actual drawing tip. At the end use the complete `_art/logo_II.png` for a pixel-exact settled numeral. All five exports are aligned with the base logo's 400×300 source canvas and share its placement scale.
- **2.667–3.167 seconds:** only AFTER the final II strike, menu glyphs (the original `_art/sf2font.png` / `_art/sf2fontsmall.png`) appear using 16-stage spatial Bayer alpha dither. **Menu music starts at this boundary**, beginning at sample zero and fading to its normal full menu gain (1.0) over the same 0.5 seconds. No additional gain reduction during the announcer tail.
- At **t=3.167 seconds:** menu input unlocks. Already-held buttons must release before they can select. `SDL_QUIT` still closes the window at any time.
- Game→menu return and automated F9 replay bypass the cinematic; the normal menu is immediately usable.

## Fight cue

Selecting **Arcade**, **Training**, or **Versus** on the main menu plays `audio/fight!.wav` in place of the normal confirmation sound. Training and Versus still use their existing setup/stage selection screens. There is no delay before handing Arcade to the native emulation. The one-shot survives menu→game transition in the original persistent WASAPI mixer, which also owns game PCM and menu music. The music ducks while the authored WAVs play; no new SDL audio device, engine tempo change, or native ROM control adjustment. The 3.75-second crash and 4.406-second welcome WAVs are both preserved at natural speed, so the welcome tail continues after the logo reveal.

The three WAVs are copied from main into this branch as identical Git blobs at `audio/crash.wav`, `audio/xmvsf_welcome.wav`, and `audio/fight!.wav`. Asset resolution is relative to the executable: `runtime/stable/../../audio`.

## Full-title centering

The completed `logo_II.png` alpha bounds are unioned with `logo.png`'s visible bounds in the shared 400×300 source canvas. Both draw calls use the same translated canvas, centered on x=480 of the 960-wide logical display. The base art fit/scale and the four independent II stroke trajectories remain unchanged; the menu glyph layout remains unchanged.

## Options / controller UI

- Main menu: **ARCADE / TRAINING / VERSUS / OPTIONS**. Controller assignment no longer has a separate main-menu entry.
- Options has three tabs: **Game** (Game Mode, Double Feature, Dizzy Bar), **Graphics** (six window presets, Fullscreen Fit, Maximize Window), and **Controller** (one device selector plus fourteen mappings per player). The top line is exactly `LB/Q   Game   Graphics   Controller   RB/E`, small glyphs, dark-tan bumper/keyboard labels, active tab white. Explanatory/noninteractive text uses desaturated blue.
- LB/RB or Q/E/Tab changes tabs. Left/right changes the selected tab on Game/Graphics; on Controller it pages between P1 and P2. Up/down chooses among fifteen compact rows, centered between the upper left/right page caption and the lower P1/P2 selector. A/Enter starts capture, B/Escape returns.
- The first Controller field shows the live SDL/OS-reported hardware name (from `SDL_JoystickNameForIndex`, with game-controller fallback) instead of generic `PAD N`. Input settings now persist in `XMVSF_CONTROLLERS_3` records with **both** hardware GUID/occurrence and a hex-encoded system device name/name occurrence. On reconnect, exact GUID matching wins; when a driver changes the GUID, an exact saved name+ordinal can recover the bindings. A separate set of up to 24 saved device profiles keeps mappings when switching between devices. Multiple identically named pads retain distinct ordinals, though their physical identity cannot be guaranteed if both GUID and connection order change.
- To select keyboard as a player's device, enter its Device row and press any nonreserved keyboard key. Each action row can then capture a physical keyboard scancode; even a pad-backed player can bind individual actions to keys. Keyboard gets sensible per-player defaults, all customizable, including **Left, Right, Up, Down**.
- After HK the list adds **3P** and **3K**: each binding simultaneously drives all three underlying CPS2 punches or kicks, so the native ROM still receives only its existing six-button inputs. Then come Coin, Start, and directional assignments.
- `xmvsf_controller_map.ini` upgrades to version 3 with fourteen mappings, saved system names, name ordinals, and per-device profiles; version-1 and version-2 mappings load without losing Coin/Start, which migrate from their old slots into indices 8/9. Keyboard scancodes are persisted as `K:index`. Existing pad swaps continue to exchange complete player maps.
- The bottom live-button highlighting strip remains removed. Windows gameplay input init skips its former up-to-600ms joystick wait because the menu already initialized SDL devices. UI page navigation additionally uses a 20ms post-release settle instead of 80ms; the separate gameplay handoff gate is untouched to prevent leaked confirms.
- Cold-boot announcer WAV now starts at t=0 (next to crash) **before the first logo frame at t=1.0**; this provides one second of audio preroll for the spoken phrase. The WAV itself is not trimmed, retimed, or sped up; audible syllable alignment requires a Windows listening check.

## TEST 87.245 ROM trace: independent attacker/defender clocks and the missing portrait-freeze gate

The user supplied `xmvsf_frame_clock_243.tsv` (2047 chronological native samples, host indices 31–2201). The trace resolves the prior contradictory fixes: the previous single Training `frame` counter *both* advanced through a super cinematic *and* remained frozen throughout a live super's attack because it treated the defender's +85 hit-stop as a universal game pause.

There are four complete and highly repeatable native 2E sequences in the supplied trace, at host samples 69–208, 519–658, 815–954 and 1943–2082. All have stage `02` for 70 samples, stage `04` for 52 samples and recovery stage `06` for 18 samples.

| 2E host sequence | Native stage 02 phase 81 (cinematic) | Old clock incorrectly advanced | Native stage 04 while defender +85 nonzero | Old clock advanced during stage 04 |
| --- | ---: | ---: | ---: | ---: |
| 69–208 | 43 | 43 | 52 | 0 |
| 519–658 | 43 | 43 | 52 | 0 |
| 815–954 | 44 | 44 | 52 | 0 |
| 1943–2082 | 44 | 44 | 52 | 0 |

During native `2E00` stage `02` phase `81`, both P1 and P2 animation frame pointers/timers are constant, **both fighter +85 counters are zero**, and no offensive box is active. This is a native *super portrait/streak freeze* distinct from ordinary hit-stop. The existing +85 clock never excluded these 43–44 samples.

During native `2E00` stage `04`, **P1 +85 remains zero**, native P1 animation and projectile box state change, and P2 health decreases across successive hits. However, P2's +85 counter repeatedly reloads at 10, and the global Training clock remains constant for all 52 samples. Specifically, host 139–190 holds `logical_before=logical_after=105` while P1 completes its actual super attack stage. Host 191–196 holds the same old clock through the start of P1's `2E02` recovery. This explains the user's point-blank freeze and loss of actual active/duration frames; it is **not** an SDL rendering delay.

TEST 87.245 source corrections:
- Add `XM30FrameData::playerFrame[2]`. Each fighter's action owner, input age, native attack duration, box lifetime, movement timestamps and red contact ticks now use **that fighter's** unfrozen step axis. A fighter's own +85 excludes its own time, not the opponent's. The shared `frame` is only a host/update display index advancing whenever at least one fighter progresses.
- Independently identify the trace-observed 2E cinematic: active native family 2E, script stage `02`, attack phase `0x81`, with no offensive box. Neither fighter's clock advances during this native hold, despite both +85 bytes reading zero. This specific ROM condition is evidence-based; other super variants still need native verification.
- Keep the accepted new-move first-impact exception if its +85 is already nonzero on the very first observed offensive sample, without counting any subsequent stopped samples.
- Draw each chart on `playerFrame[e.player]` rather than the shared display clock; no catch-up frames after the defender exits stop.
- `recordContacts` assigns each confirmed HP loss to the attacking fighter's own current tick, including while the opponent is frozen, preserving red ticks.
- Add `test_frame_data_asymmetric_stop_245.cpp` covering a 43-sample portrait freeze, 52 consecutive defender-only stop samples during a 2E super, separate P1/P2 clock values, red ticks, active frames, recovery, and simultaneous +85 stop. Existing frame-data unit tests were updated to assert the correct owning fighter clock rather than a falsely universal stop.
- Expand the optional trace to `xmvsf_frame_clock_245.tsv`, including each player's logical clock *before and after* every sample so the actual causal fix can be confirmed next run.

Source-level contract checks are **not** a substitute for local C++ compilation or real Windows/ROM gameplay. No GitHub Actions, no Stable overwrite, and no native game-state writes were introduced.

## TEST 87.244 Combo attack handoff and native diagnostic coverage

The user's `87.235.zip` contains native DHC/super diagnostic captures and an `observer_health_summary.json` confirming combat ran, but contains **no 87.243 frame-clock TSV** and no chronological native Training input/hitbox trace. It cannot validate the previous 87.243 hit-stop change. The supplied `BUILD_TEST(9).log` ends with a **87.241 regression assertion failure** after 87.240 built, not with proof of a succeeding 87.243 gameplay run.

A separate source issue caused many combo attacks to be unobserved:

- `XM30FrameData` originally made an event only on `hits[0].active && !t.previous[0]`, i.e. a collision-box falling/rising edge. In chained normals or normal->special/super cancels, a *new native ROM action selector* may begin while box70/72 remains high from the prior action, so no event is created. The old attack then incorrectly receives the new hitbox frames.
- Record the prior native family/action at each counted step, snapshot them at every new input edge, and defer recording that candidate while the original normal action is still current. A genuinely new normal selector or a transition to a different attack family now claims the already-live box, closes the older card at its real previous frame, and opens the new card on its own first observed offensive frame. Same-family internal 2C/2E script steps are not presumed to be separate moves.
- A newly accepted native combo attack can begin on the same callback that +85 becomes nonzero. The *first* offensive sample for that new action may create a row, but subsequent +85 countdown callbacks never increment either player's durations.
- Hit confirmation uses the **already resolved active teammate HP** shown in the Training HUD; the previous fixed-slot `base+0x210` 16-bit read could follow the wrong fighter and contained an unrelated adjacent byte.
- Added `test_frame_data_combo_244.cpp`: light->medium with continuously high box, normal->super with uninterrupted box, rejected/unaccepted input doesn't fabricate a combo, input queued in hit-stop, and accepted combo coinciding with first +85 stop.
- Optional `XMVSF_FRAME_TRACE=1` now produces `xmvsf_frame_clock_244.tsv` next to the TEST executable, with both ROM modes/actions, +85, hitbox state, active HP, prior action/input selector, pending status, red tick count, emulated and logical frames. It captures normals and their cancels as well as supers. This is read-only and bounded.

Native Windows compilation and combo gameplay confirmation are still needed. Stable, prior TEST executables and the persistent local build cache are untouched.

## TEST 87.243 Investigation: actual ROM hit-stop vs emulator callbacks

The 87.242 fix intentionally bypassed native +85 hit-stop once offense was visible, and therefore counted frozen emulator frames as attack-duration frames. The number only appeared once the display next painted, exacerbated by the separate ordering defect: `RunFrame` sampled `xm23OnRealFrame` **after** `VidPaint`. Both were observable source-level errors.

- At fighter +85 (the ROM hit-stop countdown), any nonzero stop suspends **both** Training logical duration clocks, even when a super's hitbox is active and even when the countdown remains at the same nonzero value. Frozen frames never increment the action, active, startup or motion durations. Defender HP reductions can still stamp a red contact tick at the existing logical position.
- If an attack is *first recognized* on the same callback on which +85 begins (a point-blank immediate hit), count that **one** offensive sample to establish its original event, then stop until +85 clears. Do not count another onset for a repeated button edge on an already-running super.
- Training samples now run once per completed emulated frame **before** the SDL `VidPaint` in the drawn path, and once after `BurnDrvFrame` on skipped-picture frames. Paused and runahead paths remain guarded. The whole game render and ROM timing remain native and unmodified.
- `test_frame_data_hit_clock_240.cpp` now explicitly asserts eight frozen countdown callbacks contribute **zero** extra frames to either P1 or P2, with a second impact retaining the original timestamp and adding a red tick. It also covers a first hitbox appearing on the same callback as a nonzero stop. `test_frame_data_paint_order_243.py` guards the rendering call order as a separate source-level regression.
- For further actual CPS2 verification, set `XMVSF_FRAME_TRACE=1` before `RUN_TEST.bat`. `xmvsf_frame_clock_243.tsv` appears next to the TEST executable and records up to 2400 read-only native samples around 2E supers and +85 stop: host callback index, Training logical clock before/after, both native mode/action/phase/step, +85, native control/animation pointer and timer, HP, fighter/projectile box presence, and number of chart events. Default OFF. This will establish whether portrait/streak freeze uses a different ROM gate; it is **not** proof of runtime correctness until a real trace is recorded.
- Local C++ regression build and Windows gameplay remain pending; no GitHub Actions, ROM/gameplay mutation, or Stable overwrites.

## TEST 87.242 Point-blank super contact / dual-meter clock

The 87.239–87.241 read-only frame-data clock used the fighter +85 hit-stop countdown as a global stop signal. During a point-blank multi-hit super that countdown continually decreases or reloads as additional attacks connect. A global return therefore made the **P1 and P2 frame-data meters appear permanently stopped**, even though the original ROM was progressing through attacks. No guest gameplay was paused.

- Limit the +85 freeze exclusion to *pre-offensive native 2E cinematic time*. The first visible offensive hitbox, any confirmed defender HP loss, carried impact, or an existing live super event proves the action is already fighting: from that moment, the shared frame-data clock advances regardless of fighter +85 countdowns. A held `FF5009` round timer is still not used as a pause signal.
- Each new point-blank hit adds a red tick to the existing super card. An ongoing super retains its original action start frame; continued hitstop cannot create new frame-data epochs or globally stop the other player's meter.
- Refresh the bounded impact action-exit grace on *changing*, nonzero +85 stop counters while that fighter owns an existing super, then leave two post-stop samples to distinguish an actual ROM exit from transient contact/controller changes. Unchanged stale counters do not extend grace.
- Expanded `test_frame_data_hit_clock_240.cpp` to exercise first-offensive-frame point-blank damage, simultaneous player hitstop, eight successive counter ticks, reloaded hitstop on a second hit, continuous dual-player clock and two red markers, and continuation across a transient ROM controller change.
- Native Windows build and gameplay confirmation remain necessary. Stable and previous TEST binaries are unchanged.

## TEST 87.241 Multi-hit meter continuity

- 87.240 had a hit-stop handoff error: the defender's HP loss was consumed inside a frozen `onFrame` callback. That callback returned without advancing the gameplay clock. The first unfrozen frame then saw *no new health delta* and could end the attack on its transient impact controller state, making both frame-data lanes appear to restart on subsequent hits.
- A per-attacker `pendingContact` latch now carries frozen damage into the first resumed gameplay frame. A bounded contact exit grace lets the original native attack controller return; a genuine exit still closes at its last actually matched ROM frame (no fabricated duration).
- Melee and projectile super hitbox lanes remain attached through that impact handoff without inventing attack-box activity during frozen or blank frames. A repeated hitbox rise on the same ongoing `2E` super and action selector does **not** append a second event, even when an intervening mapped button edge changed the pending input epoch.
- The 87.240 standalone contact test now covers hit -> native stop -> transient action -> re-enter original super -> new hit, asserting one card, a monotonic frame count, and accumulated red markers. Warm-cache native unit tests remain local. GAMEPLAY runtime verification remains pending.

## TEST 87.240 Training hit markers and horizontal stun meter

- The native Training clock no longer uses a stalled `FF5009` subsecond round-clock value to infer super freeze. That held countdown could cause the previous frame-data implementation to stop tracking **both** players indefinitely. The panel now pauses on actual per-fighter `+85` hit-stop transitions (onset/decrement), and any repeated nonzero stale hit-stop value cannot deadlock it. Normal ROM actions continue counting after release.
- A fighter's `+210` native health word is sampled read-only to detect a **confirmed opponent-damage frame**, not the visibility of a hitbox or an animation guess. Each recorded attack keeps up to 32 chronological confirmation frames (sliding window). The panel overlays a **red 1px vertical hit tick** across the hitbox and orange action-duration lanes. A contact does not close or clip the native attack action; a momentary controller change on impact is not considered an attack exit. Green collision-box activity remains independently measured.
- Stun resistance is still the exact fighter-specific 50/60-point ROM quantity. Its orange/yellow warm gradient now progresses **horizontally**, in ten discrete steps of `total/10` (five stun points for a 50-capacity bar, six for 60). The original diagonal face, right-to-left fill origin, P1 mirroring, bevel/shadow, and full-bar rainbow are preserved. Vertical position does not affect normal gradient color.
- Cached native C++11 regression tests `test_frame_data_hit_clock_240.cpp` and `test_stun_horizontal_240.cpp` now run before the TEST native build. The existing active 87.219 HUD geometry regression was adjusted to match the new per-X color. The prior 87.239 frame-clock regression now checks that a stagnant round timer cannot be mistaken for super freeze.
- Local Windows gameplay is required to verify actual damage markers, impact freeze durations, and the pixel appearance of the live HUD. No guest ROM writes or Stable publishing changed.

## TEST 87.239 Training super-frame measurements

The Training frame-data pane now samples three read-only ROM timing signals alongside its existing hitbox and native 2E action identity: the round-clock subsecond at `FF5009` (not the Training-overridden 99-second byte at `FF5008`), fighter +`85` hit-stop, and fighter +`64` attack-phase flags (`80` attack, `40` recovery). The tracker first witnesses a changing clock so a permanently held countdown cannot masquerade as a freeze.

- During a native super, clock-stalled frames and explicitly stopped fighter frames are excluded from startup, active, attack duration, projectile motion, frame popups, and all logical frame arithmetic. The *actual emulator/ROM continues unchanged*; only the read-only Training meter's clock pauses.
- A verified native `2E` super row stays active through temporary missing fighter or projectile collision boxes while the ROM still signals an attack phase. The original even `2E` continuation step at fighter +`08` (at least `04`) bridges intermittent attack-phase flags **only after a real hitbox has already established the move**, and never while the ROM reports `40` recovery. The tracker does not invent an active attack during the initial portrait animation.
- The chart distinguishes this as `ACT` / `SUPER DURATION`, rather than implying that a collision box was physically present for every sampled frame. Projectiles remain linked to their original object base for spawn, movement and disappearance.
- `test_frame_data_super_clock_239.cpp` runs as a warm-cached standalone native C++ regression before compilation. It checks cinematic freeze exclusion, a Shinryuuken-like temporary hitbox gap, continued projectile activity after the first box disappears, and unchanged normal-hitbox termination.
- Build / run scripts now name **TEST 87.239**. This is source-level code; verifying real-time super-freeze detection, move animation, and active periods remains a Windows gameplay test.

## TEST 87.238 controller layout, Coin menu and Fight cue

- **Controller tab:** The selected device sits exactly **12 canvas pixels** above the first input's glyph row (one 16px native SF2 small-font line plus a 12px blank margin). Input rows use 18px pitch and are left-aligned to one shared x position. The widest label is measured using the actual `sf2fontsmall.png` glyph advances, and the entire input column is centered as a block at x=480. The device+input group is vertically centered between the upper page prompt and lower player selector.
- A **keyboard** device exposes LP, MP, HP, LK, MK, HK, 3P, 3K, Coin, Start, Left, Right, Up, Down. A **controller** device exposes only the first ten actions; its four directional setup rows are hidden while normal built-in D-pad/analog movement stays available.
- All boot, Training, Versus, scoreboard and overlay back hints show **B/ESC BACK**. Their font renderer continues to use the authored SF2 glyph atlas before any pre-existing fallback.
- **Coin fix:** adding 3P/3K shifted Coin from index 6 to `XM13_COIN` (index 8). Arcade and Versus direct device polling now uses the correct saved binding and accepts keyboard scancodes, even when no joystick is assigned. Keyboard **5** remains a fallback Coin shortcut. Keyboard event handlers no longer double-toggle the same Coin edge; the one-shot opens/closes the relevant non-pausing overlay, from which the Main Menu/return command remains available. Training's existing Coin toggle is unchanged.
- **Fight! timing:** Selecting **Training** in the main menu plays the standard confirmation sound. The announcer's `fight!.wav` now starts only when the Training stage is confirmed, in the same WASAPI mixer; selecting Arcade or Versus in the main menu retains its existing Fight cue.
- **Build identity:** all native launcher and build paths now target **TEST 87.238**, preserving Stable and historical 87.235 source and test filenames.

## Local verification

Run `BUILD_TEST.bat` to compile local `XMVSF_Test_87_238.exe`, with ROM-free C++ timeline test `test_menu_intro_timeline_235.cpp` and Python source integration test `test_menu_intro_integration_235.py`. Run `RUN_TEST.bat` for true visual/audio verification on Windows. Confirm crash and welcome cues at boot (t=0), with the welcome already in progress when the logo enters at t=1.0, four ten-frame source-pixel II strikes with emitting tip particles, the full numeral without seams, normal-volume menu music after the strikes, full dithered menu before the first input, and Fight audio during all three game-mode confirmations. Verify replay and returning from a match skip the intro. Native compile and hardware playback are not verified in GitHub.
