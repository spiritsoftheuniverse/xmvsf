// XMVSF native menu presentation, v87.444 (controller installer scoped fix).
// Adapted from this repository's Stage14 layout and XMGlyph bitmap atlas rules.
// Draws into the native CPS-2 Recomp pixel presenter; no SDL2 or SDL_image dependency.
#pragma once
#include "../recomp-common/menu.h"
#include "../recomp-common/text.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// stb_image v2.32; upstream nothings/stb, dual public domain / MIT.
// Compiled only in frontend_sdl.cpp (this header has one includer).
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#include "xmvsf_stb_image.h"
#endif

namespace port::xmvsf_home {

enum class Action { None, Arcade, Training, Versus, Options };

struct Image {
    int width = 0, height = 0;
    std::vector<uint32_t> pixels; // 0xAABBGGRR, red in low byte (Recomp's Canvas)
    bool Valid() const { return width > 0 && height > 0 && pixels.size() == size_t(width) * size_t(height); }
};

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
};

inline uint8_t Byte(float v) {
    return uint8_t(std::clamp(int(v + 0.5f), 0, 255));
}
inline uint32_t Pixel(const uint8_t *p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
inline Image FromRGBA(const uint8_t *p, int w, int h) {
    Image result;
    if (!p || w < 1 || h < 1 || w > 4096 || h > 4096) return result;
    result.width = w;
    result.height = h;
    result.pixels.resize(size_t(w) * size_t(h));
    for (size_t i = 0; i < result.pixels.size(); ++i)
        result.pixels[i] = Pixel(p + i * 4);
    return result;
}
inline std::vector<uint8_t> ReadBytes(const std::string &filename) {
    std::ifstream stream(filename, std::ios::binary | std::ios::ate);
    if (!stream) return {};
    const auto length = stream.tellg();
    if (length <= 0 || length > std::streamoff(32 * 1024 * 1024)) return {};
    std::vector<uint8_t> bytes(size_t(length), uint8_t(0));
    stream.seekg(0, std::ios::beg);
    if (!stream.read(reinterpret_cast<char *>(bytes.data()), length)) return {};
    return bytes;
}
inline std::string FindAsset(const char *name) {
    std::vector<std::string> folders;
    if (const char *root = std::getenv("XMVSF_MENU_ASSETS"); root && *root)
        folders.emplace_back(root);
    folders.emplace_back("../_art");
    folders.emplace_back("_art");
    folders.emplace_back("../../_art");
    for (std::string dir : folders) {
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += '/';
        const std::string path = dir + name;
        std::ifstream file(path, std::ios::binary);
        if (file.good()) return path;
    }
    return {};
}
inline Image LoadPng(const char *name) {
    const std::string path = FindAsset(name);
    if (path.empty()) return {};
    int w = 0, h = 0, src = 0;
    stbi_uc *raw = stbi_load(path.c_str(), &w, &h, &src, 4);
    const Image result = FromRGBA(raw, w, h);
    stbi_image_free(raw);
    return result;
}
inline void Blend(uint32_t &dst, uint32_t src, int opacity = 255, int light = 255) {
    const int alpha = int(src >> 24) * opacity / 255;
    if (alpha < 1) return;
    const int inv = 255 - alpha;
    const int sr = int(src & 255u) * light / 255;
    const int sg = int((src >> 8) & 255u) * light / 255;
    const int sb = int((src >> 16) & 255u) * light / 255;
    const int r = (int(dst & 255u) * inv + sr * alpha + 127) / 255;
    const int g = (int((dst >> 8) & 255u) * inv + sg * alpha + 127) / 255;
    const int b = (int((dst >> 16) & 255u) * inv + sb * alpha + 127) / 255;
    dst = 0xFF000000u | uint32_t(b) << 16 | uint32_t(g) << 8 | uint32_t(r);
}
inline Rect VisibleBounds(const Image &img) {
    if (!img.Valid()) return {};
    int x0 = img.width, y0 = img.height, x1 = -1, y1 = -1;
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x)
            if ((img.pixels[size_t(y) * img.width + x] >> 24) > 12) {
                x0 = std::min(x0, x); x1 = std::max(x1, x);
                y0 = std::min(y0, y); y1 = std::max(y1, y);
            }
    return x1 >= x0 ? Rect{x0, y0, x1 - x0 + 1, y1 - y0 + 1} : Rect{};
}
// Retain the original logo's enclosed white lettering. Only erase border-connected
// opaque white: same policy as xmvsf_menu_glyph_atlas.h in the FBNeo reference.
inline void NormalizeLogo(Image &img) {
    if (!img.Valid()) return;
    const auto white = [](uint32_t p) {
        return (p & 255u) >= 250 && ((p >> 8) & 255u) >= 250 &&
               ((p >> 16) & 255u) >= 250 && (p >> 24) >= 240;
    };
    if (!white(img.pixels[0])) return;
    std::vector<uint8_t> visited(img.pixels.size());
    std::vector<size_t> queue;
    auto add = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= img.width || y >= img.height) return;
        const size_t i = size_t(y) * img.width + x;
        if (!visited[i] && white(img.pixels[i])) {
            visited[i] = 1;
            queue.push_back(i);
        }
    };
    for (int x = 0; x < img.width; ++x) { add(x, 0); add(x, img.height - 1); }
    for (int y = 0; y < img.height; ++y) { add(0, y); add(img.width - 1, y); }
    for (size_t i = 0; i < queue.size(); ++i) {
        const size_t p = queue[i];
        img.pixels[p] &= 0x00FFFFFFu;
        const int x = int(p % img.width), y = int(p / img.width);
        add(x - 1, y); add(x + 1, y); add(x, y - 1); add(x, y + 1);
    }
}
inline void Blit(std::vector<uint32_t> &target, int width, int height,
                 const Image &source, Rect crop, Rect rect, int brightness = 255, int opacity = 255) {
    if (!source.Valid() || crop.w <= 0 || crop.h <= 0 || rect.w <= 0 || rect.h <= 0) return;
    const int x0 = std::max(0, rect.x), y0 = std::max(0, rect.y);
    const int x1 = std::min(width, rect.x + rect.w), y1 = std::min(height, rect.y + rect.h);
    for (int y = y0; y < y1; ++y) {
        const int sy = std::clamp(crop.y + int((int64_t(y - rect.y) * crop.h) / rect.h), 0, source.height - 1);
        for (int x = x0; x < x1; ++x) {
            const int sx = std::clamp(crop.x + int((int64_t(x - rect.x) * crop.w) / rect.w), 0, source.width - 1);
            Blend(target[size_t(y) * width + x], source.pixels[size_t(sy) * source.width + sx], opacity, brightness);
        }
    }
}
inline void Cover(std::vector<uint32_t> &target, int w, int h, const Image &img, int brightness) {
    if (!img.Valid()) return;
    // Center-cover rather than stretch: the native 960x540 menu GIF composition.
    const double scale = std::max(double(w) / img.width, double(h) / img.height);
    const int cropW = std::max(1, int(w / scale + 0.5));
    const int cropH = std::max(1, int(h / scale + 0.5));
    Blit(target, w, h, img, {(img.width - cropW) / 2, (img.height - cropH) / 2, cropW, cropH},
         {0, 0, w, h}, brightness);
}

