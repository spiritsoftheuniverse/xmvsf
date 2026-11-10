// Module for input using SDL
#include <SDL.h>
#include "../../../burner/sdl/xmvsf_stage13_controls.h"
#include "../../../burner/sdl/xmvsf_stage14_mode.h"
#include "../../../burner/sdl/xmvsf_stage15_machine.h"
#include "../../../burner/sdl/xmvsf_stage17_auto.h"
#include "../../../burner/sdl/xmvsf_stage18_position.h" /* XMVSF_STAGE18_POSITION */
#include "../../../burner/sdl/xmvsf_stage87_110_restart.h" // XM87.110 Start x3 restart
#include "xmvsf_stage87_137_dhc_input_history.h" // XM87.137 passive mapped-input ring
#include "../../../burner/sdl/xmvsf_stage23_ui.h" /* XMVSF_STAGE23_TRAINING_MENU */ // XMVSF_STAGE17_AUTO // XMVSF_STAGE15_MACHINE // XMVSF_STAGE14_INPUT_HANDOFF // XMVSF_STAGE13_CONTROLLERS

#include "burner.h"
#include "../../../burner/sdl/xmvsf_stage87_97_debug_console.h"
// Test-only calibration: intercept physical arrow keys in Training selection.
extern "C" int xm8774SelectorCalibrationConsumesArrows();
extern "C" void xm8770SelectInput(unsigned slot,unsigned code,int down,unsigned frame,unsigned phase);

#define JOYSTICK_DEAD_ZONE 8000		// Replace DEADZONE to make it coherent with other declarations
#define MAX_JOYSTICKS 8
// TEST 87.248: joystick enumeration changes during the SAME running game.
// The SDL device index is not a stable identity. The instance ID is.

static int FBKtoSDL[512] = { 0 };
static int SDLtoFBK[512] = { -1 };

static int nInitedSubsytems = 0;
static SDL_Joystick* JoyList[MAX_JOYSTICKS];
static SDL_GameController *GCList[MAX_JOYSTICKS];
static SDL_JoystickID xm248Instance[MAX_JOYSTICKS] = {-1,-1,-1,-1,-1,-1,-1,-1};
static SDL_JoystickID xm248ListedInstance[MAX_JOYSTICKS] = {-1,-1,-1,-1,-1,-1,-1,-1};
static Uint32 xm248LastRetryAt = 0;
static int* JoyPrevAxes = NULL;
/* static */ int nJoystickCount = 0;						// Number of joysticks connected to this machine
// The Windows boot selector has already initialized/enumerated SDL pads.
// Do not stall EVERY game transition for 600ms if only the keyboard is used.
#if defined(_WIN32)
#define JOYSTICK_DETECT_MS 0
#else
#define JOYSTICK_DETECT_MS 600
#endif
int buttons[MAX_JOYSTICKS][8] = {}; // each opened SDL slot, not a 4-pad buffer

