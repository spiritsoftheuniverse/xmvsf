// XMVSF 87.482: source Ogg, one-shot native-VS handoff audio.
// First decoded PCM queued to SDL3 starts FBNeo's exact 120+30 HUD clock.
#pragma once
#include "decoders.h"
#include "xmvsf_vs_handoff.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace port::xmvsf_home {
class VersusAudio {
    static constexpr size_t kChunk = 1024;
    static constexpr int kQueueMs = 100;
    std::unique_ptr<Decoder> decoder_;
    SDL_AudioStream *stream_ = nullptr;
    std::uint64_t sequence_ = 0;
    int rate_ = 0;
    int channels_ = 0;
    bool ended_ = false;
    bool started_ = false;
public:
    ~VersusAudio() { Stop(); }
    VersusAudio(const VersusAudio &) = delete;
    VersusAudio &operator=(const VersusAudio &) = delete;
    VersusAudio() = default;
    bool Active() const { return stream_ != nullptr; }

    void Stop() {
        if (stream_) {
            SDL_ClearAudioStream(stream_); // no tail leaking into combat/home
            SDL_DestroyAudioStream(stream_);
        }
        stream_ = nullptr;
        decoder_.reset();
        sequence_ = 0;
        rate_ = channels_ = 0;
        ended_ = started_ = false;
    }

    bool Start(std::uint64_t sequence) {
        Stop();
        if (!sequence) return false;
        // RECOMP_RUN.bat normally runs from recomp/. Support direct root runs.
        static constexpr const char *paths[] = {
            "../audio/music/versusscreen.ogg",
            "audio/music/versusscreen.ogg",
            "../../audio/music/versusscreen.ogg"
        };
        std::string path;
        for (const char *candidate : paths) {
            if (std::filesystem::is_regular_file(candidate)) {
                path = candidate;
                break;
            }
        }
        if (path.empty()) {
            std::fprintf(stderr, "XMVSF 87.482 VS Ogg missing\n");
            return false;
        }
        decoder_ = OpenDecoder(path); // reuse recomp's native Ogg Vorbis decoder
        if (!decoder_ || decoder_->rate <= 0 ||
            decoder_->channels < 1 || decoder_->channels > 8) {
            std::fprintf(stderr, "XMVSF 87.482 VS Ogg decode unavailable: %s\n", path.c_str());
            Stop();
            return false;
        }
        rate_ = decoder_->rate;
        channels_ = decoder_->channels;
        const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, rate_};
        stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, nullptr, nullptr);
        if (!stream_) {
            std::fprintf(stderr, "XMVSF 87.482 VS output unavailable: %s\n", SDL_GetError());
            Stop();
            return false;
        }
        SDL_ResumeAudioStreamDevice(stream_);
        sequence_ = sequence;
        return true;
    }

    void Step(unsigned frame, bool mute) {
        if (!stream_ || !decoder_) return;
        SDL_SetAudioStreamGain(stream_, mute ? 0.0f : 1.0f);
        // Bounded 100ms queue: combat cuts the Ogg immediately, never
        // leaves a multi-second block waiting in the device.
        const int maxBytes = rate_ * kQueueMs * 8 / 1000;
        std::array<float, kChunk * 8> interleaved{};
        std::array<float, kChunk * 2> stereo{};
        for (int chunk = 0; chunk < 8 && !ended_; ++chunk) {
            const int queued = SDL_GetAudioStreamQueued(stream_);
            if (queued < 0 || queued >= maxBytes) break;
            const size_t frames = decoder_->Read(interleaved.data(), kChunk);
            if (!frames) {
                ended_ = true; // never loop: FBNeo's actual VS one-shot
                if (!started_) {
                    started_ = true;
                    XM87392::timeline().startWithSong(
                        static_cast<unsigned>(sequence_), frame);
                }
                break;
            }
            for (size_t n = 0; n < frames; ++n) {
                stereo[2 * n] = interleaved[n * static_cast<size_t>(channels_)];
                stereo[2 * n + 1] = channels_ > 1
                    ? interleaved[n * static_cast<size_t>(channels_) + 1]
                    : stereo[2 * n];
            }
            if (!SDL_PutAudioStreamData(stream_, stereo.data(),
                 static_cast<int>(frames * 2 * sizeof(float)))) {
                std::fprintf(stderr, "XMVSF VS PCM queue failed: %s\n", SDL_GetError());
                if (!started_) {
                    XM87392::timeline().startWithSong(
                        static_cast<unsigned>(sequence_), frame);
                }
                Stop();
                return;
            }
            if (!started_) {
                started_ = true;
                XM87392::timeline().startWithSong(static_cast<unsigned>(sequence_), frame);
            }
        }
        if (ended_ && SDL_GetAudioStreamQueued(stream_) == 0) Stop();
    }
};
} // namespace port::xmvsf_home
