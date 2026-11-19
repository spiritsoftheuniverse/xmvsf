// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#include "shmup.h"

#include "state.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>

#include <zlib.h>

namespace cps2recomp::shmup {

namespace {
    constexpr const char *kModes = "Alternative modes", *kPractice = "Training mode", *kAttract = "Attract mode", *kScore = "Score attack",
                          *kCaravan = "Caravan", *kRush = "Boss rush", *kViewer = "Stage viewer";
    constexpr int kStartMost = 20000;  // frames after which getting to a game (or to a boss) is given up
    constexpr int kShownFor = 6 * 60;  // frames a mode's words stay up
    constexpr int kSnapshotEvery = 60; // frames between the snapshots kept while running unseen to a boss
    constexpr int kSnapshots = 4;      // ... how many (the boss is met from two or three seconds before its clock)
    constexpr int kFps = 60;
    constexpr int kViewKeyEvery = 30; // the stage viewer: frames between the states it keeps for going back
    constexpr int kViewKeys = 2400;   // ... how many at most (20 minutes of a stage)
    constexpr int kViewBack = 2;      // ... frames it goes back for each picture while back is held
    constexpr int kViewForward = 4;   // ... times as fast while forward is held
    // The slowdown (the Game page): arcade perfect (the board's timing, Machine::SetBoardTiming), or none: every clock
    // the 68000's and this many times as many (enough for the busiest screens: none of Progear's frames runs short)
    constexpr int kOverclock = 4;

    Description d;

    enum class Mode : int32_t { None, Practice, Attract, Score, Caravan, Rush, Viewer };
    enum class Phase : int32_t {
        Starting,  // unseen from the board's start to a game's first stage (or seen at the choice of fighter)
        Choosing,  // the player chooses a fighter
        ToBoss,    // unseen, the ships out of harm, to the next boss
        Playing,
        Replaying, // practice: the last attempt shown again
        Over,      // the mode's words up, then the mode ends
    };
    struct State {
        Mode mode = Mode::None;
        Phase phase = Phase::Starting;
        int32_t frames = 0;      // frames in this phase
        Begin begin;
        bool credited = false, toBoss = false;
        // the scores: the most seen, the best kept, a new best; the mode's words (frames left)
        uint64_t score = 0, best = 0;
        bool newBest = false;
        int32_t shown = 0;
        // caravan: its length and frames left; boss rush: bosses beaten and the frames it has taken
        int32_t caravanFrames = 0, caravanLength = 0, bosses = 0, rushFrames = 0;
        bool bossWas = false;
        // the summary: what was had at the boss's or the stage's start, and what is shown (frames left)
        // (two counts: [0] since the last boss came or went, [1] since the stage began; each shown in its own place)
        struct Counts {
            int32_t frames = 0, shipsLost = 0, bombsUsed = 0;
            uint64_t multiplierAt = 0;
        } counts[2];
        int32_t stageWas = -1, summaryShown[2] = {0, 0};
        int32_t lastShips = 0, lastBombs = 0;
        char summary[2][2][96] = {};
        // replay: where the last attempt's controls are shown from
        int32_t replayAt = 0;
        // boss rush: a boss was on in the frame before (apart from the summary's, which counts only while a stage is
        // played)
        bool rushBossWas = false;
        int32_t outOfGame = 0; // (frames on the way to a boss with no game on)
    } g;
    bool g_fast = false;
    // Kept apart from the save state (they are machines' save states, and the controls of an attempt): practice's start,
    // the snapshots kept on the way to a boss, and the last attempt's controls (and the one before, while replaying)
    std::vector<uint8_t> g_start;
    std::vector<std::vector<uint8_t>> g_snapshots;
    std::vector<uint32_t> g_attempt, g_replay;
    std::map<std::string, uint64_t> g_bests;
    bool g_bestsRead = false;
    // The hit boxes go with the objects the board shows (drawing only: not in the state): the boxes of the frame before,
    // the object list the board takes next (a hash of it: a new one is a list the game has made), and the boxes of the
    // places that list was made from
    std::vector<HudBox> g_boxesBefore, g_boxesListed;
    uint32_t g_listed = 0;
    // The stage viewer's own scrolling (kept apart from the save state, as practice's start: machines' save states and
    // where the viewer is): the frames since the stage began, states kept every kViewKeyEvery (packed), going back (the
    // states of the frames behind, from the frame g_viewBehindFrom on, and the frame they are being remade to: -1, not),
    // stopped (the state it holds), and the controls of the frame before
    struct ViewKey {
        int32_t at;
        uLongf size;
        std::vector<uint8_t> packed;
    };
    std::vector<ViewKey> g_viewKeys;
    int32_t g_viewAt = 0, g_viewBehindFrom = 0, g_viewRemaking = -1;
    std::vector<std::vector<uint8_t>> g_viewBehind;
    std::vector<uint8_t> g_viewHeld;
    bool g_viewStopped = false, g_viewQuiet = false;
    uint16_t g_viewControls = 0;
    int g_cpuSpeedSet = 1; // (the 68000's speed this file last set: an option's --cpu-speed is left alone till then)

    void Fast(bool on) {
        if (on != g_fast) {
            g_fast = on;
            SetFastForward(on);
        }
    }