void setup_kemaps(void)
{
	SDLtoFBK[SDL_SCANCODE_UNKNOWN] = 0;
	SDLtoFBK[SDL_SCANCODE_A] = FBK_A;
	SDLtoFBK[SDL_SCANCODE_B] = FBK_B;
	SDLtoFBK[SDL_SCANCODE_C] = FBK_C;
	SDLtoFBK[SDL_SCANCODE_D] = FBK_D;
	SDLtoFBK[SDL_SCANCODE_E] = FBK_E;
	SDLtoFBK[SDL_SCANCODE_F] = FBK_F;
	SDLtoFBK[SDL_SCANCODE_G] = FBK_G;
	SDLtoFBK[SDL_SCANCODE_H] = FBK_H;
	SDLtoFBK[SDL_SCANCODE_I] = FBK_I;
	SDLtoFBK[SDL_SCANCODE_J] = FBK_J;
	SDLtoFBK[SDL_SCANCODE_K] = FBK_K;
	SDLtoFBK[SDL_SCANCODE_L] = FBK_L;
	SDLtoFBK[SDL_SCANCODE_M] = FBK_M;
	SDLtoFBK[SDL_SCANCODE_N] = FBK_N;
	SDLtoFBK[SDL_SCANCODE_O] = FBK_O;
	SDLtoFBK[SDL_SCANCODE_P] = FBK_P;
	SDLtoFBK[SDL_SCANCODE_Q] = FBK_Q;
	SDLtoFBK[SDL_SCANCODE_R] = FBK_R;
	SDLtoFBK[SDL_SCANCODE_S] = FBK_S;
	SDLtoFBK[SDL_SCANCODE_T] = FBK_T;
	SDLtoFBK[SDL_SCANCODE_U] = FBK_U;
	SDLtoFBK[SDL_SCANCODE_V] = FBK_V;
	SDLtoFBK[SDL_SCANCODE_W] = FBK_W;
	SDLtoFBK[SDL_SCANCODE_X] = FBK_X;
	SDLtoFBK[SDL_SCANCODE_Y] = FBK_Y;
	SDLtoFBK[SDL_SCANCODE_Z] = FBK_Z;

	SDLtoFBK[SDL_SCANCODE_1] = FBK_1;
	SDLtoFBK[SDL_SCANCODE_2] = FBK_2;
	SDLtoFBK[SDL_SCANCODE_3] = FBK_3;
	SDLtoFBK[SDL_SCANCODE_4] = FBK_4;
	SDLtoFBK[SDL_SCANCODE_5] = FBK_5;
	SDLtoFBK[SDL_SCANCODE_6] = FBK_6;
	SDLtoFBK[SDL_SCANCODE_7] = FBK_7;
	SDLtoFBK[SDL_SCANCODE_8] = FBK_8;
	SDLtoFBK[SDL_SCANCODE_9] = FBK_9;
	SDLtoFBK[SDL_SCANCODE_0] = FBK_0;

	SDLtoFBK[SDL_SCANCODE_RETURN] = FBK_RETURN;
	SDLtoFBK[SDL_SCANCODE_ESCAPE] = FBK_ESCAPE;
	SDLtoFBK[SDL_SCANCODE_BACKSPACE] = FBK_BACK;
	SDLtoFBK[SDL_SCANCODE_TAB] = FBK_TAB;
	SDLtoFBK[SDL_SCANCODE_SPACE] = FBK_SPACE;
	SDLtoFBK[SDL_SCANCODE_MINUS] = FBK_MINUS;
	SDLtoFBK[SDL_SCANCODE_EQUALS] = FBK_EQUALS;
	SDLtoFBK[SDL_SCANCODE_LEFTBRACKET] = FBK_LBRACKET;
	SDLtoFBK[SDL_SCANCODE_RIGHTBRACKET] = FBK_RBRACKET;
	SDLtoFBK[SDL_SCANCODE_BACKSLASH] = FBK_BACKSLASH;
	SDLtoFBK[SDL_SCANCODE_NONUSHASH] = -1;
	SDLtoFBK[SDL_SCANCODE_SEMICOLON] = FBK_SEMICOLON;
	SDLtoFBK[SDL_SCANCODE_APOSTROPHE] = FBK_APOSTROPHE;
	SDLtoFBK[SDL_SCANCODE_GRAVE] = FBK_GRAVE;
	SDLtoFBK[SDL_SCANCODE_COMMA] = FBK_COMMA;
	SDLtoFBK[SDL_SCANCODE_PERIOD] = FBK_PERIOD;
	SDLtoFBK[SDL_SCANCODE_SLASH] = FBK_SLASH;
	SDLtoFBK[SDL_SCANCODE_CAPSLOCK] = FBK_CAPITAL;

	SDLtoFBK[SDL_SCANCODE_F1] = FBK_F1;
	SDLtoFBK[SDL_SCANCODE_F2] = FBK_F2;
	SDLtoFBK[SDL_SCANCODE_F3] = FBK_F3;
	SDLtoFBK[SDL_SCANCODE_F4] = FBK_F4;
	SDLtoFBK[SDL_SCANCODE_F5] = FBK_F5;
	SDLtoFBK[SDL_SCANCODE_F6] = FBK_F6;
	SDLtoFBK[SDL_SCANCODE_F7] = FBK_F7;
	SDLtoFBK[SDL_SCANCODE_F8] = FBK_F8;
	SDLtoFBK[SDL_SCANCODE_F9] = FBK_F9;
	SDLtoFBK[SDL_SCANCODE_F10] = FBK_F10;
	SDLtoFBK[SDL_SCANCODE_F11] = FBK_F11;
	SDLtoFBK[SDL_SCANCODE_F12] = FBK_F12;

	SDLtoFBK[SDL_SCANCODE_PRINTSCREEN] = FBK_SYSRQ;
	SDLtoFBK[SDL_SCANCODE_SCROLLLOCK] = FBK_SCROLL;
	SDLtoFBK[SDL_SCANCODE_PAUSE] = FBK_PAUSE;
	SDLtoFBK[SDL_SCANCODE_INSERT] = FBK_INSERT;
	SDLtoFBK[SDL_SCANCODE_HOME] = FBK_HOME;
	SDLtoFBK[SDL_SCANCODE_PAGEUP] = FBK_PRIOR;
	SDLtoFBK[SDL_SCANCODE_DELETE] = FBK_DELETE;
	SDLtoFBK[SDL_SCANCODE_END] = FBK_END;
	SDLtoFBK[SDL_SCANCODE_PAGEDOWN] = FBK_NEXT;
	SDLtoFBK[SDL_SCANCODE_RIGHT] = FBK_RIGHTARROW;
	SDLtoFBK[SDL_SCANCODE_LEFT] = FBK_LEFTARROW;
	SDLtoFBK[SDL_SCANCODE_DOWN] = FBK_DOWNARROW;
	SDLtoFBK[SDL_SCANCODE_UP] = FBK_UPARROW;

	SDLtoFBK[SDL_SCANCODE_NUMLOCKCLEAR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_DIVIDE] = FBK_DIVIDE;
	SDLtoFBK[SDL_SCANCODE_KP_MULTIPLY] = FBK_MULTIPLY;
	SDLtoFBK[SDL_SCANCODE_KP_MINUS] = FBK_SUBTRACT;
	SDLtoFBK[SDL_SCANCODE_KP_PLUS] = FBK_ADD;
	SDLtoFBK[SDL_SCANCODE_KP_ENTER] = FBK_NUMPADENTER;
	SDLtoFBK[SDL_SCANCODE_KP_1] = FBK_NUMPAD1;
	SDLtoFBK[SDL_SCANCODE_KP_2] = FBK_NUMPAD2;
	SDLtoFBK[SDL_SCANCODE_KP_3] = FBK_NUMPAD3;
	SDLtoFBK[SDL_SCANCODE_KP_4] = FBK_NUMPAD4;
	SDLtoFBK[SDL_SCANCODE_KP_5] = FBK_NUMPAD5;
	SDLtoFBK[SDL_SCANCODE_KP_6] = FBK_NUMPAD6;
	SDLtoFBK[SDL_SCANCODE_KP_7] = FBK_NUMPAD7;
	SDLtoFBK[SDL_SCANCODE_KP_8] = FBK_NUMPAD8;
	SDLtoFBK[SDL_SCANCODE_KP_9] = FBK_NUMPAD9;
	SDLtoFBK[SDL_SCANCODE_KP_0] = FBK_NUMPAD0;
	SDLtoFBK[SDL_SCANCODE_KP_PERIOD] = FBK_DECIMAL;

	SDLtoFBK[SDL_SCANCODE_NONUSBACKSLASH] = -1;
	SDLtoFBK[SDL_SCANCODE_APPLICATION] = -1;
	SDLtoFBK[SDL_SCANCODE_POWER] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_EQUALS] = FBK_NUMPADEQUALS;
	SDLtoFBK[SDL_SCANCODE_F13] = -1;
	SDLtoFBK[SDL_SCANCODE_F14] = -1;
	SDLtoFBK[SDL_SCANCODE_F15] = -1;
	SDLtoFBK[SDL_SCANCODE_F16] = -1;
	SDLtoFBK[SDL_SCANCODE_F17] = -1;
	SDLtoFBK[SDL_SCANCODE_F18] = -1;
	SDLtoFBK[SDL_SCANCODE_F19] = -1;
	SDLtoFBK[SDL_SCANCODE_F20] = -1;
	SDLtoFBK[SDL_SCANCODE_F21] = -1;
	SDLtoFBK[SDL_SCANCODE_F22] = -1;
	SDLtoFBK[SDL_SCANCODE_F23] = -1;
	SDLtoFBK[SDL_SCANCODE_F24] = -1;
	SDLtoFBK[SDL_SCANCODE_EXECUTE] = -1;
	SDLtoFBK[SDL_SCANCODE_HELP] = -1;
	SDLtoFBK[SDL_SCANCODE_MENU] = -1;
	SDLtoFBK[SDL_SCANCODE_SELECT] = -1;
	SDLtoFBK[SDL_SCANCODE_STOP] = -1;
	SDLtoFBK[SDL_SCANCODE_AGAIN] = -1;
	SDLtoFBK[SDL_SCANCODE_UNDO] = -1;
	SDLtoFBK[SDL_SCANCODE_CUT] = -1;
	SDLtoFBK[SDL_SCANCODE_COPY] = -1;
	SDLtoFBK[SDL_SCANCODE_PASTE] = -1;
	SDLtoFBK[SDL_SCANCODE_FIND] = -1;
	SDLtoFBK[SDL_SCANCODE_MUTE] = -1;
	SDLtoFBK[SDL_SCANCODE_VOLUMEUP] = -1;
	SDLtoFBK[SDL_SCANCODE_VOLUMEDOWN] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_COMMA] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_EQUALSAS400] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL1] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL2] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL3] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL4] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL5] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL6] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL7] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL8] = -1;
	SDLtoFBK[SDL_SCANCODE_INTERNATIONAL9] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG1] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG2] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG3] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG4] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG5] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG6] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG7] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG8] = -1;
	SDLtoFBK[SDL_SCANCODE_LANG9] = -1;
	SDLtoFBK[SDL_SCANCODE_ALTERASE] = -1;
	SDLtoFBK[SDL_SCANCODE_SYSREQ] = -1;
	SDLtoFBK[SDL_SCANCODE_CANCEL] = -1;
	SDLtoFBK[SDL_SCANCODE_CLEAR] = -1;
	SDLtoFBK[SDL_SCANCODE_PRIOR] = -1;
	SDLtoFBK[SDL_SCANCODE_RETURN2] = -1;
	SDLtoFBK[SDL_SCANCODE_SEPARATOR] = -1;
	SDLtoFBK[SDL_SCANCODE_OUT] = -1;
	SDLtoFBK[SDL_SCANCODE_OPER] = -1;
	SDLtoFBK[SDL_SCANCODE_CLEARAGAIN] = -1;
	SDLtoFBK[SDL_SCANCODE_CRSEL] = -1;
	SDLtoFBK[SDL_SCANCODE_EXSEL] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_00] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_000] = -1;
	SDLtoFBK[SDL_SCANCODE_THOUSANDSSEPARATOR] = -1;
	SDLtoFBK[SDL_SCANCODE_DECIMALSEPARATOR] = -1;
	SDLtoFBK[SDL_SCANCODE_CURRENCYUNIT] = -1;
	SDLtoFBK[SDL_SCANCODE_CURRENCYSUBUNIT] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_LEFTPAREN] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_RIGHTPAREN] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_LEFTBRACE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_RIGHTBRACE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_TAB] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_BACKSPACE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_A] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_B] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_C] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_D] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_E] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_F] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_XOR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_POWER] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_PERCENT] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_LESS] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_GREATER] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_AMPERSAND] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_DBLAMPERSAND] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_VERTICALBAR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_DBLVERTICALBAR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_COLON] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_HASH] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_SPACE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_AT] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_EXCLAM] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMSTORE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMRECALL] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMCLEAR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMADD] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMSUBTRACT] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMMULTIPLY] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_MEMDIVIDE] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_PLUSMINUS] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_CLEAR] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_CLEARENTRY] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_BINARY] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_OCTAL] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_DECIMAL] = -1;
	SDLtoFBK[SDL_SCANCODE_KP_HEXADECIMAL] = -1;
	SDLtoFBK[SDL_SCANCODE_LCTRL] = -1;
	SDLtoFBK[SDL_SCANCODE_LSHIFT] = -1;
	SDLtoFBK[SDL_SCANCODE_LALT] = -1;
	SDLtoFBK[SDL_SCANCODE_LGUI] = -1;
	SDLtoFBK[SDL_SCANCODE_RCTRL] = -1;
	SDLtoFBK[SDL_SCANCODE_RSHIFT] = -1;
	SDLtoFBK[SDL_SCANCODE_RALT] = -1;
	SDLtoFBK[SDL_SCANCODE_RGUI] = -1;
	SDLtoFBK[SDL_SCANCODE_MODE] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIONEXT] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOPREV] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOSTOP] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOPLAY] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOMUTE] = -1;
	SDLtoFBK[SDL_SCANCODE_MEDIASELECT] = -1;
	SDLtoFBK[SDL_SCANCODE_WWW] = -1;
	SDLtoFBK[SDL_SCANCODE_MAIL] = -1;
	SDLtoFBK[SDL_SCANCODE_CALCULATOR] = -1;
	SDLtoFBK[SDL_SCANCODE_COMPUTER] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_SEARCH] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_HOME] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_BACK] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_FORWARD] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_STOP] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_REFRESH] = -1;
	SDLtoFBK[SDL_SCANCODE_AC_BOOKMARKS] = -1;
	SDLtoFBK[SDL_SCANCODE_BRIGHTNESSDOWN] = -1;
	SDLtoFBK[SDL_SCANCODE_BRIGHTNESSUP] = -1;
	SDLtoFBK[SDL_SCANCODE_DISPLAYSWITCH] = -1;
	SDLtoFBK[SDL_SCANCODE_KBDILLUMTOGGLE] = -1;
	SDLtoFBK[SDL_SCANCODE_KBDILLUMDOWN] = -1;
	SDLtoFBK[SDL_SCANCODE_KBDILLUMUP] = -1;
	SDLtoFBK[SDL_SCANCODE_EJECT] = -1;
	SDLtoFBK[SDL_SCANCODE_SLEEP] = -1;
	SDLtoFBK[SDL_SCANCODE_APP1] = -1;
	SDLtoFBK[SDL_SCANCODE_APP2] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOREWIND] = -1;
	SDLtoFBK[SDL_SCANCODE_AUDIOFASTFORWARD] = -1;
};

