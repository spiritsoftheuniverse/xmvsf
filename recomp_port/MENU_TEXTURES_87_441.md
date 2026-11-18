# 87.441 - actual source-texture SDL menu, not a full CPU frame

Previous versions 87.435-87.440 redrew the entire 960x540 menu into
RAM every frame: GIF, logo, II lettering, and each font glyph were all
blended in a CPU loop, then uploaded to the recomp presentation path.
Our working SDL2 implementation did not do this. It rendered separate
persistent GPU textures through an accelerated SDL renderer.

The 87.440 renderer switch alone did not remove the CPU work. Version
87.441 restores that original source-texture architecture in SDL3:

- Persistent textures created once for the source logo, II strokes,
  large/small font sheets and animated background.
- Update only the animated GIF's 80ms frame/hue, not the whole screen.
- Draw the existing 960x540 layout directly through SDL_RenderTexture.
- Nearest texture sampling, alpha blending, and text brightness modulation.
- SDL-managed logical presentation, no custom GPU shader/linear sampler.
- Prefer accelerated direct3d11 on Windows, with a fallback renderer.
- No changes to game ROM, native gameplay viewport, input or menu WAVs.

The source lives in recomp_port/xmvsf_menu_textures.h. It reuses decoded
art from recomp_port/xmvsf_home.h. The installer is
recomp_port/apply_textured_menu.py, which checks all source anchors
before editing the local pinned submodule. The root RECOMP_BUILD.bat
runs all three installers before the existing incremental Ninja build.

The old CPU menu implementation remains only for compatibility:
set XMVSF_MENU_CPU_FALLBACK=1 to use it temporarily without rebuilding.
Set XMVSF_RECOMP_CLASSIC_OFF=1 to select the original recomp GPU path.

Validate after building:
- python3 recomp_port/tests/test_textured_menu.py
- The stderr diagnostic "XMVSF 87.441: hardware menu textures ready"
- Windowed/maximized pixel geometry, dragging, sounds, and gameplay.

We have not compiled this native Windows build on the user's PC. The
source is committed and the local Windows build is the acceptance test.