// GIF decoder with actual frame durations. Image source never changes: the
// 36-second hue rotation is generated from original pixels at most 12.5 Hz.
struct Backdrop {
    std::vector<Image> frames;
    std::vector<int> endMs;
    Image tinted;
    int cycleMs = 0, shownFrame = -1, shownHue = -1;

    void Load() {
        const std::string file = FindAsset("menuanimation.gif");
        if (file.empty()) return;
        const std::vector<uint8_t> bytes = ReadBytes(file);
        if (bytes.empty()) return;
        int w = 0, h = 0, count = 0, channels = 0, *delays = nullptr;
        stbi_uc *raw = stbi_load_gif_from_memory(bytes.data(), int(bytes.size()),
                                                 &delays, &w, &h, &count, &channels, 4);
        if (!raw || count <= 0 || count > 512 || w <= 0 || h <= 0) {
            stbi_image_free(raw);
            stbi_image_free(delays);
            return;
        }
        frames.reserve(size_t(count));
        for (int i = 0; i < count; ++i) {
            frames.push_back(FromRGBA(raw + size_t(i) * w * h * 4, w, h));
            cycleMs += std::max(20, delays ? delays[i] : 100);
            endMs.push_back(cycleMs);
        }
        stbi_image_free(raw);
        stbi_image_free(delays);
        std::fprintf(stderr, "XMVSF 87.436 native menu: GIF %dx%d frames=%d loop=%dms\n",
                     w, h, count, cycleMs);
    }
    const Image &At(uint64_t now) {
        if (frames.empty() || cycleMs <= 0) return tinted;
        const int position = int(now % uint64_t(cycleMs));
        const int frame = int(std::upper_bound(endMs.begin(), endMs.end(), position) - endMs.begin());
        const int hueTick = int(now / 80);
        if (shownFrame == frame && shownHue == hueTick) return tinted;
        shownFrame = frame;
        shownHue = hueTick;
        const Image &src = frames[size_t(std::min(frame, int(frames.size()) - 1))];
        tinted.width = src.width;
        tinted.height = src.height;
        tinted.pixels.resize(src.pixels.size());
        constexpr float kPi = 3.14159265358979323846f;
        const float angle = float(now % 36000) * (2.0f * kPi / 36000.0f);
        const float co = std::cos(angle), si = std::sin(angle);
        for (size_t i = 0; i < src.pixels.size(); ++i) {
            const uint32_t p = src.pixels[i];
            const float r = float(p & 255u), g = float((p >> 8) & 255u), b = float((p >> 16) & 255u);
            const float nr = (.213f + .787f * co - .213f * si) * r +
                             (.715f - .715f * co - .715f * si) * g +
                             (.072f - .072f * co + .928f * si) * b;
            const float ng = (.213f - .213f * co + .143f * si) * r +
                             (.715f + .285f * co + .140f * si) * g +
                             (.072f - .072f * co - .283f * si) * b;
            const float nb = (.213f - .213f * co - .787f * si) * r +
                             (.715f - .715f * co + .715f * si) * g +
                             (.072f + .928f * co + .072f * si) * b;
            tinted.pixels[i] = (p & 0xFF000000u) | uint32_t(Byte(nb)) << 16 |
                                uint32_t(Byte(ng)) << 8 | uint32_t(Byte(nr));
        }
        return tinted;
    }
};

