// XMVSF 87.497: source-exact XMFDBIN2 animated character-select background.
// Uses the same 32-byte header / 16-byte index / compressed PNG payloads
// as the Final Destination stage. Never resizes or filters native 384x224
// source frames. Decode runs on a low-priority worker thread; one reusable
// SDL3 streaming texture is updated only when the selected frame changes.
// This does not currently replace the ROM picture: the custom selector will
// call Draw() once its own presentation layer is installed.
#pragma once
#include "xmvsf_home.h"
#include <SDL3/SDL.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <utility>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace port::xmvsf_home {
template<unsigned Width, unsigned Height, unsigned Frames>
class CharacterSelectFdb {
public:
    static constexpr unsigned kWidth = Width;
    static constexpr unsigned kHeight = Height;
    static constexpr unsigned kFrames = Frames;
    static constexpr unsigned kHoldNativeFrames = 4u; // Final Destination cadence

private:
    struct Entry {
        std::uint64_t offset = 0;
        std::uint32_t size = 0;
    };
    std::array<Entry, kFrames> index_{};
    std::vector<Image> decoded_;
    std::string path_;
    std::string asset_;
    SDL_Thread *thread_ = nullptr;
    SDL_Texture *texture_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    std::atomic<bool> stop_{false};
    std::atomic<unsigned> ready_{0};
    int uploaded_ = -1;

    static std::uint32_t U32(const unsigned char *p) {
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
               (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    }
    static std::uint64_t U64(const unsigned char *p) {
        return std::uint64_t(U32(p)) | (std::uint64_t(U32(p + 4)) << 32);
    }
    bool ReadIndex() {
        std::ifstream in(path_, std::ios::binary | std::ios::ate);
        if (!in) return false;
        const auto size = in.tellg();
        if (size < std::streamoff(32 + kFrames * 16)) return false;
        const std::uint64_t fileSize = static_cast<std::uint64_t>(size);
        in.seekg(0, std::ios::beg);
        unsigned char hdr[32] = {};
        if (!in.read(reinterpret_cast<char *>(hdr), sizeof hdr) ||
            std::memcmp(hdr, "XMFDBIN2", 8) != 0 ||
            U32(hdr + 8) != kWidth || U32(hdr + 12) != kHeight ||
            U32(hdr + 16) != kFrames || U32(hdr + 20) != 2u ||
            U32(hdr + 24) != kWidth * 4u || U32(hdr + 28) != 0u)
            return false;
        std::uint64_t next = 32u + std::uint64_t(kFrames) * 16u;
        for (auto &entry : index_) {
            unsigned char item[16] = {};
            if (!in.read(reinterpret_cast<char *>(item), sizeof item))
                return false;
            const std::uint64_t at = U64(item);
            const std::uint32_t bytes = U32(item + 8);
            if (at != next || bytes < 8u || bytes > 16777216u ||
                at > fileSize || bytes > fileSize - at ||
                U32(item + 12) != 0u)
                return false;
            entry = {at, bytes};
            next = at + bytes;
        }
        return next == fileSize;
    }
    static int SDLCALL Decode(void *opaque) {
        auto &self = *static_cast<CharacterSelectFdb *>(opaque);
        SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_LOW);
        std::ifstream stream(self.path_, std::ios::binary);
        if (!stream) return -1;
        for (unsigned i = 0; i < kFrames; ++i) {
            if (self.stop_.load(std::memory_order_relaxed)) break;
            const auto entry = self.index_[i];
            std::vector<unsigned char> compressed(entry.size);
            stream.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
            if (!stream.read(reinterpret_cast<char *>(compressed.data()),
                             static_cast<std::streamsize>(entry.size))) {
                std::fprintf(stderr, "XMVSF CS FDB READ FAILED frame=%u\n", i);
                break;
            }
            int w = 0, h = 0, components = 0;
            stbi_uc *raw = stbi_load_from_memory(
                compressed.data(), static_cast<int>(compressed.size()),
                &w, &h, &components, 4);
            if (!raw || w != static_cast<int>(kWidth) ||
                h != static_cast<int>(kHeight)) {
                if (raw) stbi_image_free(raw);
                std::fprintf(stderr, "XMVSF CS FDB DECODE FAILED frame=%u\n", i);
                break;
            }
            self.decoded_[i] = FromRGBA(raw, w, h);
            stbi_image_free(raw);
            if (!self.decoded_[i].Valid()) break;
            // Single writer; immutable indexed frame released to the
            // render thread only after store-release. No SDL GPU calls.
            self.ready_.store(i + 1u, std::memory_order_release);
        }
        return 0;
    }
public:
    explicit CharacterSelectFdb(std::string asset = "menu/characterselect/csbackground.fdb")
        : asset_(std::move(asset)) {}
    CharacterSelectFdb(const CharacterSelectFdb &) = delete;
    CharacterSelectFdb &operator=(const CharacterSelectFdb &) = delete;
    ~CharacterSelectFdb() { Close(); }

