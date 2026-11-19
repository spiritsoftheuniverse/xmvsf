// XMVSF 87.464: moving curl-noise Bayer entrance at the upper/lower menu edges.
// Standalone C++17: no SDL dependency; exercised by test_menu_film.py.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace port::xmvsf_home {
struct FilmEdgeDither {
    static constexpr int kScreenHeight = 540;
    static constexpr int kBand = 108; // 20% of the 540px canvas, at each end
    static constexpr double kEntryCoverage = 0.20;
    static constexpr std::array<std::uint8_t, 64> kBayer8 = {
         0,48,12,60, 3,51,15,63,
        32,16,44,28,35,19,47,31,
         8,56, 4,52,11,59, 7,55,
        40,24,36,20,43,27,39,23,
         2,50,14,62, 1,49,13,61,
        34,18,46,30,33,17,45,29,
        10,58, 6,54, 9,57, 5,53,
        42,26,38,22,41,25,37,21
    };
    static std::uint64_t Hash(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    static double Lattice(int x, int y, std::uint64_t seed) {
        const std::uint64_t h = Hash(seed ^
            (std::uint64_t(std::int64_t(x)) * 0x9e3779b185ebca87ULL) ^
            (std::uint64_t(std::int64_t(y)) * 0xc2b2ae3d27d4eb4fULL));
        return double(h >> 40) / double(0xFFFFFFu) * 2.0 - 1.0;
    }
    struct Gradient { double dx, dy; };
    static Gradient PotentialGradient(double x, double y, std::uint64_t seed) {
        const int ix = int(std::floor(x)), iy = int(std::floor(y));
        const double fx = x - double(ix), fy = y - double(iy);
        const double sx = fx * fx * (3.0 - 2.0 * fx);
        const double sy = fy * fy * (3.0 - 2.0 * fy);
        const double dsx = 6.0 * fx * (1.0 - fx);
        const double dsy = 6.0 * fy * (1.0 - fy);
        const double a = Lattice(ix, iy, seed);
        const double b = Lattice(ix + 1, iy, seed);
        const double c = Lattice(ix, iy + 1, seed);
        const double d = Lattice(ix + 1, iy + 1, seed);
        return {
            ((b - a) * (1.0 - sy) + (d - c) * sy) * dsx,
            ((c - a) * (1.0 - sx) + (d - b) * sx) * dsy
        };
    }
    struct Flow { double x, y; };
    static Flow Curl(double x, double y, std::uint64_t now,
                     std::uint64_t seed, bool down) {
        const double seconds = double(now) * 0.001;
        const double sign = down ? 1.0 : -1.0;
        // u = +d(psi)/dy; v = -d(psi)/dx. Perpendicular derivatives of a
        // two-octave advected noise potential yield a divergence-free flow.
        const Gradient broad = PotentialGradient(
            x / 58.0 + seconds * 0.075,
            y / 58.0 + sign * seconds * 0.22, seed);
        const Gradient detail = PotentialGradient(
            x / 25.0 - seconds * 0.11,
            y / 25.0 + sign * seconds * 0.19,
            seed ^ 0x6a09e667f3bcc909ULL);
        return {
            std::clamp(5.0 * broad.dy + 2.5 * detail.dy, -7.0, 7.0),
            std::clamp(-5.0 * broad.dx - 2.5 * detail.dx, -7.0, 7.0)
        };
    }
    static bool Visible(int screenX, int screenY, std::uint64_t now,
                        bool down, std::int64_t ordinal) {
        if (screenY < 0 || screenY >= kScreenHeight) return false;
        const int edgeDistance = std::min(screenY, kScreenHeight - 1 - screenY);
        if (edgeDistance >= kBand) return true;
        const std::uint64_t seed = Hash(std::uint64_t(ordinal) ^
            (down ? 0xd1b54a32d192ed03ULL : 0x94d049bb133111ebULL));
        const Flow curl = Curl(double(screenX), double(screenY), now, seed, down);
        const double warped = std::clamp(
            double(edgeDistance) + curl.y * 1.5, 0.0, double(kBand));
        const double t = warped / double(kBand);
        const double soft = t * t * (3.0 - 2.0 * t);
        const double coverage = kEntryCoverage + (1.0 - kEntryCoverage) * soft;
        // Ordered dither keeps individual pixels solid rather than making
        // the already translucent backing/character silhouette ghostly.
        const int dx = (screenX + int(std::floor(curl.x))) & 7;
        const int dy = (screenY + int(std::floor(curl.y))) & 7;
        const double threshold = (double(kBayer8[std::size_t(dy * 8 + dx)]) + 0.5) / 64.0;
        return threshold < coverage;
    }
};
} // namespace port::xmvsf_home
