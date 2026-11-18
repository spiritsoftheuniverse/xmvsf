// XMVSF menu audio v87.437: a single independent SDL3 stream for the three
// existing WAV assets, with bounded queueing and no FBNeo/SDL2 dependencies.
// All assets are loaded from the original repository; no copies committed.
#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace port::xmvsf_home {

inline std::string SoundPath(const char *file) {
    std::vector<std::string> roots;
    if (const char *custom = std::getenv("XMVSF_MENU_AUDIO"); custom && *custom)
        roots.emplace_back(custom);
    roots.emplace_back("../FBNeo/assets/xmvsf");
    roots.emplace_back("FBNeo/assets/xmvsf");
    roots.emplace_back("../../FBNeo/assets/xmvsf");
    for (std::string root : roots) {
        if (!root.empty() && root.back() != '/' && root.back() != '\\')
            root.push_back('/');
        const std::string full = root + file;
        std::ifstream test(full, std::ios::binary);
        if (test.good()) return full;
    }
    return {};
}

struct SoundClip {
    std::vector<int16_t> samples; // interleaved stereo, S16, 48,000Hz
    size_t Frames() const { return samples.size() / 2; }
    bool Empty() const { return samples.empty(); }

    void Load(const char *filename) {
        const std::string path = SoundPath(filename);
        if (path.empty()) {
            std::fprintf(stderr, "XMVSF menu audio: missing %s\n", filename);
            return;
        }
        SDL_AudioSpec source{};
        Uint8 *pcm = nullptr;
        Uint32 bytes = 0;
        if (!SDL_LoadWAV(path.c_str(), &source, &pcm, &bytes)) {
            std::fprintf(stderr, "XMVSF menu audio: cannot load %s: %s\n",
                         path.c_str(), SDL_GetError());
            return;
        }
        const SDL_AudioSpec wanted{SDL_AUDIO_S16, 2, 48000};
        Uint8 *converted = nullptr;
        int length = 0;
        const bool okay = SDL_ConvertAudioSamples(&source, pcm, int(bytes), &wanted,
                                                   &converted, &length);
        SDL_free(pcm);
        if (!okay || !converted || length < 4 || (length % 4) != 0) {
            std::fprintf(stderr, "XMVSF menu audio: conversion failed for %s: %s\n",
                         filename, SDL_GetError());
            SDL_free(converted);
            return;
        }
        samples.resize(size_t(length) / sizeof(int16_t));
        std::memcpy(samples.data(), converted, size_t(length));
        SDL_free(converted);
        std::fprintf(stderr, "XMVSF menu audio: %s ready (%zu frames)\n", filename, Frames());
    }
};

class MenuAudio {
    static constexpr int kRate = 48000;
    static constexpr int kBlock = 512;
    static constexpr int kQueueLimit = kRate * 4 / 10; // 100 ms at stereo S16
    static constexpr size_t kIdle = size_t(-1);

    SoundClip music_, movement_, selection_;
    SDL_AudioStream *stream_ = nullptr;
    size_t musicAt_ = 0, moveAt_ = kIdle, selectAt_ = kIdle;
    bool wasVisible_ = false;

    static int16_t Clip16(int x) {
        return int16_t(std::clamp(x, -32768, 32767));
    }

    static void AddFrame(const SoundClip &clip, size_t &at,
                         int &left, int &right, int gainPercent, bool loop) {
        if (clip.Empty() || at == kIdle) return;
        if (at >= clip.Frames()) {
            if (loop) at = 0;
            else { at = kIdle; return; }
        }
        left  += int(clip.samples[at * 2    ]) * gainPercent / 100;
        right += int(clip.samples[at * 2 + 1]) * gainPercent / 100;
        ++at;
        if (at >= clip.Frames()) {
            if (loop) at = 0;
            else at = kIdle;
        }
    }

public:
    explicit MenuAudio(bool enabled) {
        if (!enabled) return;
        music_.Load("xmvsfMenu.wav");
        movement_.Load("menu_movement1.wav");
        selection_.Load("menu_selection1.wav");
        if (music_.Empty() && movement_.Empty() && selection_.Empty()) return;
        const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, kRate};
        stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, nullptr, nullptr);
        if (!stream_) {
            std::fprintf(stderr, "XMVSF menu audio: opening output failed: %s\n", SDL_GetError());
            return;
        }
        SDL_ResumeAudioStreamDevice(stream_);
    }

    MenuAudio(const MenuAudio &) = delete;
    MenuAudio &operator=(const MenuAudio &) = delete;
    ~MenuAudio() {
        if (stream_) SDL_DestroyAudioStream(stream_);
    }

    void Move() { if (!movement_.Empty()) moveAt_ = 0; }
    void Select() { if (!selection_.Empty()) selectAt_ = 0; }

    // Call once each SDL frontend iteration, including while native game runs.
    // On menu exit the music queue is purged immediately, but the selection
    // effect is allowed to finish; there is never menu music over combat.
    void Step(bool visible) {
        if (!stream_) return;
        if (visible != wasVisible_) {
            SDL_ClearAudioStream(stream_);
            wasVisible_ = visible;
            musicAt_ = 0;
            moveAt_ = kIdle;
            if (visible) selectAt_ = kIdle;
        }
        if (!visible && selectAt_ == kIdle) return;

        // Queue only ~100ms, rather than several seconds of the 31MB WAV.
        for (int i = 0; i < 12; ++i) {
            const int queued = SDL_GetAudioStreamQueued(stream_);
            if (queued < 0 || queued >= kQueueLimit) break;
            std::array<int16_t, kBlock * 2> block{};
            bool active = false;
            for (int frame = 0; frame < kBlock; ++frame) {
                int left = 0, right = 0;
                if (visible && !music_.Empty()) {
                    AddFrame(music_, musicAt_, left, right, 72, true);
                    active = true;
                }
                if (moveAt_ != kIdle) {
                    AddFrame(movement_, moveAt_, left, right, 95, false);
                    active = true;
                }
                if (selectAt_ != kIdle) {
                    AddFrame(selection_, selectAt_, left, right, 100, false);
                    active = true;
                }
                block[size_t(frame) * 2    ] = Clip16(left);
                block[size_t(frame) * 2 + 1] = Clip16(right);
            }
            if (!active || !SDL_PutAudioStreamData(stream_, block.data(),
                      int(block.size() * sizeof(int16_t)))) break;
            if (!visible && selectAt_ == kIdle) break;
        }
    }
};

} // namespace port::xmvsf_home
