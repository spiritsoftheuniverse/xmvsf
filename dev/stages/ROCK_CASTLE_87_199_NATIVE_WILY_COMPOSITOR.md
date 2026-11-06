# XMVSF 87.199 — Rock Castle using the existing Wily native stage compositor

## Why reuse Wily

The repo already has a complete real-stage overlay pipeline:

- `src/burner/sdl/xmvsf_stage77_selection.h`: host stage selection/random pool, one stage resolution per match, recorded/restored by existing F9 host replay
- `src/burner/sdl/xmvsf_stage77_native_stage5.h`: a SINGLE early native selector write `FF5082=04` causing ROM stage05 gameplay; no direct stage byte manipulation
- `src/burner/sdl/xmvsf_stage86_49_camera.h`: real CPS2 stage05 camera from the ROM's `FF524C/FF5250` position
- `src/intf/video/sdl/xmvsf_stage86_49_painter.h`: native 384x224 compositor; Wily manifest art, filtered fighter sprite texture, HUD and original ROM flashes, lifecycle cleanup
- `src/intf/video/sdl/xmvsf_stage86_49_wily_art.h` and `runtime/stable/assets/xmvsf/wily75/`: sophisticated Wily 3-layer/parallax/captured animation/actor proof and original scene grading

Rock Castle **reuses these**, not a new competing renderer, standalone gameplay port, alternate input loop, or new ROM stage. Wily's full existing manifest/preview/painter path remains intact.

## Minimal Rock Castle test

New third boot-menu stage choice **ROCK CASTLE** (alongside RANDOM and WILY FACTORY), using the existing stageRow/controller A/B, the existing one-time host stage resolver and stage generation. Random still mostly selects ordinary ROM stages, with Wily and Rock each in its optional custom pool. The replay host stores and restores the integer choice/resolved ID as before.

The native stage05 request, camera and compositor accept whichever of Wily/Rock was selected. Rock uses the same renderer entry `xm8649Paint`, under `xm77StageResolved==XM77_ROCK_CASTLE`, and retains the actual native CPS2 gameplay, stage geometry, fighters, HUD, white wipe, visual FX and opponent. No change is made to ROM tile palettes, collision boxes, stage speed or fighter code.

New source `src/intf/video/sdl/xmvsf_stage87_199_rock_castle.h` loads the user's exact original file, `_art/mvc1-rock-castle-stage.gif`, preferring it over a static `_art/mvc1-rock-castle-stage.png` fallback. Other local candidates include `rock-castle.gif` and `runtime/stable/assets/xmvsf/rockcastle/`. It uses existing SDL2_image (already linked by the menu GIF stage87.195). SDL2_image 2.6+ decodes the genuine original GIF frames and per-frame delays; older SDL_image uses static first frame. Animation advances with *native emulation frames*, not SDL wall time, for replay/pause safety. It uploads a new texture only on a frame change.

The 636x479 source has a different aspect ratio from the native gameplay canvas (384x224), so the first pass preserves source RGB and aspect ratio through a bottom-anchored crop rather than stretching the art or converting it to CPS2 tiles. A small 7% horizontal overlap maps the actual native camera X across the artwork; elevated camera follows vertical position. The regular native sprite/HUD/white-wipe compositor runs over the original RGB stage art. No invented parallax, sprite rip, new animation, or lighting grade is applied. A second art-layer mask may be added only after a playable reference establishes which objects should cover fighters.

**Asset provenance:** the image received in this conversation as `mvc1-rock-castle-stage.gif` is actually a single-frame **PNG** (636x479 RGBA; 1 frame), not a decodable animated GIF source. It was exported without image modification as `mvc1-rock-castle-stage.png` for a static preview. To see the real animation, place the multi-frame original at `F:\xmvsf\_art\mvc1-rock-castle-stage.gif`. The code does not manufacture frames from the screenshot. The original file was not in Git-tracked `_art`; no guessed binary is committed.

**Failback:** if the original file cannot load, `xm8649Paint` returns false and the existing native ROM forest scene remains visible; no missing-asset crash. The renderer and textures are released by the established `xm8649VideoCleanup` before any SDL renderer shutdown, preserving return-to-menu behavior.

## Build / diagnostics

Only the existing `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat` names are used; TEST binary is `runtime/stable/XMVSF_Test_87_199.exe`, logs `test/87.199`, and the prior DHC 87.198 code remains intact. Stable and earlier TEST outputs are never overwritten; warm object cache retained.

`dev/stages/tests/test_rock_castle_stage_199.py` asserts the stage choice/host replay/ROM camera/painter/animation/HUD/FX contracts remain connected, but it is a **static source regression**, not a Windows native render test. Real combat and SDL_image/GIF frame playback must be observed on Windows. Expected logs: `XM87.199 ROCK_CASTLE artwork=...` and `XM87.199 ROCK_CASTLE presented...`. If missing: `XM87.199 ROCK_CASTLE asset absent` followed by native ROM stage fallback.

Further Wily-grade work (foreground/actor separation, detailed parallax, FX-specific masks) belongs in optional Rock Castle-specific manifests after first gameplay evidence; the first pass deliberately leaves Wily's highly specialized recovered art data unchanged.
