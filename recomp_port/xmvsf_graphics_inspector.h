// XMVSF 87.493: F4 tints a chosen visible OAM object using exact pixel ownership.
// Drawn ONLY over a copy of the rendered picture: not a ROM/VRAM/OAM
// filter, not a gameplay trigger, and not a fabricated sprite identifier.
#pragma once
#if __has_include("../video/video.h")
#include "../video/video.h"
#include "../recomp-common/text.h"
#include "xmvsf_source_identity.h" // source-verified wall group
#else
// Same canonical header also lives in recomp_port/ for installer parity.
#include "../recomp/video/video.h"
#include "../recomp/recomp-common/text.h"
#include "xmvsf_source_identity.h" // mirrored native source identity
#endif
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "xmvsf_vs_handoff.h" // reuse the actual native ROM HUD-source classifier
#include "xmvsf_source_tint.h" // selected source pixels only, never rectangles

namespace port::xmvsf_home {

class GraphicsInspector {
    bool active_ = false;
    unsigned mode_ = 0; // HUD, verified WALLS, all OAM, scroll 1/2/3
    unsigned page_ = 0;
    unsigned pageCount_ = 1; // recomputed on each presented source snapshot
    unsigned selectedInPage_ = 0u; // one visibly selected OAM object
    unsigned pageItems_ = 0u;
    bool tintEnabled_ = false; // F4 diagnostic-only presentation copy
    static constexpr unsigned kPageSize = 7u;

    static bool Wall(const cps2recomp::Video::GraphicRecord &r) {
        return r.kind == 0u && cps2recomp::xmvsf_sources::IsWallBlock({
            r.index, r.tile, r.attributes, r.priority,
            static_cast<unsigned>(r.w / 16), static_cast<unsigned>(r.h / 16)});
    }
    static bool Matches(const cps2recomp::Video::GraphicRecord &r,
                        unsigned mode,
                        const cps2recomp::Video::GraphicsInspection &snapshot) {
        if (mode == 1u) return Wall(r); // verified OAM wall, any bank/slot
        if (mode == 2u) return r.kind == 0u; // ALL OAM sources
        if (mode >= 3u) return r.kind == mode - 2u; // 1..3 VRAM maps
        if (r.kind != 0u || Wall(r)) return false;
        // Do NOT classify a graphic as HUD merely because its rectangle
        // overlaps y<64; that was responsible for floating source boxes.
        const int phase = cps2recomp::xmvsf_vs::HudObjectPhase(
            static_cast<int>(snapshot.fightStatus), snapshot.gameStep);
        return XM87392::objectForPhase(
            phase, r.priority, r.palette, r.tile, r.x, r.y,
            static_cast<unsigned>(r.w / 16),
            static_cast<unsigned>(r.h / 16), false, false);
    }

public:
    bool Active() const { return active_; }
    unsigned Mode() const { return mode_; }
    unsigned Page() const { return page_; }
    unsigned SelectedInPage() const { return selectedInPage_; }
    bool TintEnabled() const { return tintEnabled_; }
    void ToggleTint() {
        if (active_) tintEnabled_ = !tintEnabled_;
    }
    void Select(int delta) {
        if (!active_ || mode_ >= 3u) return; // scroll sources have no owner pixels
        if (delta < 0 && selectedInPage_ > 0u) --selectedInPage_;
        else if (delta > 0 && selectedInPage_ + 1u < pageItems_)
            ++selectedInPage_;
    }

    void Toggle(bool focusWalls = false) {
        active_ = !active_;
        if (active_) {
            mode_ = focusWalls ? 1u : 0u;
            page_ = 0u; pageCount_ = 1u; pageItems_ = 0u;
            selectedInPage_ = 0u; tintEnabled_ = false;
        } else {
            tintEnabled_ = false;
        }
    }
    void NextMode() {
        if (active_) {
            mode_ = (mode_ + 1u) % 6u;
            page_ = 0u; pageCount_ = 1u; pageItems_ = 0u;
            selectedInPage_ = 0u;
        }
    }
    void ChangePage(int delta) {
        if (!active_) return;
        if (delta < 0 && page_ > 0u) { --page_; selectedInPage_ = 0u; pageItems_ = 0u; }
        else if (delta > 0 && page_ + 1u < pageCount_) { ++page_; selectedInPage_ = 0u; pageItems_ = 0u; }
    }