// The original proportional, pixel-exact SF2 font sheets, from _art/.
// The large atlas uses a 12x12 stride; the small uses 8x8, both 16 columns.
struct Font {
    Image sheet;
    struct Glyph { Rect rect; int advance = 0; };
    std::array<Glyph, 128> glyphs{};
    int cell = 8;

    void Load(const char *filename, int cellSize) {
        cell = cellSize;
        sheet = LoadPng(filename);
        if (!sheet.Valid()) return;
        const int tracking = cell == 12 ? 2 : 1;
        for (int ch = 32; ch <= 126; ++ch) {
            const int bx = ((ch - 32) % 16) * cell;
            const int by = ((ch - 32) / 16) * cell;
            const int span = std::clamp(sheet.width - bx, 0, cell);
            const int tall = std::clamp(sheet.height - by, 0, cell);
            int left = span, right = -1;
            for (int y = 0; y < tall; ++y)
                for (int x = 0; x < span; ++x)
                    if ((sheet.pixels[size_t(by + y) * sheet.width + (bx + x)] >> 24) > 12) {
                        left = std::min(left, x);
                        right = std::max(right, x);
                    }
            Glyph &g = glyphs[size_t(ch)];
            g.advance = cell / 2 + tracking;
            if (right >= left) {
                g.rect = {bx + left, by, right - left + 1, tall};
                g.advance = g.rect.w + tracking;
            }
        }
    }
    int Measure(const char *s) const {
        int length = 0;
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; ++p)
            length += *p < 128 ? glyphs[*p].advance : cell / 2;
        return length;
    }
    void Text(std::vector<uint32_t> &pixels, int w, int h, const char *s,
              int x, int y, float scale, int brightness, int alignment) const {
        if (!sheet.Valid() || !s) return;
        if (alignment == 1) x -= int(Measure(s) * scale / 2 + .5f);
        if (alignment == 2) x -= int(Measure(s) * scale + .5f);
        int units = 0;
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; ++p) {
            const unsigned ch = *p < 128 ? *p : 32u;
            const Glyph &g = glyphs[ch];
            if (ch >= 33 && ch <= 126 && g.rect.w > 0) {
                Rect target{x + int(units * scale + .5f), y,
                            std::max(1, int(g.rect.w * scale + .5f)),
                            std::max(1, int(g.rect.h * scale + .5f))};
                Blit(pixels, w, h, sheet, g.rect, target, brightness);
            }
            units += g.advance;
        }
    }
};

class Menu {
    friend class TexturedMenu; // 87.441: reuse already decoded artwork.
    static constexpr int W = 960, H = 540;
    bool enabled_ = false;
    bool visible_ = false;
    int selection_ = 0;
    Image logo_, logoII_;
    Font large_, small_;
    Backdrop backdrop_;
    bool warned_ = false;

