// 87.492: reusable read-only native OAM visible-pixel evidence.
// Does not depend on SDL, ROM names, OpenGL or the UI. Only pen decisions
// that survived the real renderer may be sent to Note().
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace cps2recomp::native_video {
class VisibleOamInk {
public:
    struct Evidence {
        unsigned pixels = 0;
        int left = 0, top = 0, right = -1, bottom = -1;
    };
    static constexpr unsigned kObjects = 1024u;
    void Begin(int width, int height, int stride) {
        width_ = width; height_ = height; stride_ = stride;
        valid_ = width > 0 && height > 0 && stride >= width;
        entries_.fill({});
        owners_.assign(valid_ ? static_cast<size_t>(stride) *
                               static_cast<size_t>(height) : 0u, -1);
    }
    void Disable() {
        valid_ = false;
        owners_.clear();
    }
    bool Valid() const { return valid_; }
    bool Note(unsigned slot, int x, int y) {
        if (!valid_ || slot >= kObjects || x < 0 || y < 0 ||
            x >= width_ || y >= height_) return false;
        const size_t pixel = static_cast<size_t>(y) *
                             static_cast<size_t>(stride_) + static_cast<size_t>(x);
        if (pixel >= owners_.size() || owners_[pixel] >= 0) return false;
        // Front-most OAM pixel owns a screen pixel once, even when both
        // diagnostic renderers independently render the same native frame.
        owners_[pixel] = static_cast<std::int16_t>(slot);
        auto &ink = entries_[slot];
        if (ink.pixels++ == 0) {
            ink.left = ink.right = x;
            ink.top = ink.bottom = y;
        } else {
            ink.left = std::min(ink.left, x);
            ink.top = std::min(ink.top, y);
            ink.right = std::max(ink.right, x);
            ink.bottom = std::max(ink.bottom, y);
        }
        return true;
    }
    const Evidence &At(unsigned slot) const { return entries_[slot]; }
private:
    bool valid_ = false;
    int width_ = 0, height_ = 0, stride_ = 0;
    std::array<Evidence, kObjects> entries_{};
    std::vector<std::int16_t> owners_;
};
} // namespace cps2recomp::native_video