    void End() {
        if (g.mode == Mode::None) {
            return;
        }
        g = State();
        Fast(false);
        SetHud({});
        SetBoxes({});
        g_boxesBefore.clear();
        g_boxesListed.clear();
        g_snapshots.clear();
        g_viewKeys.clear();
        g_viewHeld.clear();
        g_viewAt = g_viewBehindFrom = 0;
        g_viewRemaking = -1;
        g_viewBehind.clear();
        g_viewStopped = g_viewQuiet = false;
        SetFastMotion(1);
        SetSlowMotion(1);
    }

    // --- the best scores, beside settings.ini ---

    std::string BestPath() {
        const std::string &settings = SettingsPath();
        const size_t slash = settings.find_last_of('/');
        return (slash == std::string::npos ? std::string(".") : settings.substr(0, slash)) + "/shmup_scores.txt";
    }
    void ReadBests() {
        if (g_bestsRead) {
            return;
        }
        g_bestsRead = true;
        std::ifstream file(BestPath());
        std::string line;
        while (std::getline(file, line)) {
            char name[64];
            unsigned long long value = 0;
            if (std::sscanf(line.c_str(), "%63s = %llu", name, &value) == 2) {
                g_bests[name] = value;
            }
        }
    }
    void WriteBests() {
        if (SettingsPath().empty() || RunningAhead()) {
            return; // (a test run with no settings: nothing kept)
        }
        std::ofstream file(BestPath());
        file << "# The best scores of the in-game menu's shooting modes\n";
        for (const auto &[name, value] : g_bests) {
            file << name << " = " << value << "\n";
        }
    }
    // (a game with no slowdown is another game: its bests are kept apart)
    std::string BestName() {
        const std::string less = FeatureValue("slowdown") ? "_no_slowdown" : "";
        switch (g.mode) {
        case Mode::Score: return "score_attack" + less;
        case Mode::Caravan: return "caravan_" + std::to_string(g.caravanLength) + less;
        case Mode::Rush: return "boss_rush" + less;
        default: return "";
        }
    }

    std::string Grouped(uint64_t value) {
        std::string digits = std::to_string(value), text;
        for (size_t n = 0; n < digits.size(); n++) {
            if (n && (digits.size() - n) % 3 == 0) {
                text += ',';
            }
            text += digits[n];
        }
        return text;
    }
    std::string Clock(int32_t frames) {
        const int32_t seconds = std::max(0, frames) / kFps;
        char text[16];
        std::snprintf(text, sizeof text, "%d:%02d", seconds / 60, seconds % 60);
        return text;
    }

    // --- starting the modes ---

    void Start(Machine &machine, Mode mode, const Begin &begin, bool toBoss) {
        End();
        g.mode = mode;
        g.phase = Phase::Starting;
        g.begin = begin;
        g.toBoss = toBoss;
        ReadBests();
        g.best = g_bests[BestName()];
        g_start.clear();
        g_attempt.clear();
        machine.Reset();
    }

    int Choice(const char *name, int asTheGame = -1) {
        const int value = FeatureValue(name);
        return value == 0 ? asTheGame : value - 1;
    }

    void StartPractice(Machine &machine) {
        Begin begin;
        begin.stage = std::clamp(FeatureValue("practice_stage"), 0, static_cast<int>(d.stages.size()) - 1);
        begin.fighter = FeatureValue("practice_fighter");
        Start(machine, Mode::Practice, begin, FeatureValue("practice_at") == 1);
    }
    void StartAttract(Machine &machine) {
        End();
        g.mode = Mode::Attract;
        g.phase = Phase::Starting;
        machine.Reset();
    }
    void StartScore(Machine &machine) {
        Start(machine, Mode::Score, Begin{}, false);
    }
    void StartCaravan(Machine &machine) {
        static const int kLengths[] = {2, 3, 5};
        Start(machine, Mode::Caravan, Begin{}, false);
        g.caravanLength = kLengths[std::clamp(FeatureValue("caravan_length"), 0, 2)];
        g.caravanFrames = g.caravanLength * 60 * kFps;
        g.best = g_bests[BestName()];
    }
    void StartRush(Machine &machine) {
        Start(machine, Mode::Rush, Begin{}, true);
    }
    void StartViewer(Machine &machine) {
        Begin begin;
        begin.stage = std::clamp(FeatureValue("viewer_stage"), 0, static_cast<int>(d.stages.size()) - 1);
        begin.fighter = 0;
        Start(machine, Mode::Viewer, begin, false);
    }

    // --- practice's start, kept in memory ---

    void Snapshot(Machine &machine, std::vector<uint8_t> &state) {
        state.clear();
        Archive ar(state, true);
        machine.Serialize(ar);
    }
    void Restore(Machine &machine, const std::vector<uint8_t> &state) {
        std::vector<uint8_t> copy = state;
        Archive ar(copy, false);
        machine.Serialize(ar);
    }

    void ResetSummary() {
        g.stageWas = d.stage ? d.stage() : -1;
        for (State::Counts &counts : g.counts) {
            counts = State::Counts();
            counts.multiplierAt = d.multiplier ? d.multiplier(0) : d.score ? d.score(0) : 0;
        }
        g.lastShips = d.ships ? d.ships(0) : 0;
        g.lastBombs = d.bombs ? d.bombs(0) : 0;
        g.bossWas = d.bossOn && d.bossOn();
    }

