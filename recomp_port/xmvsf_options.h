// XMVSF native Options v87.443: real SDL3 input mappings and settings hooks.
// This is the old Game / Graphics / Controller organization, not recomp's menu.
// Values marked NOT PORTED are intentionally inert. No ROM writes or patches.
#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "bindings.h"
#include "../recomp-common/menu.h"
#include "xmvsf_window.h"
#include "xmvsf_pad_owners.h"
#include "../recomp-common/settings.h"
#include "../runtime/game.h"

namespace port::xmvsf_home {

class OptionsPanel {
    bool visible_ = false;
    int tab_ = 0, gameRow_ = 0, graphicsRow_ = 0;
    int player_ = 0, controlRow_ = 0;
    std::array<bool, 2> editPad_{{false, true}};
    bool capture_ = false;
    uint64_t captureAt_ = 0, suppressUntil_ = 0;
    std::string notice_;
    uint64_t noticeUntil_ = 0;
    static constexpr std::array<int, 14> kActionOrder{{
        4, 5, 6, 7, 8, 9, 12, 13, 11, 10, 2, 3, 0, 1
    }};
    static constexpr std::array<const char *, 14> kActionTitles{{
        "LP", "MP", "HP", "LK", "MK", "HK", "3P", "3K",
        "COIN", "START", "LEFT", "RIGHT", "UP", "DOWN"
    }};

    void Notice(std::string s, uint64_t ms = 2600) {
        notice_ = std::move(s);
        noticeUntil_ = SDL_GetTicks() + ms;
    }
    void ChangeTab(int direction) {
        tab_ = (tab_ + 3 + (direction > 0 ? 1 : 2)) % 3;
        capture_ = false;
    }
    void Save(recomp::Bindings &b, const BoundTable &table,
              const std::string &file, SDL_Scancode key, recomp::PadInput pad,
              PadOwners &owners) {
        const int action = kActionOrder[size_t(controlRow_ - 2)];
        const bool isPad = editPad_[size_t(player_)];
        if (file.empty()) {
            Notice("SETTINGS FILE IS UNAVAILABLE"); capture_ = false; return;
        }
        // Stage changes first. Never say SAVED or alter live bindings if
        // writing settings.ini failed. Existing non-input settings remain.
        recomp::Bindings candidate = b;
        if (isPad) {
            candidate.pad[player_][action] = pad;
            if (pad.button >= 0 || pad.axis >= 0) {
                for (int i = 0; i < table.table.count; ++i)
                    if (i != action && candidate.pad[player_][i] == pad)
                        candidate.pad[player_][i] = {};
            }
        } else {
            candidate.key[player_][action] = key;
            if (key != SDL_SCANCODE_UNKNOWN) {
                // One physical key must never unintentionally drive both
                // players, or two actions on the same player.
                for (int p = 0; p < table.table.players; ++p)
                    for (int i = 0; i < table.table.count; ++i)
                        if ((p != player_ || i != action) && candidate.key[p][i] == key)
                            candidate.key[p][i] = SDL_SCANCODE_UNKNOWN;
            }
        }
        recomp::Ini ini(file);
        ini.load();
        // Write only changed fields, retaining comments and all other keys.
        for (int p = 0; p < table.table.players; ++p) {
            for (int i = 0; i < table.table.count; ++i) {
                if (candidate.key[p][i] != b.key[p][i]) {
                    ini.set(recomp::BindingSection(false, p), table.table.actions[i].name,
                            recomp::KeyText(candidate.key[p][i]));
                }
                if (candidate.pad[p][i] != b.pad[p][i]) {
                    ini.set(recomp::BindingSection(true, p), table.table.actions[i].name,
                            recomp::PadText(candidate.pad[p][i]));
                }
            }
        }
        if (isPad) owners.PopulateProfile(ini, player_, candidate, table);
        if (!ini.save()) {
            Notice("COULD NOT SAVE INPUT SETTINGS"); capture_ = false; return;
        }
        b = candidate; // live immediately; no game reset or relaunch
        capture_ = false;
        suppressUntil_ = SDL_GetTicks() + 350;
        Notice("INPUT SAVED");
    }
    void SaveRaw(PadOwners &owners, const BoundTable &table,
                 const std::string &file, PadOwners::RawBind bind) {
        const int action = kActionOrder[size_t(controlRow_ - 2)];
        const bool saved = owners.SaveRaw(player_, action, bind, file, table);
        capture_ = false;
        suppressUntil_ = SDL_GetTicks() + 350;
        Notice(saved ? "RAW INPUT SAVED" : "COULD NOT SAVE RAW INPUT");
    }

public:
    bool Active() const { return visible_; }
    bool Capturing() const { return visible_ && capture_; }
    int Tab() const { return tab_; }
    int GameRow() const { return gameRow_; }
    int GraphicsRow() const { return graphicsRow_; }
    int Player() const { return player_; }
    int ControlRow() const { return controlRow_; }
    bool PadMode() const { return editPad_[size_t(player_)]; }
    const char *Message() const {
        if (Capturing()) return PadMode() ? "PRESS GAMEPAD BUTTON / TRIGGER - ESC CANCEL"
                                         : "PRESS ANY KEY - DELETE CLEARS - ESC CANCEL";
        return SDL_GetTicks() < noticeUntil_ ? notice_.c_str() : "";
    }
    void Open() {
        visible_ = true; capture_ = false; tab_ = 0; suppressUntil_ = SDL_GetTicks() + 160;
        Notice("Q / E OR LB / RB TO CHANGE TABS", 2200);
    }
    void Close() { visible_ = false; capture_ = false; }
    static constexpr int ActionCount() { return 14; }
    static constexpr int TableIndex(int row) { return kActionOrder[size_t(row)]; }
    static const char *ActionTitle(int row) { return kActionTitles[size_t(row)]; }

