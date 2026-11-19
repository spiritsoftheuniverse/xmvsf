// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.

#include "okim6295.h"

#include "../runtime/state.h"

#include <algorithm>
#include <cmath>

namespace cps2recomp {

namespace {
    // The ADPCM step sizes: 49 of them, from 16, each about 1.1 times the one before
    struct Steps {
        int32_t size[49];
        Steps() {
            for (int i = 0; i < 49; i++) {
                size[i] = static_cast<int32_t>(std::floor(16.0 * std::pow(1.1, i)));
            }
        }
    };
    const Steps kSteps;
    // How a nibble's magnitude moves the place in the step table
    constexpr int32_t kStepMove[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
    // The attenuation of a start command's low nibble (0, 3, 6 ... 24 dB), out of 32; the rest are silent
    constexpr int32_t kVolume[16] = {0x20, 0x16, 0x10, 0x0B, 0x08, 0x06, 0x04, 0x03, 0x02, 0, 0, 0, 0, 0, 0, 0};
} // namespace

void Okim6295::Reset() {
    for (Voice &voice : m_voice) {
        voice = Voice{};
    }
    m_command = -1;
}

void Okim6295::Write(uint8_t value) {
    if (m_command >= 0) {
        // The second byte: the voices to start (bits 4-7) and their attenuation
        const uint32_t entry = static_cast<uint32_t>(m_command) * 8;
        const uint32_t start = ((RomByte(entry) << 16) | (RomByte(entry + 1) << 8) | RomByte(entry + 2)) & 0x3FFFF;
        const uint32_t end = ((RomByte(entry + 3) << 16) | (RomByte(entry + 4) << 8) | RomByte(entry + 5)) & 0x3FFFF;
        for (int i = 0; i < 4; i++) {
            if (!(value & (0x10 << i))) {
                continue;
            }
            Voice &voice = m_voice[i];
            if (voice.playing || start >= end) {
                continue; // (a voice playing is not interrupted)
            }
            voice.playing = true;
            voice.base = start;
            voice.nibble = 0;
            voice.count = (end - start + 1) * 2;
            voice.signal = -2;
            voice.step = 0;
            voice.volume = kVolume[value & 0x0F];
        }
        m_command = -1;
    } else if (value & 0x80) {
        m_command = value & 0x7F; // the sample, for the next byte
    } else {
        // Stop the voices of bits 3-6
        for (int i = 0; i < 4; i++) {
            if (value & (0x08 << i)) {
                m_voice[i].playing = false;
            }
        }
    }
}

uint8_t Okim6295::Read() const {
    uint8_t status = 0xF0;
    for (int i = 0; i < 4; i++) {
        if (m_voice[i].playing) {
            status |= 1 << i;
        }
    }
    return status;
}

int16_t Okim6295::Step() {
    int32_t sum = 0;
    for (Voice &voice : m_voice) {
        if (!voice.playing) {
            continue;
        }
        // The high nibble of a byte first
        const uint8_t byte = RomByte(voice.base + voice.nibble / 2);
        const int nibble = (voice.nibble & 1) ? (byte & 0x0F) : (byte >> 4);
        const int32_t size = kSteps.size[voice.step];
        int32_t delta = ((2 * (nibble & 7) + 1) * size) / 8;
        if (nibble & 8) {
            delta = -delta;
        }
        voice.signal = std::clamp(voice.signal + delta, -2048, 2047);
        voice.step = std::clamp(voice.step + kStepMove[nibble & 7], 0, 48);
        sum += voice.signal * voice.volume / 2;
        if (++voice.nibble >= voice.count) {
            voice.playing = false;
        }
    }
    return static_cast<int16_t>(std::clamp(sum, -32768, 32767));
}

void Okim6295::Serialize(Archive &ar) {
    ar.Mark(0x4F4B4936);
    ar.Pod(m_voice);
    ar.Pod(m_command);
    ar.Pod(m_pin7High);
}

} // namespace cps2recomp
