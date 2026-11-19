// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The window: SDL3 picture, sound and controls around Port::RunFrame().
#include "game_info.h"

#include "bindings.h"
#include "xmvsf_home.h" // XMVSF_RECOMP_HOME_87_435
#include "xmvsf_menu_audio.h" // XMVSF_RECOMP_HOME_87_437
#include "xmvsf_window.h"
#include "xmvsf_menu_textures.h" // XMVSF_RECOMP_MENU_TEXTURES_87_441
#include "xmvsf_native_capture.h" // XMVSF_NATIVE_CAPTURE_87_458
#include "xmvsf_options.h" // XMVSF_RECOMP_OPTIONS_87_442
#include "xmvsf_pad_owners.h" // XMVSF_RECOMP_PAD_OWNER_87_443
#include "../fight/session.h"
#include "port.h"
#include "png_write.h"
#include "present.h"
#include "sound_filter.h"
#include "touch.h"

#include "../runtime/game.h"
#include "../runtime/spare.h"
#include "../recomp-common/gamepads.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace cps2recomp;

namespace port {

#ifdef __APPLE__
void HandFrontToLauncher(); // (front_macos.mm)
#endif

namespace {

    // What each player's keys and gamepad buttons do: the player's choices (bindings.h; the launcher's
    // Controllers page), or the usual layout. A game started from the collection's launcher has the table every
    // game there shares; one with a launcher of its own, its own table.
    const BoundTable &Table() {
        static const BoundTable &table = MakeBoundTable(GamePlayers(), GameButtons(), InCollection());
        return table;
    }
    Bindings g_bindings = recomp::DefaultBindings(Table().table);
    int Players() {
        return Table().table.players;
    }

    static_assert(kBoundActions[0].bit == kUp && kBoundActions[1].bit == kDown && kBoundActions[2].bit == kLeft &&
                  kBoundActions[3].bit == kRight && kBoundActions[4].bit == kPunch1 && kBoundActions[9].bit == kKick3 &&
                  kBoundActions[10].bit == kStart && kBoundActions[11].bit == kCoin &&
                  kBoundActions[12].bit == (kPunch1 | kPunch2 | kPunch3) && kBoundActions[13].bit == (kKick1 | kKick2 | kKick3));

    constexpr int kDead = 16000; // how far a stick or a trigger must go to count

    bool KeyBound(SDL_Scancode key) {
        for (int player = 0; player < Players(); player++) {
            for (int action = 0; action < Table().table.count; action++) {
                if (g_bindings.key[player][action] == key) {
                    return true;
                }
            }
        }
        return false;
    }
    bool ButtonBound(SDL_GamepadButton button) {
        return PadInputBound(g_bindings, Table(), {button, -1});
    }

    // The camera's zoom (zoom.h) is the left stick's click held and a shoulder pressed: left for player 1, right for
    // player 2, the right stick's click for both fighters at once; the d-pad (or the stick) moves the camera. While the
    // click is held (and is not the game's), the shoulders and the directions are not the game's. On the keyboard,
    // Left Shift held, Q, E and W, and the arrows.
    bool ZoomHeld(SDL_Gamepad *gp) {
        return Zoom::Possible() && gp && !ButtonBound(SDL_GAMEPAD_BUTTON_LEFT_STICK) && SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_STICK);
    }
    bool Shoulder(int button) {
        return button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER || button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    }
    bool DPad(int button) {
        return button == SDL_GAMEPAD_BUTTON_DPAD_UP || button == SDL_GAMEPAD_BUTTON_DPAD_DOWN || button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
               button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    }
    bool KeyZoomHeld(const bool *keys) {
        return Zoom::Possible() && keys[SDL_SCANCODE_LSHIFT] && !KeyBound(SDL_SCANCODE_LSHIFT);
    }
    bool KeyZoom(SDL_Scancode key) {
        return key == SDL_SCANCODE_Q || key == SDL_SCANCODE_E || key == SDL_SCANCODE_W || key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN ||
               key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT;
    }

    uint16_t ReadKeyboard(const bool *keys, int player) {
        uint16_t pad = 0;
        const bool zoomHeld = KeyZoomHeld(keys);
        for (int action = 0; action < Table().table.count; action++) {
            const SDL_Scancode key = g_bindings.key[player][action];
            if (zoomHeld && KeyZoom(key)) {
                continue;
            }
            if (key != SDL_SCANCODE_UNKNOWN && keys[key]) {
                pad |= Table().bits[action];
            }
        }
        // (Return starts player 1's game too, unless it has been given something to do)
        if (player == 0 && keys[SDL_SCANCODE_RETURN] && !KeyBound(SDL_SCANCODE_RETURN)) {
            pad |= kStart;
        }
        return pad;
    }

    uint16_t ReadGamepad(SDL_Gamepad *gp, int player) {
        uint16_t pad = 0;
        const bool zoomHeld = ZoomHeld(gp);
        for (int action = 0; action < Table().table.count; action++) {
            const PadInput input = g_bindings.pad[player][action];
            if (zoomHeld && (Shoulder(input.button) || DPad(input.button))) {
                continue;
            }
            if ((input.button >= 0 && SDL_GetGamepadButton(gp, static_cast<SDL_GamepadButton>(input.button))) ||
                (input.axis >= 0 && SDL_GetGamepadAxis(gp, static_cast<SDL_GamepadAxis>(input.axis)) > kDead)) {
                pad |= Table().bits[action];
            }
        }
        const int x = zoomHeld ? 0 : SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX);
        const int y = zoomHeld ? 0 : SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY);
        if (x < -kDead) pad |= kLeft;
        if (x > kDead) pad |= kRight;
        if (y < -kDead) pad |= kUp;
        if (y > kDead) pad |= kDown;
        return pad;
    }

    // A Multi game run's window: where the game before left it (each game is a program of its own, and its window
    // would open where the system puts a new one). The game ending for the next writes it down; the next game reads
    // it, if it was written moments ago (not a run before), and opens its window there, as tall.
    struct Placement {
        int x = 0, y = 0, height = 0;
        bool maximized = false;
    };
    std::filesystem::path PlacementFile() {
        std::error_code error;
        const std::filesystem::path folder = std::filesystem::temp_directory_path(error);
        return error ? std::filesystem::path() : folder / "cps2recomp-multi-window.txt";
    }
    void KeepPlacement(SDL_Window *window) {
        const std::filesystem::path file = PlacementFile();
        if (file.empty() || (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)) {
            return;
        }
        Placement at;
        int width = 0;
        SDL_GetWindowPosition(window, &at.x, &at.y);
        SDL_GetWindowSize(window, &width, &at.height);
        at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
        std::ofstream(file) << at.x << ' ' << at.y << ' ' << at.height << ' ' << at.maximized << '\n';
    }
    bool TakePlacement(Placement &at) {
        const std::filesystem::path file = PlacementFile();
        std::error_code error;
        if (file.empty() || !std::filesystem::exists(file, error)) {
            return false;
        }
        const auto age = std::filesystem::file_time_type::clock::now() - std::filesystem::last_write_time(file, error);
        bool fresh = !error && age < std::chrono::minutes(1);
        {
            std::ifstream in(file);
            fresh = fresh && (in >> at.x >> at.y >> at.height >> at.maximized) && at.height > 0;
        }
        std::filesystem::remove(file, error);
        return fresh;
    }

} // namespace

