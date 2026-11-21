// XMVSF 87.489: F9 native source-identification presentation layer.
// Drawn ONLY over a copy of the rendered picture: not a ROM/VRAM/OAM
// filter, not a gameplay trigger, and not a fabricated sprite identifier.
#pragma once
#if __has_include("../video/video.h")
#include "../video/video.h"
#include "../recomp-common/text.h"
#include "xmvsf_source_identity.h" // verified native wall exclusion
#else
// Same canonical header also lives in recomp_port/ for installer parity.
#include "../recomp/video/video.h"
#include "../recomp/recomp-common/text.h"
#include "xmvsf_source_identity.h" // mirrored standalone source identity
#endif
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace port::xmvsf_home {

class GraphicsInspector {
    bool active_ = false;
    unsigned mode_ = 0; // HUD OAM, all OAM, scroll 1/2/3
    unsigned page_ = 0;
    unsigned pageCount_ = 1; // recomputed on each presented source snapshot
    static constexpr unsigned kPageSize = 7u;

    static bool Matches(const cps2recomp::Video::GraphicRecord &r,
                        unsigned mode) {
        if (mode == 0u) {
            // An already-identified 4x3 wall block is never a HUD
            // candidate. It remains visible in ALL OAM for inspection.
            const bool wall = cps2recomp::xmvsf_sources::IsWallBlock({
                r.index, r.tile, r.attributes, r.priority,
                static_cast<unsigned>(r.w / 16), static_cast<unsigned>(r.h / 16)});
            return r.kind == 0u && r.y < 64 &&
                   r.y + r.h > 0 && !wall;
        }
        if (mode == 1u) return r.kind == 0u;
        return r.kind == mode - 1u; // modes 2,3,4 -> scroll 1,2,3
    }

public:
    bool Active() const { return active_; }
    unsigned Mode() const { return mode_; }
    unsigned Page() const { return page_; }

    void Toggle() {
        active_ = !active_;
        if (active_) { mode_ = 0u; page_ = 0u; pageCount_ = 1u; }
    }
    void NextMode() {
        if (active_) { mode_ = (mode_ + 1u) % 5u; page_ = 0u; pageCount_ = 1u; }
    }
    void ChangePage(int delta) {
        if (!active_) return;
        if (delta < 0 && page_ > 0u) --page_;
        else if (delta > 0 && page_ + 1u < pageCount_) ++page_;
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
            if (r.enabled && Matches(r, mode_) &&
                r.x < width && r.x + r.w > 0 &&
                r.y < height && r.y + r.h > 0)
                matching.push_back(&r);

        const unsigned pages = std::max(1u, static_cast<unsigned>(
            (matching.size() + kPageSize - 1u) / kPageSize));
        pageCount_ = pages;
        page_ = std::min(page_, pages - 1u);
        const unsigned page = page_;
        const unsigned start = page * kPageSize;
        const unsigned stop = std::min(
            static_cast<unsigned>(matching.size()), start + kPageSize);

        auto outline = [&](const Video::GraphicRecord &r, Color c) {
            cv.Fill(float(r.x), float(r.y), float(r.w), 1.0f, c);
            cv.Fill(float(r.x), float(r.y + r.h - 1), float(r.w), 1.0f, c);
            cv.Fill(float(r.x), float(r.y), 1.0f, float(r.h), c);
            cv.Fill(float(r.x + r.w - 1), float(r.y), 1.0f, float(r.h), c);
        };
        // Only the selected page is outlined; other sources stay in
        // the original game picture and are recorded in the F6 TSV.
        for (unsigned i = start; i < stop; ++i) {
            const auto &r = *matching[i];
            outline(r, gold);
            char tag[24] = {};
            if (r.kind == 0u)
                std::snprintf(tag, sizeof(tag), "#%u", r.index);
            else
                std::snprintf(tag, sizeof(tag), "%X", r.index);
            const int tx = std::clamp(r.x, 0, width - 36);
            const int ty = std::clamp(r.y - 10, 0, height - 11);
            cv.Fill(float(tx), float(ty), float(
                std::min(36, int(recomp::text::TextWidth(tag, 1.0f) + 3))),
                10.0f, black);
            cv.Text(float(tx + 1), float(ty + 1), 1.0f, 0, gold, tag);
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
            "HUD", "ALL OAM", "SCROLL 1", "SCROLL 2", "SCROLL 3"
        };
        char text[96] = {};
        std::snprintf(text, sizeof(text),
            "F9 %s %u/%u F%llu  F8 TYPE  [ ] PAGE",
            modeName[mode_], page + 1u, pages,
            static_cast<unsigned long long>(frame));
        cv.Text(3, float(panelTop + 2), 1.0f, 0, pale, text);

        if (start < stop) {
            const auto &selected = *matching[start];
            if (selected.kind == 0u) {
                // OAM+ is bank-relative, not a fabricated bus address.
                std::snprintf(text, sizeof(text),
                    "#%u OAM%u+%04X  T%05X G%07X P%u Z%u",
                    selected.index, selected.bank, selected.sourceAddress,
                    selected.tile, selected.gfxOffset, selected.palette,
                    selected.priority);
            } else {
                std::snprintf(text, sizeof(text),
                    "#%X VRAM%06X T%04X G%07X P%u",
                    selected.index, selected.sourceAddress,
                    selected.tile, selected.gfxOffset, selected.palette);
            }
        } else {
            std::snprintf(text, sizeof(text),
                "NO SOURCES HERE  STEP %04X", snapshot.gameStep);
        }
        cv.Text(3, float(panelTop + 12), 1.0f, 0, gold, text);

    }
};

} // namespace port::xmvsf_home
