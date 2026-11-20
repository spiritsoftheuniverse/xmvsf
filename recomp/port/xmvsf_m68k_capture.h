// XMVSF 87.472: F10 native M68K/selection/OAM diagnostics.
// Diagnostic observations only. No emulated RAM/ROM writes, no timing overrides.
// A capture sees every REAL emulated frame, even while SDL3 skips presentation.
#pragma once

#include "../machine/machine.h"
#include "../fight/session.h"
#include "../runtime/game.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace cps2recomp::xmvsf_f10 {

class Capture {
public:
    static Capture &Get() {
        static Capture capture;
        return capture;
    }
    Capture(const Capture &) = delete;
    Capture &operator=(const Capture &) = delete;
    bool Active() const { return active_; }
    uint64_t FramesSeen() const { return framesSeen_; }

    bool Toggle(Machine &machine, const std::string &screenFolder) {
        if (active_) {
            Stop("F10 stop");
            return false;
        }
        return Start(machine, screenFolder);
    }

    bool Start(Machine &machine, const std::string &screenFolder) {
        if (active_) Stop("restart");
        namespace fs = std::filesystem;
        std::error_code ec;
        const char *custom = std::getenv("XMVSF_M68K_CAPTURE_DIR");
        const fs::path root = custom && *custom ? fs::path(custom) :
            (fs::path(screenFolder) / "f10_m68k");
        fs::create_directories(root, ec);
        if (ec) {
            std::fprintf(stderr, "XMVSF F10: cannot create %s: %s\n",
                         root.string().c_str(), ec.message().c_str());
            return false;
        }
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        fs::path selected;
        for (unsigned seq = 0; seq < 100; ++seq) {
            const fs::path candidate = root /
                ("xmvsf_f10_" + std::to_string(ms) + "_" + std::to_string(seq));
            if (!fs::exists(candidate, ec)) {
                if (fs::create_directory(candidate, ec)) selected = candidate;
                break;
            }
        }
        if (selected.empty()) {
            std::fprintf(stderr, "XMVSF F10: cannot reserve capture folder in %s\n",
                         root.string().c_str());
            return false;
        }
        folder_ = selected;
        frames_ = Open("frames.tsv");
        hooks_ = Open("hooks.tsv");
        writes_ = Open("oam_writes.tsv");
        oam_ = Open("oam_delta.tsv");
        ramWrites_ = Open("selection_ram_writes.tsv");
        if (!frames_ || !hooks_ || !writes_ || !oam_ || !ramWrites_) {
            CloseFiles();
            std::fprintf(stderr, "XMVSF F10: file open failed in %s\n",
                         folder_.string().c_str());
            return false;
        }
        std::fprintf(frames_,
            "emu_frame_begin\temu_frame_end\tp1_input_raw\tp2_input_raw\t"
            "p1_input_after_session\tp2_input_after_session\tp1_input_to_cpu\tp2_input_to_cpu\t"
            "game_step_before\tgame_step_after\tplayers_in_before\tplayers_in_after\t"
            "p1_phase_before\tp1_phase_after\tp2_phase_before\tp2_phase_after\t"
            "p1_cursor_before\tp1_cursor_after\tp2_cursor_before\tp2_cursor_after\t"
            "p1_panel_after\tp2_panel_after\tpicks_after_hex\t"
            "both_records_before_hex\tboth_records_after_hex\t"
            "cpu_pc_before\tcpu_pc_after\tcpu_sr_after\tcpu_cycles_after\t"
            "training_p1_accept_count\ttraining_p1_auto_frames\ttraining_fast_tail\t"
            "training_auto_fallback\ttraining_dummy_input_owner\ttraining_held_latch\t"
            "oam_next_bank\n");
        std::fprintf(hooks_,
            "emu_frame\tevent\tcpu_pc_observed\tplayer\tA4_record\t"
            "native_phase_before\tcursor_word_before\tD0\tD1\tA0\tA4\tcpu_cycles\n");
        std::fprintf(writes_,
            "emu_frame\tcpu_cycles\tobject_bank\tbus_address\tobject_slot\t"
            "word_in_slot\twritten_word_after\tcpu_pc_observed\tcpu_instruction_field\tA0\tA4\n");
        std::fprintf(ramWrites_,
            "emu_frame\tcpu_cycles\tbus_address\tbytes\tvalue\t"
            "cpu_pc_observed\tcpu_instruction_field\tD0\tD1\tA0\tA4\n");
        std::fprintf(oam_,
            "emu_frame\tobject_bank_next\tobject_slot\tx_word\ty_word\t"
            "tile_word\tattribute_word\tpalette\tobject_x_offset\tobject_y_offset\t"
            "p1_phase_after\tp2_phase_after\n");
        if (FILE *meta = Open("README.txt")) {
            std::fprintf(meta,
                "XMVSF native M68K F10 capture, source version 87.472\n"
                "Press F10 once to begin, again to stop; auto-stop after %u emulated frames.\n"
                "Frames include invisible native fast-forward ticks (not SDL render ticks).\n"
                "frames.tsv: full native P1/P2 records before/after the frame, plus all input routing.\n"
                "hooks.tsv: actual entry of the hooked ROM address 00B1E8; it does NOT prove acceptance succeeded.\n"
                "oam_writes.tsv: real M68K writes to CPS2 object RAM via the 700000/708000 bus windows.\n"
                "selection_ram_writes.tsv: bus writes to FF5180/FF51A0, FF50B0 picks, FF5100 state and FF5416 panels.\n"
                "oam_delta.tsv: changed object slots in ObjectsToCome() (next-frame bank), not an inferred cursor.\n"
                "Object-write cpu_pc_observed/cpu_instruction_field are actual CPU fields at the write;\n"
                "translated-code PC is NOT guaranteed to identify the exact instruction which wrote the word.\n"
                "hex fields have no 0x prefix. Selection records are two 32-byte records at FF5180/FF51A0.\n"
                "picks_after_hex contains 8 bytes at FF50B0. Panel words at FF5416/FF5418.\n"
                "This capture does not hide objects, change selection logic, or touch game ROM files.\n"
                "Directory: %s\n",
                kMaxFrames, folder_.string().c_str());
            std::fclose(meta);
        }
        active_ = true;
        machine_ = &machine;
        framesSeen_ = 0;
        writesSeen_ = 0;
        droppedWrites_ = 0;
        haveBefore_ = false;
        haveOldOam_ = false;
        oldBank_ = -1;
        frameTag_ = machine.frame;
        machine.objectWriteProbe = &Capture::WriteHook;
        machine.selectionRamWriteProbe = &Capture::SelectionWriteHook;
        std::fprintf(stderr, "XMVSF F10 M68K CAPTURE START: %s\n", folder_.string().c_str());
        return true;
    }

