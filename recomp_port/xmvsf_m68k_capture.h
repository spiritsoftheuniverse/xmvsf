// XMVSF 87.481: bounded, compact F10 native M68K/selection/OAM diagnostics.
// Diagnostic observations only. No emulated RAM/ROM writes, no timing overrides.
// A capture sees every REAL emulated frame, even while SDL3 skips presentation.
#pragma once

#include "../machine/machine.h"
#include "../fight/session.h"
#include "../runtime/game.h"
#include "xmvsf_vs_handoff.h" // 87.485 read-only F10 vs native HUD clock correlation
#include "xmvsf_character_select_layout.h" // 87.504 read-only custom grid/ROM ID annotation

#include <array>
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
        // A single reusable capture, not a new timestamped directory per keypress.
        // F10 replaces "latest" on each new recording; old historical folders are not touched.
        folder_ = root / "latest";
        fs::create_directories(folder_, ec);
        if (ec) {
            std::fprintf(stderr, "XMVSF F10: cannot create %s: %s\n",
                         folder_.string().c_str(), ec.message().c_str());
            return false;
        }
        const char *rawWrites = std::getenv("XMVSF_F10_RAW_OAM");
        logRawWrites_ = rawWrites && std::strcmp(rawWrites, "1") == 0;
        frames_ = Open("frames.tsv");
        hooks_ = Open("hooks.tsv");
        writes_ = Open("oam_writes.tsv");
        oam_ = Open("oam_delta.tsv");
        ramWrites_ = Open("selection_ram_writes.tsv");
        handoff_ = Open("selection_handoff.tsv");
        if (!frames_ || !hooks_ || !writes_ || !oam_ || !ramWrites_ || !handoff_) {
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
        std::fprintf(handoff_,
            "emu_frame_begin\temu_frame_end\tevents\t"
            "p1_raw_input\tp2_raw_input\tp1_cpu_input\tp2_cpu_input\t"
            "p1_custom_grid\tp1_custom_id\tp2_custom_grid\tp2_custom_id\t"
            "p1_phase_before\tp1_phase_after\tp2_phase_before\tp2_phase_after\t"
            "p1_accept_before\tp1_accept_after\tp2_accept_before\tp2_accept_after\t"
            "p1_native_cursor_before\tp1_native_cursor_after\t"
            "p2_native_cursor_before\tp2_native_cursor_after\t"
            "p1_pick1_after\tp1_pick2_after\tp2_pick1_after\tp2_pick2_after\t"
            "p1_panel_after\tp2_panel_after\t"
            "sound_queue_next_before\tsound_queue_next_after\tsound_requests_hex\t"
            "cpu_pc_after\tcpu_cycles_after\n");
        std::fprintf(oam_,
            "emu_frame\tobject_bank_next\tobject_slot\tx_word\ty_word\t"
            "tile_word\tattribute_word\tpalette\tobject_x_offset\tobject_y_offset\t"
            "p1_phase_after\tp2_phase_after\n");
        if (FILE *meta = Open("README.txt")) {
            // F10 is never a gameplay trigger. Capture BOTH timestamps so
            // the user's marker can be measured against the actual earlier
            // versus song in the SAME emulator session, not guessed from a
            // hard-coded global frame or a different match.
            const auto &shock = XM87392::timeline();
            const auto &ink = xmvsf_vs::CombatHud();
            std::fprintf(meta,
                "HUD_CALIBRATION f10_press_frame=%llu song_start_frame=%u "
                "song_started=%u primed_frame=%u primed=%u "
                "first_verified_hp_frame=%u verified_hp_seen=%u "
                "song_elapsed_at_press=%lld "
                "game_step_FF5104=%04X status_FF5000=%02X\n",
                static_cast<unsigned long long>(machine.frame),
                shock.start, static_cast<unsigned>(shock.armed),
                shock.primedAt, static_cast<unsigned>(shock.primed),
                ink.firstInkFrame, static_cast<unsigned>(ink.firstInkSeen),
                shock.armed ? static_cast<long long>(machine.frame) -
                              static_cast<long long>(shock.start) : -1ll,
                machine.ram.size() > 0x5105u ?
                    static_cast<unsigned>((machine.ram[0x5104u] << 8) |
                                           machine.ram[0x5105u]) : 0u,
                machine.ram.size() > 0x5000u ?
                    static_cast<unsigned>(machine.ram[0x5000u]) : 0u);
            std::fprintf(meta,
                "XMVSF compact native M68K F10 capture, source version 87.485\n"
                "F10 toggles capture; a new capture REPLACES the previous latest folder files.\n"
                "Stops after %u emulated frames or 512 KiB of captured records.\n"
                "Frames include invisible native fast-forward ticks, sampled every 8 frames and on selection-state changes.\n"
                "frames.tsv: native P1/P2 selection records and input routing (sampled).\n"
                "hooks.tsv: actual entry of the hooked ROM address 00B1E8; it does NOT prove acceptance succeeded.\n"
                "oam_writes.tsv: DISABLED by default; set XMVSF_F10_RAW_OAM=1 for bounded raw bus writes.\n"
                "selection_ram_writes.tsv: bounded selection, picks and game-state writes (not repetitive panel writes).\n"
                "selection_handoff.tsv: custom 18-grid position/ROM ID vs native cursor, acceptance, picks,\n"
                "  routed input and inferred queued sound-request slots ON THE SAME EMULATED FRAME.\n"
                "  sound_requests_hex is the observed 16-byte ring data at FF2648 after FF2642\n"
                "  advances; it is NOT a decoded spoken name or proof of audible playback.\n"
                "oam_delta.tsv: sampled active object changes, compared against each bank separately.\n"
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
        ramWritesSeen_ = droppedRamWrites_ = 0;
        oamRowsSeen_ = droppedOamRows_ = 0;
        ramThisFrame_ = 0;
        haveBefore_ = false;
        grid_[0] = grid_[1] = -1;
        priorGrid_[0] = priorGrid_[1] = -1;
        soundBefore_ = 0;
        handoffRowsSeen_ = 0;
        haveOldOam_[0] = haveOldOam_[1] = false;
        frameTag_ = ramFrameTag_ = machine.frame;
        // Do not attach the high-frequency object bus probe unless explicitly requested.
        machine.objectWriteProbe = logRawWrites_ ? &Capture::WriteHook : nullptr;
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
                     "XMVSF F10 STOP (%s): %s / %llu frames, raw OAM %llu (dropped %llu), OAM deltas %llu (dropped %llu), selection writes %llu (dropped %llu)\n",
                     reason, folder_.string().c_str(),
                     static_cast<unsigned long long>(framesSeen_),
                     static_cast<unsigned long long>(writesSeen_),
                     static_cast<unsigned long long>(droppedWrites_),
                     static_cast<unsigned long long>(oamRowsSeen_),
                     static_cast<unsigned long long>(droppedOamRows_),
                     static_cast<unsigned long long>(ramWritesSeen_),
                     static_cast<unsigned long long>(droppedRamWrites_));
    }

    // Called immediately BEFORE and AFTER session::BeforeFrame, not once per
    // SDL window frame. Run-ahead speculative frames are explicitly excluded.
    void BeforeRouting(const Machine &machine) {
        if (!active_ || RunningAhead()) return;
        frameTag_ = machine.frame;
        before_ = ReadSelection(machine);
        soundBefore_ = SoundQueueNext(machine);
        grid_[0] = grid_[1] = -1;
        raw_[0] = machine.players[0];
        raw_[1] = machine.players[1];
        routed_[0] = raw_[0];
        routed_[1] = raw_[1];
        final_[0] = raw_[0];
        final_[1] = raw_[1];
        pcBefore_ = machine.cpu.pc;
        haveBefore_ = true;
    }
    // Called from the existing chooser input mapping, after real Training
    // owner routing. This only annotates F10's frame; it never edits cursor
    // state, native RAM, sound commands, or the game's selected characters.
    void CustomGridBeforeCpu(const Machine &machine, int p1, int p2) {
        if (!active_ || !haveBefore_ || RunningAhead() ||
            frameTag_ != machine.frame) return;
        grid_[0] = p1 >= 0 && p1 < 18 ? p1 : -1;
        grid_[1] = p2 >= 0 && p2 < 18 ? p2 : -1;
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
        const uint16_t soundAfter = SoundQueueNext(machine);
        WriteSelectionHandoff(machine, post, soundAfter);
        const auto training = session::SelectionDiagnostics();
        const bool stateChanged = before_.gameStep != post.gameStep ||
                                  before_.playersIn != post.playersIn ||
                                  before_.phase != post.phase ||
                                  before_.cursor != post.cursor ||
                                  before_.picks != post.picks;
        const uint8_t *objects = machine.ObjectsToCome();
        const int bank = (objects && !machine.objectRam.empty()) ?
            static_cast<int>((objects - machine.objectRam.data()) / 0x2000) : -1;
        if ((framesSeen_ % 8u == 0u) || stateChanged) {
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
        std::fprintf(frames_,
            "\t%06X\t%06X\t%04X\t%llu\t%u\t%u\t%u\t%u\t%u\t%04X\t%d\n",
            pcBefore_ & 0xFFFFFFu, machine.cpu.pc & 0xFFFFFFu,
            machine.cpu.SR(), static_cast<unsigned long long>(machine.cpu.cycles),
            static_cast<unsigned>(training.accepted), training.autoFrames,
            training.fastTail ? 1u : 0u, training.fallback ? 1u : 0u,
            training.dummyOwner ? 1u : 0u, training.heldOver, bank);
        }
        // Actual object updates need no per-write probe: observe the rendered
        // next-frame bank every sixth native frame, plus state transitions.
        if ((framesSeen_ % 6u == 0u) || stateChanged)
            WriteOamDelta(machine, post, bank);
        haveBefore_ = false;
        ++framesSeen_;
        if (framesSeen_ % 30u == 0u) {
            std::fflush(frames_);
            std::fflush(hooks_);
            std::fflush(oam_);
            std::fflush(writes_);
            std::fflush(ramWrites_);
            std::fflush(handoff_);
        }
        if (BytesWritten() >= kMaxBytes) Stop("size limit");
        else if (framesSeen_ >= kMaxFrames) Stop("frame limit");
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
    static constexpr unsigned kMaxFrames = 900; // at most 15 seconds at 60 native fps
    static constexpr uint64_t kMaxBytes = 512u * 1024u; // combined output data budget
    static constexpr uint64_t kMaxWrites = 750; // opt-in raw OAM only
    static constexpr uint64_t kMaxRamWrites = 750;
    static constexpr unsigned kMaxRamPerFrame = 6;
    static constexpr uint64_t kMaxOamRows = 1800;
    static constexpr uint64_t kMaxHandoffRows = 360; // event changes + sparse context
    static constexpr unsigned kMaxOamPerSample = 32;
    struct Selection {
        uint16_t gameStep = 0;
        uint8_t playersIn = 0;
        std::array<uint8_t, 2> phase{};
        std::array<uint8_t, 2> accepted{};
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
            s.accepted[n] = r[at + 3u];
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

    // The ROM's 16-byte sound request ring is at FF2648 (0x400 bytes);
    // FF2642 holds the next request position. Read-only snapshots can
    // correlate a command with cursor movement without decoding QSound.
    static uint16_t SoundQueueNext(const Machine &m) {
        if (m.ram.size() < 0x2A48u) return 0xFFFFu;
        return Word(m.ram.data() + 0x2642u);
    }
    static std::string SoundRequests(const Machine &m, uint16_t before,
                                     uint16_t after) {
        if (m.ram.size() < 0x2A48u || before > 0x3F0u ||
            after > 0x3F0u || (before & 15u) || (after & 15u))
            return "invalid_index";
        const unsigned delta = (static_cast<unsigned>(after) + 0x400u -
                                static_cast<unsigned>(before)) & 0x3FFu;
        if (!delta) return "-";
        static constexpr char kHex[] = "0123456789ABCDEF";
        std::string out;
        const unsigned requests = delta / 16u;
        const unsigned shown = requests < 8u ? requests : 8u;
        for (unsigned n = 0; n < shown; ++n) {
            if (n) out += ',';
            const unsigned offset = (static_cast<unsigned>(before) + 16u*n) & 0x3F0u;
            const uint8_t *bytes = m.ram.data() + 0x2648u + offset;
            for (unsigned i = 0; i < 16u; ++i) {
                out += kHex[bytes[i] >> 4u];
                out += kHex[bytes[i] & 15u];
            }
        }
        if (requests > shown) out += ",truncated";
        return out;
    }
    void WriteSelectionHandoff(const Machine &machine, const Selection &post,
                               uint16_t soundAfter) {
        if (!handoff_ || handoffRowsSeen_ >= kMaxHandoffRows) return;
        // Only the actual ROM character chooser (or a transition leaving
        // it) is relevant. No large trace during menus or combat.
        if (before_.gameStep != 4u && post.gameStep != 4u) return;
        const bool phase = before_.phase != post.phase;
        const bool accepted = before_.accepted != post.accepted;
        const bool cursor = before_.cursor != post.cursor;
        const bool picks = before_.picks != post.picks;
        const bool grid = grid_[0] != priorGrid_[0] ||
                          grid_[1] != priorGrid_[1];
        const bool sound = soundBefore_ != soundAfter;
        const bool inputs = (raw_[0] != final_[0] || raw_[1] != final_[1]);
        const bool periodic = framesSeen_ % 8u == 0u;
        if (!phase && !accepted && !cursor && !picks &&
            !grid && !sound && !inputs && !periodic) return;
        std::string events;
        const auto event = [&events](bool condition, const char *label) {
            if (!condition) return;
            if (!events.empty()) events += ',';
            events += label;
        };
        event(phase, "phase"); event(accepted, "accept");
        event(cursor, "cursor"); event(picks, "picks");
        event(grid, "grid"); event(sound, "sound_queue");
        event(inputs, "routed"); event(periodic && events.empty(), "sample");
        const auto id = [](int index) -> unsigned {
            return index >= 0 && index < 18 ?
                ::port::xmvsf_select::kSlots[static_cast<unsigned>(index)].rom : 0xFFFFu;
        };
        const auto pick = [&post](size_t at) -> unsigned {
            return Word(post.picks.data() + at);
        };
        const std::string requests = SoundRequests(machine, soundBefore_, soundAfter);
        std::fprintf(handoff_,
            "%llu\t%llu\t%s\t%04X\t%04X\t%04X\t%04X\t"
            "%d\t%04X\t%d\t%04X\t"
            "%02X\t%02X\t%02X\t%02X\t"
            "%u\t%u\t%u\t%u\t"
            "%04X\t%04X\t%04X\t%04X\t"
            "%04X\t%04X\t%04X\t%04X\t"
            "%04X\t%04X\t%04X\t%04X\t%s\t%06X\t%llu\n",
            static_cast<unsigned long long>(frameTag_),
            static_cast<unsigned long long>(machine.frame), events.c_str(),
            raw_[0], raw_[1], final_[0], final_[1],
            grid_[0], id(grid_[0]), grid_[1], id(grid_[1]),
            before_.phase[0], post.phase[0],
            before_.phase[1], post.phase[1],
            static_cast<unsigned>(before_.accepted[0]), static_cast<unsigned>(post.accepted[0]),
            static_cast<unsigned>(before_.accepted[1]), static_cast<unsigned>(post.accepted[1]),
            before_.cursor[0], post.cursor[0],
            before_.cursor[1], post.cursor[1],
            pick(0u), pick(2u), pick(4u), pick(6u),
            post.panel[0], post.panel[1],
            soundBefore_, soundAfter, requests.c_str(),
            machine.cpu.pc & 0xFFFFFFu,
            static_cast<unsigned long long>(machine.cpu.cycles));
        ++handoffRowsSeen_;
        priorGrid_[0] = grid_[0]; priorGrid_[1] = grid_[1];
        // Accept boundaries are rare and vital: flush immediately.
        if (accepted) std::fflush(handoff_);
    }

    uint64_t BytesWritten() const {
        uint64_t total = 0;
        for (FILE *f : {frames_, hooks_, writes_, oam_, ramWrites_, handoff_}) {
            if (!f) continue;
            const long n = std::ftell(f);
            if (n > 0) total += static_cast<uint64_t>(n);
        }
        return total;
    }
    FILE *Open(const char *name) const {
        return std::fopen((folder_ / name).string().c_str(), "wb");
    }
    void CloseFiles() {
        for (FILE **f : {&frames_, &hooks_, &writes_, &oam_, &ramWrites_, &handoff_}) {
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
        // Panel words are rewritten every frame without useful selection events.
        if (!selecting && !picks && !phase) return;
        if (ramFrameTag_ != frameTag_) {
            ramFrameTag_ = frameTag_;
            ramThisFrame_ = 0;
        }
        if (ramWritesSeen_ >= kMaxRamWrites || ramThisFrame_ >= kMaxRamPerFrame) {
            ++droppedRamWrites_;
            return;
        }
        ++ramWritesSeen_;
        ++ramThisFrame_;
        std::fprintf(ramWrites_,
            "%llu\t%llu\t%06X\t%u\t%04X\t%06X\t%06X\t%08X\t%08X\t%08X\t%08X\n",
            static_cast<unsigned long long>(frameTag_),
            static_cast<unsigned long long>(machine.cpu.cycles),
            at, bytes, value, machine.cpu.pc & 0xFFFFFFu,
            machine.cpu.instruction & 0xFFFFFFu, machine.cpu.d[0],
            machine.cpu.d[1], machine.cpu.a[0], machine.cpu.a[4]);
    }
    void ObjectWritten(const Machine &machine, int bank, uint32_t address) {
        if (!active_ || !logRawWrites_ || !writes_ || RunningAhead() || bank < 0 || bank > 1 ||
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
        unsigned emitted = 0;
        // CPS2 alternates two object banks. Comparing each against the other
        // incorrectly reports the whole list as changed on every other frame.
        const bool hadSnapshot = haveOldOam_[bank];
        for (unsigned slot = 0; slot < 1024; ++slot) {
            const uint8_t *p = bytes + slot * 8u;
            const uint8_t *prior = oldOam_[bank].data() + slot * 8u;
            if (hadSnapshot && std::memcmp(prior, p, 8u) == 0) continue;
            const uint16_t tile = Word(p + 4);
            // Preserve a disappearing object as well as a newly visible one;
            // ignore unused zero-tile slots, including the first snapshot.
            if (tile == 0 && (!hadSnapshot || Word(prior + 4) == 0)) continue;
            if (emitted >= kMaxOamPerSample || oamRowsSeen_ >= kMaxOamRows) {
                ++droppedOamRows_;
                continue;
            }
            const uint16_t x = Word(p), y = Word(p + 2);
            const uint16_t attributes = Word(p + 6);
            std::fprintf(oam_, "%llu\t%d\t%u\t%04X\t%04X\t%04X\t%04X\t"
                               "%02X\t%04X\t%04X\t%02X\t%02X\n",
                         static_cast<unsigned long long>(frameTag_), bank, slot,
                         x, y, tile, attributes, attributes & 0x1Fu,
                         machine.objectRegs[4], machine.objectRegs[5],
                         post.phase[0], post.phase[1]);
            ++emitted;
            ++oamRowsSeen_;
        }
        std::memcpy(oldOam_[bank].data(), bytes, oldOam_[bank].size());
        haveOldOam_[bank] = true;
    }

    Machine *machine_ = nullptr;
    bool active_ = false, haveBefore_ = false, logRawWrites_ = false;
    bool haveOldOam_[2] = {false, false};
    uint64_t framesSeen_ = 0, writesSeen_ = 0, droppedWrites_ = 0;
    uint64_t ramWritesSeen_ = 0, droppedRamWrites_ = 0;
    uint64_t oamRowsSeen_ = 0, droppedOamRows_ = 0;
    uint64_t frameTag_ = 0, ramFrameTag_ = 0;
    unsigned ramThisFrame_ = 0;
    uint32_t pcBefore_ = 0;
    uint16_t raw_[2] = {}, routed_[2] = {}, final_[2] = {};
    Selection before_;
    int grid_[2] = {-1, -1}, priorGrid_[2] = {-1, -1};
    uint16_t soundBefore_ = 0;
    uint64_t handoffRowsSeen_ = 0;
    std::array<uint8_t, 0x2000> oldOam_[2]{};
    std::filesystem::path folder_;
    FILE *frames_ = nullptr, *hooks_ = nullptr, *writes_ = nullptr;
    FILE *oam_ = nullptr, *ramWrites_ = nullptr, *handoff_ = nullptr;
};

} // namespace cps2recomp::xmvsf_f10
