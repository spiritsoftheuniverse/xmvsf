// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The model viewer's exports as files: a picture with its see-through pixels as a PNG, frames with their times as an
// animated PNG (APNG) and as a GIF. A pixel is red in the low byte, then green, then blue, then how solid it is (0:
// not there, 255: there; nothing between: the board's pixels are one or the other).
//
// Pictures are kept at the board's own size and made larger as they are written (`scale`: each pixel a square of that
// many), a row at a time: at 16x a long animation's frames would be gigabytes held whole.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <zlib.h>

namespace cps2recomp::image_files {

struct Picture {
    int width = 0, height = 0;
    std::vector<uint32_t> pixels;
};

// A file opened for writing by its UTF-8 name (on Windows by its wide name: a folder with an accent is not in the
// system's code page)
inline FILE *OpenToWrite(const std::string &path) {
#ifdef _WIN32
    return _wfopen(std::filesystem::path(std::u8string(path.begin(), path.end())).wstring().c_str(), L"wb");
#else
    return fopen(path.c_str(), "wb");
#endif
}
inline bool WriteAll(const std::string &path, const std::vector<uint8_t> &bytes) {
    FILE *f = OpenToWrite(path);
    if (!f) {
        return false;
    }
    const bool written = fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    return fclose(f) == 0 && written;
}

namespace detail {
    inline void Put32(std::vector<uint8_t> &out, uint32_t v) {
        out.push_back(static_cast<uint8_t>(v >> 24));
        out.push_back(static_cast<uint8_t>(v >> 16));
        out.push_back(static_cast<uint8_t>(v >> 8));
        out.push_back(static_cast<uint8_t>(v));
    }
    inline void Put16(std::vector<uint8_t> &out, uint16_t v) {
        out.push_back(static_cast<uint8_t>(v >> 8));
        out.push_back(static_cast<uint8_t>(v));
    }
    inline void Chunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data) {
        Put32(out, static_cast<uint32_t>(data.size()));
        const size_t from = out.size();
        out.insert(out.end(), type, type + 4);
        out.insert(out.end(), data.begin(), data.end());
        Put32(out, static_cast<uint32_t>(crc32(0, out.data() + from, static_cast<uInt>(out.size() - from))));
    }
    // A picture's rows as PNG has them (RGBA, no filter), made `scale` times larger and packed a row at a time
    inline std::vector<uint8_t> Packed(const Picture &picture, int scale) {
        z_stream z = {};
        deflateInit(&z, 6);
        std::vector<uint8_t> out, row(static_cast<size_t>(picture.width) * scale * 4 + 1);
        uint8_t buffer[1 << 16];
        auto pump = [&](int flush) {
            do {
                z.next_out = buffer;
                z.avail_out = sizeof(buffer);
                deflate(&z, flush);
                out.insert(out.end(), buffer, buffer + (sizeof(buffer) - z.avail_out));
            } while (z.avail_out == 0);
        };
        for (int y = 0; y < picture.height; y++) {
            row[0] = 0;
            uint8_t *at = row.data() + 1;
            for (int x = 0; x < picture.width; x++) {
                const uint32_t p = picture.pixels[static_cast<size_t>(y) * picture.width + x];
                for (int n = 0; n < scale; n++) {
                    *at++ = static_cast<uint8_t>(p);
                    *at++ = static_cast<uint8_t>(p >> 8);
                    *at++ = static_cast<uint8_t>(p >> 16);
                    *at++ = static_cast<uint8_t>(p >> 24);
                }
            }
            for (int n = 0; n < scale; n++) { // (the same row again for each of its copies)
                z.next_in = row.data();
                z.avail_in = static_cast<uInt>(row.size());
                pump(Z_NO_FLUSH);
            }
        }
        z.next_in = nullptr;
        z.avail_in = 0;
        pump(Z_FINISH);
        deflateEnd(&z);
        return out;
    }
    inline std::vector<uint8_t> Header(int width, int height) {
        std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
        std::vector<uint8_t> ihdr;
        Put32(ihdr, static_cast<uint32_t>(width));
        Put32(ihdr, static_cast<uint32_t>(height));
        ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0}); // (8 bits, RGBA)
        Chunk(out, "IHDR", ihdr);
        return out;
    }
} // namespace detail