int getFBKfromScancode(int key)
{
	return SDLtoFBK[key];
}

SDL_Scancode getScancodefromFBK(int key)
{
	return (SDL_Scancode)FBKtoSDL[key];
}

// SDL enumerated indices compact when one controller disconnects. Never
// assume JoyList[i] remains the physical device currently numbered i.
static void SDLinpCloseJoystickSlot(int i) {
    if(i<0 || i>=MAX_JOYSTICKS)return;
    if(GCList[i]){SDL_GameControllerClose(GCList[i]);GCList[i]=NULL;}
    if(JoyList[i]){SDL_JoystickClose(JoyList[i]);JoyList[i]=NULL;}
    xm248Instance[i]=-1;
    for(int j=0;j<8;++j){
        buttons[i][j]=-1;
        if(JoyPrevAxes)JoyPrevAxes[(i<<3)+j]=0;
    }
}
static int SDLinpJoystickInit(int i) {
    if(i<0 || i>=MAX_JOYSTICKS || i>=SDL_NumJoysticks())return 1;
    if(JoyList[i])SDLinpCloseJoystickSlot(i);
    SDL_Joystick* joy=SDL_JoystickOpen(i);
    if(!joy)return 1; // may be present but not ready; retry while running
    if(!SDL_JoystickGetAttached(joy)){SDL_JoystickClose(joy);return 1;}
    JoyList[i]=joy;
    xm248Instance[i]=SDL_JoystickInstanceID(joy);
    if(SDL_IsGameController(i)) GCList[i]=SDL_GameControllerOpen(i);
    const SDL_GameControllerButton names[8]={
        SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START
    };
    if(GCList[i])for(int j=0;j<8;++j) {
        const SDL_GameControllerButtonBind b=
            SDL_GameControllerGetBindForButton(GCList[i],names[j]);
        if(b.bindType==SDL_CONTROLLER_BINDTYPE_BUTTON)
            buttons[i][j]=b.value.button;
    }
    const char* deviceName=SDL_JoystickName(joy);
    fprintf(stderr,"XM87.248 PAD_OPEN slot=%d instance=%d gamecontroller=%d name=%s\n",
        i,(int)xm248Instance[i],GCList[i]?1:0,
        deviceName?deviceName:"UNKNOWN");
    return 0;
}
// Called once per emulated input frame and again before the first read.
// Enumerating instance IDs is cheap (<=8) and catches late availability,
// USB/Bluetooth reconnect, changed indices, and previously failed opens.
static void SDLinpRecheckJoysticks(bool forceRetry=false) {
    const Uint32 now=SDL_GetTicks();
    const bool retry=forceRetry || (Uint32)(now-xm248LastRetryAt)>=250u;
    if(retry)xm248LastRetryAt=now;
    const int discovered=SDL_NumJoysticks();
    const int count=discovered<=0?0:
        (discovered<MAX_JOYSTICKS?discovered:MAX_JOYSTICKS);
    for(int i=0;i<MAX_JOYSTICKS;++i){
        const SDL_JoystickID id=i<count?SDL_JoystickGetDeviceInstanceID(i):-1;
        const bool newlyListed=xm248ListedInstance[i]!=id;
        xm248ListedInstance[i]=id;
        const bool current=JoyList[i] && SDL_JoystickGetAttached(JoyList[i]) &&
            xm248Instance[i]==id && SDL_JoystickInstanceID(JoyList[i])==id;
        if(!current && (JoyList[i] || GCList[i] || xm248Instance[i]!=-1)) {
            fprintf(stderr,"XM87.248 PAD_LOST slot=%d instance=%d replacement=%d\n",
                i,(int)xm248Instance[i],(int)id);
            SDLinpCloseJoystickSlot(i);
        }
        if(i>=count || id<0)continue;
        if(!JoyList[i]) {
            // A newly enumerated device is attempted immediately. If open
            // fails while the driver initializes, retry it every 250 ms.
            if(forceRetry || retry || newlyListed)
                SDLinpJoystickInit(i);
        }else if(GCList[i] && (!SDL_GameControllerGetAttached(GCList[i]) ||
                               !SDL_IsGameController(i))) {
            SDL_GameControllerClose(GCList[i]);GCList[i]=NULL;
        }
        if(JoyList[i] && !GCList[i] && retry && SDL_IsGameController(i)) {
            // A pad that initially opened only as a raw joystick can
            // later obtain an SDL GameController mapping without restart.
            GCList[i]=SDL_GameControllerOpen(i);
            if(GCList[i])
                fprintf(stderr,"XM87.248 PAD_MAPPING_READY slot=%d instance=%d\n",
                    i,(int)xm248Instance[i]);
        }
    }
    nJoystickCount=count;
}
// Read one d-pad direction off a pad.  Returns 0 when the device isn't a
// recognised game controller (a bare joystick has no d-pad mapping to read).
static int SDLinpDPad(int i, SDL_GameControllerButton nButton)
{
	if (GCList[i] == NULL || !SDL_GameControllerGetAttached(GCList[i])) {
		return 0;
	}

	return SDL_GameControllerGetButton(GCList[i], nButton) ? 1 : 0;
}

