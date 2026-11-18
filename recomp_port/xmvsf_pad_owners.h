// XMVSF v87.448 — original SDL2 per-player capture and physical profiles.
// Port of Stage13/87.248/87.249: live instance first, saved GUID/ordinal,
// then exact joystick/OS name. Profiles survive disconnect and reassignment.
#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "bindings.h"
#include "../recomp-common/menu.h"
#include "../recomp-common/settings.h"

namespace port::xmvsf_home {

class PadOwners {
public:
    static constexpr int kPlayers = 2;
    struct RawBind {
        char type = 'n'; // b: button; a: axis sign; h: hat bit; n: no binding
        int index = -1, direction = 0;
    };
    struct Device {
        SDL_JoystickID instance = 0;
        SDL_Gamepad *gamepad = nullptr;
        SDL_Joystick *joystick = nullptr;
        bool raw = false;
        std::string guid, serial, name;
        int ordinal = 0, nameOrdinal = 0;
    };
    struct Slot {
        bool configured = false;
        std::string mode = "auto"; // auto, none, keyboard, gamepad, raw
        std::string guid, serial, name, profile;
        int ordinal = 0, nameOrdinal = 0;
        SDL_JoystickID live = 0;
    };

private:
    std::array<Slot, kPlayers> slots_{};
    std::array<std::array<RawBind, 14>, kPlayers> rawBindings_{};
    std::array<SDL_JoystickID, kPlayers> profileInstance_{{0, 0}};
    std::array<std::string, kPlayers> loadedProfile_{};
    std::vector<std::pair<SDL_JoystickID, SDL_Joystick *>> rawOpened_;
    std::vector<Device> devices_;
    std::array<int, kPlayers> routed_{{-1, -1}};
    std::string iniPath_;
    bool initialized_ = false;

    static std::string SafeName(const char *s) {
        std::string result;
        if (!s) return result;
        for (; *s && result.size() < 90; ++s) {
            const unsigned char c = static_cast<unsigned char>(*s);
            if (c == '\n' || c == '\r' || c == '\t') result.push_back(' ');
            else if (c >= 32 && c < 127) result.push_back(char(c));
            else if (!result.empty() && result.back() != ' ') result.push_back(' ');
        }
        return result;
    }
    static std::string IdentityGuid(SDL_Joystick *js) {
        if (!js) return {};
        char buffer[33] = {};
        SDL_GUIDToString(SDL_GetJoystickGUID(js), buffer, sizeof buffer);
        return buffer;
    }
    static std::string ProfileKey(const Device &d) {
        // GUID and occurrence stable across sessions; the stored player
        // record also preserves the previous key if its driver GUID changes.
        return std::string("xmvsf_profile_") + (d.raw ? "raw_" : "pad_") +
               d.guid + "_" + std::to_string(d.ordinal);
    }
    static std::string SlotSection(int p) {
        return "xmvsf_player" + std::to_string(p + 1);
    }
    static bool SameIdentity(const Slot &s, const Device &d) {
        if (s.mode != (d.raw ? "raw" : "gamepad")) return false;
        if (!s.serial.empty() && !d.serial.empty() && s.serial == d.serial)
            return true;
        if (!s.guid.empty() && s.guid == d.guid && s.ordinal == d.ordinal)
            return true;
        return !s.name.empty() && s.name == d.name && s.nameOrdinal == d.nameOrdinal;
    }
    static bool IsRawDown(SDL_Joystick *js, RawBind bind) {
        // Original Stage13 allows a keyboard scancode on any individual
        // action, including a controller-owned player's action row.
        if (bind.type == 'k') {
            const bool *keys = SDL_GetKeyboardState(nullptr);
            return keys && bind.index >= 0 && bind.index < SDL_SCANCODE_COUNT &&
                   keys[bind.index];
        }
        if (!js) return false;
        if (bind.type == 'b') {
            return bind.index >= 0 && bind.index < SDL_GetNumJoystickButtons(js) &&
                   SDL_GetJoystickButton(js, bind.index);
        }
        if (bind.type == 'a') {
            if (bind.index < 0 || bind.index >= SDL_GetNumJoystickAxes(js)) return false;
            const int axis = SDL_GetJoystickAxis(js, bind.index);
            return bind.direction > 0 ? axis > 14000 : axis < -14000;
        }
        if (bind.type == 'h') {
            return bind.index >= 0 && bind.index < SDL_GetNumJoystickHats(js) &&
                   (SDL_GetJoystickHat(js, bind.index) & bind.direction) != 0;
        }
        return false;
    }
    static std::string RawText(RawBind b) {
        if (b.type == 'k') {
            const char *name = SDL_GetScancodeName(static_cast<SDL_Scancode>(b.index));
            return std::string("KEY ") + (name && *name ? name : "UNKNOWN");
        }
        if (b.type == 'b') return "BUTTON " + std::to_string(b.index);
        if (b.type == 'a') return "AXIS " + std::to_string(b.index) +
                                  (b.direction > 0 ? " +" : " -");
        if (b.type == 'h') return "HAT " + std::to_string(b.index) +
                                  " DIR " + std::to_string(b.direction);
        return "NONE";
    }
    static std::string RawSetting(RawBind b) {
        if (b.type == 'n') return "none";
        return std::string(1, b.type) + ":" + std::to_string(b.index) + ":" +
               std::to_string(b.direction);
    }
    static RawBind ParseRaw(const std::string &s) {
        RawBind b{};
        if (s.empty() || s == "none") return b;
        int index = 0, dir = 0;
        char type = 0, extra = 0;
        if (std::sscanf(s.c_str(), "%c:%d:%d%c", &type, &index, &dir, &extra) != 3)
            return b;
        if ((type == 'b' || type == 'a' || type == 'h' || type == 'k') &&
            index >= 0 && index <= 63 && dir >= -32768 && dir <= 32768)
            return RawBind{type, index, dir};
        return b;
    }