inline bool WritePng(const std::string &path, const Picture &picture, int scale = 1) {
    std::vector<uint8_t> out = detail::Header(picture.width * scale, picture.height * scale);
    detail::Chunk(out, "IDAT", detail::Packed(picture, scale));
    detail::Chunk(out, "IEND", {});
    return WriteAll(path, out);
}

// Frames of one size, each shown for `times[n]` sixtieths of a second, repeated for ever. A viewer that does not know
// APNG shows the first frame.
inline bool WriteApng(const std::string &path, const std::vector<Picture> &frames, const std::vector<int> &times, int scale = 1) {
    if (frames.empty()) {
        return false;
    }
    const int width = frames[0].width * scale, height = frames[0].height * scale;
    std::vector<uint8_t> out = detail::Header(width, height);
    std::vector<uint8_t> actl;
    detail::Put32(actl, static_cast<uint32_t>(frames.size()));
    detail::Put32(actl, 0); // (for ever)
    detail::Chunk(out, "acTL", actl);
    uint32_t sequence = 0;
    for (size_t n = 0; n < frames.size(); n++) {
        std::vector<uint8_t> fctl;
        detail::Put32(fctl, sequence++);
        detail::Put32(fctl, static_cast<uint32_t>(width));
        detail::Put32(fctl, static_cast<uint32_t>(height));
        detail::Put32(fctl, 0);
        detail::Put32(fctl, 0);
        detail::Put16(fctl, static_cast<uint16_t>(std::max(1, times[n])));
        detail::Put16(fctl, 60);
        fctl.push_back(1); // (cleared to nothing after it: the next frame is not drawn over this one)
        fctl.push_back(0); // (drawn in place of what is there)
        detail::Chunk(out, "fcTL", fctl);
        const std::vector<uint8_t> z = detail::Packed(frames[n], scale);
        if (n == 0) {
            detail::Chunk(out, "IDAT", z);
        } else {
            std::vector<uint8_t> fdat;
            detail::Put32(fdat, sequence++);
            fdat.insert(fdat.end(), z.begin(), z.end());
            detail::Chunk(out, "fdAT", fdat);
        }
    }
    detail::Chunk(out, "IEND", {});
    return WriteAll(path, out);
}