    // Practice's choices of ships, bombs and power, put in as the start is reached
    void PutStocks() {
        if (g.mode != Mode::Practice) {
            return;
        }
        const int ships = Choice("practice_ships"), bombs = Choice("practice_bombs"), power = Choice("practice_power");
        if (ships >= 0 && d.setShips) {
            d.setShips(0, ships + 1);
        }
        if (bombs >= 0 && d.setBombs) {
            d.setBombs(0, bombs);
        }
        if (power >= 0 && d.setPower) {
            d.setPower(0, power);
        }
    }

    // The start is reached: what the mode plays from
    void Reached(Machine &machine) {
        g.phase = Phase::Playing;
        g.frames = 0;
        Fast(false);
        PutStocks();
        if (g.mode == Mode::Practice && !RunningAhead()) {
            Snapshot(machine, g_start);
            g_attempt.clear();
        }
        g_snapshots.clear();
        ResetSummary();
        g.rushBossWas = d.bossOn && d.bossOn();
        g.shown = kShownFor;
        g_viewAt = 0;
    }

    void RestartPractice(Machine &machine) {
        if (g.mode != Mode::Practice || g_start.empty() || (g.phase != Phase::Playing && g.phase != Phase::Replaying)) {
            return;
        }
        Restore(machine, g_start);
        g.phase = Phase::Playing;
        g.frames = 0;
        g_attempt.clear();
        ResetSummary();
        SetHud({});
    }

    void ReplayPractice(Machine &machine) {
        if (g.mode != Mode::Practice || g_start.empty() || (g_attempt.empty() && g_replay.empty())) {
            return;
        }
        if (g.phase != Phase::Replaying) {
            g_replay = g_attempt;
        }
        Restore(machine, g_start);
        g.phase = Phase::Replaying;
        g.replayAt = 0;
        g_attempt.clear();
        ResetSummary();
    }

    // --- each frame ---

    void ClearControls(Machine &machine) {
        for (uint16_t &held : machine.players) {
            held = 0;
        }
    }

    void ScoreOver();

    // Unseen from the board's start to the game's first stage. In the scored modes the choice of fighter is the
    // player's, and seen.
    void StartingBeforeFrame(Machine &machine) {
        Fast(g.phase == Phase::Starting);
        if (++g.frames > kStartMost) {
            End();
            return;
        }
        if (!d.inGame()) {
            ClearControls(machine);
            if (!g.credited && d.canStart(machine)) {
                d.credit();
                g.credited = true;
            }
            if (g.credited) {
                machine.players[0] = (g.frames & 4) ? kStart : 0;
            }
            return;
        }
        if (g.begin.fighter < 0) {
            if (g.phase == Phase::Starting) {
                g.phase = Phase::Choosing;
                Fast(false);
            }
            // (one player: the others' Starts are not taken)
            for (int n = 1; n < Machine::kMaxPlayers; n++) {
                machine.players[n] = 0;
            }
        } else {
            ClearControls(machine);
        }
        if (d.toPlay(machine, g.begin, g.frames)) {
            if (g.toBoss) {
                g.phase = Phase::ToBoss;
                g.frames = 0;
            } else {
                Reached(machine);
            }
        }
    }

    void ToBossBeforeFrame(Machine &machine) {
        Fast(true);
        ClearControls(machine);
        if (!d.inGame()) {
            // (the game over on the way: a boss rush's last ship lost at the boss before, say. A moment out of a game is
            // let by: a game's change of stage)
            if (++g.outOfGame > 2 * kFps) {
                if (g.mode == Mode::Rush) {
                    Fast(false);
                    ScoreOver();
                } else {
                    End();
                }
            }
            return;
        }
        g.outOfGame = 0;
        if (d.bossOn()) {
            // (met from a snapshot two or three seconds before, as the boss comes on)
            if (!g_snapshots.empty() && !RunningAhead()) {
                Restore(machine, g_snapshots.front());
            }
            Reached(machine);
            return;
        }
        if (d.bossStage && d.playing() && d.bossStage(d.stage())) {
            // (a stage that is its boss from the start, with no clock: met as it begins)
            Reached(machine);
            return;
        }
        if (++g.frames > kStartMost) {
            End();
            return;
        }
        if (g.frames % kSnapshotEvery == 0 && !RunningAhead()) {
            g_snapshots.emplace_back();
            Snapshot(machine, g_snapshots.back());
            if (static_cast<int>(g_snapshots.size()) > kSnapshots) {
                g_snapshots.erase(g_snapshots.begin());
            }
        }
    }