    // Resolve SDL3's semantic gamepad controls to their actual joystick
    // button/axis/hat indices, like the old SDL2 GetBindForButton path.
    static RawBind FromSDLGamepad(SDL_Gamepad *gp, int button, int axis = -1) {
        if (!gp) return {};
        int count = 0;
        SDL_GamepadBinding **maps = SDL_GetGamepadBindings(gp, &count);
        RawBind result{};
        for (int i = 0; maps && i < count; ++i) {
            const SDL_GamepadBinding &b = *maps[i];
            if (button >= 0 &&
                (b.output_type != SDL_GAMEPAD_BINDTYPE_BUTTON ||
                 int(b.output.button) != button)) continue;
            if (button < 0 &&
                (b.output_type != SDL_GAMEPAD_BINDTYPE_AXIS ||
                 int(b.output.axis.axis) != axis)) continue;
            if (b.input_type == SDL_GAMEPAD_BINDTYPE_BUTTON)
                result = {'b', b.input.button, 0};
            else if (b.input_type == SDL_GAMEPAD_BINDTYPE_AXIS)
                result = {'a', b.input.axis.axis,
                          b.input.axis.axis_max > 0 ? 1 : -1};
            else if (b.input_type == SDL_GAMEPAD_BINDTYPE_HAT)
                result = {'h', b.input.hat.hat, b.input.hat.hat_mask};
            break;
        }
        SDL_free(maps); // SDL3 returns one allocation, not separate entries
        return result;
    }

    void DefaultsForPhysical(int p, const Device &device) {
        rawBindings_[p].fill({});
        // Exact Stage13 initial physical button order:
        // X/Y/LB, A/B/RB, Back/Start (3P/3K intentionally unbound).
        constexpr int index[8] = {4, 5, 6, 7, 8, 9, 11, 10};
        constexpr int fallback[8] = {2, 3, 4, 0, 1, 5, 6, 7};
        constexpr int semantic[8] = {
            SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
            SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_SOUTH,
            SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
            SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_START
        };
        for (int i = 0; i < 8; ++i) {
            RawBind b = FromSDLGamepad(device.gamepad, semantic[i]);
            if (b.type == 'n') b = {'b', fallback[i], 0};
            rawBindings_[p][size_t(index[i])] = b;
        }
    }