    void Stop(const char *reason = "end") {
        if (!active_) return;
        active_ = false;
        if (machine_ && machine_->objectWriteProbe == &Capture::WriteHook)
            machine_->objectWriteProbe = nullptr;
        if (machine_ && machine_->selectionRamWriteProbe == &Capture::SelectionWriteHook)
            machine_->selectionRamWriteProbe = nullptr;
        machine_ = nullptr;
        CloseFiles();
        std::fprintf(stderr,
                     "XMVSF F10 M68K CAPTURE STOP (%s): %s / %llu frames, %llu writes, %llu dropped\n",
                     reason, folder_.string().c_str(),
                     static_cast<unsigned long long>(framesSeen_),
                     static_cast<unsigned long long>(writesSeen_),
                     static_cast<unsigned long long>(droppedWrites_));
    }

    // Called immediately BEFORE and AFTER session::BeforeFrame, not once per
    // SDL window frame. Run-ahead speculative frames are explicitly excluded.
    void BeforeRouting(const Machine &machine) {
        if (!active_ || RunningAhead()) return;
        frameTag_ = machine.frame;
        before_ = ReadSelection(machine);
        raw_[0] = machine.players[0];
        raw_[1] = machine.players[1];
        routed_[0] = raw_[0];
        routed_[1] = raw_[1];
        final_[0] = raw_[0];
        final_[1] = raw_[1];
        pcBefore_ = machine.cpu.pc;
        haveBefore_ = true;
    }
    void AfterRouting(const Machine &machine) {
        if (!active_ || !haveBefore_ || RunningAhead()) return;
        routed_[0] = machine.players[0];
        routed_[1] = machine.players[1];
    }
    void BeforeCpu(const Machine &machine) {
        if (!active_ || !haveBefore_ || RunningAhead()) return;
        final_[0] = machine.players[0];
        final_[1] = machine.players[1];
    }
    void AfterFrame(const Machine &machine) {
        if (!active_ || !haveBefore_ || RunningAhead()) return;
        const Selection post = ReadSelection(machine);
        const auto training = session::SelectionDiagnostics();
        std::fprintf(frames_, "%llu\t%llu\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t"
                                  "%04X\t%04X\t%02X\t%02X\t"
                                  "%02X\t%02X\t%02X\t%02X\t"
                                  "%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t",
                     static_cast<unsigned long long>(frameTag_),
                     static_cast<unsigned long long>(machine.frame),
                     raw_[0], raw_[1], routed_[0], routed_[1], final_[0], final_[1],
                     before_.gameStep, post.gameStep, before_.playersIn, post.playersIn,
                     before_.phase[0], post.phase[0], before_.phase[1], post.phase[1],
                     before_.cursor[0], post.cursor[0], before_.cursor[1], post.cursor[1],
                     post.panel[0], post.panel[1]);
        HexBytes(frames_, post.picks.data(), post.picks.size());
        std::fputc('\t', frames_);
        HexBytes(frames_, before_.records.data(), before_.records.size());
        std::fputc('\t', frames_);
        HexBytes(frames_, post.records.data(), post.records.size());
        const uint8_t *objects = machine.ObjectsToCome();
        const int bank = (objects && !machine.objectRam.empty()) ?
            static_cast<int>((objects - machine.objectRam.data()) / 0x2000) : -1;
        std::fprintf(frames_,
            "\t%06X\t%06X\t%04X\t%llu\t%u\t%u\t%u\t%u\t%u\t%04X\t%d\n",
            pcBefore_ & 0xFFFFFFu, machine.cpu.pc & 0xFFFFFFu,
            machine.cpu.SR(), static_cast<unsigned long long>(machine.cpu.cycles),
            static_cast<unsigned>(training.accepted), training.autoFrames,
            training.fastTail ? 1u : 0u, training.fallback ? 1u : 0u,
            training.dummyOwner ? 1u : 0u, training.heldOver, bank);
        WriteOamDelta(machine, post, bank);
        haveBefore_ = false;
        if (++framesSeen_ % 60u == 0u) {
            std::fflush(frames_);
            std::fflush(hooks_);
            std::fflush(oam_);
            std::fflush(writes_);
            std::fflush(ramWrites_);
        }
        if (framesSeen_ >= kMaxFrames) Stop("auto limit");
    }

