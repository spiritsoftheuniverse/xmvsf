// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.

#include "cps1_sound.h"

#include "../runtime/state.h"
#include "../sound/okim6295.h"
#include "../sound/qsound.h"
#include "../third_party/ymfm_opm.h"

#include <algorithm>
#include <limits>

namespace cps2recomp {

namespace {
    constexpr uint64_t kNever = std::numeric_limits<uint64_t>::max();
    constexpr uint64_t kYmDivider = 64; // (the YM2151 makes a sample every 64 of its clocks)
    // The mix, as the board's: the YM2151's two outputs and the OKI to one speaker (the shares MAME's CPS-1 driver
    // gives them, in hundredths)
    constexpr int32_t kYmShare = 35, kOkiShare = 30;
} // namespace

struct Cps1Sound::Chips : public ymfm::ymfm_interface {
    Chips(const uint8_t *samples, size_t size)
        : ym(*this)
        , oki(samples, size) {}

    ymfm::ym2151 ym;
    Okim6295 oki;

    // What is kept in a save state
    struct Times {
        uint64_t now = 0;                    // the time the YM2151's timers are counted from (WriteYm, RunTo)
        uint64_t timerAt[2] = {kNever, kNever};
        uint64_t ymAt = 0;                   // the YM2151's next sample
        uint64_t okiClocks = 0, okiAt = 0;   // the OKI's next sample (in its clocks, and in the Z80's)
        uint64_t okiLast = 0;                // when it made its last
        uint64_t outCount = 0, outAt = 0;    // the output's samples made, and the next
        int64_t ymSum = 0;                   // the YM2151's samples since the last output (both outputs added)
        int32_t ymCount = 0;
        int32_t ymLast = 0;
        int32_t okiPrev = 0, okiCur = 0;     // the OKI's last two samples
        bool irq = false;
    } t;

    // --- ymfm_interface ---
    void ymfm_set_timer(uint32_t tnum, int32_t duration) override {
        t.timerAt[tnum] = duration < 0 ? kNever : t.now + static_cast<uint64_t>(duration);
    }
    void ymfm_update_irq(bool asserted) override {
        t.irq = asserted;
    }
    void TimerExpired(uint32_t tnum) {
        m_engine->engine_timer_expired(tnum);
        runOut++;
    }
    uint64_t runOut = 0;

    uint64_t OkiAt() const {
        return t.okiClocks * kClock / kOkiClock;
    }
    uint64_t OutAt() const {
        return (t.outCount + 1) * kClock / QSound::kSampleRate;
    }

    void Reset() {
        ym.reset();
        oki.Reset();
        oki.SetPin7(true);
        t = Times{};
        t.ymAt = kYmDivider;
        t.okiClocks = oki.Divider();
        t.okiAt = OkiAt();
        t.outAt = OutAt();
    }
};

Cps1Sound::Cps1Sound(const uint8_t *samples, size_t size)
    : m(std::make_unique<Chips>(samples, size)) {
    m->Reset();
}

Cps1Sound::~Cps1Sound() = default;

void Cps1Sound::Reset() {
    m->Reset();
}

void Cps1Sound::WriteYm(unsigned port, uint8_t value, uint64_t now) {
    // (a timer the Z80 starts counts from now; one restarted as it runs out, from when it did: RunTo)
    m->t.now = now;
    if (port == 0) {
        m->ym.write_address(value);
    } else {
        m->ym.write_data(value);
    }
}

uint8_t Cps1Sound::ReadYm(uint64_t now) {
    return m->ym.read_status();
}

void Cps1Sound::WriteOki(uint8_t value) {
    m->oki.Write(value);
}

uint8_t Cps1Sound::ReadOki() const {
    return m->oki.Read();
}

void Cps1Sound::SetPin7(bool high) {
    m->oki.SetPin7(high);
}

uint64_t Cps1Sound::TimersRunOut() const {
    return m->runOut;
}

bool Cps1Sound::Interrupt() const {
    return m->t.irq;
}

uint64_t Cps1Sound::NextEvent() const {
    const auto &t = m->t;
    return std::min({t.timerAt[0], t.timerAt[1], t.ymAt, t.okiAt, t.outAt});
}

void Cps1Sound::RunTo(uint64_t now, std::vector<int16_t> &audio) {
    auto &t = m->t;
    for (;;) {
        const uint64_t at = NextEvent();
        if (at > now) {
            break;
        }
        if (t.timerAt[0] == at || t.timerAt[1] == at) {
            t.now = at; // (the chip restarts the timer: it counts from here, so its period stays exact)
        }
        if (t.timerAt[0] == at) {
            t.timerAt[0] = kNever;
            m->TimerExpired(0);
        } else if (t.timerAt[1] == at) {
            t.timerAt[1] = kNever;
            m->TimerExpired(1);
        } else if (t.ymAt == at) {
            ymfm::ym2151::output_data out;
            m->ym.generate(&out);
            t.ymLast = out.data[0] + out.data[1];
            t.ymSum += t.ymLast;
            t.ymCount++;
            t.ymAt += kYmDivider;
        } else if (t.okiAt == at) {
            t.okiPrev = t.okiCur;
            t.okiCur = m->oki.Step();
            t.okiLast = at;
            t.okiClocks += m->oki.Divider();
            t.okiAt = m->OkiAt();
        } else {
            // The output: the YM2151's samples since the last averaged; the OKI's last two drawn a line between
            // (so it is a sample late)
            const int32_t ym = t.ymCount ? static_cast<int32_t>(t.ymSum / t.ymCount) : t.ymLast;
            t.ymSum = 0;
            t.ymCount = 0;
            const uint64_t span = t.okiAt - t.okiLast;
            const int32_t oki = span ? t.okiPrev + static_cast<int32_t>((t.okiCur - t.okiPrev) * static_cast<int64_t>(at - t.okiLast) / static_cast<int64_t>(span)) : t.okiCur;
            const int32_t mix = (ym * kYmShare + oki * kOkiShare) / 100;
            const int16_t sample = static_cast<int16_t>(std::clamp(mix, -32768, 32767));
            audio.push_back(sample);
            audio.push_back(sample);
            t.outCount++;
            t.outAt = m->OutAt();
        }
    }
}

void Cps1Sound::Serialize(Archive &ar) {
    ar.Mark(0x43505331);
    std::vector<uint8_t> chip;
    if (ar.Saving()) {
        ymfm::ymfm_saved_state state(chip, true);
        m->ym.save_restore(state);
    }
    ar.Vec(chip);
    if (!ar.Saving() && ar.Ok()) {
        ymfm::ymfm_saved_state state(chip, false);
        m->ym.save_restore(state);
    }
    m->oki.Serialize(ar);
    ar.Pod(m->t);
}

} // namespace cps2recomp