    void RefreshPhysicalProfiles(recomp::Bindings &bindings, const BoundTable &table) {
        for (int p = 0; p < kPlayers; ++p) {
            const Device *d = Assigned(p);
            if (!d || !Physical(p)) {
                profileInstance_[p] = 0;
                continue;
            }
            if (profileInstance_[p] == d->instance &&
                loadedProfile_[p] == slots_[p].profile) continue;
            DefaultsForPhysical(p, *d);
            LoadProfile(p, bindings, table);
            profileInstance_[p] = d->instance;
            loadedProfile_[p] = slots_[p].profile;
        }
    }

    void LoadProfile(int p, recomp::Bindings &bindings, const BoundTable &table) {
        if (iniPath_.empty() || slots_[p].profile.empty()) return;
        recomp::Ini ini(iniPath_);
        ini.load();
        const std::string section = slots_[p].profile;
        bool hasAny = false;
        for (int i = 0; i < table.table.count; ++i) {
            const std::string value = ini.get(section, table.table.actions[i].name);
            if (value.empty()) continue;
            hasAny = true;
            if (value == "none" || (value.size() > 2 && value[1] == ':'))
                rawBindings_[p][size_t(i)] = ParseRaw(value);
            else if (slots_[p].mode == "gamepad")
                bindings.pad[p][i] = recomp::PadFromText(value);
        }
        if (hasAny)
            std::fprintf(stderr, "XMVSF 87.443 P%d remembered input profile %s\n",
                         p + 1, section.c_str());
    }

    void Rebuild(const std::vector<SDL_Gamepad *> &open) {
        devices_.clear();
        for (SDL_Gamepad *gp : open) {
            if (!gp || !SDL_GamepadConnected(gp)) continue;
            SDL_Joystick *js = SDL_GetGamepadJoystick(gp);
            if (!js) continue;
            Device d;
            d.instance = SDL_GetGamepadID(gp);
            d.gamepad = gp; d.joystick = js;
            d.guid = IdentityGuid(js);
            d.serial = SafeName(SDL_GetJoystickSerial(js));
            // Preserve the real OS/SDL joystick name ("Fighter PI") instead
            // of displaying a generic mapped Gamepad type/name.
            d.name = SafeName(SDL_GetJoystickName(js));
            if (d.name.empty()) d.name = SafeName(SDL_GetGamepadName(gp));
            if (d.name.empty()) d.name = "UNKNOWN CONTROLLER";
            devices_.push_back(std::move(d));
        }
        for (const auto &[instance, js] : rawOpened_) {
            if (!js || !SDL_JoystickConnected(js) || SDL_IsGamepad(instance)) continue;
            Device d;
            d.instance = instance;
            d.joystick = js; d.raw = true;
            d.guid = IdentityGuid(js);
            d.serial = SafeName(SDL_GetJoystickSerial(js));
            d.name = SafeName(SDL_GetJoystickName(js));
            if (d.name.empty()) d.name = "UNKNOWN JOYSTICK";
            devices_.push_back(std::move(d));
        }
        std::sort(devices_.begin(), devices_.end(),
                  [](const Device &a, const Device &b) { return a.instance < b.instance; });
        std::map<std::string, int> seenGuid, seenName;
        for (Device &d : devices_) {
            const std::string kind = d.raw ? "r" : "g";
            d.ordinal = seenGuid[kind + d.guid]++;
            d.nameOrdinal = seenName[kind + d.name]++;
        }
    }

