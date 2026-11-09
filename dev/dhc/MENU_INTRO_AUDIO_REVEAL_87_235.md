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
- The first Controller field shows the live SDL/OS-reported hardware name (from `SDL_JoystickNameForIndex`, with game-controller fallback) instead of generic `PAD N`. Names are **not** saved as hardware identity; GUID plus GUID-occurrence still tracks multiple identical devices and remains stable when a controller is re-enumerated.
- To select keyboard as a player's device, enter its Device row and press any nonreserved keyboard key. Each action row can then capture a physical keyboard scancode; even a pad-backed player can bind individual actions to keys. Keyboard gets sensible per-player defaults, all customizable, including **Left, Right, Up, Down**.
- After HK the list adds **3P** and **3K**: each binding simultaneously drives all three underlying CPS2 punches or kicks, so the native ROM still receives only its existing six-button inputs. Then come Coin, Start, and directional assignments.
- `xmvsf_controller_map.ini` upgrades to version 2 with fourteen mappings; version-1 mappings load without losing Coin/Start, which migrate from their old slots into indices 8/9. Keyboard scancodes are persisted as `K:index`. Existing pad swaps continue to exchange complete player maps.
- The bottom live-button highlighting strip remains removed. Windows gameplay input init skips its former up-to-600ms joystick wait because the menu already initialized SDL devices. UI page navigation additionally uses a 20ms post-release settle instead of 80ms; the separate gameplay handoff gate is untouched to prevent leaked confirms.
- Cold-boot announcer WAV now starts at t=0 (next to crash) **before the first logo frame at t=1.0**; this provides one second of audio preroll for the spoken phrase. The WAV itself is not trimmed, retimed, or sped up; audible syllable alignment requires a Windows listening check.

## Local verification

Run `BUILD_TEST.bat` to compile local `XMVSF_Test_87_236.exe`, with ROM-free C++ timeline test `test_menu_intro_timeline_235.cpp` and Python source integration test `test_menu_intro_integration_235.py`. Run `RUN_TEST.bat` for true visual/audio verification on Windows. Confirm crash and welcome cues at boot (t=0), with the welcome already in progress when the logo enters at t=1.0, four ten-frame source-pixel II strikes with emitting tip particles, the full numeral without seams, normal-volume menu music after the strikes, full dithered menu before the first input, and Fight audio during all three game-mode confirmations. Verify replay and returning from a match skip the intro. Native compile and hardware playback are not verified in GitHub.