    // A summary of counts `which` (0 a boss's part, 1 the stage), shown in its place; the counts start again. What it
    // says was gained is the multiplier's growth, or the score's in a game with no multiplier (kept in multiplierAt).
    void Summary(const char *what, int which) {
        State::Counts &counts = g.counts[which];
        const uint64_t gauge = d.multiplier ? d.multiplier(0) : d.score ? d.score(0) : 0;
        if ((d.multiplier || d.score) && what && counts.frames >= kFps) {
            std::snprintf(g.summary[which][0], sizeof g.summary[which][0], "%s in %s", what, Clock(counts.frames).c_str());
            std::snprintf(g.summary[which][1], sizeof g.summary[which][1], "%s +%s, %d ship%s lost, %d bomb%s",
                          d.multiplier ? "multiplier" : "score", Grouped(gauge - std::min(gauge, counts.multiplierAt)).c_str(),
                          counts.shipsLost, counts.shipsLost == 1 ? "" : "s", counts.bombsUsed, counts.bombsUsed == 1 ? "" : "s");
            g.summaryShown[which] = kShownFor;
        }
        // (fewer than a second played since the last: a boss's flag set again at a stage's end, say; nothing to say)
        counts = State::Counts();
        counts.multiplierAt = gauge;
    }

    // The summary's counts, a stage's and a boss's
    void CountFrame() {
        if (!d.playing || !d.playing()) {
            return;
        }
        int lost = 0, used = 0;
        if (d.ships) {
            const int ships = d.ships(0);
            lost = ships < g.lastShips ? g.lastShips - ships : 0;
            g.lastShips = ships;
        }
        if (d.bombs) {
            const int bombs = d.bombs(0);
            used = bombs < g.lastBombs ? g.lastBombs - bombs : 0;
            g.lastBombs = bombs;
        }
        for (State::Counts &counts : g.counts) {
            counts.frames++;
            counts.shipsLost += lost;
            counts.bombsUsed += used;
        }
        const bool boss = d.bossOn();
        const int stage = d.stage();
        if (boss && !g.bossWas) {
            // (practice begun just before the boss: nothing to say yet)
            Summary(g.counts[0].frames > 10 * kFps ? "To the boss" : nullptr, 0);
        } else if (!boss && g.bossWas) {
            Summary("The boss", 0);
        }
        if (stage != g.stageWas && g.stageWas >= 0) {
            Summary("The stage", 1);
            Summary(nullptr, 0);
        }
        g.bossWas = boss;
        g.stageWas = stage;
    }

    void ScoreOver() {
        g.phase = Phase::Over;
        g.shown = kShownFor;
        g.newBest = g.score > g.best;
        if (g.newBest) {
            g.best = g.score;
            g_bests[BestName()] = g.score;
            if (g.mode == Mode::Rush) {
                g_bests[BestName() + "_frames"] = static_cast<uint64_t>(g.rushFrames);
            }
            WriteBests();
        }
    }

    void Hud() {
        std::vector<HudLine> lines;
        if (g.mode == Mode::Practice && g.phase == Phase::Replaying) {
            lines.push_back({0.5f, 0.92f, 0, "Replay: the controls take over"});
        }
        if (g.phase == Phase::Over) {
            const char *name = g.mode == Mode::Caravan ? "Caravan over" : g.mode == Mode::Rush ? "Boss rush over" : "Score attack over";
            std::string text = g.newBest ? "A new best: " + Grouped(g.score) + "!" : "Score " + Grouped(g.score) + ", best " + Grouped(g.best);
            if (g.mode == Mode::Rush) {
                text += " (" + std::to_string(g.bosses) + " bosses, " + Clock(g.rushFrames) + ")";
            }
            lines.push_back({0.5f, 0.74f, 0, name});
            lines.push_back({0.5f, 0.82f, 0, text});
        } else if (g.phase == Phase::Playing || g.phase == Phase::Replaying) {
            if (g.mode == Mode::Caravan) {
                lines.push_back({0.5f, 0.06f, 0, Clock(g.caravanFrames), 255, g.caravanFrames < 10 * kFps ? uint8_t(80) : uint8_t(255),
                                 g.caravanFrames < 10 * kFps ? uint8_t(80) : uint8_t(255)});
            }
            if (g.shown > 0 && g.mode == Mode::Viewer) {
                lines.push_back({0.5f, 0.88f, 0, "Up or right: faster   Down or left: back   Shot: stop, go on"});
            }
            if (g.shown > 0 && g.mode != Mode::Practice && g.mode != Mode::Viewer) {
                lines.push_back({0.5f, 0.88f, 0, std::string(g.mode == Mode::Caravan ? "Caravan" : g.mode == Mode::Rush ? "Boss rush" : "Score attack") + ": best " + Grouped(g.best)});
            }
            if (InPractice()) {
                if (FeatureValue("practice_readout") && d.readout) {
                    std::vector<std::string> readout;
                    d.readout(0, readout);
                    float y = 0.22f;
                    for (const std::string &line : readout) {
                        lines.push_back({0.01f, y, -1, line});
                        y += 0.05f;
                    }
                }
                for (int which = 0; which < 2; which++) {
                    if (FeatureValue("practice_summary") && g.summaryShown[which] > 0) {
                        const float y = which == 0 ? 0.36f : 0.50f;
                        lines.push_back({0.5f, y, 0, g.summary[which][0], 255, 230, 120});
                        lines.push_back({0.5f, y + 0.06f, 0, g.summary[which][1], 255, 230, 120});
                    }
                }
            }
        }
        SetHud(lines);
    }