    void Route() {
        routed_ = {{-1, -1}};
        std::set<int> used;
        // Reserve both explicitly configured LIVE player instances before
        // reconnect recovery. An unconfigured automatic slot must not steal
        // another player's Fighter PI or identical GUID device.
        for (int p = 0; p < kPlayers; ++p) {
            const Slot &owner = slots_[p];
            if (!owner.configured || !owner.live ||
                owner.mode == "none" || owner.mode == "keyboard") continue;
            for (int i = 0; i < int(devices_.size()); ++i) {
                if (devices_[size_t(i)].instance != owner.live || used.count(i)) continue;
                routed_[p] = i; used.insert(i); break;
            }
        }
        // Recover explicitly configured slots by serial, GUID+ordinal
        // or OS name+ordinal, never substituting unrelated physical pads.
        for (int p = 0; p < kPlayers; ++p) {
            Slot &owner = slots_[p];
            if (!owner.configured || routed_[p] >= 0) continue;
            if (owner.mode == "none" || owner.mode == "keyboard") {
                owner.live = 0; continue;
            }
            int matched = -1;
            for (int i = 0; i < int(devices_.size()); ++i) {
                if (!used.count(i) && SameIdentity(owner, devices_[size_t(i)])) {
                    matched = i; break;
                }
            }
            if (matched >= 0) {
                routed_[p] = matched;
                owner.live = devices_[size_t(matched)].instance;
                used.insert(matched);
            } else owner.live = 0;
        }
        // Only now allocate unused controllers to legacy auto slots.
        for (int p = 0; p < kPlayers; ++p) {
            Slot &owner = slots_[p];
            if (owner.configured) continue;
            int found = -1;
            // Reuse this auto slot's live device if still unclaimed.
            for (int i = 0; i < int(devices_.size()); ++i)
                if (!used.count(i) && owner.live &&
                    devices_[size_t(i)].instance == owner.live) {
                    found = i; break;
                }
            if (found < 0) for (int i = 0; i < int(devices_.size()); ++i)
                if (!used.count(i)) { found = i; break; }
            routed_[p] = found;
            owner.live = found < 0 ? 0 : devices_[size_t(found)].instance;
            if (found >= 0) used.insert(found);
        }
    }

public:
    ~PadOwners() { Close(); }
    PadOwners(const PadOwners &) = delete;
    PadOwners &operator=(const PadOwners &) = delete;
    PadOwners() = default;

    void Init(const std::string &path) {
        if (initialized_) return;
        initialized_ = true; iniPath_ = path;
        if (!path.empty()) {
            recomp::Ini ini(path);
            ini.load();
            for (int p = 0; p < kPlayers; ++p) {
                const std::string sec = SlotSection(p);
                Slot &s = slots_[p];
                s.mode = ini.get(sec, "mode");
                if (s.mode.empty()) { s.mode = "auto"; continue; }
                if (s.mode != "keyboard" && s.mode != "none" &&
                    s.mode != "gamepad" && s.mode != "raw") {
                    s.mode = "auto"; continue;
                }
                s.configured = true;
                s.guid = ini.get(sec, "guid");
                s.name = ini.get(sec, "name");
                s.serial = ini.get(sec, "serial");
                s.profile = ini.get(sec, "profile");
                try {
                    s.ordinal = std::max(0, std::stoi(ini.get(sec, "ordinal")));
                } catch (...) { s.ordinal = 0; }
                try {
                    s.nameOrdinal = std::max(0, std::stoi(ini.get(sec, "name_ordinal")));
                } catch (...) { s.nameOrdinal = 0; }
            }
        }
        // SDL3 uses instance IDs rather than joystick indices; raw HID
        // joysticks that aren't mapped as Gamepads need an explicit handle.
        int count = 0;
        SDL_JoystickID *ids = SDL_GetJoysticks(&count);
        if (ids) {
            for (int i = 0; i < count; ++i) if (!SDL_IsGamepad(ids[i]))
                if (SDL_Joystick *js = SDL_OpenJoystick(ids[i]))
                    rawOpened_.emplace_back(ids[i], js);
            SDL_free(ids);
        }
    }

