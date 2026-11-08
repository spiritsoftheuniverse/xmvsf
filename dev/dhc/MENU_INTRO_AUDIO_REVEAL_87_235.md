# TEST 87.235: Cold-boot reveal and three authored WAVs

Branch: `feature/menu-intro-reveal-audio-87-235`, descended from 87.234. No main merge, GitHub Actions, source deletions, or Stable changes.

## First launch timeline

- 0.0–1.0 seconds: start PURE WHITE (including the navy canvas behind the GIF). Its native animated `_art/menuanimation.gif` pixels flash white, then fast hue-strobe with source-space Bayer dither until settling to the exact original animated GIF colors and normal 30% brightness/36-second hue cycle. `audio/crash.wav` fires at t=0.
- 1.0–4.5 seconds: logo (original `_art/logo.png`) appears PURE WHITE. It hue-strobes downward while fixed source-pixel Bayer dither progressively releases the original colors and existing authored logo palette animation. `audio/xmvsf_welcome.wav` fires at t=1.0. This is **3.5 seconds of logo animation**; the WAV is never sped up or truncated if its natural duration differs.
- 4.5–5.0 seconds: menu glyphs (the original `_art/sf2font.png` / `_art/sf2fontsmall.png`) appear using 16-stage spatial Bayer alpha dither. **Menu music starts at this boundary**, beginning at sample zero and rising over the same 0.5 seconds. Prior crash/logo phases have no menu music; the authored one-shot effects still play.
- At t=5.0 seconds: menu input unlocks. Already-held buttons must release before they can select. `SDL_QUIT` still closes the window at any time.
- Game→menu return and automated F9 replay bypass the cinematic; the normal menu is immediately usable.

## Fight cue

Selecting **Arcade**, **Training**, or **Versus** on the main menu plays `audio/fight!.wav` in place of the normal confirmation sound. Training and Versus still use their existing setup/stage selection screens. There is no delay before handing Arcade to the native emulation. The one-shot survives menu→game transition in the original persistent WASAPI mixer, which also owns game PCM and menu music. The music ducks while the authored WAVs play; no new SDL audio device, engine tempo change, or native ROM control adjustment. The 3.75-second crash and 4.406-second welcome WAVs are both preserved at natural speed, so the welcome tail continues after the logo reveal.

The three WAVs are copied from main into this branch as identical Git blobs at `audio/crash.wav`, `audio/xmvsf_welcome.wav`, and `audio/fight!.wav`. Asset resolution is relative to the executable: `runtime/stable/../../audio`.

## Local verification

Run `BUILD_TEST.bat` to compile local `XMVSF_Test_87_235.exe`, with ROM-free C++ timeline test `test_menu_intro_timeline_235.cpp` and Python source integration test `test_menu_intro_integration_235.py`. Run `RUN_TEST.bat` for true visual/audio verification on Windows. Confirm crash at first frame, welcome at logo start, full dithered menu before the first input, and Fight audio during all three game mode confirmations. Verify replay and returning from a match skip the intro. Native compile and hardware playback are not verified in GitHub.
