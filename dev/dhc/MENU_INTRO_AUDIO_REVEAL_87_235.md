# TEST 87.235: Cold-boot reveal and three authored WAVs

Branch: `feature/menu-intro-reveal-audio-87-235`, descended from 87.234. No main merge, GitHub Actions, source deletions, or Stable changes.

## First launch timeline

- 0.0–3.0 seconds: start PURE WHITE (including the navy canvas behind the GIF). Its native animated `_art/menuanimation.gif` pixels flash white, then fast hue-strobe with source-space Bayer dither over **three seconds**, settling to the exact original GIF colors and normal 30% brightness/36-second hue cycle. This visual fade **overlaps the full title reveal from t=1.0 through t=3.0**. `audio/crash.wav` still fires at t=0; no other cue moves.
- 1.0–3.0 seconds: logo (original `_art/logo.png`) appears PURE WHITE. It hue-strobes downward while fixed source-pixel Bayer dither progressively releases the original colors and existing authored logo palette animation. `audio/xmvsf_welcome.wav` now fires at t=4.5, 1.5 seconds later than its previous t=3.0 cue. The logo and II visual timing is unchanged. This remains **2.0 seconds of logo animation**; the WAV is never sped up or truncated if its natural duration differs.
- **3.000–3.667 seconds:** independently animate `_art/logo_II_A.png`, `_art/logo_II_B.png`, `_art/logo_II_C.png`, and `_art/logo_II_D.png` in that exact sequence at 60 Hz / ten source-mask frames each (40 frames, 667 ms total). **A up-right, B up, C down, D up-right**. The leading edge of each authored shape is computed from its nontransparent pixels, Bayer-dithered at the boundary, and emits a small colored/white particle spray that flies off the actual drawing tip. At the end use the complete `_art/logo_II.png` for a pixel-exact settled numeral. All five exports are aligned with the base logo's 400×300 source canvas and share its placement scale.
- **3.667–4.167 seconds:** only AFTER the final II strike, menu glyphs (the original `_art/sf2font.png` / `_art/sf2fontsmall.png`) appear using 16-stage spatial Bayer alpha dither. **Menu music starts at this boundary**, beginning at sample zero and fading to its normal full menu gain (1.0) over the same 0.5 seconds. No additional gain reduction during the announcer tail.
- At **t=4.167 seconds:** menu input unlocks. Already-held buttons must release before they can select. `SDL_QUIT` still closes the window at any time.
- Game→menu return and automated F9 replay bypass the cinematic; the normal menu is immediately usable.

## Fight cue

Selecting **Arcade**, **Training**, or **Versus** on the main menu plays `audio/fight!.wav` in place of the normal confirmation sound. Training and Versus still use their existing setup/stage selection screens. There is no delay before handing Arcade to the native emulation. The one-shot survives menu→game transition in the original persistent WASAPI mixer, which also owns game PCM and menu music. The music ducks while the authored WAVs play; no new SDL audio device, engine tempo change, or native ROM control adjustment. The 3.75-second crash and 4.406-second welcome WAVs are both preserved at natural speed, so the welcome tail continues after the logo reveal.

The three WAVs are copied from main into this branch as identical Git blobs at `audio/crash.wav`, `audio/xmvsf_welcome.wav`, and `audio/fight!.wav`. Asset resolution is relative to the executable: `runtime/stable/../../audio`.

## Full-title centering

The completed `logo_II.png` alpha bounds are unioned with `logo.png`'s visible bounds in the shared 400×300 source canvas. Both draw calls use the same translated canvas, centered on x=480 of the 960-wide logical display. The base art fit/scale and the four independent II stroke trajectories remain unchanged; the menu glyph layout remains unchanged.

## Local verification

Run `BUILD_TEST.bat` to compile local `XMVSF_Test_87_235.exe`, with ROM-free C++ timeline test `test_menu_intro_timeline_235.cpp` and Python source integration test `test_menu_intro_integration_235.py`. Run `RUN_TEST.bat` for true visual/audio verification on Windows. Confirm crash at first frame, welcome 3.5 seconds after logo start (t=4.5), four ten-frame source-pixel II strikes with emitting tip particles, the full numeral without seams, normal-volume menu music after the strikes, full dithered menu before the first input, and Fight audio during all three game-mode confirmations. Verify replay and returning from a match skip the intro. Native compile and hardware playback are not verified in GitHub.