    void Close() {
        stop_.store(true, std::memory_order_relaxed);
        if (thread_) { SDL_WaitThread(thread_, nullptr); thread_ = nullptr; }
        if (texture_) { SDL_DestroyTexture(texture_); texture_ = nullptr; }
        renderer_ = nullptr;
        decoded_.clear();
        path_.clear();
        ready_.store(0u, std::memory_order_relaxed);
        uploaded_ = -1;
    }
    bool Open(SDL_Renderer *renderer) {
        if (renderer_ == renderer && texture_ && thread_) return true;
        Close();
        if (!renderer) return false;
        path_ = FindAsset(asset_.c_str());
        if (path_.empty() || !ReadIndex()) {
            std::fprintf(stderr, "XMVSF CS FDB MISSING/INVALID: %s\n",
                         path_.empty() ? asset_.c_str() : path_.c_str());
            return false;
        }
        texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     static_cast<int>(kWidth), static_cast<int>(kHeight));
        if (!texture_) return false;
        SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
        decoded_.resize(kFrames);
        stop_.store(false, std::memory_order_relaxed);
        renderer_ = renderer;
        thread_ = SDL_CreateThread(&Decode, "XMVSF CS FDB preload", this);
        if (!thread_) { Close(); return false; }
        std::fprintf(stderr, "XMVSF CS FDB READY native=%ux%u frames=%u hold=%u\n",
                     kWidth, kHeight, kFrames, kHoldNativeFrames);
        return true;
    }
    unsigned ReadyCount() const {
        return ready_.load(std::memory_order_acquire);
    }
    const Image *Frame(std::uint64_t elapsedNativeFrames) const {
        const unsigned count = ReadyCount();
        if (count == 0) return nullptr;
        unsigned frame = static_cast<unsigned>((elapsedNativeFrames / kHoldNativeFrames) % kFrames);
        if (frame >= count) frame = count - 1u;
        return decoded_[frame].Valid() ? &decoded_[frame] : nullptr;
    }
    // elapsedNativeFrames starts at zero on entry to character selection.
    // The custom screen owns the target native rectangle (normally 384x224);
    // overlay drawing will follow without forcing any particular scale.
    bool Draw(SDL_Renderer *renderer, std::uint64_t elapsedNativeFrames,
              const SDL_FRect &dest) {
        if (!Open(renderer)) return false;
        const unsigned ready = ReadyCount();
        if (ready == 0u) return false;
        unsigned chosen = static_cast<unsigned>(
            (elapsedNativeFrames / kHoldNativeFrames) % kFrames);
        if (chosen >= ready) chosen = ready - 1u; // hold last decoded frame
        if (uploaded_ != static_cast<int>(chosen)) {
            const Image &img = decoded_[chosen];
            if (!img.Valid() ||
                !SDL_UpdateTexture(texture_, nullptr, img.pixels.data(),
                                   static_cast<int>(kWidth * 4u)))
                return false;
            uploaded_ = static_cast<int>(chosen);
        }
        return SDL_RenderTexture(renderer, texture_, nullptr, &dest);
    }
};
using CharacterSelectBackdrop = CharacterSelectFdb<384u, 224u, 166u>;
using CharacterSelectPortraitBackdrop = CharacterSelectFdb<160u, 160u, 89u>;
} // namespace port::xmvsf_home
