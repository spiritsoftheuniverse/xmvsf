# 87.440 — XMVSF classic SDL renderer bridge

The working legacy menu uses SDL2 accelerated 2D rendering, nearest-neighbour
textures, a persistent window, and a 960x540 SDL logical presentation.
The pinned recomp normally uses a different custom SDL3 GPU pipeline with
edge-blending and a linear-filtered sampler. That changes pixel geometry
at maximized window sizes.

This port uses the SDL3 equivalent 2D renderer, not SDL2 itself. It
restores the old presentation model with no changes to ROM translation,
the authored menu artwork, input mapping, or gameplay.

The deterministic installer in recomp_port/apply_classic_renderer.py:
- Forces the accelerated SDL renderer for interactive XMVSF only.
- Bypasses the custom sharp-then-linear intermediary and uses nearest sampling.
- Enables SDL-managed 960x540 logical letterboxing only for our home menu.
- Keeps the original game FitPicture viewport, native source and zoom code.
- Enables VSync to match the SDL2 menu renderer.
- Leaves the upstream GPU and shaders available as a diagnostic fallback.

The diagnostic opt-out XMVSF_RECOMP_CLASSIC_OFF=1 restores the upstream
renderer at runtime without modifying any source file.

RECOMP_BUILD.bat installs this patch after the regular menu patch.
Ninja recompiles the changed native files incrementally; no M68000
retranslation or additional dependencies. Failure to match any pinned
source anchor stops before any file is modified.

Update and build:

    git -C F:\xmvsf-recomp fetch origin feature/recomp-v2-foundation
    git -C F:\xmvsf-recomp merge --ff-only FETCH_HEAD
    F:\xmvsf-recomp\RECOMP_BUILD.bat
    F:\xmvsf-recomp\RECOMP_RUN.bat

This code has not been compiled or benchmarked on the user's Windows host.
The first local build and maximized-window test remain the acceptance checks.
