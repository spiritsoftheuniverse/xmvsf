#!/usr/bin/env python3
"""87.440: Restore XMVSF's SDL2-style presentation policy via SDL3 Renderer.

Upstream is a pinned submodule. Patch only three known native files;
all three must match expected anchors before any file is changed.
Idempotent and reversible via a single environment variable.
"""
from __future__ import annotations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "recomp" / "port"
FRONTEND = PORT / "frontend_sdl.cpp"
PRESENT = PORT / "present.cpp"
PRESENT_H = PORT / "present.h"
TAG = "XMVSF_RECOMP_CLASSIC_87_440"


def substitute(source: str, old: str, new: str, label: str) -> str:
    if new in source:
        return source
    count = source.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 upstream anchor, found {count}; no files modified")
    return source.replace(old, new, 1)


def patch(files: dict[Path, str]) -> dict[Path, str]:
    frontend, cpp, header = files[FRONTEND], files[PRESENT], files[PRESENT_H]

    frontend = substitute(
        frontend,
        '''    const bool wantGpu = port.opt.renderer != "sdl" && strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    const bool canShade = strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;''',
        '''    // XMVSF_RECOMP_CLASSIC_87_440: SDL3's accelerated 2D renderer,
    // not the recomp's custom GPU shader/linear sampler.
    const bool xmvsfClassic = !std::strcmp(GAME_ID, "xmvsf") &&
                              port.opt.frames == 0 && !port.net && MultiGame().empty() &&
                              std::getenv("XMVSF_RECOMP_CLASSIC_OFF") == nullptr;
    const bool wantGpu = !xmvsfClassic && port.opt.renderer != "sdl" &&
                         strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    const bool canShade = !xmvsfClassic &&
                          strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;''',
        "force XMVSF SDL renderer",
    )
    frontend = substitute(
        frontend,
        '''    auto openPresenter = [&]() {''',
        '''    presenter.SetClassicPixels(xmvsfClassic);
    auto openPresenter = [&]() {''',
        "configure classic renderer",
    )
    frontend = substitute(
        frontend,
        '''    presenter.SetVsync(FeatureOn("display_vsync")); // (kept when it is opened again)''',
        '''    // SDL2 boot always used PRESENTVSYNC when accelerated.
    presenter.SetVsync(xmvsfClassic ? true : FeatureOn("display_vsync")); // (kept when it is opened again)''',
        "SDL2 menu vsync",
    )
    frontend = substitute(
        frontend,
        '''        presenter.SetVsync(FeatureOn("display_vsync")); // (the Display page's VSync, as changed in the menu)''',
        '''        presenter.SetVsync(xmvsfClassic ? true : FeatureOn("display_vsync")); // 87.440 SDL2-style VSync''',
        "retain renderer Vsync",
    )
    frontend = substitute(
        frontend,
        '''    sayPresenter();
    // The Display page's Shader changed in the menu:''',
        '''    sayPresenter();
    if (xmvsfClassic) {
        std::fprintf(stderr,
            "XMVSF 87.440 CLASSIC_RENDER SDL3_2D nearest=true menu=960x540/letterbox vsync=true backend=%s\\n",
            presenter.Renderer() ? SDL_GetRendererName(presenter.Renderer()) : "missing");
    }
    // The Display page's Shader changed in the menu:''',
        "log selected backend",
    )

    header = substitute(
        header,
        '''    // How the picture's pixels are drawn (FitPicture says where): Sharp unless set
    void SetScaling(Scaling scaling) {''',
        '''    // XMVSF_RECOMP_CLASSIC_87_440: nearest-neighbour SDL renderer and
    // SDL-managed 960x540 logical presentation for the home menu.
    void SetClassicPixels(bool on) { m_classicPixels = on; }
    // How the picture's pixels are drawn (FitPicture says where): Sharp unless set
    void SetScaling(Scaling scaling) {''',
        "public classic mode",
    )
    header = substitute(
        header,
        '''    SDL_Renderer *m_renderer = nullptr;
    SDL_Texture *m_texture''',
        '''    SDL_Renderer *m_renderer = nullptr;
    bool m_classicPixels = false, m_classicLogicalMenu = false;
    SDL_Texture *m_texture''',
        "classic renderer state",
    )

    # Bypass the custom sharp-then-linear output. Keep existing source pixels,
    # game viewport and zoom. The menu alone uses SDL's 960x540 letterbox.
    cpp = substitute(
        cpp,
        '''    if (m_scaling == Scaling::Smooth) {
        // (blended all over: the picture as it is, fitted smoothly)''',
        '''    if (m_classicPixels) { // XMVSF_RECOMP_CLASSIC_87_440
        const bool homeCanvas = !shade && !zoomed && width == 960 && height == 540;
        if (homeCanvas != m_classicLogicalMenu) {
            // Reset stale state FIRST, apply logical presentation LAST.
            SDL_SetRenderTarget(m_renderer, nullptr);
            SDL_SetRenderLogicalPresentation(m_renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            SDL_SetRenderViewport(m_renderer, nullptr);
            SDL_SetRenderClipRect(m_renderer, nullptr);
            SDL_SetRenderScale(m_renderer, 1.0f, 1.0f);
            if (homeCanvas) {
                SDL_SetRenderLogicalPresentation(m_renderer, 960, 540, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            }
            m_classicLogicalMenu = homeCanvas;
        }
        SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        if (homeCanvas) {
            // SDL computes centered letterboxing using the physical output.
            SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);
        } else {
            // Native game pixels, existing game aspect and zoom handling.
            draw(m_texture, 1.0f);
        }
        overlay();
        SDL_RenderPresent(m_renderer);
        return;
    }
    if (m_scaling == Scaling::Smooth) {
        // (blended all over: the picture as it is, fitted smoothly)''',
        "bypass sharp-plus-linear intermediate texture",
    )
    return {FRONTEND: frontend, PRESENT: cpp, PRESENT_H: header}


def main() -> int:
    originals: dict[Path, str] = {}
    for path in (FRONTEND, PRESENT, PRESENT_H):
        if not path.is_file():
            raise RuntimeError(f"initialize the pinned recomp submodule first: {path}")
        originals[path] = path.read_text(encoding="utf-8")
    changes = patch(originals)
    changed = [path for path in originals if changes[path] != originals[path]]
    for path in changed:
        path.write_text(changes[path], encoding="utf-8")
        print(f"XMVSF 87.440 SDL2-style renderer updated: {path}")
    if not changed:
        print("XMVSF 87.440 SDL2-style renderer already integrated")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("XMVSF 87.440 renderer installation FAILED:", exc, file=sys.stderr)
        sys.exit(1)