    // The hit boxes (the Cheats page), wherever a stage is played but the stage viewer and the frames run unseen. They
    // are shown with the objects: the picture just made has the object list the board took as the frame before ended,
    // so the boxes shown are those of the places that list was made from. A game makes its list from the places of the
    // frame it makes it in, or of the frame before (Description::listLag); and not always every frame (Giga Wing makes
    // one every other frame): the boxes of a list are taken when a new list is seen, and kept with it.
    void Boxes(const Machine &machine) {
        std::vector<HudBox> boxes;
        if (d.playing && d.playing() && g.mode != Mode::Viewer && !Starting()) {
            if (FeatureValue("enemy_hitboxes") && d.bodyBoxes) {
                d.bodyBoxes(boxes);
            }
            if (FeatureValue("bullet_hitboxes") && d.shotBoxes) {
                d.shotBoxes(boxes);
            }
            if (FeatureValue("player_hitboxes") && d.shipBoxes) {
                d.shipBoxes(boxes);
            }
        }
        if (boxes.empty() && g_boxesBefore.empty() && g_boxesListed.empty() && cps2recomp::Boxes().empty()) {
            return;
        }
        // (the list the board shows in the next picture: a new one is made from these places, or those of the frame before)
        const std::vector<HudBox> shown = g_boxesListed;
        const uint32_t listed = static_cast<uint32_t>(crc32(0, machine.ObjectsToCome(), 0x2000));
        if (listed != g_listed) {
            g_listed = listed;
            g_boxesListed = d.listLag ? g_boxesBefore : boxes;
        }
        g_boxesBefore = std::move(boxes);
        SetBoxes(shown);
    }

    // The slowdown (the Game page): with none, the 68000 overclocked (only what this file set is put back: an option's
    // --cpu-speed is left as it is till the choice is made). The board's timing goes with it (BoardTiming, below).
    void Slowdown(Machine &machine) {
        const int speed = FeatureValue("slowdown") ? kOverclock : 1;
        if (speed != g_cpuSpeedSet) {
            g_cpuSpeedSet = speed;
            machine.cpuSpeed = speed;
        }
    }
    bool BoardTiming() {
        return FeatureValue("slowdown") == 0;
    }

    // --- the stage viewer's own scrolling ---

    void Pack(Machine &machine, ViewKey &key) {
        std::vector<uint8_t> state;
        Snapshot(machine, state);
        key.size = static_cast<uLongf>(state.size());
        uLongf packed = compressBound(key.size);
        key.packed.resize(packed);
        compress2(key.packed.data(), &packed, state.data(), key.size, 1);
        key.packed.resize(packed);
    }
    void Unpack(Machine &machine, const ViewKey &key) {
        std::vector<uint8_t> state(key.size);
        uLongf size = key.size;
        if (uncompress(state.data(), &size, key.packed.data(), static_cast<uLong>(key.packed.size())) == Z_OK) {
            Restore(machine, state);
        }
    }

    // The viewer's frame: the controls (taken from the game: nobody plays) scroll it. Forward (up or right) is faster.
    // Back (down or left) goes back the way it came, kViewBack frames a picture: each frame shown is run from the state
    // kept before it (g_viewBehind: the states of the frames back to the last kept one, remade from it unseen, the run
    // ending at the frame to be shown, so that only those are seen). The shot button stops it and starts it again;
    // stopped, the frame it stopped at is run again and again. (g_viewAt: the frame about to be run, from the stage's
    // start.)
    void ViewerBeforeFrame(Machine &machine) {
        uint16_t controls = 0;
        for (const uint16_t held : machine.players) {
            controls |= held;
        }
        ClearControls(machine);
        if (RunningAhead()) {
            return;
        }
        if (g_viewRemaking >= 0) {
            // (remaking the states behind, unseen: from the kept state, to the frame to be shown, which is run seen)
            if (g_viewBehind.empty()) {
                auto key = std::find_if(g_viewKeys.rbegin(), g_viewKeys.rend(), [&](const ViewKey &k) { return k.at <= g_viewRemaking; });
                if (key == g_viewKeys.rend()) {
                    g_viewRemaking = -1;
                    Fast(false);
                    return;
                }
                Unpack(machine, *key);
                g_viewAt = g_viewBehindFrom = key->at;
            }
            g_viewBehind.emplace_back();
            Snapshot(machine, g_viewBehind.back());
            if (g_viewAt >= g_viewRemaking) {
                g_viewRemaking = -1;
                Fast(false);
            }
            g_viewAt++;
            return;
        }
        const uint16_t pressed = controls & ~g_viewControls;
        g_viewControls = controls;
        if (pressed & kPunch1) {
            g_viewStopped = !g_viewStopped;
        }
        if (g_viewAt % kViewKeyEvery == 0 && (g_viewKeys.empty() || g_viewKeys.back().at < g_viewAt) &&
            static_cast<int>(g_viewKeys.size()) < kViewKeys) {
            g_viewKeys.push_back({g_viewAt, 0, {}});
            Pack(machine, g_viewKeys.back());
        }
        const bool forward = controls & (kUp | kRight), back = (controls & (kDown | kLeft)) && !forward;
        // (the speed chosen: Normal, Stopped, a half, a quarter, twice, four times)
        const int speed = FeatureValue("viewer_speed");
        const bool stopped = !forward && !back && (g_viewStopped || speed == 1);
        SetSlowMotion(!forward && !back ? (speed == 2 ? 2 : speed == 3 ? 4 : 1) : 1);
        SetFastMotion(forward ? kViewForward : !back && speed == 4 ? 2 : !back && speed == 5 ? 4 : 1);
        g_viewQuiet = stopped || back;
        if (!back) {
            g_viewBehind.clear();
        } else if (g_viewAt > 1) {
            g_viewHeld.clear();
            const int32_t to = std::max(0, g_viewAt - 1 - kViewBack);
            const int32_t had = g_viewBehindFrom + static_cast<int32_t>(g_viewBehind.size());
            if (to >= g_viewBehindFrom && to < had) {
                // (the frame shown: run from the state before it)
                Restore(machine, g_viewBehind[static_cast<size_t>(to - g_viewBehindFrom)]);
                g_viewBehind.resize(static_cast<size_t>(to - g_viewBehindFrom));
                g_viewAt = to + 1;
                if (to - kViewBack < g_viewBehindFrom && to > 0) {
                    // (none behind for the next: remade in the next picture's time, unseen)
                    g_viewBehind.clear();
                    g_viewRemaking = std::max(0, to - kViewBack);
                    Fast(true);
                }
                return;
            }
            // (the first frame going back: this one is run forward, and the states behind are remade in the next)
            g_viewBehind.clear();
            g_viewRemaking = to;
            Fast(true);
            g_viewAt++;
            return;
        }
        if (stopped || (back && g_viewAt <= 1)) {
            // (the frame it stopped at, again)
            if (g_viewHeld.empty()) {
                Snapshot(machine, g_viewHeld);
                g_viewAt++;
            } else {
                Restore(machine, g_viewHeld);
            }
            return;
        }
        g_viewHeld.clear();
        g_viewAt++;
    }