// Set up the keyboard
static int SDLinpKeyboardInit()
{
	for (int i = 0; i < 512; i++) {
		if (SDLtoFBK[i] > 0)
			FBKtoSDL[SDLtoFBK[i]] = i;
	}

	return 0;
}

// Get an interface to the mouse
static int SDLinpMouseInit()
{
	return 0;
}

int SDLinpSetCooperativeLevel(bool bExclusive, bool /*bForeGround*/)
{
	SDL_SetRelativeMouseMode((bDrvOkay && (bExclusive || nVidFullscreen)) ? SDL_TRUE : SDL_FALSE);

	return 0;
}

int SDLinpExit()
{
    for(int i=0;i<MAX_JOYSTICKS;++i){
        SDLinpCloseJoystickSlot(i);
        xm248ListedInstance[i]=-1;
    }
    nJoystickCount=0;
    free(JoyPrevAxes);
    JoyPrevAxes=NULL;
    nInitedSubsytems=0;
    xm248LastRetryAt=0;
    return 0;
}

int SDLinpInit()
{
    setup_kemaps();
    SDLinpExit();
    const int nSize=MAX_JOYSTICKS * 8 * (int)sizeof(int);
    JoyPrevAxes=(int*)malloc(nSize);
    if(!JoyPrevAxes)return 1;
    memset(JoyPrevAxes,0,nSize);
    nInitedSubsytems=SDL_WasInit(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    if((nInitedSubsytems & (SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER)) !=
       (SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER))
        SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    // Preserve event generation for the persistent boot-menu controller
    // capture when the game returns to the same SDL process.
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);
    SDL_PumpEvents();
    SDLinpRecheckJoysticks(true);
    // Delayed availability is recovered by SDLinpStart, not a blocking
    // 600-ms cold-boot scan. This also works with no connected devices.
    SDLinpKeyboardInit();
    SDLinpMouseInit();
    return 0;
}