// The same as a GIF: one table of up to 255 colours for all the frames (the board's characters have far fewer; past
// that, a colour is the nearest of the table's) and the 256th for see-through. A GIF counts in hundredths of a second,
// so the times are rounded with what each loses carried to the next, to keep the whole the board's.
inline bool WriteGif(const std::string &path, const std::vector<Picture> &frames, const std::vector<int> &times, int scale = 1) {
    if (frames.empty()) {
        return false;
    }
    const int width = frames[0].width * scale, height = frames[0].height * scale;
    std::unordered_map<uint32_t, uint8_t> index;
    std::vector<uint32_t> colours;
    for (const Picture &frame : frames) {
        for (uint32_t p : frame.pixels) {
            if ((p >> 24) && !index.count(p & 0xFFFFFF) && colours.size() < 255) {
                index[p & 0xFFFFFF] = static_cast<uint8_t>(colours.size());
                colours.push_back(p & 0xFFFFFF);
            }
        }
    }
    const uint8_t clear = static_cast<uint8_t>(colours.size());
    auto lookup = [&](uint32_t p) -> uint8_t {
        if (!(p >> 24)) {
            return clear;
        }
        const auto at = index.find(p & 0xFFFFFF);
        if (at != index.end()) {
            return at->second;
        }
        int best = 0, nearest = 1 << 30;
        for (size_t n = 0; n < colours.size(); n++) {
            const int dr = int(p & 0xFF) - int(colours[n] & 0xFF), dg = int((p >> 8) & 0xFF) - int((colours[n] >> 8) & 0xFF),
                      db = int((p >> 16) & 0xFF) - int((colours[n] >> 16) & 0xFF);
            if (dr * dr + dg * dg + db * db < nearest) {
                nearest = dr * dr + dg * dg + db * db;
                best = static_cast<int>(n);
            }
        }
        return static_cast<uint8_t>(best);
    };

    std::vector<uint8_t> out = {'G', 'I', 'F', '8', '9', 'a'};
    auto put16 = [&](int v) {
        out.push_back(static_cast<uint8_t>(v));
        out.push_back(static_cast<uint8_t>(v >> 8));
    };
    put16(width);
    put16(height);
    out.push_back(0xF7); // (a table of 256 colours)
    out.push_back(clear);
    out.push_back(0);
    for (int n = 0; n < 256; n++) {
        const uint32_t c = n < static_cast<int>(colours.size()) ? colours[static_cast<size_t>(n)] : 0;
        out.push_back(static_cast<uint8_t>(c));
        out.push_back(static_cast<uint8_t>(c >> 8));
        out.push_back(static_cast<uint8_t>(c >> 16));
    }
    out.insert(out.end(), {0x21, 0xFF, 11, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 3, 1, 0, 0, 0}); // (for ever)

    // (the LZW table: for each code and the byte after it, the code of that string; 0: none yet)
    std::vector<uint16_t> table(static_cast<size_t>(4096) * 256);
    std::vector<uint32_t> used; // (its entries set, to clear only those when it starts again)
    auto restart = [&] {
        for (uint32_t at : used) {
            table[at] = 0;
        }
        used.clear();
    };
    std::vector<uint8_t> row;
    int owed = 0; // (in sixtieths of a hundredth: the time the rounding has lost so far)
    for (size_t n = 0; n < frames.size(); n++) {
        const int wanted = std::max(1, times[n]) * 100 + owed; // (in sixtieths of a hundredth of a second)
        const int delay = std::max(1, (wanted + 30) / 60);
        owed = wanted - delay * 60;
        out.insert(out.end(), {0x21, 0xF9, 4, 0x09, static_cast<uint8_t>(delay), static_cast<uint8_t>(delay >> 8), clear, 0});
        out.push_back(0x2C);
        put16(0);
        put16(0);
        put16(width);
        put16(height);
        out.push_back(0);

        // LZW with 8-bit codes, the table started again when it is full
        constexpr int kClear = 256, kEnd = 257;
        std::vector<uint8_t> data;
        uint32_t bits = 0;
        int count = 0, size = 9, next = kEnd + 1;
        auto code = [&](int c) {
            bits |= static_cast<uint32_t>(c) << count;
            count += size;
            while (count >= 8) {
                data.push_back(static_cast<uint8_t>(bits));
                bits >>= 8;
                count -= 8;
            }
        };
        restart();
        code(kClear);
        int prefix = -1;
        const Picture &frame = frames[n];
        row.resize(static_cast<size_t>(width));
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; x++) {
                std::fill_n(&row[static_cast<size_t>(x) * scale], scale, lookup(frame.pixels[static_cast<size_t>(y) * frame.width + x]));
            }
            for (int copy = 0; copy < scale; copy++) {
                for (uint8_t c : row) {
                    if (prefix < 0) {
                        prefix = c;
                        continue;
                    }
                    const uint32_t key = static_cast<uint32_t>(prefix) * 256 + c;
                    uint16_t &entry = table[key];
                    if (entry) {
                        prefix = entry;
                        continue;
                    }
                    code(prefix);
                    if (next < 4096) {
                        entry = static_cast<uint16_t>(next++);
                        used.push_back(key);
                        if (next > (1 << size) && size < 12) {
                            size++;
                        }
                    } else {
                        code(kClear);
                        restart();
                        next = kEnd + 1;
                        size = 9;
                    }
                    prefix = c;
                }
            }
        }
        if (prefix >= 0) {
            code(prefix);
        }
        code(kEnd);
        if (count > 0) {
            data.push_back(static_cast<uint8_t>(bits));
        }
        out.push_back(8);
        for (size_t at = 0; at < data.size(); at += 255) {
            const size_t piece = std::min<size_t>(255, data.size() - at);
            out.push_back(static_cast<uint8_t>(piece));
            out.insert(out.end(), data.begin() + static_cast<long>(at), data.begin() + static_cast<long>(at + piece));
        }
        out.push_back(0);
    }
    out.push_back(0x3B);
    return WriteAll(path, out);
}

} // namespace cps2recomp::image_files