    void LoadSavedProfiles(recomp::Bindings &b, const BoundTable &table) {
        for (int p = 0; p < kPlayers; ++p) if (slots_[p].configured)
            LoadProfile(p, b, table);
    }
    void Event(const SDL_Event &ev) {
        if (ev.type == SDL_EVENT_JOYSTICK_ADDED) {
            if (SDL_IsGamepad(ev.jdevice.which)) return;
            for (const auto &[id, js] : rawOpened_)
                if (id == ev.jdevice.which) return;
            if (SDL_Joystick *js = SDL_OpenJoystick(ev.jdevice.which))
                rawOpened_.emplace_back(ev.jdevice.which, js);
        } else if (ev.type == SDL_EVENT_JOYSTICK_REMOVED) {
            for (auto it = rawOpened_.begin(); it != rawOpened_.end(); ++it) {
                if (it->first != ev.jdevice.which) continue;
                SDL_CloseJoystick(it->second);
                rawOpened_.erase(it);
                break;
            }
        }
    }
    void Refresh(const std::vector<SDL_Gamepad *> &open,
                 recomp::Bindings &bindings, const BoundTable &table) {
        Rebuild(open);
        Route();
        RefreshPhysicalProfiles(bindings, table);
    }
    void Close() {
        for (const auto &[id, js] : rawOpened_) SDL_CloseJoystick(js);
        rawOpened_.clear();
        devices_.clear();
        routed_ = {{-1, -1}};
    }
    const std::vector<Device> &Devices() const { return devices_; }
    const Device *Assigned(int p) const {
        if (p < 0 || p >= kPlayers) return nullptr;
        const int i = routed_[p];
        return i >= 0 && i < int(devices_.size()) ? &devices_[size_t(i)] : nullptr;
    }
    SDL_Gamepad *Gamepad(int p) const {
        const Device *d = Assigned(p);
        return d ? d->gamepad : nullptr;
    }
    SDL_Gamepad *AdditionalGamepad(int n) const {
        // Keep the recomp's 3P/4P support alive without recycling a
        // controller already claimed by P1 or P2.
        for (const Device &d : devices_) {
            if (!d.gamepad) continue;
            if ((Assigned(0) && Assigned(0)->instance == d.instance) ||
                (Assigned(1) && Assigned(1)->instance == d.instance)) continue;
            if (n-- == 0) return d.gamepad;
        }
        return nullptr;
    }
    SDL_Joystick *RawJoystick(int p) const {
        const Device *d = Assigned(p);
        return d ? d->joystick : nullptr;
    }
    bool Raw(int p) const {
        const Device *d = Assigned(p);
        return d && d->raw;
    }
    bool Keyboard(int p) const {
        return p >= 0 && p < kPlayers && slots_[p].configured &&
               slots_[p].mode == "keyboard";
    }
    bool Physical(int p) const {
        return p >= 0 && p < kPlayers && slots_[p].configured &&
               (slots_[p].mode == "raw" || slots_[p].mode == "gamepad");
    }
    int VisibleActions(int p) const { return Keyboard(p) ? 14 : 10; }
    SDL_JoystickID AssignedInstance(int p) const {
        const Device *d = Assigned(p);
        return d ? d->instance : 0;
    }
    std::string DeviceName(int p) const {
        if (p < 0 || p >= kPlayers) return "INVALID PLAYER";
        const Slot &s = slots_[p];
        if (s.configured && s.mode == "keyboard") return "KEYBOARD";
        if (s.configured && s.mode == "none") return "NOT ASSIGNED";
        const Device *d = Assigned(p);
        if (!d) return s.configured ? "DISCONNECTED : " + s.name : "NOT ASSIGNED";
        return d->name + (d->raw ? " (JOYSTICK)" : "");
    }
    int CurrentChoice(int p) const {
        if (p < 0 || p >= kPlayers) return 0;
        const Slot &s = slots_[p];
        if (s.configured && s.mode == "none") return 0;
        if (s.configured && s.mode == "keyboard") return 1;
        const int index = routed_[p];
        return index >= 0 ? index + 2 : 0;
    }
    // 0: unassigned; 1: keyboard; 2+: actual live devices.
    bool AssignNext(int p, const std::string &path, recomp::Bindings &bindings,
                    const BoundTable &table, std::string &message) {
        if (p < 0 || p >= kPlayers) return false;
        const int choices = 2 + int(devices_.size());
        // First explicit selection keeps the auto-routed physical device;
        // subsequent presses cycle choices including KEYBOARD and NONE.
        int selected = slots_[p].configured ? (CurrentChoice(p) + 1) % choices :
                       (routed_[p] >= 0 ? routed_[p] + 2 : 1);
        // Skip devices that have been explicitly claimed by the other
        // player; a rejected choice must not trap the user in one row.
        for (int attempt = 0; attempt < choices && selected >= 2; ++attempt) {
            const Device &d = devices_[size_t(selected - 2)];
            const int other = 1 - p;
            if (!(slots_[other].configured && Assigned(other) &&
                  Assigned(other)->instance == d.instance)) break;
            selected = (selected + 1) % choices;
        }
        Slot candidate{};
        candidate.configured = true;
        if (selected == 0) candidate.mode = "none";
        else if (selected == 1) candidate.mode = "keyboard";
        else {
            const Device &d = devices_[size_t(selected - 2)];
            // Auto ownership yields to an explicit assignment; a saved
            // physical device is protected by the skip loop above.
            candidate.mode = d.raw ? "raw" : "gamepad";
            candidate.guid = d.guid;
            candidate.name = d.name;
            candidate.serial = d.serial;
            candidate.ordinal = d.ordinal;
            candidate.nameOrdinal = d.nameOrdinal;
            candidate.live = d.instance;
            candidate.profile = ProfileKey(d);
            // A reconnected known profile wins even if the driver changed GUID.
            const Slot &old = slots_[p];
            if (!old.profile.empty() && SameIdentity(old, d))
                candidate.profile = old.profile;
        }
        if (path.empty()) { message = "SETTINGS FILE UNAVAILABLE"; return false; }
        recomp::Ini ini(path);
        ini.load();
        const std::string section = SlotSection(p);
        ini.set(section, "mode", candidate.mode);
        ini.set(section, "guid", candidate.guid);
        ini.set(section, "name", candidate.name);
        ini.set(section, "serial", candidate.serial);
        ini.set(section, "ordinal", std::to_string(candidate.ordinal));
        ini.set(section, "name_ordinal", std::to_string(candidate.nameOrdinal));
        ini.set(section, "profile", candidate.profile);
        if (!ini.save()) { message = "COULD NOT SAVE CONTROLLER ASSIGNMENT"; return false; }
        const bool hadDifferentAssignment = slots_[p].configured &&
            slots_[p].profile != candidate.profile;
        slots_[p] = candidate;
        if (hadDifferentAssignment && candidate.mode == "gamepad") {
            const recomp::Bindings defaults = recomp::DefaultBindings(table.table);
            for (int a = 0; a < table.table.count; ++a)
                bindings.pad[p][a] = defaults.pad[p][a];
        }
        // Re-evaluate physical bindings with the newly selected controller.
        // The profile remains on disk even when switching to Keyboard.
        profileInstance_[p] = 0;
        loadedProfile_[p].clear();
        Route();
        RefreshPhysicalProfiles(bindings, table);
        message = "P" + std::to_string(p + 1) + " DEVICE : " + DeviceName(p);
        std::fprintf(stderr, "XMVSF 87.443 %s\n", message.c_str());
        return true;
    }