static unsigned char bKeyboardRead = 0;
const Uint8* SDLinpKeyboardState = NULL;

static unsigned char bJoystickRead = 0;

static unsigned char bMouseRead = 0;
static struct { unsigned char buttons; int xdelta; int ydelta; } SDLinpMouseState;

bool do_reset_game = false;		// To reset game without reloading

// Simulate F3 key pressed if reset game is requested
int SDL_KEY_IS_DOWN(int key)
{
    if(key==FBK_D && XM8797Debug::blockChordD())return 0; // Shift+D never reaches game input
    if(key==FBK_G && (SDL_GetModState()&KMOD_SHIFT))return 0; // XM87.140 Shift+G is chat UI only
	if (FBKtoSDL[key] > 0) {
		switch (key) {
			case FBK_F3:
				if (do_reset_game) {
					do_reset_game = false;
					return 1;
				} else {
					return SDLinpKeyboardState[SDL_SCANCODE_F3];
				}
			default:
				return SDLinpKeyboardState[FBKtoSDL[key]];
		}
	} else return 0;
}

// Call before checking for Input in a frame
int SDLinpStart()
{
	// Update SDL event queue
	SDL_PumpEvents();
    SDLinpRecheckJoysticks(); // hotplug / reconnect / previously failed open

	// Keyboard not read this frame
	bKeyboardRead = 0;

	// No joysticks have been read for this frame
	bJoystickRead = 0;

	// Mouse not read this frame
	bMouseRead = 0;

	return 0;
}

// Read one of the joysticks
static int ReadJoystick()
{
	if (bJoystickRead) {
		return 0;
	}

    // Always validate the current instances before dereferencing a slot.
    SDLinpRecheckJoysticks();
    XM8797Debug::state().beforeControllerPoll();
	SDL_GameControllerUpdate();		// This updates Joysticks too
    XM8797Debug::state().afterControllerPoll();

	// All joysticks have been Read this frame
	bJoystickRead = 1;

	return 0;
}

// Read one joystick axis
int SDLinpJoyAxis(int i, int nAxis)
{
	if (i < 0 || i >= nJoystickCount) {				// This joystick number isn't connected
		return 0;
	}

	if (ReadJoystick() != 0) {						// There was an error polling the joystick
		return 0;
	}

	if (!JoyList[i] || !SDL_JoystickGetAttached(JoyList[i]) ||
        nAxis<0 || nAxis >= SDL_JoystickNumAxes(JoyList[i])) {
		return 0;
	}

	return SDL_JoystickGetAxis(JoyList[i], nAxis) << 1;
}

// Read the keyboard
static int ReadKeyboard()
{
	if (bKeyboardRead) {							// already read this frame - ready to go
		return 0;
	}

	if (SDLinpKeyboardState == NULL) SDLinpKeyboardState = SDL_GetKeyboardState(NULL);	// Maybe SDLinpKeyboardState was not initialized?
	if (SDLinpKeyboardState == NULL) return 1;
	// The keyboard has been successfully Read this frame
	bKeyboardRead = 1;

	return 0;
}

static int ReadMouse()
{
	if (bMouseRead) {
		return 0;
	}

	SDLinpMouseState.buttons = SDL_GetRelativeMouseState(&(SDLinpMouseState.xdelta), &(SDLinpMouseState.ydelta));

	bMouseRead = 1;

	return 0;
}

// Read one mouse axis
int SDLinpMouseAxis(int i, int nAxis)
{
	if (i < 0 || i >= 1) {									// Only the system mouse is supported by SDL
		return 0;
	}

	switch (nAxis) {
	case 0:
		return SDLinpMouseState.xdelta;
	case 1:
		return SDLinpMouseState.ydelta;
	}

	return 0;
}

