// XMVSF menu audio v87.469: isolated one-shot Fight voice on Training stage confirm;
// existing WAV assets, with bounded queueing and no FBNeo/SDL2 dependencies.
// All assets are loaded from the original repository; no copies committed.
#pragma once
#include <SDL3/SDL.h>
#include "xmvsf_intro_timeline.h"
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
    roots.emplace_back("../audio");
    roots.emplace_back("audio");
    roots.emplace_back("../../audio");
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
    // A gentle one-time attack: audible from the first sample, then
    // smoothly reaches the established 72% music level in 280ms.
    static constexpr int kMusicNormalGain = 72;
    static constexpr int kMusicInitialGain = 34;
    static constexpr size_t kMusicAttackFrames = kRate * 28 / 100;

    SoundClip music_, movement_, selection_, crash_, welcome_, fight_;
    SDL_AudioStream *stream_ = nullptr;
    // Separate one-shot stream: the 87.463 menu music queue is cleared on
    // exit, so mixing "Fight!" into it would cut off the voice immediately.
    SDL_AudioStream *fightStream_ = nullptr;
    uint64_t fightUntil_ = 0;
    size_t musicAt_ = 0, moveAt_ = kIdle, selectAt_ = kIdle;
    size_t crashAt_ = kIdle, welcomeAt_ = kIdle;
    bool wasVisible_ = false;
    bool musicEntranceStarted_ = false; // deliberately NOT reset at WAV loop
    size_t musicEntranceFrames_ = 0;    // deliberately NOT reset on menu return

    // Smoothstep (zero slope at full level). Saturates permanently so
    // every later WAV loop and subsequent menu visit plays at normal gain.
    static int MusicEntranceGain(size_t elapsedFrames) {
        if (elapsedFrames >= kMusicAttackFrames) return kMusicNormalGain;
        const std::uint64_t t = std::uint64_t(elapsedFrames);
        const std::uint64_t n = std::uint64_t(kMusicAttackFrames);
        const std::uint64_t smoothNumerator = t * t * (3u * n - 2u * t);
        const std::uint64_t smoothDenominator = n * n * n;
        return kMusicInitialGain + int(
            std::uint64_t(kMusicNormalGain - kMusicInitialGain) *
            smoothNumerator / smoothDenominator);
    }

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
        // Original v87.235 simultaneous boot crash + "X-Men vs Street Fighter II" voice.
        crash_.Load("crash.wav");
        welcome_.Load("xmvsf_welcome.wav");
        fight_.Load("fight!.wav"); // existing 48kHz stereo voice from audio/
        if (music_.Empty() && movement_.Empty() && selection_.Empty() &&
            crash_.Empty() && welcome_.Empty() && fight_.Empty()) return;
        const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, kRate};
        stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, nullptr, nullptr);
        if (!stream_) {
            std::fprintf(stderr, "XMVSF menu audio: opening output failed: %s\n", SDL_GetError());
            return;
        }
        SDL_ResumeAudioStreamDevice(stream_);
        if (!fight_.Empty()) {
            fightStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                    &spec, nullptr, nullptr);
            if (fightStream_) {
                SDL_ResumeAudioStreamDevice(fightStream_);
            } else {
                std::fprintf(stderr, "XMVSF 87.469 Fight voice device: %s\n", SDL_GetError());
            }
        }
    }

    MenuAudio(const MenuAudio &) = delete;
    MenuAudio &operator=(const MenuAudio &) = delete;
    ~MenuAudio() { Close(); }

    // Called explicitly before the native frontend invokes SDL_Quit.
    // Destructor remains idempotent if it runs afterward.
    void Close() {
        if (fightStream_) SDL_DestroyAudioStream(fightStream_);
        fightStream_ = nullptr;
        fightUntil_ = 0;
        if (stream_) SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }

    // One-shot at Training stage CONFIRM, never at character-select reveal.
    // Queue the full existing WAV separately, so Step(false)'s menu
    // SDL_ClearAudioStream cannot truncate its last syllable.
    bool Fight(bool audible = true) {
        if (!audible || !fightStream_ || fight_.Empty()) return false;
        if (!SDL_ClearAudioStream(fightStream_)) return false;
        const int bytes = static_cast<int>(fight_.samples.size() * sizeof(int16_t));
        if (!SDL_PutAudioStreamData(fightStream_, fight_.samples.data(), bytes))
            return false;
        constexpr uint64_t kDeviceTailMs = 100; // finish any audio already sent to the device
        const uint64_t durationMs = (fight_.Frames() * 1000 + kRate - 1) / kRate;
        fightUntil_ = SDL_GetTicks() + durationMs + kDeviceTailMs;
        return true;
    }
    bool FightPlaying() const {
        return fightUntil_ && SDL_GetTicks() < fightUntil_;
    }

    void Move() { if (!movement_.Empty()) moveAt_ = 0; }
    void Select() { if (!selection_.Empty()) selectAt_ = 0; }

    // Call once each SDL frontend iteration, including while native game runs.
    // On menu exit the music queue is purged immediately, but the selection
    // effect is allowed to finish; there is never menu music over combat.
    void Step(bool visible) {
        // The same monotonic epoch drives SDL3 audio and the animated textures.
        // Start at the first visible menu frame, never on the return from a match.
        auto &boot = XM87235Intro::State();
        if (visible && !boot.started) {
            boot.Start(SDL_GetTicks());
            if (!crash_.Empty()) crashAt_ = 0;
            if (!welcome_.Empty()) welcomeAt_ = 0;
            std::fprintf(stderr, "XMVSF 87.452 INTRO_BEGIN t=0 crash=%d voice=%d\n",
                         !crash_.Empty(), !welcome_.Empty());
        }
        if (!stream_) return;
        if (visible != wasVisible_) {
            SDL_ClearAudioStream(stream_);
            wasVisible_ = visible;
            musicAt_ = 0;
            moveAt_ = kIdle;
            if (visible) selectAt_ = kIdle;
            else {
                // Cold-boot voice/stinger must never resume on a menu return.
                crashAt_ = kIdle;
                welcomeAt_ = kIdle;
            }
        }
        if (!visible && selectAt_ == kIdle) return;
        const bool playMenuMusic = visible &&
            boot.Elapsed(SDL_GetTicks()) >= XM87235Intro::MenuStart;

        // Queue only ~100ms, rather than several seconds of the 31MB WAV.
        for (int i = 0; i < 12; ++i) {
            const int queued = SDL_GetAudioStreamQueued(stream_);
            if (queued < 0 || queued >= kQueueLimit) break;
            std::array<int16_t, kBlock * 2> block{};
            bool active = false;
            for (int frame = 0; frame < kBlock; ++frame) {
                int left = 0, right = 0;
                if (playMenuMusic && !music_.Empty()) {
                    // The envelope is tied to the initial music entrance,
                    // never to musicAt_ (which rewinds each loop or return).
                    if (!musicEntranceStarted_) musicEntranceStarted_ = true;
                    const int gain = MusicEntranceGain(musicEntranceFrames_);
                    AddFrame(music_, musicAt_, left, right, gain, true);
                    if (musicEntranceFrames_ < kMusicAttackFrames)
                        ++musicEntranceFrames_;
                    active = true;
                }
                // Both cues start from the beginning of the original 0ms lead-in.
                if (visible && crashAt_ != kIdle) {
                    AddFrame(crash_, crashAt_, left, right, 100, false);
                    active = true;
                }
                if (visible && welcomeAt_ != kIdle) {
                    AddFrame(welcome_, welcomeAt_, left, right, 100, false);
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