int RunWindowed(Port &port) {
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "256");
    if (!port.opt.settingsPath.empty()) {
        g_bindings = LoadBindings(port.opt.settingsPath, Table());
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    // (Show pictures folder, the model viewer's: the folder as a file:// address, its bytes outside the plain ones as %XX)
    SetFolderOpener([](const std::string &folder) {
        std::string url = "file://";
#ifdef _WIN32
        url += '/';
#endif
        for (const unsigned char c : folder) {
            const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-' ||
                               c == '_' || c == '.' || c == '~' || c == ':';
            if (plain) {
                url += static_cast<char>(c);
            } else if (c == '\\') {
                url += '/';
            } else {
                static const char kHex[] = "0123456789ABCDEF";
                url += '%';
                url += kHex[c >> 4];
                url += kHex[c & 15];
            }
        }
        SDL_OpenURL(url.c_str());
    });
    // (controllers SDL doesn't know, issue #213: gamecontrollerdb.txt beside the program, in the launcher's folder or in
    // the game's own; recomp-common/gamepads.h)
    {
        const char *base = SDL_GetBasePath();
        const std::string mine = port.PlayerFolder();
        recomp::AddGamepadMappings({base ? std::filesystem::u8path(base) : std::filesystem::path(), LauncherFolder(),
                                    mine.empty() ? std::filesystem::path() : std::filesystem::u8path(mine)});
    }
    // (the window: so many times the board's 224 lines, --scale's or the Display page's)
    const int scale = port.opt.scale > 0 ? port.opt.scale : WindowSize();
    // (the window's name: game.cfg's "title" where a game has one, else its name)
#ifdef GAME_TITLE
    const char *title = GAME_TITLE;
#else
    const char *title = GAME_NAME;
#endif
    // (the picture's shape: the board's 384 x 224 shown 4:3 unless the Display page says otherwise, and as
    // much wider as the widest picture the game's sources may ask for; 4:3 for a picture that fills it)
    const int columns = TheGame().widestPicture ? std::clamp(TheGame().widestPicture(), +Video::kWidth, +Video::kMaxWidth) : Video::kWidth;
    auto windowShape = [](int across) {
        // (a vertical game's picture turned upright: Turned)
        return Turned() ? PictureShape(Video::kHeight, across, DisplayShape(), 3.0f / 4.0f * Video::kWidth / across)
                        : PictureShape(across, Video::kHeight, DisplayShape(), 4.0f / 3.0f * across / Video::kWidth);
    };
    // (the window's height for so many times the board's lines: a third more for a picture stood upright, its rows the
    // board's columns)
    auto windowTall = [](int times) { return Turned() ? Video::kHeight * times * 4 / 3 : Video::kHeight * times; };
    // (in a Multi game run, a window where the game before's was: a full screen one is full screen as asked)
    Placement placed;
    const bool carried = !MultiGame().empty() && TakePlacement(placed) && !port.opt.fullscreen;
    const int windowHeight = carried ? placed.height : windowTall(scale);
    // The window, as an OpenGL window for a shader (shading.h: nothing else draws to it), `at` a place for it (`width`
    // 0: as wide as the picture's shape makes it)
    auto makeWindow = [&](bool opengl, const Placement *at, int width, int height, bool fullscreen) {
        const SDL_PropertiesID properties = SDL_CreateProperties();
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width > 0 ? width : std::lround(height * windowShape(columns)));
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
        if (at) {
            SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, at->x);
            SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, at->y);
        }
#ifdef __ANDROID__
        fullscreen = true; // (a phone's whole screen, its bars hidden)