    // SDL input capture runs BEFORE the regular frontend keyboard/pad
    // shortcuts.  Only the currently assigned physical gamepad is accepted.
    bool HandleEvent(const SDL_Event &ev, recomp::Bindings &bindings,
                     const BoundTable &table, const std::string &file,
                     PadOwners &owners) {
        if (!visible_) return false;
        if (capture_) {
            if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                if (ev.key.scancode == SDL_SCANCODE_ESCAPE) {
                    capture_ = false; suppressUntil_ = SDL_GetTicks() + 250;
                    Notice("CAPTURE CANCELLED"); return true;
                }
                if (ev.key.scancode == SDL_SCANCODE_DELETE ||
                    ev.key.scancode == SDL_SCANCODE_BACKSPACE) {
                    if (PadMode() && owners.Raw(player_)) SaveRaw(owners, table, file, {});
                    else Save(bindings, table, file, SDL_SCANCODE_UNKNOWN, {}, owners);
                    return true;
                }
                if (!PadMode() && SDL_GetTicks() >= captureAt_ + 180 &&
                    ev.key.scancode != SDL_SCANCODE_UNKNOWN &&
                    ev.key.scancode != SDL_SCANCODE_F1 &&
                    ev.key.scancode != SDL_SCANCODE_F2 &&
                    ev.key.scancode != SDL_SCANCODE_F9 &&
                    ev.key.scancode != SDL_SCANCODE_F11 &&
                    ev.key.scancode != SDL_SCANCODE_F12) {
                    Save(bindings, table, file, ev.key.scancode, {}, owners);
                }
                return true;
            }
            SDL_Gamepad *assignedGamepad = PadMode() ? owners.Gamepad(player_) : nullptr;
            const bool selectedPad = assignedGamepad != nullptr;
            if (selectedPad && ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                ev.gbutton.which == SDL_GetGamepadID(assignedGamepad) &&
                SDL_GetTicks() >= captureAt_ + 180) {
                Save(bindings, table, file, SDL_SCANCODE_UNKNOWN,
                     {int(ev.gbutton.button), -1}, owners);
                return true;
            }
            if (selectedPad && ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION &&
                ev.gaxis.which == SDL_GetGamepadID(assignedGamepad) &&
                (ev.gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
                 ev.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) &&
                ev.gaxis.value >= 24000 && SDL_GetTicks() >= captureAt_ + 180) {
                Save(bindings, table, file, SDL_SCANCODE_UNKNOWN,
                     {-1, int(ev.gaxis.axis)}, owners);
                return true;
            }
            // A Fighter PI may be a genuine SDL joystick rather than an
            // XInput gamepad. Capture its physical button/hat/axis numbers.
            SDL_Joystick *raw = PadMode() ? owners.RawJoystick(player_) : nullptr;
            const SDL_JoystickID id = raw ? SDL_GetJoystickID(raw) : 0;
            if (id && SDL_GetTicks() >= captureAt_ + 180) {
                if (ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN &&
                    ev.jbutton.which == id) {
                    SaveRaw(owners, table, file, {'b', int(ev.jbutton.button), 0});
                    return true;
                }
                if (ev.type == SDL_EVENT_JOYSTICK_HAT_MOTION &&
                    ev.jhat.which == id && ev.jhat.value != SDL_HAT_CENTERED) {
                    SaveRaw(owners, table, file,
                            {'h', int(ev.jhat.hat), int(ev.jhat.value)});
                    return true;
                }
                if (ev.type == SDL_EVENT_JOYSTICK_AXIS_MOTION &&
                    ev.jaxis.which == id &&
                    (ev.jaxis.value >= 24000 || ev.jaxis.value <= -24000)) {
                    SaveRaw(owners, table, file,
                            {'a', int(ev.jaxis.axis), ev.jaxis.value > 0 ? 1 : -1});
                    return true;
                }
            }
            // Prevent keyboard/pad events from reaching gameplay while a
            // field waits for input; SDL_QUIT still passes through.
            return ev.type == SDL_EVENT_KEY_DOWN ||
                   ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                   ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION ||
                   ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN ||
                   ev.type == SDL_EVENT_JOYSTICK_AXIS_MOTION ||
                   ev.type == SDL_EVENT_JOYSTICK_HAT_MOTION;
        }
        if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
            if (ev.key.scancode == SDL_SCANCODE_Q ||
                (ev.key.scancode == SDL_SCANCODE_TAB && (ev.key.mod & SDL_KMOD_SHIFT))) {
                ChangeTab(-1); return true;
            }
            if (ev.key.scancode == SDL_SCANCODE_E || ev.key.scancode == SDL_SCANCODE_TAB) {
                ChangeTab(+1); return true;
            }
            if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_F1) {
                Close(); return true;
            }
        }
        if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                ChangeTab(-1); return true;
            }
            if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                ChangeTab(+1); return true;
            }
        }
        return false;
    }

    // Called on the same menu-repeat signal as the old main menu.
    // Result: 0 idle, 1 move, 2 confirm, 3 return-to-home.
    int Press(unsigned pressed, SDL_Window *window, recomp::Bindings &bindings,
              const BoundTable &table, const std::string &settingsPath,
              PadOwners &owners) {
        if (!visible_ || capture_ || !pressed || SDL_GetTicks() < suppressUntil_) return 0;
        if (pressed & recomp::kMenuBack) { Close(); return 3; }
        const int dy = (pressed & recomp::kMenuDown ? 1 : 0) -
                       (pressed & recomp::kMenuUp ? 1 : 0);
        const int dx = (pressed & recomp::kMenuRight ? 1 : 0) -
                       (pressed & recomp::kMenuLeft ? 1 : 0);
        if (dy) {
            int *row = tab_ == 0 ? &gameRow_ : tab_ == 1 ? &graphicsRow_ : &controlRow_;
            const int count = tab_ == 0 ? 7 : tab_ == 1 ? 8 : 16;
            *row = (*row + count + dy) % count;
            return 1;
        }
        if (dx) {
            if (tab_ == 2) player_ = (player_ + 2 + dx) % 2;
            else ChangeTab(dx);
            return 1;
        }
        if (!(pressed & recomp::kMenuConfirm)) return 0;
        if (tab_ == 0) {
            switch (gameRow_) {
            case 0: Notice("V2 IS THE CURRENT RECOMP GAME"); break;
            case 1: case 2: case 3: Notice("FEATURE NOT PORTED TO V2"); break;
            case 4: cps2recomp::SetFeature("free_play", !cps2recomp::FeatureOn("free_play")); break;
            case 5: cps2recomp::SetFeatureValue("board_difficulty",
                        (cps2recomp::FeatureSetting("board_difficulty") + 1) % 9);
                    Notice("RESTART ARCADE TO APPLY"); break;
            case 6: cps2recomp::SetFeatureValue("board_speed",
                        (cps2recomp::FeatureSetting("board_speed") + 1) % 6);
                    Notice("RESTART ARCADE TO APPLY"); break;
            }
        } else if (tab_ == 1 && window) {
            if (graphicsRow_ < 6) {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                    SDL_SetWindowFullscreen(window, false);
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
                    SDL_RestoreWindow(window);
                SDL_SetWindowSize(window, kWindowSizes[size_t(graphicsRow_)].width,
                                 kWindowSizes[size_t(graphicsRow_)].height);
            } else if (graphicsRow_ == 6) {
                SDL_SetWindowFullscreen(window, true);
            } else if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) {
                SDL_RestoreWindow(window);
            } else {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                    SDL_SetWindowFullscreen(window, false);
                SDL_MaximizeWindow(window);
            }
        } else if (tab_ == 2) {
            if (controlRow_ == 0) {
                std::string status;
                owners.AssignNext(player_, settingsPath, bindings, table, status);
                Notice(std::move(status));
            } else if (controlRow_ == 1) {
                editPad_[size_t(player_)] = !editPad_[size_t(player_)];
                if (PadMode() && !owners.Assigned(player_))
                    Notice("ASSIGN A LIVE CONTROLLER FIRST");
            } else if (PadMode() && !owners.Assigned(player_)) {
                Notice("CONTROLLER NOT CONNECTED");
            } else {
                capture_ = true;
                captureAt_ = SDL_GetTicks();
            }
        }
        return 2;
    }

    std::string GameLabel(int row) const {
        if (row == 0) return "GAME VERSION : V2";
        if (row == 1) return "TOURNAMENT : NOT PORTED";
        if (row == 2) return "DOUBLE FEATURE : NOT PORTED";
        if (row == 3) return "DIZZY BAR : NOT PORTED";
        if (row == 4) return std::string("FREE PLAY : ") +
                             (cps2recomp::FeatureOn("free_play") ? "ON" : "OFF");
        if (row == 5) {
            const char *choices[] = {"BOARD", "EASY", "NORMAL", "HARD 1", "HARD 2",
                                     "HARD 3", "HARD 4", "HARD 5", "EXPERT"};
            const int value = std::clamp(cps2recomp::FeatureSetting("board_difficulty"), 0, 8);
            return std::string("DIFFICULTY : ") + choices[value];
        }
        const char *choices[] = {"BOARD", "NORMAL", "TURBO 1", "TURBO 2",
                                 "FREE 1", "FREE 2"};
        const int value = std::clamp(cps2recomp::FeatureSetting("board_speed"), 0, 5);
        return std::string("GAME SPEED : ") + choices[value];
    }

    std::string GraphicsLabel(int row) const {
        static constexpr const char *presets[] = {
            "NATIVE 384 X 224", "2X 768 X 448", "3X 1152 X 672",
            "4X 1536 X 896", "5X 1920 X 1120", "6X 2304 X 1344"
        };
        return row < 6 ? presets[row] : row == 6 ? "FULLSCREEN FIT" : "MAXIMIZE WINDOW";
    }

    std::string DeviceLabel(const PadOwners &owners) const {
        return "P" + std::to_string(player_ + 1) + " DEVICE : " +
               owners.DeviceName(player_);
    }
    std::string EditLabel(const PadOwners &owners) const {
        return std::string("EDIT INPUT : ") +
               (PadMode() ? (owners.Raw(player_) ? "RAW JOYSTICK" : "GAMEPAD")
                          : "KEYBOARD");
    }

    std::string BindingLabel(int row, const recomp::Bindings &bindings,
                             const BoundTable &table, const PadOwners &owners) const {
        const int action = kActionOrder[size_t(row)];
        const std::string label = kActionTitles[size_t(row)];
        if (action >= table.table.count) return label + " : UNSUPPORTED";
        if (!PadMode()) {
            const SDL_Scancode key = bindings.key[player_][action];
            return label + " : KEY " + recomp::KeyText(key);
        }
        if (owners.Raw(player_)) return label + " : " + owners.RawLabel(player_, action);
        return label + " : " + recomp::PadText(bindings.pad[player_][action]);
    }
};

} // namespace port::xmvsf_home