// Check a subcode (the 40xx bit in 4001, 4102 etc) for a joystick input code
static int JoystickState(int i, int nSubCode)
{
	if (i < 0 || i >= nJoystickCount) {							// This joystick isn't connected
		return 0;
	}

	if (ReadJoystick() != 0) {									// Error polling the joystick
		return 0;
	}

    if(!JoyList[i] || !SDL_JoystickGetAttached(JoyList[i]))return 0;
	if (nSubCode < 0x10) {										// Joystick directions

		// we have two checks per axis
		if (SDL_JoystickNumAxes(JoyList[i])*2 <= nSubCode) {
			return 0;
		}

		switch (nSubCode) {
		// The first stick doubles as the d-pad: on a gamepad the digital direction
		// an arcade game wants is the pad, not the stick.  Neither is exposed as an
		// SDL hat here, so ask the game controller layer for it.
		case 0x00: return (SDL_JoystickGetAxis(JoyList[i], 0) < -JOYSTICK_DEAD_ZONE)
							|| SDLinpDPad(i, SDL_CONTROLLER_BUTTON_DPAD_LEFT);			// Left
		case 0x01: return (SDL_JoystickGetAxis(JoyList[i], 0) > JOYSTICK_DEAD_ZONE)
							|| SDLinpDPad(i, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);			// Right
		case 0x02: return (SDL_JoystickGetAxis(JoyList[i], 1) < -JOYSTICK_DEAD_ZONE)
							|| SDLinpDPad(i, SDL_CONTROLLER_BUTTON_DPAD_UP);			// Up
		case 0x03: return (SDL_JoystickGetAxis(JoyList[i], 1) > JOYSTICK_DEAD_ZONE)
							|| SDLinpDPad(i, SDL_CONTROLLER_BUTTON_DPAD_DOWN);			// Down
		case 0x04: return SDL_JoystickGetAxis(JoyList[i], 2) < -JOYSTICK_DEAD_ZONE;
		case 0x05: return SDL_JoystickGetAxis(JoyList[i], 2) > JOYSTICK_DEAD_ZONE;
		case 0x06: return SDL_JoystickGetAxis(JoyList[i], 3) < -JOYSTICK_DEAD_ZONE;
		case 0x07: return SDL_JoystickGetAxis(JoyList[i], 3) > JOYSTICK_DEAD_ZONE;
		case 0x08: return SDL_JoystickGetAxis(JoyList[i], 4) < -JOYSTICK_DEAD_ZONE;
		case 0x09: return SDL_JoystickGetAxis(JoyList[i], 4) > JOYSTICK_DEAD_ZONE;
		case 0x0A: return SDL_JoystickGetAxis(JoyList[i], 5) < -JOYSTICK_DEAD_ZONE;
		case 0x0B: return SDL_JoystickGetAxis(JoyList[i], 5) > JOYSTICK_DEAD_ZONE;
		case 0x0C: return SDL_JoystickGetAxis(JoyList[i], 6) < -JOYSTICK_DEAD_ZONE;
		case 0x0D: return SDL_JoystickGetAxis(JoyList[i], 6) > JOYSTICK_DEAD_ZONE;
		case 0x0E: return SDL_JoystickGetAxis(JoyList[i], 7) < -JOYSTICK_DEAD_ZONE;
		case 0x0F: return SDL_JoystickGetAxis(JoyList[i], 7) > JOYSTICK_DEAD_ZONE;
		}
	}
	if (nSubCode < 0x20) {										// POV hat controls
		if (SDL_JoystickNumHats(JoyList[i]) <= ((nSubCode & 0x0F) >> 2)) {
			return 0;
		}

		switch (nSubCode & 3) {
		case 0:												// Left
			return SDL_JoystickGetHat(JoyList[i], (nSubCode & 0x0F) >> 2)& SDL_HAT_LEFT;
		case 1:												// Right
			return SDL_JoystickGetHat(JoyList[i], (nSubCode & 0x0F) >> 2)& SDL_HAT_RIGHT;
		case 2:												// Up
			return SDL_JoystickGetHat(JoyList[i], (nSubCode & 0x0F) >> 2)& SDL_HAT_UP;
		case 3:												// Down
			return SDL_JoystickGetHat(JoyList[i], (nSubCode & 0x0F) >> 2)& SDL_HAT_DOWN;
		}

		return 0;
	}
	if (nSubCode < 0x80) {										// Undefined
		return 0;
	}
	if (nSubCode < 0x80 + SDL_JoystickNumButtons(JoyList[i])) {	// Joystick buttons
		return SDL_JoystickGetButton(JoyList[i], nSubCode & 0x7F);
	}

	return 0;
}

