// XMVSF native Options v87.448: real SDL3 input mappings and settings hooks.
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
    void Finished(bool saved, const std::string &success,
                  const std::string &failure = "SAVE FAILED") {
        capture_ = false;
        suppressUntil_ = SDL_GetTicks() + 350;
        Notice(saved ? success : failure, 1800);
    }

    void StoreInput(PadOwners &owners, recomp::Bindings &bindings,
                    const BoundTable &table, const std::string &file,
                    PadOwners::RawBind binding) {
        if (controlRow_ < 1) return;
        const int a = kActionOrder[size_t(controlRow_ - 1)];
        if (owners.Keyboard(player_)) {
            if (binding.type != 'k') return;
            Finished(owners.SaveKeyboardAction(player_, a,
                         static_cast<SDL_Scancode>(binding.index),
                         file, bindings, table),
                     "PLAYER " + std::to_string(player_ + 1) +
                         " " + kActionTitles[size_t(controlRow_ - 1)] + " KEY SAVED");
        } else {
            Finished(owners.SaveRaw(player_, a, binding, file, table),
                     "PLAYER " + std::to_string(player_ + 1) +
                         " " + kActionTitles[size_t(controlRow_ - 1)] + " SAVED");
        }
    }

    void StoreDevice(PadOwners &owners, recomp::Bindings &bindings,
                     const BoundTable &table, const std::string &file,
                     SDL_JoystickID instance, bool keyboard) {
        bool swapped = false;
        const bool saved = keyboard
             ? owners.AssignKeyboard(player_, file, bindings, table)
             : owners.AssignFromInstance(player_, instance, file, bindings,
                                          table, swapped);
        Finished(saved,
            "PLAYER " + std::to_string(player_ + 1) +
               (keyboard ? " KEYBOARD SAVED" :
                  (swapped ? " SLOT SWAPPED" : " CONTROLLER SAVED")),
            "CONTROLLER ASSIGNMENT SAVE FAILED");
    }

public:
    bool Active() const { return visible_; }
    bool Capturing() const { return visible_ && capture_; }
    int Tab() const { return tab_; }
    int GameRow() const { return gameRow_; }
    int GraphicsRow() const { return graphicsRow_; }
    int Player() const { return player_; }
    int ControlRow() const { return controlRow_; }
    const char *Message() const {
        if (Capturing()) return controlRow_ == 0
              ? "PRESS KEY OR DEVICE BUTTON"
              : "PRESS KEY OR CONTROLLER BUTTON";
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
            // Stage13 capture is driven by the NEXT physical event.
            // The A/Enter that selected this field must not itself become
            // the assigned device or binding. Never react to key repeats.
            if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                const SDL_Scancode key = ev.key.scancode;
                if (key == SDL_SCANCODE_ESCAPE) {
                    capture_ = false;
                    suppressUntil_ = SDL_GetTicks() + 250;
                    Notice("CAPTURE CANCELLED", 1500);
                    return true;
                }
                if (SDL_GetTicks() < captureAt_ + 180) return true;
                // Reserve diagnostics exactly as in our SDL2 frontend.
                if (key == SDL_SCANCODE_UNKNOWN || key == SDL_SCANCODE_F9 ||
                    key == SDL_SCANCODE_F10 || key == SDL_SCANCODE_F12) return true;
                if (controlRow_ == 0)
                    StoreDevice(owners, bindings, table, file, 0, true);
                else
                    StoreInput(owners, bindings, table, file, {'k', int(key), 0});
                return true;
            }
            if (SDL_GetTicks() < captureAt_ + 180) {
                return ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                       ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN ||
                       ev.type == SDL_EVENT_JOYSTICK_AXIS_MOTION ||
                       ev.type == SDL_EVENT_JOYSTICK_HAT_MOTION;
            }
            if (controlRow_ == 0) {
                // Pressing a physical BUTTON on any device owns that exact
                // SDL instance. If it belongs to the other player, SWAP
                // their entire saved player configurations (Stage13).
                if (ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN) {
                    StoreDevice(owners, bindings, table, file,
                                ev.jbutton.which, false);
                    return true;
                }
                if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                    StoreDevice(owners, bindings, table, file,
                                ev.gbutton.which, false);
                    return true;
                }
            } else {
                // Only the controller currently owned by this player can
                // change their per-action physical binding.
                const SDL_JoystickID owner = owners.AssignedInstance(player_);
                if (owner && ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN &&
                    ev.jbutton.which == owner) {
                    StoreInput(owners, bindings, table, file,
                               {'b', int(ev.jbutton.button), 0});
                    return true;
                }
                if (owner && ev.type == SDL_EVENT_JOYSTICK_AXIS_MOTION &&
                    ev.jaxis.which == owner && ev.jaxis.value > 20000) {
                    StoreInput(owners, bindings, table, file,
                               {'a', int(ev.jaxis.axis), 1});
                    return true;
                }
                if (owner && ev.type == SDL_EVENT_JOYSTICK_HAT_MOTION &&
                    ev.jhat.which == owner && ev.jhat.value != SDL_HAT_CENTERED) {
                    StoreInput(owners, bindings, table, file,
                               {'h', int(ev.jhat.hat), int(ev.jhat.value)});
                    return true;
                }
            }
            // Suppress navigation until capture finishes; don't route
            // events to the game while editing, but allow SDL_QUIT.
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
            const int count = tab_ == 0 ? 7 : tab_ == 1 ? 8 : owners.VisibleActions(player_) + 1;
            *row = (*row + count + dy) % count;
            return 1;
        }
        if (dx) {
            if (tab_ == 2) {
                player_ = (player_ + 2 + dx) % 2;
                controlRow_ = 0; // exactly Stage14 P1/P2 page handoff
            }
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
            // Original SDL2: DEVICE row does not cycle a type.
            // A arms capture; the next key or joystick button assigns P1/P2.
            // Subsequent input rows capture one key or physical device input.
            if (controlRow_ > 0 && !owners.Keyboard(player_) &&
                !owners.Assigned(player_)) {
                Notice("ASSIGN CONTROLLER FIRST", 2400);
            } else {
                capture_ = true;
                captureAt_ = SDL_GetTicks();
                Notice("PRESS KEY OR DEVICE BUTTON", 5000);
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
    std::string BindingLabel(int row, const recomp::Bindings &bindings,
                             const BoundTable &table, const PadOwners &owners) const {
        if (row < 0 || row >= owners.VisibleActions(player_))
            return "NONE";
        const int action = kActionOrder[size_t(row)];
        const std::string label = kActionTitles[size_t(row)];
        if (action >= table.table.count) return label + " : UNSUPPORTED";
        if (owners.Keyboard(player_)) {
            const SDL_Scancode key = bindings.key[player_][action];
            return label + " : KEY " + recomp::KeyText(key);
        }
        if (!owners.Physical(player_)) return label + " : NONE";
        return label + " : " + owners.RawLabel(player_, action);
    }

};

} // namespace port::xmvsf_home
