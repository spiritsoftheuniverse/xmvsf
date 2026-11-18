#!/usr/bin/env python3
"""87.441: replace CPU-composited menu with original-style SDL GPU textures.

Run after apply_menu.py and apply_classic_renderer.py. All source anchors
are checked before any write; no ROM, saved data or build cache is changed.
"""
from __future__ import annotations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "recomp" / "port"
FRONTEND = PORT / "frontend_sdl.cpp"
PRESENT_H = PORT / "present.h"
PRESENT_CPP = PORT / "present.cpp"
HOME = PORT / "xmvsf_home.h"
SOURCE = ROOT / "recomp_port" / "xmvsf_menu_textures.h"
HEADER = PORT / "xmvsf_menu_textures.h"
MARK = "XMVSF_RECOMP_MENU_TEXTURES_87_441"

def replace_exact(text: str, old: str, new: str, label: str) -> str:
    if new in text: return text
    count = text.count(old)
    if count != 1: raise RuntimeError(
        f"87.441 {label}: expected one anchored source block, found {count}; no files modified")
    return text.replace(old, new, 1)

def patch(inputs: dict[Path, str]) -> dict[Path, str]:
    front, ph, pc, home = (inputs[FRONTEND], inputs[PRESENT_H],
                           inputs[PRESENT_CPP], inputs[HOME])
    front = replace_exact(
        front,
        '#include "xmvsf_window.h"\n',
        '#include "xmvsf_window.h"\n'
        '#include "xmvsf_menu_textures.h" // ' + MARK + '\n',
        "menu texture include"
    )
    front = replace_exact(
        front,
        '    xmvsf_home::MenuAudio homeAudio(homeEnabled);\n',
        '    xmvsf_home::MenuAudio homeAudio(homeEnabled);\n'
        '    xmvsf_home::TexturedMenu homeTextures(home);\n',
        "texture compositor lifetime"
    )
    front = replace_exact(
        front,
        '''            home.Draw(homePicture, SDL_GetTicks());
            // Original Stage14 menu uses its own 960x540 canvas. Preserve the
            // 16:9 composition in the current window without stretching.
            const float fitHome = std::max(0.001f, std::min(float(width) / 960.0f, float(height) / 540.0f));
            const SDL_FRect homeWhere{(float(width) - 960.0f * fitHome) * 0.5f,
                                      (float(height) - 540.0f * fitHome) * 0.5f,
                                      960.0f * fitHome, 540.0f * fitHome};
            presenter.Show(homePicture.data(), 960, 540, homeWhere, nullptr, false);''',
        '''            // XMVSF_RECOMP_MENU_TEXTURES_87_441: actual SDL texture layers,
            // matching our original SDL2 frontend. No CPU full-menu blending.
            if (homeTextures.Renderable(presenter.Renderer(), xmvsfClassic)) {
                presenter.UseClassicMenuCanvas(true);
                if (!homeTextures.Render(presenter.Renderer(), SDL_GetTicks())) {
                    home.Draw(homePicture, SDL_GetTicks());
                    const float fallbackScale = std::max(0.001f, std::min(
                        float(width) / 960.0f, float(height) / 540.0f));
                    const SDL_FRect fallbackRect{
                        (float(width) - 960.0f * fallbackScale) * 0.5f,
                        (float(height) - 540.0f * fallbackScale) * 0.5f,
                        960.0f * fallbackScale, 540.0f * fallbackScale};
                    presenter.Show(homePicture.data(), 960, 540, fallbackRect, nullptr, false);
                }
            } else {
                home.Draw(homePicture, SDL_GetTicks());
                const float fallbackScale = std::max(0.001f, std::min(
                    float(width) / 960.0f, float(height) / 540.0f));
                const SDL_FRect fallbackRect{
                    (float(width) - 960.0f * fallbackScale) * 0.5f,
                    (float(height) - 540.0f * fallbackScale) * 0.5f,
                    960.0f * fallbackScale, 540.0f * fallbackScale};
                presenter.Show(homePicture.data(), 960, 540, fallbackRect, nullptr, false);
            }''',
        "remove full-frame CPU compositor from live menu",
    )
    front = replace_exact(
        front,
        '''    presenter.Close();
    SDL_DestroyWindow(window);
    SDL_Quit();''',
        '''    // XMVSF_RECOMP_MENU_TEXTURES_87_441: release SDL objects before
    // upstream closes the renderer/window and calls SDL_Quit.
    homeTextures.Release();
    homeAudio.Close();
    presenter.Close();
    SDL_DestroyWindow(window);
    SDL_Quit();''',
        "SDL shutdown ownership order",
    )
    ph = replace_exact(
        ph,
        '''    void SetScaling(Scaling scaling) {
        m_scaling = scaling;
    }
''',
        '''    void SetScaling(Scaling scaling) {
        m_scaling = scaling;
    }
    // 87.441: let SDL handle 960x540 menu letterboxing once per state change.
    void UseClassicMenuCanvas(bool on);
''',
        "logical presentation transition",
    )
    pc = replace_exact(
        pc,
        '''void Presenter::Show(const uint32_t *pixels, int width, int height, const SDL_FRect &where, const Zoomed *zoomed, bool shade) {''',
        '''// XMVSF_RECOMP_MENU_TEXTURES_87_441: identical logical-canvas order
// to legacy SDL2, and independent of the native gameplay video path.
void Presenter::UseClassicMenuCanvas(bool on) {
    if (!m_classicPixels || !m_renderer || on == m_classicLogicalMenu) return;
    SDL_SetRenderTarget(m_renderer, nullptr);
    SDL_SetRenderLogicalPresentation(m_renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetRenderViewport(m_renderer, nullptr);
    SDL_SetRenderClipRect(m_renderer, nullptr);
    SDL_SetRenderScale(m_renderer, 1.0f, 1.0f);
    if (on) {
        SDL_SetRenderLogicalPresentation(m_renderer, 960, 540, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    m_classicLogicalMenu = on;
}

void Presenter::Show(const uint32_t *pixels, int width, int height, const SDL_FRect &where, const Zoomed *zoomed, bool shade) {''',
        "SDL-renderer menu canvas lifecycle",
    )
    # Prefer the hardware-accelerated Windows SDL renderer used by the old
    # working frontend, rather than relying on a different default backend.
    pc = replace_exact(
        pc,
        '''    m_renderer = SDL_CreateRenderer(window, nullptr);
    if (!m_renderer) {''',
        '''    // XMVSF_RECOMP_MENU_TEXTURES_87_441: prefer accelerated D3D11
    // on Windows (SDL2's native renderer approach), then SDL's best fallback.
#if defined(_WIN32)
    if (m_classicPixels) m_renderer = SDL_CreateRenderer(window, "direct3d11");
#endif
    if (!m_renderer) m_renderer = SDL_CreateRenderer(window, nullptr);
    if (!m_renderer) {''',
        "accelerated SDL 2D backend",
    )
    home = replace_exact(
        home,
        '''class Menu {
    static constexpr int W = 960, H = 540;''',
        '''class Menu {
    friend class TexturedMenu; // 87.441: reuse already decoded artwork.
    static constexpr int W = 960, H = 540;''',
        "give texture renderer access to the original source assets",
    )
    if not any(label in home for label in ('"TEST 87.442"', '"TEST 87.443"', '"TEST 87.444"', '"TEST 87.445"', '"TEST 87.446"', '"TEST 87.447"', '"TEST 87.448"', '"TEST 87.449"', '"TEST 87.450"', '"TEST 87.451", "TEST 87.452"')):
        home = replace_exact(
            home,
            '''"TEST 87.440"''',
            '''"TEST 87.441"''',
            "87.441 menu version",
        )
    return {FRONTEND: front, PRESENT_H: ph, PRESENT_CPP: pc, HOME: home}

def main() -> int:
    original = {}
    for path in (FRONTEND, PRESENT_H, PRESENT_CPP, HOME):
        if not path.exists(): raise RuntimeError(f"missing pinned source: {path}")
        original[path] = path.read_text(encoding="utf-8")
    # The newer frontend keeps the classic textured renderer, but changes
    # its shutdown/order. Don't reinsert 87.441 blocks into 87.443.
    if "XMVSF_RECOMP_PAD_OWNER_87_443" in original[FRONTEND]:
        assert "XMVSF_RECOMP_MENU_TEXTURES_87_441" in original[FRONTEND]
        updated = original
    else:
        updated = patch(original)  # checks every anchor before writing anything
    content = SOURCE.read_bytes()
    for path in original:
        if original[path] != updated[path]:
            path.write_text(updated[path], encoding="utf-8")
            print(f"87.441 texture layer integration: {path}")
    if not HEADER.is_file() or HEADER.read_bytes() != content:
        HEADER.write_bytes(content)
        print(f"87.441 original-style SDL renderer: {HEADER}")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("87.441 texture renderer patch FAILED:", exc, file=sys.stderr)
        sys.exit(1)