    // The genuine hooked 68000 routine: this event is an ENTRY, not a
    // presumed successful character commit or a guessed phase boundary.
    void AcceptHook(const M68K &cpu, int player, uint32_t record,
                    uint8_t phase, uint16_t cursor) {
        if (!active_ || !hooks_ || RunningAhead()) return;
        std::fprintf(hooks_, "%llu\t00B1E8_entry\t%06X\t%d\t%06X\t"
                             "%02X\t%04X\t%08X\t%08X\t%08X\t%08X\t%llu\n",
                     static_cast<unsigned long long>(frameTag_),
                     cpu.pc & 0xFFFFFFu, player, record & 0xFFFFFFu,
                     phase, cursor, cpu.d[0], cpu.d[1], cpu.a[0], cpu.a[4],
                     static_cast<unsigned long long>(cpu.cycles));
        std::fflush(hooks_);
    }

private:
    Capture() = default;
    static constexpr unsigned kMaxFrames = 3600; // one minute of native frames
    static constexpr uint64_t kMaxWrites = 650000;
    struct Selection {
        uint16_t gameStep = 0;
        uint8_t playersIn = 0;
        std::array<uint8_t, 2> phase{};
        std::array<uint16_t, 2> cursor{};
        std::array<uint16_t, 2> panel{};
        std::array<uint8_t, 8> picks{};
        std::array<uint8_t, 64> records{};
    };
    static uint16_t Word(const uint8_t *p) {
        return static_cast<uint16_t>((static_cast<unsigned>(p[0]) << 8) | p[1]);
    }
    static Selection ReadSelection(const Machine &m) {
        Selection s;
        if (m.ram.size() < 65536) return s;
        const auto &r = m.ram;
        s.gameStep = Word(r.data() + 0x5104);
        s.playersIn = r[0x5101];
        for (int n = 0; n < 2; ++n) {
            const size_t at = 0x5180 + n * 0x20;
            s.phase[n] = r[at];
            s.cursor[n] = Word(r.data() + at + 0x0C);
            s.panel[n] = Word(r.data() + 0x5416 + 2 * n);
            std::memcpy(s.records.data() + n * 0x20, r.data() + at, 0x20);
        }
        std::memcpy(s.picks.data(), r.data() + 0x50B0, 8);
        return s;
    }
    static void HexBytes(FILE *out, const uint8_t *bytes, size_t size) {
        static constexpr char kHex[] = "0123456789ABCDEF";
        for (size_t n = 0; n < size; ++n) {
            std::fputc(kHex[bytes[n] >> 4], out);
            std::fputc(kHex[bytes[n] & 0xF], out);
        }
    }
    FILE *Open(const char *name) const {
        return std::fopen((folder_ / name).string().c_str(), "wb");
    }
    void CloseFiles() {
        for (FILE **f : {&frames_, &hooks_, &writes_, &oam_, &ramWrites_}) {
            if (*f) std::fclose(*f);
            *f = nullptr;
        }
    }
    static void WriteHook(const Machine &machine, int bank, uint32_t address) {
        Get().ObjectWritten(machine, bank, address);
    }
    static void SelectionWriteHook(const Machine &machine, uint32_t address,
                                   uint16_t value, unsigned bytes) {
        Get().SelectionWritten(machine, address, value, bytes);
    }
    void SelectionWritten(const Machine &machine, uint32_t address,
                          uint16_t value, unsigned bytes) {
        if (!active_ || !ramWrites_ || RunningAhead()) return;
        const uint32_t at = address & 0xFFFFFFu;
        const bool selecting = at >= 0xFF5180u && at < 0xFF51C0u;
        const bool picks = at >= 0xFF50B0u && at < 0xFF50B8u;
        const bool phase = at >= 0xFF5100u && at < 0xFF5108u;
        const bool panel = at >= 0xFF5416u && at < 0xFF541Au;
        if (!selecting && !picks && !phase && !panel) return;
        std::fprintf(ramWrites_,
            "%llu\t%llu\t%06X\t%u\t%04X\t%06X\t%06X\t%08X\t%08X\t%08X\t%08X\n",
            static_cast<unsigned long long>(frameTag_),
            static_cast<unsigned long long>(machine.cpu.cycles),
            at, bytes, value, machine.cpu.pc & 0xFFFFFFu,
            machine.cpu.instruction & 0xFFFFFFu, machine.cpu.d[0],
            machine.cpu.d[1], machine.cpu.a[0], machine.cpu.a[4]);
    }
    void ObjectWritten(const Machine &machine, int bank, uint32_t address) {
        if (!active_ || !writes_ || RunningAhead() || bank < 0 || bank > 1 ||
            machine.objectRam.size() < 0x4000) return;
        if (writesSeen_ >= kMaxWrites) {
            ++droppedWrites_;
            return;
        }
        const unsigned offset = address & 0x1FFEu;
        const unsigned slot = offset / 8u, word = (offset & 6u) / 2u;
        const uint8_t *bytes = machine.objectRam.data() + bank * 0x2000u + offset;
        const uint16_t value = Word(bytes);
        // cpu.pc and cpu.instruction are observed register fields: translated
        // native blocks do not promise an exact per-instruction writer PC.
        std::fprintf(writes_,
            "%llu\t%llu\t%d\t%06X\t%u\t%u\t%04X\t%06X\t%06X\t%08X\t%08X\n",
            static_cast<unsigned long long>(frameTag_),
            static_cast<unsigned long long>(machine.cpu.cycles), bank,
            address & 0xFFFFFFu, slot, word, value,
            machine.cpu.pc & 0xFFFFFFu, machine.cpu.instruction & 0xFFFFFFu,
            machine.cpu.a[0], machine.cpu.a[4]);
        ++writesSeen_;
    }
    void WriteOamDelta(const Machine &machine, const Selection &post, int bank) {
        if (!oam_ || bank < 0 || bank > 1) return;
        const uint8_t *bytes = machine.ObjectsToCome();
        if (!bytes) return;
        for (unsigned slot = 0; slot < 1024; ++slot) {
            const uint8_t *p = bytes + slot * 8u;
            if (haveOldOam_ && oldBank_ == bank &&
                std::memcmp(oldOam_.data() + slot * 8u, p, 8u) == 0) continue;
            const uint16_t x = Word(p), y = Word(p + 2);
            const uint16_t tile = Word(p + 4), attributes = Word(p + 6);
            std::fprintf(oam_, "%llu\t%d\t%u\t%04X\t%04X\t%04X\t%04X\t"
                               "%02X\t%04X\t%04X\t%02X\t%02X\n",
                         static_cast<unsigned long long>(frameTag_), bank, slot,
                         x, y, tile, attributes, attributes & 0x1Fu,
                         machine.objectRegs[4], machine.objectRegs[5],
                         post.phase[0], post.phase[1]);
        }
        std::memcpy(oldOam_.data(), bytes, oldOam_.size());
        oldBank_ = bank;
        haveOldOam_ = true;
    }

    Machine *machine_ = nullptr;
    bool active_ = false, haveBefore_ = false, haveOldOam_ = false;
    uint64_t framesSeen_ = 0, writesSeen_ = 0, droppedWrites_ = 0;
    uint64_t frameTag_ = 0;
    uint32_t pcBefore_ = 0;
    uint16_t raw_[2] = {}, routed_[2] = {}, final_[2] = {};
    int oldBank_ = -1;
    Selection before_;
    std::array<uint8_t, 0x2000> oldOam_{};
    std::filesystem::path folder_;
    FILE *frames_ = nullptr, *hooks_ = nullptr, *writes_ = nullptr;
    FILE *oam_ = nullptr, *ramWrites_ = nullptr;
};

} // namespace cps2recomp::xmvsf_f10