    void Assets() {
        logo_ = LoadPng("logo.png");
        NormalizeLogo(logo_);
        logoII_ = LoadPng("logo_II.png");
        large_.Load("sf2font.png", 12);
        small_.Load("sf2fontsmall.png", 8);
        backdrop_.Load();
        if (!logo_.Valid() || !large_.sheet.Valid() || !small_.sheet.Valid()) {
            std::fprintf(stderr, "XMVSF 87.436 menu: source art missing; check ../_art or XMVSF_MENU_ASSETS\n");
            warned_ = true;
        }
    }
    void Text(std::vector<uint32_t> &pixels, const char *s, int x, int y,
              bool small, float scale, int brightness, int align = 1) const {
        const Font &font = small ? small_ : large_;
        if (font.sheet.Valid()) {
            font.Text(pixels, W, H, s, x, y, scale, brightness, align);
            return;
        }
        const recomp::text::Canvas canvas{pixels.data(), W, H};
        const float dot = small ? scale : scale * 1.6f;
        const int textW = int(recomp::text::TextWidth(s, dot));
        const int at = x - (align == 1 ? textW / 2 : align == 2 ? textW : 0);
        canvas.Text(float(at), float(y), dot, 0,
                    {uint8_t(brightness), uint8_t(brightness), uint8_t(brightness), 255}, s);
    }
    void Logo(std::vector<uint32_t> &pixels, uint64_t atMs) const {
        if (!logo_.Valid()) {
            Text(pixels, "X-MEN VS. STREET FIGHTER", 480, 170, false, 2.8f, 255);
            return;
        }
        const Rect source = VisibleBounds(logo_);
        if (source.w < 1 || source.h < 1) return;
        // Source-native alpha bounds. Port of MainLayout::LogoX/Y/W/H.
        const float scale = std::min(544.0f / source.w, 289.0f / source.h);
        const int drawnW = int(source.w * scale + .5f), drawnH = int(source.h * scale + .5f);
        const int x = 208 + (544 - drawnW) / 2, y = 51 + (289 - drawnH) / 2;
        const int fullX = x - int(source.x * scale + .5f);
        const int fullY = y - int(source.y * scale + .5f);
        const int pulse = std::clamp(246 + int(9.0 * std::sin(double(atMs % 1600) * 6.28318530718 / 1600.0)), 220, 255);
        Blit(pixels, W, H, logo_, source, {x, y, drawnW, drawnH}, pulse);
        // II was authored separately on the exact same full source canvas.
        if (logoII_.Valid())
            Blit(pixels, W, H, logoII_, {0, 0, logoII_.width, logoII_.height},
                 {fullX, fullY, int(logoII_.width * scale + .5f), int(logoII_.height * scale + .5f)}, pulse);
    }

public:
    explicit Menu(bool enabled) : enabled_(enabled), visible_(enabled) {
        if (enabled) Assets();
    }
    bool Enabled() const { return enabled_; }
    bool Visible() const { return visible_; }
    int Selection() const { return selection_; }
    void Open() { if (enabled_) visible_ = true; }
    void Hide() { visible_ = false; }
    void Toggle() { if (enabled_) visible_ = !visible_; }

    Action Press(unsigned input) {
        if (!visible_) return Action::None;
        if (input & recomp::kMenuUp) selection_ = (selection_ + 3) % 4;
        if (input & recomp::kMenuDown) selection_ = (selection_ + 1) % 4;
        if (input & recomp::kMenuBack) { Hide(); return Action::None; }
        if (!(input & (recomp::kMenuConfirm | recomp::kMenuRight))) return Action::None;
        switch (selection_) {
        case 0: return Action::Arcade;
        case 1: return Action::Training;
        case 2: return Action::Versus;
        case 3: return Action::Options;
        default: return Action::None;
        }
    }
    // 960x540, exactly the native source canvas of our old Stage14 menu.
    void Draw(std::vector<uint32_t> &pixels, uint64_t atMs) {
        pixels.resize(size_t(W) * H);
        std::fill(pixels.begin(), pixels.end(), 0xFF241008u); // old (8,16,36) navy
        const Image &gif = backdrop_.At(atMs);
        if (gif.Valid()) Cover(pixels, W, H, gif, 77); // authentic 30% RGB GIF treatment
        else {
            // Missing assets: restrained animated background, not a blank menu.
            const recomp::text::Canvas canvas{pixels.data(), W, H};
            for (int i = -4; i < 40; ++i) {
                const int x = i * 41 + int((atMs / 70) % 41);
                canvas.Fill(float(x), 0, 1, H, {26, 41, 71, 60});
            }
        }
        Logo(pixels, atMs);
        static constexpr std::array<const char *, 4> names = {"ARCADE", "TRAINING", "VERSUS", "OPTIONS"};
        for (int i = 0; i < 4; ++i)
            Text(pixels, names[size_t(i)], 489, 347 + i * 31, false, 1.35f,
                 i == selection_ ? 255 : 80);

        Text(pixels, "A SELECT   B / ESC BACK", 489, 495, true, 1.375f, 235);
        Text(pixels, "ESC TO RETURN", 18, 522, true, 1.08f, 185, 0);
        Text(pixels, "TEST 87.444", 942, 522, true, 1.08f, 185, 2);
    }
};

} // namespace port::xmvsf_home