    void PopulateProfile(recomp::Ini &ini, int p, const recomp::Bindings &b,
                         const BoundTable &table) const {
        if (p < 0 || p >= kPlayers || slots_[p].profile.empty()) return;
        const std::string &sec = slots_[p].profile;
        if (slots_[p].mode == "gamepad" || slots_[p].mode == "raw") {
            for (int a = 0; a < table.table.count; ++a)
                ini.set(sec, table.table.actions[a].name,
                        RawSetting(rawBindings_[p][size_t(a)]));
        }
    }
    bool SaveRaw(int p, int action, RawBind bind, const std::string &file,
                 const BoundTable &table) {
        if (p < 0 || p >= kPlayers || action < 0 || action >= table.table.count ||
            file.empty() || !Physical(p) || slots_[p].profile.empty())
            return false;
        auto replacement = rawBindings_[p];
        for (int a = 0; a < table.table.count; ++a)
            if (a != action && replacement[size_t(a)].type == bind.type &&
                replacement[size_t(a)].index == bind.index &&
                replacement[size_t(a)].direction == bind.direction && bind.type != 'n')
                replacement[size_t(a)] = {};
        replacement[size_t(action)] = bind;
        recomp::Ini ini(file);
        ini.load();
        const std::string &sec = slots_[p].profile;
        for (int a = 0; a < table.table.count; ++a)
            ini.set(sec, table.table.actions[a].name, RawSetting(replacement[size_t(a)]));
        if (!ini.save()) return false;
        rawBindings_[p] = replacement;
        return true;
    }
    std::string RawLabel(int p, int action) const {
        if (p < 0 || p >= kPlayers || action < 0 || action >= 14) return "NONE";
        return RawText(rawBindings_[p][size_t(action)]);
    }
    uint16_t ReadRaw(int p, const BoundTable &table) const {
        SDL_Joystick *js = RawJoystick(p);
        if (!js) return 0;
        uint16_t out = 0;
        const int count = std::min(table.table.count, 14);
        for (int a = 0; a < count; ++a)
            if (IsRawDown(js, rawBindings_[p][size_t(a)]))
                out |= table.bits[a];
        // Out-of-box directional input for genuine, unmapped arcade sticks.
        const int x = SDL_GetNumJoystickAxes(js) > 0 ? SDL_GetJoystickAxis(js, 0) : 0;
        const int y = SDL_GetNumJoystickAxes(js) > 1 ? SDL_GetJoystickAxis(js, 1) : 0;
        const Uint8 hat = SDL_GetNumJoystickHats(js) > 0 ? SDL_GetJoystickHat(js, 0) : 0;
        const uint16_t dirs[] = {table.bits[0], table.bits[1],
                                 table.bits[2], table.bits[3]};
        const bool defaults[] = {(hat & SDL_HAT_UP) || y < -14000,
                                 (hat & SDL_HAT_DOWN) || y > 14000,
                                 (hat & SDL_HAT_LEFT) || x < -14000,
                                 (hat & SDL_HAT_RIGHT) || x > 14000};
        for (int i = 0; i < 4; ++i)
            if (rawBindings_[p][size_t(i)].type == 'n' && defaults[i]) out |= dirs[i];
        return out;
    }
    unsigned RawMenu(const BoundTable &table) const {
        unsigned out = 0;
        for (int p = 0; p < kPlayers; ++p) {
            SDL_Joystick *js = RawJoystick(p);
            if (!js || !Raw(p)) continue;
            const uint16_t buttons = ReadRaw(p, table);
            out |= buttons & table.bits[0] ? recomp::kMenuUp : 0;
            out |= buttons & table.bits[1] ? recomp::kMenuDown : 0;
            out |= buttons & table.bits[2] ? recomp::kMenuLeft : 0;
            out |= buttons & table.bits[3] ? recomp::kMenuRight : 0;
            if ((buttons & table.bits[4]) ||
                (SDL_GetNumJoystickButtons(js) > 0 && SDL_GetJoystickButton(js, 0)))
                out |= recomp::kMenuConfirm;
            if (SDL_GetNumJoystickButtons(js) > 1 && SDL_GetJoystickButton(js, 1))
                out |= recomp::kMenuBack;
        }
        return out;
    }
};

} // namespace port::xmvsf_home