#endif
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                              SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | (at && at->maximized ? SDL_WINDOW_MAXIMIZED : 0) |
                                  (opengl ? SDL_WINDOW_OPENGL : 0) | (getenv("CPS2RECOMP_HIDDEN_WINDOW") ? SDL_WINDOW_HIDDEN : 0)); // (hidden: for tests)
        SDL_Window *made = SDL_CreateWindowWithProperties(properties);
        SDL_DestroyProperties(properties);
        // (in front: on macOS a program started by one that is not, as the launcher is behind a Multi game run's
        // last game, opens behind the other windows)
        if (made && !getenv("CPS2RECOMP_HIDDEN_WINDOW")) {
            SDL_RaiseWindow(made);
        }
        return made;
    };
    // The picture goes to the window through SDL's GPU API, or through SDL's renderer where there is no
    // device for that (and the dummy video driver of the tests has none) or when asked: --renderer sdl. With a
    // shader preset chosen (the Display page's Shader, or --shader) it goes through the preset with OpenGL instead,
    // where librashader is beside the game and makes the preset; else as without one.
    Presenter presenter;
    std::string presentError;
    // XMVSF_RECOMP_CLASSIC_87_440: SDL3's accelerated 2D renderer,
    // not the recomp's custom GPU shader/linear sampler.
    const bool xmvsfClassic = !std::strcmp(GAME_ID, "xmvsf") &&
                              port.opt.frames == 0 && !port.net && MultiGame().empty() &&
                              std::getenv("XMVSF_RECOMP_CLASSIC_OFF") == nullptr;
    const bool wantGpu = !xmvsfClassic && port.opt.renderer != "sdl" &&
                         strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    const bool canShade = !xmvsfClassic &&
                          strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    std::string preset = canShade ? port.ShaderPreset() : "";
    int shaderValue = FeatureValue("display_shader");
    std::string shaderError;
    if (!preset.empty() && !Shading::Available(shaderError)) {
        fprintf(stderr, "shader: %s; the picture is drawn without it\n", shaderError.c_str());
        preset.clear();
    }
    SDL_Window *window = makeWindow(!preset.empty(), carried ? &placed : nullptr, 0, windowHeight, port.opt.fullscreen);
    // (opened as the preset says, or as without one when that fails)
    presenter.SetClassicPixels(xmvsfClassic);
    auto openPresenter = [&]() {
        if (!preset.empty()) {
            if (presenter.OpenShaded(window, preset, shaderError)) {
                return true;
            }
            fprintf(stderr, "shader %s: %s; the picture is drawn without it\n", preset.c_str(), shaderError.c_str());
            preset.clear();
            // (an OpenGL window is drawn to by nothing else: one that is not, where this one is)
            Placement at;
            int width = 0;
            SDL_GetWindowPosition(window, &at.x, &at.y);
            SDL_GetWindowSize(window, &width, &at.height);
            at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
            const bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
            SDL_DestroyWindow(window);
            window = makeWindow(false, &at, width, at.height, fullscreen);
            if (!window) {
                presentError = SDL_GetError();
                return false;
            }
        }
        return presenter.Open(window, wantGpu, presentError);
    };
    // SDL2 boot always used PRESENTVSYNC when accelerated.
    presenter.SetVsync(xmvsfClassic ? true : FeatureOn("display_vsync")); // (kept when it is opened again)
    if (!window || !openPresenter()) {
        fprintf(stderr, "SDL window: %s\n", window ? presentError.c_str() : SDL_GetError());
        return 1;
    }
    if (wantGpu && !presenter.UsesGpu() && !presenter.Shaded()) {
        fprintf(stderr, "no GPU device (%s): the picture goes through SDL's renderer\n", presentError.c_str());
    }
    auto sayPresenter = [&]() {
        if (presenter.Shaded() && getenv("CPS2RECOMP_SHADER_SHOT")) {
            presenter.Shader()->keepLast = true;
        }
        if (port.opt.stats) {
            fprintf(stderr, "picture: %s\n", presenter.Shaded()  ? ("OpenGL, shader " + preset).c_str()
                                             : presenter.UsesGpu() ? SDL_GetGPUDeviceDriver(presenter.Device())
                                                                   : SDL_GetRendererName(presenter.Renderer()));
        }
    };
    sayPresenter();
    if (xmvsfClassic) {
        std::fprintf(stderr,
            "XMVSF 87.440 CLASSIC_RENDER SDL3_2D nearest=true menu=960x540/letterbox vsync=true backend=%s\n",
            presenter.Renderer() ? SDL_GetRendererName(presenter.Renderer()) : "missing");
    }
    // The Display page's Shader changed in the menu: another preset in the same window, or the window made again
    // for or without OpenGL, where it is and as large
    auto followShader = [&]() {
        const int value = FeatureValue("display_shader");
        if (value == shaderValue || !port.opt.shader.empty() || !canShade) {
            return;
        }
        shaderValue = value;
        std::string next = port.ShaderPreset();
        if (!next.empty() && !Shading::Available(shaderError)) {
            fprintf(stderr, "shader: %s; the picture is drawn without it\n", shaderError.c_str());
            next.clear();
        }
        if (next == preset) {
            return;
        }
        if (!next.empty() && presenter.Shaded()) {
            if (presenter.SetPreset(next, shaderError)) {
                preset = next;
                sayPresenter();
            } else {
                fprintf(stderr, "shader %s: %s; the one before is kept\n", next.c_str(), shaderError.c_str());
            }
            return;
        }
        Placement at;
        int width = 0;
        SDL_GetWindowPosition(window, &at.x, &at.y);
        SDL_GetWindowSize(window, &width, &at.height);
        at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
        const bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
        port.gpuPicture.Close(); // (its resources are the presenter's device's)
        for (auto &g : port.guestPictures) {
            g.picture->Close();
        }
        presenter.Close();
        SDL_DestroyWindow(window);
        preset = next;
        window = makeWindow(!preset.empty(), &at, width, at.height, fullscreen);
        if (!window || !openPresenter()) {
            fprintf(stderr, "SDL window: %s\n", window ? presentError.c_str() : SDL_GetError());
            port.quit = true;
            return;
        }
        sayPresenter();
    };

    // Sound: how much is kept waiting to be played (seconds), and how much is too much. The device is asked
    // for small buffers, so that little waits there either. (CPS2RECOMP_AUDIO_LOG: what is waiting, printed
    // once a second, and nothing heard.)
    constexpr double kAudioWaiting = 0.028, kAudioMost = 0.11;
    const bool audioLog = getenv("CPS2RECOMP_AUDIO_LOG") != nullptr;
    double audioLeast = 1;
    int audioDropped = 0;
    // (--mute starts with no sound; the menu's Sound row switches it on, so the device is opened all the same)
    SDL_AudioStream *audio = nullptr;
    SoundFilter soundFilter; // (the menu's Sound filter: sound_filter.h)
    port.menu.muted = port.opt.mute;
    bool silent = false;
    {
        const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, QSound::kSampleRate};
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (audio) {
            SDL_ResumeAudioStreamDevice(audio);
            if (audioLog) {
                SDL_AudioSpec device{};
                int frames = 0;
                SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audio), &device, &frames);
                fprintf(stderr, "audio: device %d Hz, %d channels, %d frames a buffer (%s)\n", device.freq, device.channels, frames, SDL_GetCurrentAudioDriver());
            }
        } else {
            fprintf(stderr, "Audio: %s\n", SDL_GetError());
        }
    }
    // The custom soundtrack (soundtrack.h): a stream of its own, at its files' rate, opened when it first plays
    SDL_AudioStream *music = nullptr;
    SDL_AudioSpec musicSpec{SDL_AUDIO_F32, 2, 0};
    std::vector<float> musicMade;
    bool musicSilent = false;

    // XMVSF_RECOMP_HOME_87_435: our native front end is off for headless/automation runs.
    const bool homeEnabled = !std::strcmp(GAME_ID, "xmvsf") && port.opt.frames == 0 && !port.net &&
                             MultiGame().empty() && std::getenv("XMVSF_RECOMP_HOME_OFF") == nullptr;
    xmvsf_home::Menu home(homeEnabled);
    xmvsf_home::MenuAudio homeAudio(homeEnabled);
    xmvsf_home::TexturedMenu homeTextures(home);
    // Always write F6 PNGs to the repo's screenshots folder, never to
    // the EEPROM/player directory. RECOMP_RUN.bat supplies its absolute path.
    const char *f6Folder = std::getenv("XMVSF_SCREENSHOT_DIR");
    xmvsf_home::NativeCapture nativeCapture(
        f6Folder && *f6Folder ? f6Folder : "../screenshots/");
    xmvsf_home::OptionsPanel options; // XMVSF_RECOMP_OPTIONS_87_442
    xmvsf_home::PadOwners owners; // XMVSF_RECOMP_PAD_OWNER_87_443
    owners.Init(port.opt.settingsPath);
    owners.LoadSavedProfiles(g_bindings, Table());
    bool homeReturnFromOptions = false;
    std::vector<uint32_t> homePicture;
    std::vector<SDL_Gamepad *> gamepads;
    // The in-game menu: opened and closed by Esc, F1, a gamepad stick's click or its Start and Select
    // together (the game is left from its "Quit game"). While it is open the game stands still and the
    // controls work the menu; buttons held as it closes do not reach the game until let go.
    std::vector<uint32_t> menuBase, menuPicture, shown;
    uint32_t menuWidth = 0, menuHeight = 0;
    bool menuDirty = false;
    uint16_t previousPad = 0, heldAtClose = 0;
    // XMVSF_RECOMP_PAD_OWNER_87_443: one deliberate key/button press
    // per menu action. Releasing all bits rearms the corresponding input.
    unsigned previousMenuButtons = 0;
    int triggerHeld = 0;
    bool bothHeld = false; // a gamepad's Start and Select (Back) together: the menu
    // (with a zoom to work, the left stick's click opens the menu when it is let go, unless a shoulder was pressed
    // while it was held: then it was the zoom's)
    bool stickWaiting = false;
    auto toggleMenu = [&]() {
        if (port.net) {
            // (online: no menu, the game goes on for both players; Esc gives up a wait, and in a match asks whether to
            // leave it: Port::leaving)
            if (port.connecting && !port.net->Arcade()) {
                port.LeaveOnline();
            } else {
                port.leaving = !port.leaving;
            }
            return;
        }
        if (options.Active()) {
            options.Close();
            home.Open();
        } else if (port.menu.IsOpen()) {
            port.menu.Close();
        } else if (home.Enabled()) {
            home.Toggle(); // XMVSF_RECOMP_HOME_87_435: Esc / gamepad returns to the XMVSF menu.
        } else {
            port.MenuPicture(menuBase, menuWidth, menuHeight);
            port.menu.Open(true);
            menuDirty = true;
        }
    };
    // Online, the chat (Port::typing): T on the keyboard, the right stick's click or the touch screen's Chat button
    // opens a line to type, and the system's text input with it (on a Steam Deck or a phone, its own on-screen
    // keyboard); Enter sends it, Esc (or the click again) puts it away. Meanwhile the keyboard is the chat's, and after
    // it, not the game's until every key is let go (the Enter that sent it is not the game's Start).
    uint64_t openedByT = 0; // (when T opened it: its own letter, if the system sends it as text too, is not the line's)
    bool keysAfterChat = false;
    auto chatOpen = [&](bool open) {
        if (open == port.typing || (open && (!port.net || port.connecting || port.leaving))) {
            return;
        }
        port.typing = open;
        port.typed.clear();
        if (open) {
            int w = 0, h = 0;
            SDL_GetWindowSize(window, &w, &h);
            const SDL_Rect area{0, h * 3 / 4, w / 2, h / 12};
            SDL_SetTextInputArea(window, &area, 0);
            SDL_StartTextInput(window);
        } else {
            SDL_StopTextInput(window);
            keysAfterChat = true;
        }
    };
    auto quickState = [&](bool save) {
        if (port.net) {
            return; // (online: no save states)
        }
        std::string error;
        const std::string path = port.StatePath(port.menu.Slot());
        if (!(save ? port.SaveState(path, error) : port.LoadState(path, error))) {
            fprintf(stderr, "state %d: %s\n", port.menu.Slot(), error.c_str());
        }
    };
    // A Multi game run (the launcher's Multi game page): N or a gamepad's left shoulder for the next game, F or
    // the left trigger held to fast-forward (each when the player has not given it to the game)
    const bool multi = !MultiGame().empty();
    // (a touch screen's controls: touch.h)
    TouchControls touch;
    std::vector<uint32_t> touchPicture;
    // (the menu's Zoom row, as last seen: a change there is the camera's, and a zoom's press is the row's)
    int zoomRow = port.zoom.Choice();
    const double kFrameNs = 1e9 / Machine::kFramesPerSecond;
    uint64_t nextFrame = SDL_GetTicksNS();
    bool running = true;
    while (running && !port.quit && (port.opt.frames == 0 || port.frame < port.opt.frames)) {
        SDL_Event ev;
        if (port.typing && (!port.net || port.connecting)) {
            chatOpen(false); // (the match over, or waited for again)
        }
        while (SDL_PollEvent(&ev)) {
            // F6 captures the next rendered frame, regardless of home,
            // Options, in-game menu, gameplay, chat, or button assignment.
            if (ev.type == SDL_EVENT_KEY_DOWN &&
                ev.key.scancode == SDL_SCANCODE_F6) {
                if (!ev.key.repeat) nativeCapture.Request();
                continue;
            }
            if (touch.Event(ev)) {
                continue;
            }
            if (port.typing && ev.type == SDL_EVENT_TEXT_INPUT) {
                if (openedByT && SDL_GetTicks() - openedByT < 250 && (!strcmp(ev.text.text, "t") || !strcmp(ev.text.text, "T"))) {
                    openedByT = 0;
                    continue;
                }
                openedByT = 0;
                for (const char *c = ev.text.text; *c; c++) {
                    if (*c >= 32 && *c < 127 && port.typed.size() < Netplay::kChatMost) {
                        port.typed += *c;
                    }
                }
                continue;
            }
            if (port.typing && ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.scancode == SDL_SCANCODE_RETURN || ev.key.scancode == SDL_SCANCODE_KP_ENTER) {
                    port.Say();
                    chatOpen(false);
                } else if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_AC_BACK) {
                    chatOpen(false);
                } else if (ev.key.scancode == SDL_SCANCODE_BACKSPACE && !port.typed.empty()) {
                    port.typed.pop_back();
                }
                continue; // (every other key the line's, as text)
            }
            // XMVSF_RECOMP_PAD_OWNER_87_443: hotplug raw HID sticks;
            // event capture uses assigned SDL instance ID, not vector index.
            owners.Event(ev);
            // XMVSF_RECOMP_OPTIONS_87_442: capture key / pad remaps
            // before game shortcuts. Captured inputs never reach the game.
            if (options.Active() && options.HandleEvent(
                    ev, g_bindings, Table(), port.opt.settingsPath, owners)) {
                if (!options.Active()) home.Open();
                continue;
            }
            switch (ev.type) {
            case SDL_EVENT_QUIT: running = false; break;
            case SDL_EVENT_KEY_DOWN:
                if (home.Enabled() && !ev.key.repeat &&
                    xmvsf_home::WindowShortcut(window, ev.key.scancode, ev.key.mod)) {
                    break; // native window shortcut, not an arcade input
                }
                if (!port.menu.IsOpen() && !ev.key.repeat &&
                    (ev.key.scancode == SDL_SCANCODE_Q || ev.key.scancode == SDL_SCANCODE_E || ev.key.scancode == SDL_SCANCODE_W) &&
                    KeyZoomHeld(SDL_GetKeyboardState(nullptr))) {
                    if (ev.key.scancode == SDL_SCANCODE_W) {
                        port.zoom.Both();
                    } else {
                        port.zoom.Press(ev.key.scancode == SDL_SCANCODE_E ? 1 : 0);
                    }
                } else if (home.Enabled() && ev.key.scancode == SDL_SCANCODE_F1 &&
                           home.Visible() && !ev.key.repeat) {
                    // 87.442 F1 = our Options; Shift+F1 retains the separate
                    // upstream diagnostics as an explicit engineering escape.
                    home.Hide();
                    if (presenter.Renderer() && !(ev.key.mod & SDL_KMOD_SHIFT)) {
                        options.Open();
                    } else {
                        homeReturnFromOptions = true;
                        port.MenuPicture(menuBase, menuWidth, menuHeight);
                        port.menu.Open(true);
                        menuDirty = true;
                    }
                } else if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_F1 || ev.key.scancode == SDL_SCANCODE_AC_BACK ||
                           (ev.key.scancode == SDL_SCANCODE_TAB && !KeyBound(SDL_SCANCODE_TAB))) {
                    // (AC_BACK: a phone's Back)
                    toggleMenu();
                } else if (KeyBound(ev.key.scancode)) {
                    // (a key the player has given to the game does only that)
                } else if (port.net && ev.key.scancode == SDL_SCANCODE_T && !ev.key.repeat) {
                    chatOpen(true);
                    openedByT = port.typing ? SDL_GetTicks() : 0;
                } else if (multi && !port.menu.IsOpen() && ev.key.scancode == SDL_SCANCODE_N && !ev.key.repeat) {
                    port.Key('n');
                } else if (ev.key.scancode == SDL_SCANCODE_F5 || ev.key.scancode == SDL_SCANCODE_F9) {
                    // F5 saves the current slot; F9 restores it. F6 captures
                    // native screenshots; F7 changes the save slot.
                    if (!port.menu.IsOpen()) {
                        quickState(ev.key.scancode == SDL_SCANCODE_F5);
                    }
                } else if (ev.key.scancode == SDL_SCANCODE_F7) {
                    // F6 is reserved for native screenshots. Shift+F7 keeps
                    // the previous save-state slot shortcut available.
                    port.menu.SetSlot(port.menu.Slot() + ((ev.key.mod & SDL_KMOD_SHIFT) ? -1 : 1));
                } else if (!port.menu.IsOpen() && ev.key.scancode == SDL_SCANCODE_P && !ev.key.repeat) {
                    port.Key('p'); // stops the game, and starts it again
                } else if (!port.menu.IsOpen() && ev.key.scancode == SDL_SCANCODE_O) {
                    port.Key('o'); // on by one frame (held: frame after frame)
                } else if (ev.key.scancode == SDL_SCANCODE_MINUS || ev.key.scancode == SDL_SCANCODE_EQUALS) {
                    port.Key(ev.key.scancode == SDL_SCANCODE_MINUS ? '-' : '='); // slower, faster
                } else if (!port.menu.IsOpen() && ev.key.scancode == SDL_SCANCODE_BACKSPACE && !ev.key.repeat) {
                    port.Key('k', kKeyRestart);
                } else if (!port.menu.IsOpen() && ev.key.scancode == SDL_SCANCODE_BACKSLASH && !ev.key.repeat) {
                    port.Key('k', kKeyExtra);
                } else if (ev.key.scancode == SDL_SCANCODE_F12 && !ev.key.repeat && (ev.key.mod & SDL_KMOD_SHIFT)) {
                    port.SaveScene(); // (the scene export: scene.h)
                } else if (ev.key.scancode == SDL_SCANCODE_F12 && !ev.key.repeat && (ev.key.mod & SDL_KMOD_CTRL)) {
                    port.SceneClip();
                } else if (ev.key.scancode == SDL_SCANCODE_F12 && !ev.key.repeat) {
                    port.Screenshot();
                } else if (ev.key.scancode == SDL_SCANCODE_F2) {
                    port.test = !port.test; // the operator's test switch
                } else if (ev.key.scancode == SDL_SCANCODE_F11 ||
                           (ev.key.scancode == SDL_SCANCODE_RETURN && (ev.key.mod & SDL_KMOD_ALT))) {
                    SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
                }
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                if (!port.menu.IsOpen() && Shoulder(ev.gbutton.button) && ZoomHeld(SDL_GetGamepadFromID(ev.gbutton.which))) {
                    port.zoom.Press(ev.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER ? 1 : 0);
                    stickWaiting = false;
                } else if (!port.menu.IsOpen() && ev.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK &&
                           !ButtonBound(SDL_GAMEPAD_BUTTON_RIGHT_STICK) && ZoomHeld(SDL_GetGamepadFromID(ev.gbutton.which))) {
                    port.zoom.Both(); // (not the right stick's own pause)
                    stickWaiting = false;
                } else if (ButtonBound(static_cast<SDL_GamepadButton>(ev.gbutton.button))) {
                    // (a button the player has given to the game does only that)
                } else if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK && Zoom::Possible() && !port.menu.IsOpen()) {
                    stickWaiting = true;
                } else if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK || ev.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
                    toggleMenu();
                } else if (port.menu.IsOpen()) {
                    break;
                } else if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK) {
                    if (port.net) {
                        chatOpen(!port.typing);
                    } else {
                        port.Key('p');
                    }
                } else if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                    port.Key(multi ? 'n' : 'k', kKeyRestart);
                }
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (ev.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK && stickWaiting) {
                    stickWaiting = false;
                    if (!port.menu.IsOpen()) {
                        toggleMenu();
                    }
                }
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
                if (SDL_Gamepad *gp = SDL_OpenGamepad(ev.gdevice.which)) {
                    gamepads.push_back(gp);
                }
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                for (auto it = gamepads.begin(); it != gamepads.end(); ++it) {
                    if (SDL_GetGamepadID(*it) == ev.gdevice.which) {
                        SDL_CloseGamepad(*it);
                        gamepads.erase(it);
                        break;
                    }
                }
                break;
            default: break;
            }
        }

        if (touch.TakeMenuPress()) {
            toggleMenu();
        }
        port.PumpHost(); // (a match hosted from the menu, listened for, the menu open or not)
        touch.Chat(port.net != nullptr); // (online, from the start or a match hosted from the menu)
        if (touch.TakeZoomPress()) {
            if (port.net) {
                chatOpen(!port.typing);
            } else {
                port.zoom.Both();
            }
        }
        if (Zoom::Possible()) {
            const int asked = FeatureValue("zoom");
            if (asked != zoomRow) {
                port.zoom.Choose(asked);
            }
            zoomRow = port.zoom.Choice();
            if (asked != zoomRow) {
                SetFeatureForNow("zoom", zoomRow);
                menuDirty = true;
            }
        }
        // XMVSF_RECOMP_PAD_OWNER_87_443: resolve identities once per
        // frame. Live instance pins P1/P2 against SDL hotplug reordering.
        owners.Refresh(gamepads, g_bindings, Table());
        if (port.opt.moviePlay.empty() && !port.opt.autoPlay) {
            // The keyboard is every player's (each has the keys chosen for it); the first gamepad is player
            // 1's, a second player 2's, and so on
            uint16_t pads[Machine::kMaxPlayers] = {};
            int keyCount = 0;
            const bool *keys = SDL_GetKeyboardState(&keyCount);
            if (keysAfterChat) {
                keysAfterChat = std::any_of(keys, keys + keyCount, [](bool down) { return down; });
            }
            if (!(SDL_GetModState() & SDL_KMOD_ALT) && !port.typing && !keysAfterChat) {
                for (int player = 0; player < Players(); player++) {
                    // Stage13: a controller-owned player only
                    // receives explicitly bound physical/K actions,
                    // not the recomp's default keyboard at the same time.
                    if (player >= 2 || !owners.Physical(player))
                        pads[player] |= ReadKeyboard(keys, player);
                }
                port.service = keys[SDL_SCANCODE_9] && !KeyBound(SDL_SCANCODE_9);
            }
            bool hurry = multi && keys[SDL_SCANCODE_F] && !KeyBound(SDL_SCANCODE_F);
            // (the camera moved by the arrows with Left Shift held, and by each gamepad's d-pad or stick with its click held)
            if (!port.menu.IsOpen()) {
                int across = 0, down = 0;
                if (KeyZoomHeld(keys) && !(SDL_GetModState() & SDL_KMOD_ALT)) {
                    across += keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT];
                    down += keys[SDL_SCANCODE_DOWN] - keys[SDL_SCANCODE_UP];
                }
                for (SDL_Gamepad *gp : gamepads) {
                    if (ZoomHeld(gp)) {
                        const int x = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX), y = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY);
                        const int right = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || x > kDead;
                        const int left = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT) || x < -kDead;
                        const int lower = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN) || y > kDead;
                        const int higher = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP) || y < -kDead;
                        across += right - left;
                        down += lower - higher;
                        // (the click was the camera's, not the menu's, once it has been moved with it)
                        if (right || left || lower || higher) {
                            stickWaiting = false;
                        }
                    }
                }
                port.zoom.Pan(std::clamp(across, -1, 1), std::clamp(down, -1, 1));
            }
            pads[0] |= touch.Read(port.menu.IsOpen());
            bool trigger = false, both = false;
            const bool stepOnTrigger = !PadInputBound(g_bindings, Table(), {-1, SDL_GAMEPAD_AXIS_LEFT_TRIGGER});
            for (int p = 0; p < Players(); ++p) {
                // Saved P1/P2 identities are authoritative. P3/P4 retain
                // remaining unclaimed gamepads for the recomp's co-op modes.
                SDL_Gamepad *gp = p < 2 ? owners.Gamepad(p) : owners.AdditionalGamepad(p - 2);
                if (p < 2 && (owners.Physical(p) || owners.Raw(p)))
                    pads[p] |= owners.ReadRaw(p, Table());
                if (!gp) continue;
                // Once a physical device is explicitly assigned, do NOT
                // also apply its generic SDL semantic pad mapping.
                if (p >= 2 || !owners.Physical(p))
                    pads[p] |= ReadGamepad(gp, p);
                const bool left = stepOnTrigger && SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > kDead;
                trigger = trigger || left;
                hurry = hurry || (multi && left);
                if (left && !port.paused && !multi) pads[p] |= kTag;
                both = both || (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START) &&
                                SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_BACK));
            }
            // The left trigger goes on by one frame (held: frame after frame, after a moment), once the game
            // has been stopped: it does not stop it, so a touch of it while playing does nothing
            port.hurry = hurry && !port.menu.IsOpen();
            triggerHeld = trigger && port.paused ? triggerHeld + 1 : 0;
            if (!port.menu.IsOpen() && (triggerHeld == 1 || (triggerHeld > 24 && triggerHeld % 4 == 0))) {
                port.Key('o');
            }
            uint16_t all = 0;
            for (const uint16_t one : pads) {
                all |= one;
            }
            uint16_t pressed = all & ~previousPad;
            previousPad = all;
            // Start and Select held together open and close the menu, whatever they are bound to (the second
            // of them to go down is not also a press in the menu; neither reaches the game until let go)
            if (both && !bothHeld) {
                toggleMenu();
                pressed = 0;
                heldAtClose = previousPad;
            }
            bothHeld = both;
            // The menu's buttons are the same whatever the game's are set to: the arrows, the d-pad and the
            // left stick (and the game's own directions); Space, Enter and A choose; Backspace and B go back
            unsigned menuNow = 0;
            menuNow |= (previousPad & kUp) || keys[SDL_SCANCODE_UP] ? recomp::kMenuUp : 0;
            menuNow |= (previousPad & kDown) || keys[SDL_SCANCODE_DOWN] ? recomp::kMenuDown : 0;
            menuNow |= (previousPad & kLeft) || keys[SDL_SCANCODE_LEFT] ? recomp::kMenuLeft : 0;
            menuNow |= (previousPad & kRight) || keys[SDL_SCANCODE_RIGHT] ? recomp::kMenuRight : 0;
            menuNow |= keys[SDL_SCANCODE_SPACE] || (keys[SDL_SCANCODE_RETURN] && !(SDL_GetModState() & SDL_KMOD_ALT)) ? recomp::kMenuConfirm : 0;
            menuNow |= keys[SDL_SCANCODE_BACKSPACE] ? recomp::kMenuBack : 0;
            for (SDL_Gamepad *gp : gamepads) {
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP) ? recomp::kMenuUp : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN) ? recomp::kMenuDown : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT) ? recomp::kMenuLeft : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ? recomp::kMenuRight : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH) ? recomp::kMenuConfirm : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST) ? recomp::kMenuBack : 0;
            }
            menuNow |= owners.RawMenu(Table());
            menuNow |= touch.Menu();
            // 87.443: no auto-repeat in any menu. Key/button must be
            // released before another action, irrespective of frame rate.
            unsigned menuPressed = menuNow & ~previousMenuButtons;
            previousMenuButtons = menuNow;
            if (both) {
                menuPressed = 0;
            }
            if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: main choices in the native SDL3 loop
                const int oldMenuRow = home.Selection();
                const xmvsf_home::Action choice = home.Press(menuPressed);
                if (home.Selection() != oldMenuRow) homeAudio.Move();
                if (choice != xmvsf_home::Action::None) homeAudio.Select();
                switch (choice) {
                case xmvsf_home::Action::Arcade:
                    EndSessionFeatures();
                    port.machine->Reset();
                    home.Hide();
                    break;
                case xmvsf_home::Action::Training:
                    session::StartSession(*port.machine);
                    home.Hide();
                    break;
                case xmvsf_home::Action::Versus:
                    // First slice: original two-player local arcade with free play;
                    // both players press Start. Full selection lifecycle comes next.
                    EndSessionFeatures();
                    SetFeature("free_play", true);
                    port.machine->Reset();
                    home.Hide();
                    break;
                case xmvsf_home::Action::Options:
                    home.Hide();
                    if (presenter.Renderer()) {
                        options.Open(); // XMVSF_RECOMP_OPTIONS_87_442
                    } else {
                        // GPU/headless diagnostic fallback only. Our ordinary
                        // SDL3 2D path always uses the real custom Options.
                        homeReturnFromOptions = true;
                        port.MenuPicture(menuBase, menuWidth, menuHeight);
                        port.menu.Open(true);
                        menuDirty = true;
                    }
                    break;
                case xmvsf_home::Action::None: break;
                }
                heldAtClose = previousPad;
            } else if (options.Active()) {
                const int optionAction = options.Press(
                    menuPressed, window, g_bindings, Table(),
                    port.opt.settingsPath, owners);
                if (optionAction == 1) homeAudio.Move();
                if (optionAction == 2 || optionAction == 3) homeAudio.Select();
                if (!options.Active()) home.Open();
                heldAtClose = previousPad;
            } else if (port.menu.IsOpen()) {
                if (menuPressed) {
                    port.MenuAct(port.menu.Press(menuPressed));
                    menuDirty = true;
                } else if (pressed) {
                    port.menu.Note(""); // (a press of something else still takes the note away)
                    menuDirty = true;
                }
                if (!port.menu.IsOpen()) {
                    heldAtClose = previousPad;
                }
            } else if (port.leaving) {
                // (online, Esc's overlay: the host's mode, this player's fighters, leaving: Port::OnlineMenu; this side's
                // player stands still meanwhile)
                if (menuPressed) {
                    port.OnlineMenu(menuPressed);
                }
                heldAtClose = previousPad;
            }
            if (homeReturnFromOptions && !port.menu.IsOpen()) {
                homeReturnFromOptions = false;
                home.Open();
            }
            heldAtClose &= previousPad;
            for (int player = 0; player < Machine::kMaxPlayers; player++) {
                port.pad[player] = pads[player] & ~heldAtClose;
            }
        }

        // The window takes the picture's shape again when that changes (widescreen, or the Display page's
        // shape), and its size when the Display page's size does: a window only, not full screen or maximized
        {
            static int shownColumns = columns, shownSize = scale;
            static Shape shownShape = DisplayShape();
            static bool shownTurned = Turned();
            const int now = port.FramedColumns() ? port.FramedColumns()
                            : TheGame().widestPicture ? std::clamp(TheGame().widestPicture(), +Video::kWidth, +Video::kMaxWidth) : columns;
            const int size = port.opt.scale > 0 ? port.opt.scale : WindowSize();
            if (now != shownColumns || DisplayShape() != shownShape || size != shownSize || Turned() != shownTurned) {
                if (!(SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED))) {
                    int windowWidth = 0, windowHeight = 0;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    if (size != shownSize || Turned() != shownTurned) {
                        windowHeight = windowTall(size);
                    }
                    SDL_SetWindowSize(window, static_cast<int>(std::lround(windowHeight * windowShape(now))), windowHeight);
                }
                shownColumns = now;
                shownShape = DisplayShape();
                shownSize = size;
                shownTurned = Turned();
            }
        }
        followShader();
        presenter.SetVsync(xmvsfClassic ? true : FeatureOn("display_vsync")); // 87.440 SDL2-style VSync
        if (port.quit) {
            break;
        }
        // The picture in the window as the Display page says (present.h)
        int width = 0, height = 0;
        presenter.Size(width, height);
        presenter.SetScaling(DisplayScaling());
        // (a vertical game's picture turned upright: as many columns as the board's lines, as many rows as its columns;
        // the menu stays as it is, over the turned picture in its middle: Port::MenuPicture)
        const bool turned = Turned();
        // (a framed picture, Port::Framed: in the shape of its columns)
        const int shapeColumns = port.FramedColumns() ? port.FramedColumns() : port.ShownWidth();
        const SDL_FRect where = turned ? FitPicture(width, height, Video::kHeight, port.ShownWidth(), DisplayShape(), DisplayScaling())
                                       : FitPicture(width, height, shapeColumns, Video::kHeight, DisplayShape(), DisplayScaling());
        const SDL_FRect menuWhere = turned ? FitPicture(width, height, port.ShownWidth(), Video::kHeight, DisplayShape(), DisplayScaling()) : where;
        touch.Layout(width, height, where);
        int touchWidth = 0, touchHeight = 0;
        if (touch.TakePicture(touchPicture, touchWidth, touchHeight)) {
            presenter.SetOverlay(touchPicture.empty() ? nullptr : touchPicture.data(), touchWidth, touchHeight);
        }

        homeAudio.Step(home.Visible() || options.Active()); // XMVSF_RECOMP_OPTIONS_87_442
        if (options.Active()) { // XMVSF_RECOMP_OPTIONS_87_442: three native tabs
            if (presenter.Renderer()) {
                presenter.UseClassicMenuCanvas(true);
                const uint64_t menuTick = SDL_GetTicks();
                if (nativeCapture.Pending()) {
                    nativeCapture.SaveMenu(presenter.Renderer(), "options", [&]() {
                        return homeTextures.RenderOptions(
                            presenter.Renderer(), menuTick,
                            options, g_bindings, Table(), owners, false);
                    });
                }
                homeTextures.RenderOptions(presenter.Renderer(), menuTick,
                                           options, g_bindings, Table(), owners);
            } else if (nativeCapture.Pending()) {
                nativeCapture.Consume();
                std::fprintf(stderr, "F6 native capture: Options renderer unavailable\n");
            }
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.
            // XMVSF_RECOMP_MENU_TEXTURES_87_441: actual SDL texture layers,
            // matching our original SDL2 frontend. No CPU full-menu blending.
            if (homeTextures.Renderable(presenter.Renderer(), xmvsfClassic)) {
                presenter.UseClassicMenuCanvas(true);
                const uint64_t menuTick = SDL_GetTicks();
                if (nativeCapture.Pending()) {
                    nativeCapture.SaveMenu(presenter.Renderer(), "home", [&]() {
                        return homeTextures.Render(presenter.Renderer(), menuTick, false);
                    });
                }
                if (!homeTextures.Render(presenter.Renderer(), menuTick)) {
                    home.Draw(homePicture, SDL_GetTicks());
                    if (nativeCapture.Pending())
                        nativeCapture.SavePixels("home-fallback", homePicture.data(), 960, 540);
                    const float fallbackScale = std::max(0.001f, std::min(
                        float(width) / 960.0f, float(height) / 540.0f));
                    const SDL_FRect fallbackRect{
                        (float(width) - 960.0f * fallbackScale) * 0.5f,
                        (float(height) - 540.0f * fallbackScale) * 0.5f,
                        960.0f * fallbackScale, 540.0f * fallbackScale};
                    presenter.Show(homePicture.data(), 960, 540, fallbackRect, nullptr, false);
                }
            } else {
                home.Draw(homePicture, SDL_GetTicks());
                if (nativeCapture.Pending())
                    nativeCapture.SavePixels("home-fallback", homePicture.data(), 960, 540);
                const float fallbackScale = std::max(0.001f, std::min(
                    float(width) / 960.0f, float(height) / 540.0f));
                const SDL_FRect fallbackRect{
                    (float(width) - 960.0f * fallbackScale) * 0.5f,
                    (float(height) - 540.0f * fallbackScale) * 0.5f,
                    960.0f * fallbackScale, 540.0f * fallbackScale};
                presenter.Show(homePicture.data(), 960, 540, fallbackRect, nullptr, false);
            }
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
        if (port.menu.IsOpen()) {
            // The game's picture as it was, with the menu over it
            if (menuDirty) {
                menuDirty = false;
                menuPicture = menuBase;
                port.menu.Draw(menuPicture.data(), static_cast<int>(menuWidth), static_cast<int>(menuHeight));
            }
            if (nativeCapture.Pending())
                nativeCapture.SavePixels("ingame-menu", menuPicture.data(),
                                         static_cast<int>(menuWidth), static_cast<int>(menuHeight));
            // (not through a shader: the menu is drawn three times the board's size, and is not the game's picture)
            presenter.Show(menuPicture.data(), static_cast<int>(menuWidth), static_cast<int>(menuHeight), menuWhere, nullptr, false);
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }

        // (the game's time to the screen's: under 1 slowed, over 1 faster than the board)
        const auto rate = port.menu.GameRate();
        const double pace = port.paused ? 1.0 : static_cast<double>(rate.frames) / rate.ticks;
        const bool ran = port.Tick();
        if (nativeCapture.Pending()) {
            // One native game-frame screenshot, at its actual source pixel
            // width and 224 rows. F12 continues to use Port::Screenshot().
            std::vector<uint32_t> nativeGamePixels;
            port.Shown(nativeGamePixels);
            nativeCapture.SavePixels("gameplay", nativeGamePixels.data(),
                                     port.ShownWidth(), Video::kHeight);
        }

        if (audio && silent != (port.menu.muted || audioLog)) {
            silent = port.menu.muted || audioLog;
            SDL_SetAudioStreamGain(audio, silent ? 0.0f : 1.0f);
        }
        std::vector<int16_t> &made = port.machine->audio;
        if (audio && ran && !made.empty()) {
            // The game makes sound at its own pace and the sound device takes it at its own, and the two
            // clocks never quite agree. So the sound is played a touch faster when it is piling up and a
            // touch slower when it is running out (a fraction of a percent, far too little to hear), which
            // keeps a frame or two of it waiting: no gaps, no growing delay.
            // (Slowed, the sound is played slower by as much; faster, faster.)
            double waiting = SDL_GetAudioStreamQueued(audio) / (4.0 * QSound::kSampleRate) / pace;
            // (When much more than that is waiting (the game caught up after being held up, say) what waits
            // is thrown away: a click once, rather than sound that stays late for the seconds it would take
            // to play it off.)
            if (waiting > kAudioMost) {
                SDL_ClearAudioStream(audio);
                waiting = 0;
                audioDropped++;
            }
            const double ratio = (1.0 + std::clamp((waiting - kAudioWaiting) * 0.5, -0.02, 0.02)) * pace;
            SDL_SetAudioStreamFrequencyRatio(audio, static_cast<float>(ratio));
            audioLeast = std::min(audioLeast, waiting);
            if (audioLog && port.frame % 60 == 0) {
                fprintf(stderr, "audio: frame %llu, %.0f ms waiting (least %.0f), ratio %.4f, thrown away %d times\n",
                        static_cast<unsigned long long>(port.frame), waiting * 1000, audioLeast * 1000, ratio, audioDropped);
                audioLeast = 1;
            }
            soundFilter.Apply(made.data(), made.size() / 2, FeatureValue("sound_filter"), QSound::kSampleRate);
            SDL_PutAudioStreamData(audio, made.data(), static_cast<int>(made.size() * sizeof(int16_t)));
        }
        made.clear();
        if (ran && port.soundtrackOn && port.soundtrack.Any()) {
            // (fed as the game's sound is: the game's time's worth each frame, kept a frame or two ahead of the device)
            port.soundtrack.Make(port.machine->soundtrack.state, !FastForward() && !port.hurry, musicMade);
            if (!musicMade.empty()) {
                const int rate = port.soundtrack.Rate();
                if (!music) {
                    musicSpec.freq = rate;
                    music = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &musicSpec, nullptr, nullptr);
                    if (music) {
                        SDL_ResumeAudioStreamDevice(music);
                    } else {
                        fprintf(stderr, "Audio (soundtrack): %s\n", SDL_GetError());
                    }
                } else if (rate != musicSpec.freq) {
                    musicSpec.freq = rate;
                    SDL_SetAudioStreamFormat(music, &musicSpec, nullptr);
                }
            }
            if (music && !musicMade.empty()) {
                if (musicSilent != (port.menu.muted || audioLog)) {
                    musicSilent = port.menu.muted || audioLog;
                    SDL_SetAudioStreamGain(music, musicSilent ? 0.0f : 1.0f);
                }
                double waiting = SDL_GetAudioStreamQueued(music) / (8.0 * musicSpec.freq) / pace;
                if (waiting > kAudioMost) {
                    SDL_ClearAudioStream(music);
                    waiting = 0;
                }
                const double ratio = (1.0 + std::clamp((waiting - kAudioWaiting) * 0.5, -0.02, 0.02)) * pace;
                SDL_SetAudioStreamFrequencyRatio(music, static_cast<float>(ratio));
                SDL_PutAudioStreamData(music, musicMade.data(), static_cast<int>(musicMade.size() * sizeof(float)));
            }
        }

        // (zoomed: the part of the picture the camera looks at, the readout's panels as they are)
        Zoomed zoomed;
        // (--video gpu: the game's picture drawn on the GPU from the native records, with the readouts over it, when the
        // presenter draws with SDL_GPU and the GPU can draw the frame: gpu_picture.h; else the CPU's picture)
        bool drawn = false;
        if (port.gpuWanted && presenter.UsesGpu() && !presenter.Shaded() && !turned && !port.FramedColumns()) {
            if (port.gpuPicture.Device() != presenter.Device()) {
                port.gpuPicture.Close();
                for (auto &g : port.guestPictures) {
                    g.picture->Close();
                }
                std::string gpuError;
                bool guestsOpen = true;
                for (auto &g : port.guestPictures) {
                    guestsOpen = guestsOpen && g.picture->Open(presenter.Device(), g.roms->graphics.data(), g.roms->graphics.size(), gpuError);
                }
                if (!guestsOpen ||
                    !port.gpuPicture.Open(presenter.Device(), port.roms.graphics.data(), port.roms.graphics.size(), gpuError) ||
                    (!port.extraRoms.graphics.empty() &&
                     !port.gpuPicture.SetExtraGraphics(port.extraRoms.graphics.data(), port.extraRoms.graphics.size(), gpuError))) {
                    fprintf(stderr, "--video gpu: %s; the CPU's picture is shown\n", gpuError.c_str());
                    port.gpuWanted = false;
                }
            }
            // (not with the panel drawn apart, which the GPU's picture does not do: zoom.h, by distance)
            if (port.gpuWanted && port.gpuPicture.CanDrawKept() && port.gpuPicture.KeptWidth() == port.ShownWidth() &&
                !cps2recomp::PictureMade() && !port.zoom.KeepsPanel()) {
                static std::vector<uint32_t> readouts;
                const bool isZoomed = port.Windowed(readouts, zoomed, true);
                drawn = presenter.ShowDrawn(
                    [&](SDL_GPUCommandBuffer *commands) { return port.gpuPicture.DrawKept(commands, readouts.data(), isZoomed); },
                    port.ShownWidth(), Video::kHeight, where, isZoomed ? &zoomed : nullptr);
            }
        }
        if (!drawn && turned) {
            // (the camera's zoom is a fighting game's: not with the picture turned)
            static std::vector<uint32_t> upright;
            port.Windowed(shown, zoomed);
            TurnPicture(shown.data(), port.ShownWidth(), upright);
            presenter.Show(upright.data(), Video::kHeight, port.ShownWidth(), where, nullptr);
        } else if (!drawn) {
            const bool isZoomed = port.Windowed(shown, zoomed);
            if (zoomed.columns) {
                presenter.Show(shown.data(), zoomed.width, zoomed.height, where, nullptr); // (framed: Port::Framed)
            } else {
                presenter.Show(shown.data(), port.ShownWidth(), Video::kHeight, where, isZoomed ? &zoomed : nullptr);
            }
        }

        if (!port.opt.unthrottled && !FastForward()) {
            nextFrame += static_cast<uint64_t>(kFrameNs);
            const uint64_t now = SDL_GetTicksNS();
            if (nextFrame > now) {
                SDL_DelayPrecise(nextFrame - now);
            } else if (now - nextFrame > 100000000ull) {
                nextFrame = now; // (fell far behind: start counting again)
            }
        }
    }
    // (for the checks: what a shaded window showed last, CPS2RECOMP_SHADER_SHOT=FILE.png)
    if (const char *shot = getenv("CPS2RECOMP_SHADER_SHOT"); shot && presenter.Shaded()) {
        std::vector<uint32_t> pixels;
        int width = 0, height = 0;
        if (presenter.Shader()->ReadBack(pixels, width, height)) {
            WritePng(shot, pixels.data(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }
    for (SDL_Gamepad *gp : gamepads) {
        SDL_CloseGamepad(gp);
    }
    if (audio) {
        SDL_DestroyAudioStream(audio);
    }
    if (music) {
        SDL_DestroyAudioStream(music);
    }
    // (on to a Multi game run's next game: its window where this one is; on macOS the launcher starts it, and must
    // be in front for it to come forward)
    if (ProgramEnded() == kNextGame && !MultiGame().empty()) {
        KeepPlacement(window);
#ifdef __APPLE__
        HandFrontToLauncher();
#endif
    }
    port.gpuPicture.Close();
    for (auto &g : port.guestPictures) {
        g.picture->Close();
    }
    // XMVSF_RECOMP_MENU_TEXTURES_87_441: release SDL objects before
    // upstream closes the renderer/window and calls SDL_Quit.
    homeTextures.Release();
    homeAudio.Close();
    owners.Close(); // 87.443 raw SDL_Joystick resources before SDL_Quit.
    presenter.Close();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace port
