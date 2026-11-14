# XMVSF 87.363 — Boomer Kuwanger Stage

## Assets already in the repository

- `_art/boomerkuwanger/a.png`: stationary foreground platform and stage, frontmost of the stage layers
- `_art/boomerkuwanger/b.png`: shaft sides, behind A, tiling downward at 1px per 2 committed game frames
- `_art/boomerkuwanger/c.png`: far background, behind B, tiling downward at 1px per 5 committed game frames, horizontal native-camera parallax 30% of A/B
- `audio/music/boomer01.ogg`: one-shot opening song
- `audio/music/boomer02.ogg`: repeating loop after 01 completes

No animation frames or new extracted assets are needed. Each PNG is loaded as a GPU texture once per renderer; no per-frame PNG decoding, surface uploads, resizing of authored pixels to fit 384 world pixels, or game clock mutation. SDL nearest-neighbor is used for all textures, and B/C tile seamlessly in X/Y under the clipping viewport.

## Integration

- New selectable **BOOMER KUWANGER STAGE**, index 7 in the menu, including the randomized stage pool (12 original, now seven custom).
- Uses the same original native stage-05 asset/ROM selector and read-only 68000/OAM camera as Final Destination, Guile, Sigma, Akuma and Ryu. Combat collision, actor positions, HUD and fighter/FX sprite overlays remain native.
- Stage art composes in world coordinates for a 768×471 authored stage, cropped by the original 384×224 camera; A is fixed to the stage world, bottom-anchored to preserve floor alignment. B tracks the native camera without independent horizontal drift; C uses slight horizontal parallax. All layers receive the same native camera Y translation plus their independent downward elevator scroll, never a jump-dependent vertical parallax.
- B/C texture sizes are taken from the PNG at runtime. Larger art is centered in the 768px world; smaller art tiles without altering its original pixel coordinates or dimensions.
- Native audio uses the shared WASAPI mix and async Vorbis loader. The stage picker preloads both provided OGGs. Boomer01 plays once at native combat/Fight handoff, then Boomer02 loops. Existing native CPS2 SFX remain, while character BGM is muted. Music fades on last team KO and resets on the next game/stage session.
- No new native build dependency or Actions. Existing local BUILD_TEST.bat/RUN_TEST.bat use version 87.363 and preserve Stable. Replacing the PNG/OGG assets requires a relaunch, not a C++ recompile; changes to stage code require the incremental TEST build.

## Verification

The source branch contains all necessary selection, native camera/selector, compositor, music mix, preload and cleanup hooks. The four filenames (three PNG plus both OGG tracks) exist at the specified paths on the parent main tree.

The Windows TEST executable, real image dimensions, visual seams, native camera alignment, and audio playback have not been exercised here. The exact sound roles (01 intro, 02 loop) are inferred from two supplied filenames and their small/large filesizes and follow the already-established FD convention.
