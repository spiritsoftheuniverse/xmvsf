// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The in-game menu (menu.h): the shared menu told what a CPS-2 game's is made of.
#include "menu.h"

#include <cstring>

#include "../machine/machine.h"
#include "../runtime/game.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>

namespace port {

namespace {
    // The first page's own rows
    enum { kCoin = 1, kStart2, kReset, kTest, kResetOptions };

    // The speeds, slowest first, each about 1.4 times the one before: the game runs `frames` frames in `ticks` of
    // the screen's
    struct Speed {
        const char *name;
        int frames, ticks;
    };
    const Speed kSpeeds[] = {{"1/8", 1, 8}, {"1/6", 1, 6}, {"1/4", 1, 4}, {"1/3", 1, 3}, {"1/2", 1, 2}, {"3/4", 3, 4}, {"Full", 1, 1},
                             {"1.5x", 3, 2}, {"2x", 2, 1}, {"3x", 3, 1}, {"4x", 4, 1}, {"6x", 6, 1}, {"8x", 8, 1}};
    constexpr int kFull = 6;
} // namespace

void Menu::Open(bool withStates) {
    const cps2recomp::Game &game = cps2recomp::TheGame();
    recomp::MenuConfig config;
    // Online, Display, Game and Stage first (issue #196), then the game's pages that belong at the head; then three groups
    // with a line between them: the save states, what the game offers, and leaving. Under Game: the speed, Mute, and
    // the Board, Cheats and Sound pages (a first page of fewer rows).
    config.groups.push_back({{recomp::MenuRow::Page, 0, "Online"}, {recomp::MenuRow::Page, 0, "Display"},
                             {recomp::MenuRow::Page, 0, "Game"}});
    // (and Stage, for a game with a choice of stage: where the fights are, whatever is played; fight/session.h)
    if (std::any_of(game.features.begin(), game.features.end(),
                    [](const cps2recomp::Feature &feature) { return !strcmp(feature.category, "Stage"); })) {
        config.groups.back().push_back({recomp::MenuRow::Page, 0, "Stage"});
    }
    config.pages.push_back({"Game", "", "", true, {recomp::MenuRow::Speed, recomp::MenuRow::Mute}});
    for (const char *under : {"Board", "Cheats", "Sound"}) {
        config.pages.push_back({under, "Game"});
    }
    std::vector<recomp::MenuRow> top;
    for (const cps2recomp::Page &page : game.pages) {
        // (such pages are ways of running the game: their rows do not count features that are on)
        config.pages.push_back({page.name, page.parent, page.description, false});
        if (page.top && !*page.parent) {
            top.push_back({recomp::MenuRow::Page, 0, page.name});
        }
    }
    if (!top.empty()) {
        config.groups.push_back(std::move(top));
    }
    if (withStates) {
        config.groups.push_back({{recomp::MenuRow::Save}, {recomp::MenuRow::Load}, {recomp::MenuRow::Slot}});
    }
    config.groups.push_back({
        {recomp::MenuRow::Pages},
        {recomp::MenuRow::Action, kCoin, "Insert coin", "Puts a coin in the first slot."},
        {recomp::MenuRow::Action, kStart2, "Player 2 joins",
         "Presses player 2's Start: with a coin in (or free play), a second player to practise against."},
        {recomp::MenuRow::Action, kTest, "Test menu",
         "The game's own operator menu (the board's test switch, F2): its settings, such as difficulty and rounds. "
         "Its Exit row goes back."},
    });
    config.groups.push_back({
        {recomp::MenuRow::Action, kResetOptions, "Reset all options",
         "Puts every option back as it first was (the launcher's Enhanced preset), and the speed to full. The game's own "
         "settings (its Test menu) and the controls stay as they are."},
        {recomp::MenuRow::Action, kReset, "Reset game", "Starts the board again, as if switched off and on."},
        {recomp::MenuRow::Quit},
        {recomp::MenuRow::Resume},
    });
    // (during an online match, the features the match decides for both players are left as it has them: a change on
    // one side alone would leave the two games apart; the host sets them before hosting)
    const std::function<bool(size_t)> decided = matchDecides;
    for (size_t i = 0; i < game.features.size(); i++) {
        const cps2recomp::Feature &feature = game.features[i];
        std::string description = feature.description;
        if (decided && decided(i)) {
            description += " During an online match it stays as the match has it (the host's, set before hosting, or off "
                           "for both), and can't be changed here.";
        }
        config.features.push_back({feature.category, feature.title, description, {feature.choices.begin(), feature.choices.end()},
                                   (feature.flags & cps2recomp::kFeatureFavourites) != 0});
    }
    // (the player's favourite stages, by name, the same for every list of them: settings.ini's [favourites] stages, the
    // names a "|" apart)
    {
        recomp::Ini settings(cps2recomp::SettingsPath());
        settings.load();
        auto favourites = std::make_shared<std::set<std::string>>();
        const std::string kept = settings.get("favourites", "stages");
        for (size_t at = 0; at < kept.size();) {
            const size_t bar = std::min(kept.find('|', at), kept.size());
            if (bar > at) {
                favourites->insert(kept.substr(at, bar - at));
            }
            at = bar + 1;
        }
        auto name = [](size_t feature, int choice) -> std::string {
            const cps2recomp::Feature &f = cps2recomp::TheGame().features[feature];
            return choice >= 0 && choice < static_cast<int>(f.choices.size()) ? f.choices[static_cast<size_t>(choice)] : "";
        };
        config.favourite = [favourites, name](size_t feature, int choice) { return favourites->count(name(feature, choice)) != 0; };
        config.setFavourite = [favourites, name](size_t feature, int choice, bool marked) {
            const std::string stage = name(feature, choice);
            if (stage.empty()) {
                return;
            }
            if (marked) {
                favourites->insert(stage);
            } else {
                favourites->erase(stage);
            }
            std::string all;
            for (const std::string &one : *favourites) {
                all += (all.empty() ? "" : "|") + one;
            }
            recomp::Ini file(cps2recomp::SettingsPath());
            file.load();
            file.set("favourites", "stages", all);
            file.save();
        };
    }
    for (const cps2recomp::Command &command : game.commands) {
        if (hosting && !strcmp(command.name, "online_host")) {
            config.commands.push_back({command.category, "Stop hosting",
                                       "Others can no longer join: the match's code stops working. Your game goes on as it is."});
            continue;
        }
        if (recordingClip && !strcmp(command.name, "scene_clip")) {
            config.commands.push_back({command.category, "Stop recording", "Ends the clip being recorded here; its frames so far are kept."});
            continue;
        }
        config.commands.push_back({command.category, command.title, command.description});
    }
    config.value = [](size_t feature) { return cps2recomp::FeatureSetting(cps2recomp::TheGame().features[feature].name); };
    config.setValue = [decided](size_t feature, int value) {
        if (decided && decided(feature)) {
            return;
        }
        cps2recomp::SetFeatureValue(cps2recomp::TheGame().features[feature].name, value);
    };
    // (of a long choice, those the launcher leaves in the menu: settings.ini's [menu], recomp-common/settings.h)
    {
        recomp::Ini settings(cps2recomp::SettingsPath());
        settings.load();
        std::vector<std::vector<bool>> offered;
        for (const recomp::FeatureInfo &info : cps2recomp::FeatureList()) {
            offered.push_back(recomp::MenuChoices(settings, info));
        }
        config.offered = [offered](size_t feature, int choice) {
            return feature >= offered.size() || choice < 0 || choice >= static_cast<int>(offered[feature].size()) || offered[feature][static_cast<size_t>(choice)];
        };
    }
    Speeds(config);
    config.speedDescription = "Left/Right: slow motion, or faster than the board. While playing, P (or a click of the right stick) stops "
                              "the game and starts it again; O (or, once stopped, the left trigger) goes on by one frame; - and = change "
                              "the speed.";
    config.muted = [this] { return muted; };
    config.setMuted = [this](bool to) { muted = to; };
    config.confirmName = "Enter or A";
    config.closeName = "Esc";
    m_menu.Configure(std::move(config));
    m_menu.Open();
}

Menu::Menu() {
    // (so that - and = work before the menu is first opened)
    recomp::MenuConfig config;
    Speeds(config);
    m_menu.Configure(std::move(config));
}

void Menu::Speeds(recomp::MenuConfig &config) {
    for (const Speed &speed : kSpeeds) {
        config.speeds.push_back(speed.name);
    }
    config.normalSpeed = kFull;
}

Menu::Rate Menu::GameRate() const {
    const Speed &speed = kSpeeds[std::clamp(m_menu.Speed(), 0, static_cast<int>(std::size(kSpeeds)) - 1)];
    // (the slower of the player's choice and what the game's sources ask for: runtime/game.h)
    const int slowBy = cps2recomp::SlowMotion();
    if (slowBy > 1 && speed.frames * slowBy > speed.ticks) {
        return {1, slowBy};
    }
    const int fastBy = cps2recomp::FastMotion();
    if (fastBy > 1 && speed.frames == speed.ticks) {
        return {fastBy, 1};
    }
    return {speed.frames, speed.ticks};
}

void Menu::SetFullSpeed() {
    m_menu.SetSpeed(kFull);
}

void Menu::Faster(int by) {
    m_menu.SetSpeed(m_menu.Speed() + by);
}

Menu::Action Menu::Buttons(uint16_t pressed) {
    unsigned buttons = 0;
    buttons |= (pressed & cps2recomp::kUp) ? recomp::kMenuUp : 0;
    buttons |= (pressed & cps2recomp::kDown) ? recomp::kMenuDown : 0;
    buttons |= (pressed & cps2recomp::kLeft) ? recomp::kMenuLeft : 0;
    buttons |= (pressed & cps2recomp::kRight) ? recomp::kMenuRight : 0;
    if (pressed & (cps2recomp::kPunch1 | cps2recomp::kPunch2 | cps2recomp::kPunch3 | cps2recomp::kKick1 | cps2recomp::kKick2 |
                   cps2recomp::kKick3 | cps2recomp::kStart)) {
        buttons |= recomp::kMenuConfirm;
    }
    // (a press of something else, a coin button say, still takes the note away, as it always did)
    if (pressed && !buttons && m_menu.IsOpen()) {
        m_menu.Note("");
    }
    return Press(buttons);
}

Menu::Action Menu::Press(unsigned buttons) {
    const recomp::Menu::Chosen chosen = m_menu.Buttons(buttons);
    switch (chosen.kind) {
    case recomp::Menu::Chosen::Save: return Action::Save;
    case recomp::Menu::Chosen::Load: return Action::Load;
    case recomp::Menu::Chosen::Quit: return Action::Quit;
    case recomp::Menu::Chosen::Command:
        m_command = static_cast<size_t>(chosen.id);
        return Action::Command;
    case recomp::Menu::Chosen::Action:
        return chosen.id == kCoin           ? Action::Coin
               : chosen.id == kStart2       ? Action::Start2
               : chosen.id == kTest         ? Action::Test
               : chosen.id == kResetOptions ? Action::ResetOptions
                                            : Action::Reset;
    default: return Action::None;
    }
}

} // namespace port