    bool HideAll(uint32_t, uint16_t) {
        return true;
    }
} // namespace

bool Practice() {
    return g.mode == Mode::Practice;
}
bool AttractMode() {
    return g.mode == Mode::Attract;
}
int AttractShow() {
    return g.mode == Mode::Attract ? FeatureValue("attract_show") : 0;
}
bool ScoreAttack() {
    return g.mode == Mode::Score;
}
bool Caravan() {
    return g.mode == Mode::Caravan;
}
bool BossRush() {
    return g.mode == Mode::Rush;
}
bool StageViewer() {
    return g.mode == Mode::Viewer;
}
bool AnyMode() {
    return g.mode != Mode::None;
}
bool NoContinue() {
    return g.mode == Mode::Score || g.mode == Mode::Caravan || g.mode == Mode::Rush;
}
int WantedStage() {
    return g.mode != Mode::None && g.mode != Mode::Attract && (g.phase == Phase::Starting || g.phase == Phase::Choosing) ? g.begin.stage : -1;
}
bool Protected() {
    return g.mode == Mode::Viewer || (g.mode != Mode::None && g.phase == Phase::ToBoss);
}
bool Starting() {
    return g.mode != Mode::None && (g.phase == Phase::Starting || g.phase == Phase::ToBoss);
}
bool InPractice() {
    return g.mode == Mode::Practice && (g.phase == Phase::Playing || g.phase == Phase::Replaying);
}
void EndModes() {
    End();
}

void BeforeFrame(Machine &machine) {
    Slowdown(machine);
    switch (g.mode) {
    case Mode::None: return;
    case Mode::Attract:
        // (Start or a coin ends it, and is the game's: a game can begin from the show)
        if ((machine.players[0] | machine.players[1]) & (kStart | kCoin)) {
            End();
            return;
        }
        ClearControls(machine);
        return;
    default: break;
    }
    if (g.phase == Phase::Starting || g.phase == Phase::Choosing) {
        StartingBeforeFrame(machine);
        return;
    }
    if (g.phase == Phase::ToBoss) {
        ToBossBeforeFrame(machine);
        return;
    }
    if (g.mode == Mode::Viewer) {
        ViewerBeforeFrame(machine);
        return;
    }
    // (one player only: the others' Starts and coins are not taken)
    for (int n = 1; n < Machine::kMaxPlayers; n++) {
        machine.players[n] &= static_cast<uint16_t>(~(kStart | kCoin));
    }
    machine.players[0] &= static_cast<uint16_t>(~kCoin);
    if (g.mode == Mode::Practice) {
        if (g.phase == Phase::Replaying) {
            // (the last attempt's controls, until they run out or the player touches the controls)
            if (machine.players[0] != 0 || g.replayAt >= static_cast<int32_t>(g_replay.size())) {
                g.phase = Phase::Playing;
            } else {
                machine.players[0] = static_cast<uint16_t>(g_replay[static_cast<size_t>(g.replayAt++)]);
            }
        }
        if (!RunningAhead()) {
            g_attempt.push_back(machine.players[0]);
        }
    }
}

void Frame(Machine &machine) {
    if (g.mode == Mode::None || g.mode == Mode::Attract) {
        Boxes(machine);
    }
    if (g.mode == Mode::None) {
        return;
    }
    if (g.mode == Mode::Attract) {
        if (g.phase == Phase::Starting) {
            Fast(FeatureValue("attract_show") != 0);
            if ((d.showBegun && d.showBegun(machine)) || ++g.frames > kStartMost) {
                g.phase = Phase::Playing;
                Fast(false);
            }
        } else {
            // (the parts not wanted run unseen)
            const int show = FeatureValue("attract_show");
            Fast(show != 0 && d.showPart && d.showPart() != show);
        }
        return;
    }
    if (g.phase == Phase::Playing || g.phase == Phase::Replaying) {
        if (!d.inGame()) {
            // (the game is over: the scored modes show how it went)
            if (g.mode == Mode::Score || g.mode == Mode::Caravan || g.mode == Mode::Rush) {
                ScoreOver();
            } else {
                End();
                return;
            }
        } else {
            g.score = std::max(g.score, d.score(0));
            g.frames++;
            if (g.shown > 0) {
                g.shown--;
            }
            for (int32_t &shown : g.summaryShown) {
                shown -= shown > 0 ? 1 : 0;
            }
            CountFrame();
            if (g.mode == Mode::Caravan) {
                // (as many ships as it takes)
                if (d.ships && d.setShips && d.ships(0) < 2) {
                    d.setShips(0, 2);
                }
                if (d.playing() && --g.caravanFrames <= 0) {
                    ScoreOver();
                }
            }
            if (g.mode == Mode::Rush) {
                g.rushFrames++;
                // (its own flag: the summary's counts only while a stage is played, and a boss's flag goes as the next
                // stage begins. A boss is beaten when its flag goes with the stage still played: the last ship lost
                // takes the flag too, and the game over follows)
                const bool boss = d.bossOn();
                if (d.playing()) {
                    if (g.rushBossWas && !boss) {
                        g.bosses++;
                        g.phase = Phase::ToBoss;
                        g.frames = 0;
                    }
                    g.rushBossWas = boss;
                }
            }
        }
    } else if (g.phase == Phase::Over) {
        if (--g.shown <= 0) {
            End();
            machine.Reset();
            return;
        }
    }
    Hud();
    Boxes(machine);
    // (the viewer stopped or going back: the frame run again is not heard again)
    if (g.mode == Mode::Viewer && g_viewQuiet) {
        machine.audio.clear();
    }
    // (nobody shown while viewing: and only what this file set is put back)
    static bool hid = false;
    const bool viewing = g.mode == Mode::Viewer && g.phase == Phase::Playing;
    if (viewing || hid) {
        machine.video.hideObject = viewing ? HideAll : nullptr;
        machine.video.layerMask = viewing ? d.viewLayers : 0xF;
        hid = viewing;
    }
    if (g.mode == Mode::Viewer && g.phase == Phase::Playing && !d.inGame()) {
        End();
    }
}

void State(Archive &ar) {
    ar.Mark(0x53484D55); // SHMU
    ar.Pod(g);
    if (!ar.Saving() && !RunningAhead()) {
        g_fast = !Starting(); // (put the window's running unseen as the state has it)
        Fast(Starting());
    }
}

void Register(const Description &description) {
    d = description;
    Game &game = TheGame();
    game.pages.push_back({kModes, "", "Other ways to play: training mode, attract mode, score attack, caravan, boss rush, the stage viewer.", true});
    game.pages.push_back({kPractice, kModes, "Any stage, or straight at its boss, with what you choose; back to the start with one button."});
    game.commands.push_back({"practice_start", kPractice, "Start",
                             "Starts training mode: the stage chosen below, with the fighter, ships, bombs and power chosen. Backspace (a gamepad's "
                             "left shoulder) puts it back as it began; Reset ends it.",
                             StartPractice});
    game.commands.push_back({"practice_restart", kPractice, "Start over",
                             "Training mode put back as it began, at once. Backspace or a gamepad's left shoulder does this while playing.",
                             RestartPractice, kKeyRestart});
    game.commands.push_back({"practice_replay", kPractice, "Replay",
                             "The last attempt shown again from the start, the controls as they were pressed; touch the controls to take "
                             "over. Backslash, or a gamepad's left trigger held with the left shoulder, does this while playing.",
                             ReplayPractice, kKeyExtra});
    game.features.push_back({"practice_stage", kPractice, "Stage", "The stage training mode begins at.", 0, d.stages});
    game.features.push_back({"practice_at", kPractice, "Begin at", "The stage's start, or its boss (the way there is run unseen, out of harm).", 0,
                             {"The stage's start", "The boss"}});
    game.features.push_back({"practice_fighter", kPractice, "Fighter", "The fighter training mode is played with.", 0, d.fighters});
    std::vector<const char *> ships = {"As the game has them"}, bombs = {"As the game has them"}, power = {"As the game has it"};
    static const char *const kNumbers[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    for (int n = 1; n <= d.maxShips && n < 10; n++) {
        ships.push_back(kNumbers[n]);
    }
    for (int n = 0; n <= d.maxBombs && n < 10; n++) {
        bombs.push_back(kNumbers[n]);
    }
    static const char *const kPowers[] = {"Lowest", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    for (int n = 0; n <= d.maxPower && n < 10; n++) {
        power.push_back(n == d.maxPower ? "Full" : kPowers[n]);
    }
    game.features.push_back({"practice_ships", kPractice, "Ships", "The ships at training mode's start (with the one in play).", 0, ships});
    game.features.push_back({"practice_bombs", kPractice, "Bombs", "The bombs at training mode's start.", 0, bombs});
    if (d.setPower) {
        game.features.push_back({"practice_power", kPractice, "Power", "The shot's power at training mode's start.", 0, power});
    }
    if (d.readout) {
        game.features.push_back({"practice_readout", kPractice, "Readout",
                                 d.readoutWords ? d.readoutWords
                                                : "The scoring as it happens, at the left: the multiplier, the medal chain, the power, the reflect "
                                                  "gauge and the boss's clock.",
                                 kFeatureTraining});
    }
    game.features.push_back({"practice_summary", kPractice, "Summary",
                             d.summaryWords ? d.summaryWords
                                            : "When a boss comes, when it is over and at a stage's end: how long it took, what the multiplier "
                                              "gained, the ships lost and the bombs used.",
                             kFeatureTraining});
    // (a game's own words for a row the collection's other shooters have too)
    if (d.readoutWords) {
        OwnFeatures({"practice_readout"});
    }
    if (d.summaryWords) {
        OwnFeatures({"practice_summary"});
    }

    game.pages.push_back({kAttract, kModes, "The game's own show on a loop, with no coin in and its sound on: all of it, or one part."});
    game.commands.push_back({"attract_start", kAttract, "Start", "Starts attract mode: the game's own show, over and over. Start (or a coin) ends it.", StartAttract});
    std::vector<const char *> shows = {"Everything"};
    shows.insert(shows.end(), d.shows.begin(), d.shows.end());
    game.features.push_back({"attract_show", kAttract, "Shows", d.showsWords, 0, shows});

    game.pages.push_back({kScore, kModes, "One player and one credit from the first stage, for the best score."});
    game.commands.push_back({"score_start", kScore, "Start",
                             "Starts a game for player 1 alone: one credit and no continue, from the first stage, with free play and the "
                             "cheats left out. The best score is kept. The game over ends it.",
                             StartScore});
    game.pages.push_back({kCaravan, kModes, "A score attack against the clock: a few minutes from the first stage, as many ships as it takes."});
    game.commands.push_back({"caravan_start", kCaravan, "Start",
                             "Starts a caravan: player 1 alone from the first stage, for as long as chosen below, with a ship for every one "
                             "lost. The best score is kept for each length.",
                             StartCaravan});
    game.features.push_back({"caravan_length", kCaravan, "Length", "How long a caravan lasts.", 0, {"2 minutes", "3 minutes", "5 minutes"}});
    game.pages.push_back({kRush, kModes, "Each boss in turn, the way between them run unseen."});
    game.commands.push_back({"rush_start", kRush, "Start",
                             "Starts a boss rush: player 1 alone, one credit and no continue; each boss is met as it comes on, the way to the "
                             "next run unseen and out of harm. The best score is kept, and how long it took.",
                             StartRush});
    game.pages.push_back({kViewer, kModes, "A stage scrolling by with nobody on it, at the speed chosen, or as the controls take it."});
    game.commands.push_back({"viewer_start", kViewer, "Start",
                             "Shows the stage chosen below scrolling by, with no ships, enemies, shots or panel. Up or right scrolls it faster, "
                             "down or left back the way it came, and the shot button stops it and starts it again. Reset ends it.",
                             StartViewer});
    game.features.push_back({"viewer_stage", kViewer, "Stage", "The stage the viewer shows.", 0, d.stages});
    game.features.push_back({"viewer_speed", kViewer, "Scroll speed", "How fast the stage scrolls by when the controls are left alone.", 0,
                             {"Normal", "Stopped", "Half", "A quarter", "Twice as fast", "Four times as fast"}});
    OwnFeatures({"attract_show", "viewer_stage", "viewer_speed"});
    game.inTraining = InPractice;
    // (the viewer goes back to states it kept: run-ahead's taking back frames would undo that)
    game.noRunAhead = StageViewer;

    // The slowdown, and the hit boxes (each game's sources register this after their own cheats, to come after them)
    game.features.push_back({"slowdown", "Game", "Slowdown",
                             "Arcade perfect: the processor runs as on the board, waiting for its memory and giving the bus to the "
                             "video chip, so the game slows down where and as much as the board does (checked frame by frame against "
                             "a cycle-exact model of the board). None: the processor overclocked, with every clock and four times as "
                             "many, so the game never runs short of time. Score attack, caravan and boss rush keep their best scores "
                             "apart for each.",
                             0, {"Arcade perfect", "None (overclocked)"}});
    game.boardTiming = BoardTiming;
    game.features.push_back({"player_hitboxes", "Cheats", "Player hit boxes",
                             "Each ship's true hit box, the only part of it an enemy shot can hit (far smaller than the ship). In every "
                             "mode; drawn over the picture only, it changes nothing in the game."});
    game.features.push_back({"enemy_hitboxes", "Cheats", "Enemy hit boxes",
                             "Where each enemy's body can hit a ship (the faint orange boxes). In every mode; drawn over the picture "
                             "only."});
    game.features.push_back({"bullet_hitboxes", "Cheats", "Bullet hit boxes",
                             "Where each enemy shot can hit a ship: the point the game tests against the ships' hit boxes (the yellow "
                             "dots). In every mode; drawn over the picture only."});
}

} // namespace cps2recomp::shmup