    // The 5x7 font and painting canvas are the recomp's own, with
    // RGB-low-byte pixels (native Windowed and native F6 copies).
    void Paint(std::vector<std::uint32_t> &pixels, int width, int height,
               const cps2recomp::Video::GraphicsInspection &snapshot,
               std::uint64_t frame) {
        if (!active_ || width < 128 || height < 96 ||
            pixels.size() < static_cast<std::size_t>(width) *
                            static_cast<std::size_t>(height)) return;
        using cps2recomp::Video;
        using recomp::text::Color;
        const recomp::text::Canvas cv{pixels.data(), width, height};
        const Color pale{210, 234, 241, 255};
        const Color gold{255, 203, 63, 255};
        const Color black{7, 14, 22, 225};
        std::vector<const Video::GraphicRecord *> matching;
        matching.reserve(snapshot.records.size());
        for (const auto &r : snapshot.records)
            if (r.enabled && Matches(r, mode_, snapshot) &&
                (r.kind != 0u ? (r.x < width && r.x + r.w > 0 &&
                                 r.y < height && r.y + r.h > 0)
                              : (r.inkSampled && r.visibleInkPixels > 0u)))
                matching.push_back(&r);

        const unsigned pages = std::max(1u, static_cast<unsigned>(
            (matching.size() + kPageSize - 1u) / kPageSize));
        pageCount_ = pages;
        page_ = std::min(page_, pages - 1u);
        const unsigned page = page_;
        const unsigned start = page * kPageSize;
        const unsigned stop = std::min(
            static_cast<unsigned>(matching.size()), start + kPageSize);
        pageItems_ = stop - start;
        selectedInPage_ = pageItems_ ? std::min(selectedInPage_, pageItems_ - 1u) : 0u;
        const unsigned focus = start + selectedInPage_;

        // F4 changes ONLY the displayed copy. The native recorder already
        // knows exactly which OAM slot owns each surviving opaque pixel.
        // Overlapping text/sprites, transparent holes, and all other
        // sources inside the same rectangle remain unchanged.
        if (tintEnabled_ && mode_ < 3u && focus < stop && snapshot.ownerInk) {
            const auto &selected = *matching[focus];
            source_tint::Paint(pixels, width, height, *snapshot.ownerInk,
                selected.index, selected.inkLeft, selected.inkTop,
                selected.inkRight, selected.inkBottom);
        }

        // Ink bounds come from visible, nontransparent pixels surviving
        // native scroll priority and the HUD dither. The original OAM
        // source rectangle is only reported in TSV; NEVER paint it as
        // visible art when it has no pixels in the final raster.
        auto outline = [&](const Video::GraphicRecord &r, Color c) {
            if (!r.inkSampled || !r.visibleInkPixels) return;
            const int x = r.inkLeft, y = r.inkTop;
            const int w = r.inkRight - x + 1;
            const int h = r.inkBottom - y + 1;
            if (w <= 0 || h <= 0) return;
            cv.Fill(float(x), float(y), float(w), 1.0f, c);
            cv.Fill(float(x), float(y + h - 1), float(w), 1.0f, c);
            cv.Fill(float(x), float(y), 1.0f, float(h), c);
            cv.Fill(float(x + w - 1), float(y), 1.0f, float(h), c);
        };
        // WALLS are the specific previously concealed 64x48 source
        // objects. Show the entire family outline as a group and emphasize
        // the seven selected source slots. Do not overlay SCROLL boxes
        // onto graphics that may completely occlude those VRAM sources:
        // we have no per-pixel ownership proof for scroll records yet.
        if (mode_ == 1u)
            for (const auto *r : matching)
                outline(*r, Color{80, 210, 205, 170});
        for (unsigned i = start; i < stop && mode_ < 3u; ++i) {
            const auto &r = *matching[i];
            const bool focused = i == focus;
            outline(r, focused ? gold : Color{72, 155, 175, 130});
            char tag[24] = {};
            if (r.kind == 0u)
                std::snprintf(tag, sizeof(tag), "#%u", r.index);
            else
                std::snprintf(tag, sizeof(tag), "%X", r.index);
            const int tx = std::clamp(r.inkLeft, 0, width - 36);
            const int ty = std::clamp(r.inkTop, 0, height - 11); // label touches actual ink
            cv.Fill(float(tx), float(ty), float(
                std::min(36, int(recomp::text::TextWidth(tag, 1.0f) + 3))),
                10.0f, black);
            cv.Text(float(tx + 1), float(ty + 1), 1.0f, 0,
                focused ? gold : pale, tag);
        }

        // A small, left-aligned 24px native-pixel source badge. All the
        // verbose addresses remain in the paired F6 .graphics.tsv.
        // One REAL source record from the selected page is shown in
        // a crisp (1px dot) font; no tiny fractional-dot glyphs.
        const int panelTop = height - 24;
        const int panelWidth = std::min(width, 298);
        cv.Fill(0, float(panelTop), float(panelWidth), 24.0f, black);
        cv.Fill(0, float(panelTop), float(panelWidth), 1.0f, gold);
        const char *modeName[] = {
            "HUD INK", "WALL INK", "OAM INK", "MAP 1", "MAP 2", "MAP 3"
        };
        char text[96] = {};
        // Separate format calls: exact types/argument counts, no
        // overloaded varargs from conditional formats.
        if (mode_ >= 3u) {
            std::snprintf(text, sizeof(text),
                "F9 %s %u/%u F%llu MAP SOURCE ONLY",
                modeName[mode_], page + 1u, pages,
                static_cast<unsigned long long>(frame));
        } else {
            std::snprintf(text, sizeof(text),
                "F9 %s %u/%u F4 TINT %s",
                modeName[mode_], page + 1u, pages,
                tintEnabled_ ? "ON" : "OFF");
        }
        cv.Text(3, float(panelTop + 2), 1.0f, 0, pale, text);

        if (start < stop) {
            const auto &selected = *matching[focus];
            if (selected.kind == 0u) {
                // OAM+ is bank-relative, not a fabricated bus address.
                std::snprintf(text, sizeof(text),
                    "%s#%u T%05X P%u Z%u INK%u OAM%u+%04X",
                    tintEnabled_ ? "*" : ">", selected.index,
                    selected.tile, selected.palette,
                    selected.priority, selected.visibleInkPixels,
                    selected.bank, selected.sourceAddress);
            } else {
                std::snprintf(text, sizeof(text),
                    "MAP ONLY #%X VRAM%06X T%04X P%u",
                    selected.index, selected.sourceAddress,
                    selected.tile, selected.gfxOffset, selected.palette);
            }
        } else {
            std::snprintf(text, sizeof(text),
                mode_ < 3u ? "NO VISIBLE OAM INK (OR FRAME NOT SAMPLED)" :
                              "NO SOURCE MAP ENTRIES");
        }
        cv.Text(3, float(panelTop + 12), 1.0f, 0, gold, text);

    }
};

} // namespace port::xmvsf_home
