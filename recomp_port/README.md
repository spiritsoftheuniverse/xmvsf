# XMVSF native front end — 87.436 (source-art presentation)

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
- **OPTIONS:** currently opens the existing recomp settings as a temporary
  bridge. Native Game / Graphics / Controller UI remains a follow-up.

Navigation uses both keyboard and gamepad: arrows / D-pad, Enter or Space / A.
Esc returns to the home menu, and F1 there opens the original recomp diagnostics.
Use `XMVSF_RECOMP_HOME_OFF=1` when comparing against untouched upstream.

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

Menu movement/selection audio; source-authored full intro/reveal timing;
accurate Game/Graphics/Controller tabs with persistence; and our dedicated
Versus/Training lifecycle. These should be ported as **separate functional
modules**, not more version-specific hooks.