// Check a subcode (the 80xx bit in 8001, 8102 etc) for a mouse input code
static int CheckMouseState(unsigned int nSubCode)
{
	switch (nSubCode & 0x7F) {
	case 0:
		return (SDLinpMouseState.buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	case 1:
		return (SDLinpMouseState.buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	case 2:
		return (SDLinpMouseState.buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	}

	return 0;
}

// Get the state (pressed = 1, not pressed = 0) of a particular input code
int SDLinpState(int nCode)
{
    extern bool xm258ArcadeMenuOpen;
    extern int xm14TrainingMode;
    extern int xm24VersusMode;
    if(xm258ArcadeMenuOpen)return 0; // Arcade menu captures all gameplay inputs.
    // After a snapshot reload no button, direction, or keyboard key may
    // leak into the new match. Stage25 provides the extra neutral dwell.
    if(XM87110::state().pending!=XM87110::NONE)return 0;
    if(XM87110::state().releaseArmed &&
       XM87110::state().gate(xm25HandoffAnyInputHeld()))return 0;
    if(!xm14TrainingMode){
        // Coin belongs to the Arcade or Versus overlay (both use the same key).
        if(nCode==SDL_SCANCODE_5 || (nCode>=0x4000 && nCode<0x4200 && (nCode&0xff)==0x86))return 0;
    }
    // XMVSF_STAGE25_7_1_INPUT_GATE: Gate ALL FBNeo game-input routes at
    // the UI -> emulation handoff, not just mapped joystick or keyboard.
    // The shared latch is inert unless a menu/snapshot transition armed it.
    // Do not reference game-mode globals from this independent translation unit.
    if (xm25InputHandoffBlock()) return 0;
    // Stage 13: route virtual P1/P2 joystick codes to saved SDL hardware.
    // ReadJoystick() already updates joystick state once per game frame.
    static XM13_Settings xm13Config;
    static bool xm13Loaded=false;
    static Uint32 xm13LastReload=0;
    // Options can change a mapping while SDL input remains initialized across
    // game->menu->game. Refresh it periodically without reopening the driver.
    const Uint32 xm13Now=SDL_GetTicks();
    if(!xm13Loaded || (Uint32)(xm13Now-xm13LastReload)>=250u){
        xm13Load(&xm13Config);
        xm13Loaded=true;
        xm13LastReload=xm13Now;
    }
    if(nCode>=0x4000 && nCode<0x4200) {
        int p=(nCode-0x4000)>>8;
        // XMVSF_STAGE14_INPUT_HANDOFF: F6 switches P1's controller from P1 to P2
        // and back during Training character selection. Arcade is untouched.
        static int xm14F6WasDown=0;
        const Uint8* xm14Keys=SDL_GetKeyboardState(NULL);
        int xm14F6=xm14Keys && xm14Keys[SDL_SCANCODE_F6];
        if (xm14TrainingMode && xm14F6 && !xm14F6WasDown) {
            xm14InputTarget=1-xm14InputTarget;
            fprintf(stderr,"XMVSF Training: P1 controller now controls %s.\n",
                    xm14InputTarget ? "P2 character selection" : "P1; P2 physical controller restored");
        }
        xm14F6WasDown=xm14F6;
        if (xm17BlockSlot(p)) return 0; // XMVSF_STAGE17_AUTO P2 waits for P1 team
        const int xm14Source=xm14ReadControllerFor(p);
        if (xm14Source<0) return xm15SyntheticButton(p,nCode&0xff);
        // The menu-selected Xbox/DS4Windows pad is a real P1 owner even
        // when the saved controller map is inactive. The old saved-only
        // check returned synthetic 0 for EVERY CPS2 control, so all menu
        // buttons worked while Arcade received absolutely nothing.
        if(XM249Policy::needsVirtualMap(
               xm13Config.player[xm14Source].active!=0,
               xm249SessionPad[xm14Source].selected)) {
            if(ReadJoystick()!=0) return 0;
            // Read-only route evidence: distinguish a saved assignment
            // not found from an enumerated-but-not-ready device. Emit only
            // on changes; no per-frame log flood.
            if(!xm13IsKeyboard(&xm13Config.player[xm14Source])){
                static int lastRoute[2]={-2,-2},lastLive[2]={-1,-1};
                static SDL_JoystickID lastInstance[2]={-2,-2};
                const int route=xm248FindAssignedDevice(&xm13Config,xm14Source);
                const bool live=route>=0 && route<nJoystickCount &&
                    JoyList[route] && SDL_JoystickGetAttached(JoyList[route]);
                const SDL_JoystickID id=live?SDL_JoystickInstanceID(JoyList[route]):-1;
                if(lastRoute[xm14Source]!=route ||
                   lastLive[xm14Source]!=(int)live ||
                   lastInstance[xm14Source]!=id){
                    fprintf(stderr,
                        "XM87.248 PAD_ROUTE saved_p=%d device=%d instance=%d ready=%d\n",
                        xm14Source+1,route,(int)id,live?1:0);
                    lastRoute[xm14Source]=route;
                    lastLive[xm14Source]=(int)live;
                    lastInstance[xm14Source]=id;
                }
            }
            if(xm249SessionPad[xm14Source].selected) {
                static SDL_JoystickID reported[2]={-1,-1};
                const int d=xm249FindSessionDevice(xm14Source);
                const SDL_JoystickID id=d>=0?SDL_JoystickGetDeviceInstanceID(d):-1;
                if(reported[xm14Source]!=id) {
                    reported[xm14Source]=id;
                    fprintf(stderr,
                        "XM87.249 PAD_GAME_OWNER p=%d device=%d instance=%d attached=%d profile=%d\n",
                        xm14Source+1,d,(int)id,
                        d>=0 && d<nJoystickCount && JoyList[d] &&
                        SDL_JoystickGetAttached(JoyList[d])?1:0,
                        xm13Config.player[xm14Source].active?1:0);
                }
            }
            const int xm15Sub=nCode&0xff;
            const int xm15Value=xm13ReadInput(&xm13Config,xm14Source,xm15Sub,JoyList,GCList,nJoystickCount);
            XM8797Debug::state().mappedInput((unsigned)p,(unsigned)xm15Sub,xm15Value);
            XM8798Latency::state().mapped((unsigned)p,(unsigned)xm15Sub,xm15Value);
            xm15ObserveInput(p,xm15Sub,xm15Value);
            // XM87.68: ROM accepted-pick count owns handoff, not input edges.
            const int xm18Left=(xm15Sub==0x87)?xm13ReadInput(&xm13Config,xm14Source,0,JoyList,GCList,nJoystickCount):0;
            const int xm18Right=(xm15Sub==0x87)?xm13ReadInput(&xm13Config,xm14Source,1,JoyList,GCList,nJoystickCount):0;
            const bool xm87110Restart=(xm15Sub==0x87 &&
                XM87110::state().observe((unsigned)xm14Source,xm15Value!=0,
                   xm18Left!=0,xm18Right!=0,(unsigned)SDL_GetTicks(),
                   xm14TrainingMode && xm18Position.combat && !xm23State.menuOpen));
            const int xm18Val=xm87110Restart?0:xm18ProcessInput(p,xm15Sub,xm15Value,xm18Left,xm18Right);
            const int xm8766Delivered=xm23FilterInput(p,xm15Sub,(xm18Val || xm15SyntheticButton(p,xm15Sub)),xm14TrainingMode);
            const int xm8768Delivered=xm17FilterTransferredConfirm(p,xm15Sub,xm8766Delivered);
            XM87137Input::state().mapped((unsigned)p,(unsigned)xm15Sub,xm8768Delivered,(unsigned)GetCurrentFrame()); // XM87.137 no input injection
            xm8770SelectInput((unsigned)p,(unsigned)xm15Sub,xm8768Delivered,(unsigned)GetCurrentFrame(),(unsigned)xm17.phase);
            XM8766::RecordInput(p,xm15Sub,xm14Source,xm8768Delivered,(unsigned)GetCurrentFrame(),xm17.phase,xm14InputTarget); // XM87.66 routed input evidence
            return xm8768Delivered; /* XMVSF_STAGE23_TRAINING_MENU */ // XMVSF_STAGE15_MACHINE
        }
        return xm15SyntheticButton(p,nCode&0xff); // XMVSF_STAGE15_MACHINE
    }

	if (nCode < 0) {
		return 0;
	}

	if (nCode < 0x100) {
		if (ReadKeyboard() != 0) {							// Check keyboard has been read - return not pressed on error
			return 0;
		}
		        if(xm8774SelectorCalibrationConsumesArrows() &&
           (nCode==FBK_LEFTARROW || nCode==FBK_RIGHTARROW ||
            nCode==FBK_UPARROW || nCode==FBK_DOWNARROW))return 0;
return SDL_KEY_IS_DOWN(nCode);							// Return key state
	}

	if (nCode < 0x4000) {
		return 0;
	}

	if (nCode < 0x8000) {
		// Codes 4000-8000 = Joysticks
		int nJoyNumber = (nCode - 0x4000) >> 8;

		// Find the joystick state in our array
		return JoystickState(nJoyNumber, nCode & 0xFF);
	}

	if (nCode < 0xC000) {
		// Codes 8000-C000 = Mouse
		if ((nCode - 0x8000) >> 8) {						// Only the system mouse is supported by SDL
			return 0;
		}
		if (ReadMouse() != 0) {								// Error polling the mouse
			return 0;
		}
		return CheckMouseState(nCode & 0xFF);
	}

	return 0;
}

// This function finds which key is pressed, and returns its code
int SDLinpFind(bool CreateBaseline)
{
	int nRetVal = -1;										// assume nothing pressed

	// check if any keyboard keys are pressed
	if (ReadKeyboard() == 0) {
		for (int i = 0; i < 0x100; i++) {
			if (SDL_KEY_IS_DOWN(i) > 0) {
				nRetVal = i;
				goto End;
			}
		}
	}

	// Now check all the connected joysticks
	for (int i = 0; i < nJoystickCount; i++) {
		int j;
		if (ReadJoystick() != 0) {							// There was an error polling the joystick
			continue;
		}

		for (j = 0; j < 0x10; j++) {						// Axes
			int nDelta = JoyPrevAxes[(i << 3) + (j >> 1)] - SDLinpJoyAxis(i, (j >> 1));
			if (nDelta < -0x4000 || nDelta > 0x4000) {
				if (JoystickState(i, j)) {
					nRetVal = 0x4000 | (i << 8) | j;
					goto End;
				}
			}
		}

		for (j = 0x10; j < 0x20; j++) {						// POV hats
			if (JoystickState(i, j)) {
				nRetVal = 0x4000 | (i << 8) | j;
				goto End;
			}
		}

		for (j = 0x80; JoyList[i] && j < 0x80 + SDL_JoystickNumButtons(JoyList[i]); j++) {
			if (JoystickState(i, j)) {
				nRetVal = 0x4000 | (i << 8) | j;
				goto End;
			}
		}
	}

	// Now the mouse
	if (ReadMouse() == 0) {
		int nDeltaX, nDeltaY;

		for (unsigned int j = 0x80; j < 0x80 + 0x80; j++) {
			if (CheckMouseState(j)) {
				nRetVal = 0x8000 | j;
				goto End;
			}
		}

		nDeltaX = SDLinpMouseAxis(0, 0);
		nDeltaY = SDLinpMouseAxis(0, 1);
		if (abs(nDeltaX) < abs(nDeltaY)) {
			if (nDeltaY != 0) {
				return 0x8000 | 1;
			}
		}
		else {
			if (nDeltaX != 0) {
				return 0x8000 | 0;
			}
		}
	}

End:

	if (CreateBaseline) {
		for (int i = 0; i < nJoystickCount; i++) {
			for (int j = 0; j < 8; j++) {
				JoyPrevAxes[(i << 3) + j] = SDLinpJoyAxis(i, j);
			}
		}
	}

	return nRetVal;
}

int SDLinpGetControlName(int nCode, TCHAR* pszDeviceName, TCHAR* pszControlName)
{
	if (pszDeviceName) {
		pszDeviceName[0] = _T('\0');
	}
	if (pszControlName) {
		pszControlName[0] = _T('\0');
	}

	switch (nCode & 0xC000) {
	case 0x0000: {
		_tcscpy(pszDeviceName, _T("System keyboard"));

		break;
	}
	case 0x4000: {
		int i = (nCode >> 8) & 0x3F;

		if (i >= nJoystickCount) {				// This joystick isn't connected
			return 0;
		}
		//	_tsprintf(pszDeviceName, "%hs", SDL_JoystickName(i));

		break;
	}
	case 0x8000: {
		int i = (nCode >> 8) & 0x3F;

		if (i >= 1) {
			return 0;
		}
		_tcscpy(pszDeviceName, _T("System mouse"));

		break;
	}
	}

	return 0;
}

// XM87.137: native DHC parser can inspect the ACTUAL forwarded mapped inputs,
// never altering the inputs the CPS-2 ROM receives. ago=0 is newest frame.
extern "C" int xm87137ReadMappedInput(unsigned p,unsigned ago,
                                     unsigned* frame,unsigned* direction,
                                     unsigned* heldButtons,unsigned* pressedButtons) {
    XM87137Input::Sample s;
    if(!XM87137Input::state().sample(p,ago,s))return 0;
    if(frame)*frame=s.frame;
    if(direction)*direction=s.direction;
    if(heldButtons)*heldButtons=s.buttons;
    if(pressedButtons)*pressedButtons=s.pressed;
    return 1;
}

struct InputInOut InputInOutSDL2 = { SDLinpInit, SDLinpExit, SDLinpSetCooperativeLevel, SDLinpStart, SDLinpState, SDLinpJoyAxis, SDLinpMouseAxis, SDLinpFind, SDLinpGetControlName, NULL, _T("SDL input") };
