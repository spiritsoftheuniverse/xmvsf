// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The menu's Sound filter (sound_filter): the board's sound softened before it is played. The QSound chip plays
// 8-bit samples at any pitch without smoothing between them, at about 24 kHz, and what it hands over is harsh in
// the treble (a hiss and grit that phone speakers hide, and headphones and good speakers do not). On the board the
// sound went on through a DAC and amplifier that took some of that away; this is a low-pass in its place, by ear,
// not a model of those parts. Only what is heard: the sound the game makes, recordings and online play are as
// they were.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace port {

class SoundFilter {
public:
    // The choices, the first being off: how far down the treble is taken (Hz, at the 12 dB an octave of a
    // second-order Butterworth low-pass)
    static constexpr double kCutoff[] = {0, 8000, 5000};
    static constexpr int kChoices = 3;

    // Interleaved stereo, filtered in place, at `rate` samples a second; `choice` from FeatureValue
    void Apply(int16_t *samples, size_t frames, int choice, int rate) {
        choice = std::clamp(choice, 0, kChoices - 1);
        if (choice != m_choice || rate != m_rate) {
            Set(choice, rate);
        }
        if (!m_choice) {
            return;
        }
        for (size_t i = 0; i < frames; i++) {
            for (int c = 0; c < 2; c++) {
                int16_t &s = samples[i * 2 + c];
                State &z = m_state[c];
                // (direct form II transposed)
                const double in = s;
                const double out = m_b0 * in + z.z1;
                z.z1 = m_b1 * in - m_a1 * out + z.z2;
                z.z2 = m_b2 * in - m_a2 * out;
                s = static_cast<int16_t>(std::clamp(std::lround(out), -32768L, 32767L));
            }
        }
    }

private:
    struct State {
        double z1 = 0, z2 = 0;
    };
    void Set(int choice, int rate) {
        m_choice = choice;
        m_rate = rate;
        m_state[0] = m_state[1] = {};
        if (!choice) {
            return;
        }
        // (the RBJ cookbook's low-pass, Q = 1/sqrt 2)
        const double w = 2 * 3.14159265358979323846 * std::min(kCutoff[choice], rate * 0.45) / rate;
        const double alpha = std::sin(w) / (2 * std::sqrt(0.5));
        const double a0 = 1 + alpha;
        m_b0 = m_b2 = (1 - std::cos(w)) / 2 / a0;
        m_b1 = (1 - std::cos(w)) / a0;
        m_a1 = -2 * std::cos(w) / a0;
        m_a2 = (1 - alpha) / a0;
    }

    int m_choice = 0, m_rate = 0;
    double m_b0 = 1, m_b1 = 0, m_b2 = 0, m_a1 = 0, m_a2 = 0;
    State m_state[2];
};

} // namespace port
