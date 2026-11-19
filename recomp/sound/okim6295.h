// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The OKI MSM6295 of the classic CPS-1 board (this fork's sound of Street Fighter II Champion Edition and Hyper
// Fighting): four voices playing 4-bit ADPCM samples from the board's sample ROMs, which the Z80 starts and stops.
//
// Written from the chip's data sheet: the ROM begins with a table of 128 samples (eight bytes each: where the
// sample starts and ends, 18 bits each), a two-byte command starts a sample on a voice with an attenuation, a
// one-byte command stops voices, and the chip makes a sample every 132 or 165 of its clocks (pin 7 high or low).
#pragma once

#include <cstddef>
#include <cstdint>

namespace cps2recomp {

class Archive;

class Okim6295 {
public:
    Okim6295(const uint8_t *samples, size_t size)
        : m_rom(samples)
        , m_romSize(size) {
        Reset();
    }
    void Reset();
    // The command port, and the status port (bits 0-3: voices playing)
    void Write(uint8_t value);
    uint8_t Read() const;
    // Pin 7: high, a sample every 132 clocks; low, every 165
    void SetPin7(bool high) {
        m_pin7High = high;
    }
    int Divider() const {
        return m_pin7High ? 132 : 165;
    }
    // One output sample (mono, about the range of 16 bits)
    int16_t Step();
    void Serialize(Archive &ar);

private:
    struct Voice {
        bool playing = false;
        uint32_t base = 0;   // the sample's first byte
        uint32_t nibble = 0; // nibbles played
        uint32_t count = 0;  // nibbles in the sample
        int32_t signal = 0;  // the decoder's 12-bit signal
        int32_t step = 0;    // its place in the step table
        int32_t volume = 0;  // out of 32
    };
    const uint8_t *m_rom;
    size_t m_romSize;
    Voice m_voice[4];
    int32_t m_command = -1; // the sample chosen by a command's first byte, waiting for its second
    bool m_pin7High = true;

    uint8_t RomByte(uint32_t address) const {
        address &= 0x3FFFF;
        return address < m_romSize ? m_rom[address] : 0;
    }
};

} // namespace cps2recomp
