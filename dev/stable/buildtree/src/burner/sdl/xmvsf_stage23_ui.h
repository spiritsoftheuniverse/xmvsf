#ifndef XMVSF_STAGE23_TRAINING_UI_H
#define XMVSF_STAGE23_TRAINING_UI_H
// Stage 23: UI state and SDL presentation only. No ROM writes here.
// Grounded in peon2/fbneo-training-mode XMVSF & Marvel hitbox profiles.
#include <SDL.h>
#include "xmvsf_menu_glyph_atlas.h" /* Native SF2 small menu glyphs */
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include "xmvsf_stage13_controls.h"
#include "xmvsf_stage87_105_training_hp.h"
#include "xmvsf_stage87_153_training_options.h" // 87.153 persisted Training menu choices
#include "xmvsf_stage14_mode.h" // XM86.72 training player-select uses existing input ownership
#include "xmvsf_stage30_frame_data.h" /* XMVSF_STAGE30_FRAME_DATA */
#include "xmvsf_stage31_external_pane.h" /* XMVSF_STAGE31_EXTERNAL_PANE */
extern SDL_Window* sdlWindow;
extern void AdjustImageSize();

struct XM23Box { int x,y,w,h,kind,owner; unsigned sourceBase=0; };
struct XM23State {
    bool menuOpen=false, hitboxes=false, dataDisplay=false, infiniteMeter=false;
    bool dizzyAllowed=true; // XM87.37: ON = normal stun; OFF = prevent stun accumulation in Training
    bool combat=false, loaded=false, returnMainRequested=false;
    bool restoreSavedPlayerSelect=false;
    unsigned savedPlayerSelect=0u; // applied only AFTER character selection ends
    int selected=0, count=0;
    unsigned char pressed[2][12]={{0}};
    int health[2]={0,0}, meter[2]={0,0}, combo[2]={0,0};
    XM23Box box[192];
    XM30FrameData frameData;
};
extern XM23State xm23State;
extern int xm87102DamageMode; // 0: Regenerate 1: KO Enabled

// User's nine Training OPTIONS only. Match save states, actor inputs,
// stage, menu row, and Return/Main Menu are intentionally NOT preferences.
static inline XM87153Training::Options xm257TrainingOptionsSnapshot(){
    XM87153Training::Options p;
    p.hitboxes=xm23State.hitboxes?1u:0u;
    p.displayData=xm23State.dataDisplay?1u:0u;
    p.infiniteMeter=xm23State.infiniteMeter?1u:0u;
    p.dizzyAllowed=xm23State.dizzyAllowed?1u:0u;
    p.damageMode=xm87102DamageMode==1?1u:0u;
    p.dummy1HP=XM87105HP::state().tick[0];
    p.dummy2HP=XM87105HP::state().tick[1];
    p.playerSelect=xm14InputTarget==1?1u:0u;
    p.frameData=xm23State.frameData.enabled?1u:0u;
    return p;
}
static inline void xm257SaveTrainingOptions(){
    if(!xm14TrainingMode)return;
    const XM87153Training::Options settings=xm257TrainingOptionsSnapshot();
    if(!XM87153Training::save(XM87153Training::PATH,settings))
        std::fprintf(stderr,
            "XM87.153 TRAINING_OPTIONS_SAVE_FAILED path=%s (previous file preserved)\n",
            XM87153Training::PATH);
    else std::fprintf(stderr,"XM87.153 TRAINING_OPTIONS_SAVED path=%s\n",
                      XM87153Training::PATH);
}
static inline void xm257LoadTrainingOptions(){
    if(!xm14TrainingMode)return;
    XM87153Training::Options settings;
    if(!XM87153Training::load(XM87153Training::PATH,settings)){
        std::fprintf(stderr,
            "XM87.153 TRAINING_OPTIONS_DEFAULTS no valid settings at %s\n",
            XM87153Training::PATH);
        return;
    }
    xm23State.hitboxes=settings.hitboxes!=0u;
    xm23State.dataDisplay=settings.displayData!=0u;
    xm23State.infiniteMeter=settings.infiniteMeter!=0u;
    xm23State.dizzyAllowed=settings.dizzyAllowed!=0u;
    xm87102DamageMode=(int)settings.damageMode;
    XM87105HP::state().tick[0]=settings.dummy1HP;
    XM87105HP::state().tick[1]=settings.dummy2HP;
    XM87105HP::state().queueAll();
    // Stage17 must own P1->P2 handoff during character selection. That stage
    // resets xm14InputTarget as part of its legitimate two-team selection.
    // Apply remembered Training PLAYER SELECT only after combat begins.
    xm23State.savedPlayerSelect=settings.playerSelect;
    xm23State.restoreSavedPlayerSelect=true;
    // Frame Data's event history remains per-match. Restore only the toggle.
    xm23State.frameData.setEnabled(settings.frameData!=0u);
    std::fprintf(stderr,
        "XM87.153 TRAINING_OPTIONS_LOADED hitboxes=%u display=%u meter=%u dizzy=%u damage=%u hp=%u,%u player=%u frame=%u\n",
        settings.hitboxes,settings.displayData,settings.infiniteMeter,
        settings.dizzyAllowed,settings.damageMode,settings.dummy1HP,
        settings.dummy2HP,settings.playerSelect,settings.frameData);
}

// Called from the EXISTING mapped SDL input route: zero input buffering.
// Codes 0-3 are L/R/U/D; 0x80-0x85 attack; 0x86 Coin; 0x87 Start.
static inline int xm23FilterInput(int player,int sub,int value,int training) {
    if(!training || player<0 || player>1) {
        if(!training) {xm23State.menuOpen=false;xm23State.frameData.clear();}
        return value;
    }
    if(!xm23State.combat) { xm23State.menuOpen=false; return value; }
    int key=-1;
    if(sub>=0 && sub<=3)key=sub;
    else if(sub>=0x80 && sub<=0x87)key=sub-0x80+4;
    if(key<0) return xm23State.menuOpen?0:value;
    const bool down=value!=0;
    bool edge=down && !xm23State.pressed[player][key];
    xm23State.pressed[player][key]=down?1:0;
    if(sub>=0 && sub<=3){
        xm23State.frameData.direction(player,
            xm23State.pressed[player][2]!=0,
            xm23State.pressed[player][3]!=0,
            xm23State.pressed[player][0]!=0,
            xm23State.pressed[player][1]!=0);
    }
    // Either physical Coin opens/closes; it never falls through into the game.
    if(sub==0x86) {
        if(edge) {xm23State.menuOpen=!xm23State.menuOpen;xm23State.selected=0;}
        return 0;
    }
    if(!xm23State.menuOpen){
        if(edge && sub>=0x80 && sub<=0x85)xm23State.frameData.press(player,sub);
        return value;
    }
    // Navigation is polled separately using physical XInput A/B. The combat
    // mappings (LP/START etc.) never activate menu options.
    return 0;
}

static inline void xm257TrainingNavigate(int dir){
    if(!xm23State.menuOpen)return;
    xm23State.selected=(xm23State.selected+11+(dir>0?1:-1))%11;
}
static inline void xm257TrainingBack(){
    if(xm23State.menuOpen){xm23State.menuOpen=false;xm25BeginInputHandoff();}
}
static inline void xm257TrainingAccept(){
    if(!xm23State.menuOpen)return;
    switch(xm23State.selected){
      case 0:xm23State.hitboxes=!xm23State.hitboxes;break;
      case 1:xm23State.dataDisplay=!xm23State.dataDisplay;break;
      case 2:xm23State.infiniteMeter=!xm23State.infiniteMeter;break;
      case 3:
          xm23State.dizzyAllowed=!xm23State.dizzyAllowed;
          std::fprintf(stderr,"XM87.37 DIZZY_MENU allowed=%d training=1\n",(int)xm23State.dizzyAllowed);
          break;
      case 4:
          xm87102DamageMode=1-xm87102DamageMode;
          std::fprintf(stderr,"XM87.102 TRAINING_DAMAGE_MODE %s\n",
              xm87102DamageMode?"KO Enabled":"Regenerate");
          break;
      case 5:XM87105HP::state().change(0,1);break;
      case 6:XM87105HP::state().change(1,1);break;
      case 7:
          // XM86.72: Reuse Stage14's verified P1-pad -> P2 game slot route.
          // Only available during live Training. Never rewrite controller maps,
          // character/health RAM, or Versus ownership.
          if(xm23State.combat){
              xm14InputTarget=(xm14InputTarget==0)?1:0;
              xm23State.savedPlayerSelect=(unsigned)xm14InputTarget;
              xm23State.restoreSavedPlayerSelect=false;
              xm25BeginInputHandoff(); // release the confirm before gameplay
              std::fprintf(stderr,"XM86.72 TRAINING_CONTROL_CHANGED selected=P%d\n",xm14InputTarget+1);
              std::fflush(stderr);
          }
          break;
      case 8:xm23State.menuOpen=false;xm25BeginInputHandoff();break;
      case 9:xm23State.returnMainRequested=true;break;
      case 10:
          xm23State.frameData.setEnabled(!xm23State.frameData.enabled);
          xm31SetPaneWindow(sdlWindow,xm23State.frameData.enabled);
          AdjustImageSize(); // update logical canvas and game rectangle immediately
          break;
    }
    // Save immediately after any actual Training option edit. Menu commands
    // RETURN (8) and MAIN MENU (9) are not persistent configuration.
    if(xm23State.selected!=8 && xm23State.selected!=9)
        xm257SaveTrainingOptions();
}
// Menu owner is P1's saved controller; physical XInput face layout is fixed
// for UI independently of each player's remappable arcade attack buttons.
struct XM257MenuPad {
    SDL_Joystick* joystick=nullptr;
    SDL_GameController* controller=nullptr;
    SDL_JoystickID instance=-1;
    Uint32 lastProbeAt=0;
    bool previousA=false, previousB=false, previousUp=false, previousDown=false;
    bool wasOpen=false;
};
static inline void xm257CloseTrainingPad(XM257MenuPad& p){
    xm248ReconcileMenuPad(-1,p.joystick,p.controller,p.instance);
    p=XM257MenuPad{};
}
static inline bool xm87105TrainingHPHotkey(const SDL_Event& e){
    if(!xm14TrainingMode || !xm23State.combat || e.type!=SDL_KEYDOWN)return false;
    unsigned player=0;int direction=0;
    switch(e.key.keysym.sym){
        case SDLK_KP_PLUS:player=0;direction=1;break;
        case SDLK_KP_MINUS:player=0;direction=-1;break;
        case SDLK_KP_MULTIPLY:player=1;direction=1;break;
        case SDLK_KP_DIVIDE:player=1;direction=-1;break;
        default:return false;
    }
    if(!e.key.repeat){
        XM87105HP::state().change(player,direction);
        xm257SaveTrainingOptions();
    }
    return true; // do not also adjust global volume
}
static inline bool xm257HandleTrainingMenuEvent(const SDL_Event& e, bool training){
    if(!training || !xm23State.menuOpen)return false;
    if(e.type==SDL_KEYDOWN && !e.key.repeat){
        switch(e.key.keysym.sym){
           case SDLK_UP:xm257TrainingNavigate(-1);return true;
           case SDLK_DOWN:xm257TrainingNavigate(1);return true;
           case SDLK_RETURN:case SDLK_SPACE:xm257TrainingAccept();return true;
           case SDLK_ESCAPE:xm257TrainingBack();return true;
           default:break;
        }
    }
    // Consume menu face/key events so the emulator doesn't receive them as a
    // second input path; window/quit events still propagate normally.
    return e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_CONTROLLERBUTTONUP ||
           e.type==SDL_JOYBUTTONDOWN || e.type==SDL_JOYBUTTONUP;
}
static inline void xm257PollTrainingMenu(bool training){
    static XM257MenuPad p{};
    if(!training || !xm23State.menuOpen){
        if(p.wasOpen)xm257CloseTrainingPad(p);
        return;
    }
    SDL_GameControllerUpdate();
    const Uint32 now=SDL_GetTicks();
    const bool first=!p.wasOpen;
    if(first)p.wasOpen=true;
    bool deviceChanged=false;
    // Recheck identity and retry missing or not-yet-ready SDL devices
    // while the Training menu stays open. A reconnection is not a press.
    if(first || !p.lastProbeAt ||
       (Uint32)(now-p.lastProbeAt)>=150u ||
       (p.joystick && !SDL_JoystickGetAttached(p.joystick))){
        p.lastProbeAt=now ? now : 1;
        XM13_Settings settings{};xm13Load(&settings);
        const int id=xm248FindAssignedDevice(&settings,0);
        deviceChanged=xm248ReconcileMenuPad(id,p.joystick,p.controller,p.instance);
        if(deviceChanged)SDL_GameControllerUpdate(); // seed the newly opened device
    }
    if(!p.joystick || !SDL_JoystickGetAttached(p.joystick)){
        p.previousA=p.previousB=p.previousUp=p.previousDown=false;
        return;
    }
    const bool a=p.controller?SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_A)!=0:
        SDL_JoystickGetButton(p.joystick,0)!=0;
    const bool b=p.controller?SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_B)!=0:
        SDL_JoystickGetButton(p.joystick,1)!=0;
    bool up=false,down=false;
    if(p.controller){
        up=SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_DPAD_UP)!=0;
        down=SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_DPAD_DOWN)!=0;
    }
    if(SDL_JoystickNumHats(p.joystick)>0){
        const Uint8 hat=SDL_JoystickGetHat(p.joystick,0);
        up=up || (hat&SDL_HAT_UP)!=0;
        down=down || (hat&SDL_HAT_DOWN)!=0;
    }
    if(SDL_JoystickNumAxes(p.joystick)>1){
        const int y=SDL_JoystickGetAxis(p.joystick,1);
        up=up||y< -XM13_DEADZONE;
        down=down||y>XM13_DEADZONE;
    }
    // Reconnected buttons can already be held. Seed their states and
    // require a fresh edge instead of selecting/backing automatically.
    if(first || deviceChanged){
        p.previousA=a;p.previousB=b;p.previousUp=up;p.previousDown=down;
        return;
    }
    if(b && !p.previousB)xm257TrainingBack();
    else if(a && !p.previousA)xm257TrainingAccept();
    else if(up && !p.previousUp)xm257TrainingNavigate(-1);
    else if(down && !p.previousDown)xm257TrainingNavigate(1);
    p.previousA=a;p.previousB=b;p.previousUp=up;p.previousDown=down;
}

// Stage 23.1: Render the overlay font directly with SDL2 primitives.
// Unlike the Stage 23 version, this does not call an unavailable
// FBNeo-specific inprint_shadowed() function from the input translation unit.
struct XM23Glyph { char c; unsigned char rows[7]; };
static const XM23Glyph xm23Font[] = {
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'B', {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}},
    {'C', {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f}},
    {'D', {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'F', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}},
    {'G', {0x0f, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0f}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'Q', {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1b, 0x11}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x06, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e}},
    {'6', {0x0f, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x1e}},
    {'+', {0x00,0x04,0x04,0x1f,0x04,0x04,0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {'/', {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}},
    {'>', {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10}},
    {'?', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
};
static inline const unsigned char* xm23Glyph(char ch) {
    if (ch >= 'a' && ch <= 'z') ch = char(ch - 'a' + 'A');
    for (unsigned i=0;i<sizeof(xm23Font)/sizeof(xm23Font[0]);++i)
        if(xm23Font[i].c==ch) return xm23Font[i].rows;
    return nullptr;  // Spaces are deliberately blank.
}
static inline void xm23DrawTextSized(SDL_Renderer* r,const char* text,int x,int y,int maxWidth,int requestPixel=2) {
    if(!r || !text)return;
    const int length=(int)std::strlen(text);
    const int spriteScale=(length*8*std::max(1,requestPixel)>maxWidth)?1:std::max(1,requestPixel);
    if(XMGlyph::left(r,text,x,y,maxWidth,spriteScale,235))return;
    const int pixel=(length*6*requestPixel>maxWidth)?1:requestPixel;
    const int step=6*pixel;
    // A subtle dark shadow matches the native menu without FBNeo font APIs.
    for(int pass=0;pass<2;++pass) {
        SDL_SetRenderDrawColor(r,pass?210:0,pass?225:0,pass?255:0,255);
        int pen=x+(pass?0:pixel/2);
        const int top=y+(pass?0:pixel/2);
        for(const char* p=text;*p && pen+5*pixel<=x+maxWidth;++p,pen+=step) {
            const unsigned char* rows=xm23Glyph(*p);
            if(!rows)continue;
            for(int row=0;row<7;++row)for(int col=0;col<5;++col) {
                if(rows[row] & (1u<<(4-col))) {
                    SDL_Rect dot={pen+col*pixel,top+row*pixel,pixel,pixel};
                    SDL_RenderFillRect(r,&dot);
                }
            }
        }
    }
}

// Palette-constrained colored pixel text, with a subtle dark shadow.
// S = yellow, A = green, numeric values = light peach. Single source glyphs.
static inline void xm32ColoredText(SDL_Renderer* r,const char* value,int x,int y,int px,
                                  Uint8 red,Uint8 green,Uint8 blue){
    if(!r||!value)return;
    for(int pass=0;pass<2;++pass){
        SDL_SetRenderDrawColor(r,pass?red:0,pass?green:0,pass?blue:0,255);
        int xx=x+(pass?0:1),yy=y+(pass?0:1),pen=xx;
        for(const char* t=value;*t;++t,pen+=6*px){
            const unsigned char* glyph=xm23Glyph(*t);
            if(!glyph)continue;
            for(int row=0;row<7;++row)for(int col=0;col<5;++col)
                if(glyph[row]&(1u<<(4-col))){
                    SDL_Rect dot={pen+col*px,yy+row*px,px,px};
                    SDL_RenderFillRect(r,&dot);
                }
        }
    }
}
// Single-pixel rounded rectangular outline and dark navy card fill.
static inline void xm39RoundedCard(SDL_Renderer* r,const SDL_Rect& a){
    SDL_SetRenderDrawColor(r,10,20,39,225);
    SDL_Rect inner={a.x+1,a.y,a.w-2,a.h};SDL_RenderFillRect(r,&inner);
    SDL_Rect mid={a.x,a.y+1,a.w,a.h-2};SDL_RenderFillRect(r,&mid);
    SDL_SetRenderDrawColor(r,83,119,165,255);
    SDL_RenderDrawLine(r,a.x+2,a.y,a.x+a.w-3,a.y);
    SDL_RenderDrawLine(r,a.x+2,a.y+a.h-1,a.x+a.w-3,a.y+a.h-1);
    SDL_RenderDrawLine(r,a.x,a.y+2,a.x,a.y+a.h-3);
    SDL_RenderDrawLine(r,a.x+a.w-1,a.y+2,a.x+a.w-1,a.y+a.h-3);
    SDL_RenderDrawPoint(r,a.x+1,a.y+1);
    SDL_RenderDrawPoint(r,a.x+a.w-2,a.y+1);
    SDL_RenderDrawPoint(r,a.x+1,a.y+a.h-2);
    SDL_RenderDrawPoint(r,a.x+a.w-2,a.y+a.h-2);
}
// Stable input display: only actual pressed directions and buttons.
static inline void xm41AppendInput(char* out,unsigned cap,const char* part){
    if(!cap)return;
    const unsigned used=(unsigned)std::strlen(out);
    if(used<cap)std::snprintf(out+used,cap-used,"%s",part);
}
static inline void xm41InputTitle(const XM30Event& e,char* out,unsigned cap){
    if(!cap)return;
    out[0]=0;
    if(!e.inputObserved){
        std::snprintf(out,cap,"INPUT ?");
        return;
    }
    for(unsigned i=0;i<e.inputDirectionCount && i<XM30Event::INPUT_DIRS;++i){
        const unsigned dir=e.inputDirections[i];
        if(dir<1u || dir>9u)continue;
        char part[4];std::snprintf(part,sizeof(part),"%u",dir);
        if(out[0])xm41AppendInput(out,cap,">");
        xm41AppendInput(out,cap,part);
    }
    unsigned buttons=e.inputButtons;
    if(!buttons && e.code>=0x80 && e.code<=0x85)
        buttons=1u<<(unsigned)(e.code-0x80);
    static const char* buttonNames[6]={"LP","MP","HP","LK","MK","HK"};
    bool first=true;
    for(unsigned i=0;i<6;++i){
        if(!(buttons&(1u<<i)))continue;
        if(first && out[0])xm41AppendInput(out,cap," + ");
        else if(!first)xm41AppendInput(out,cap,"+");
        xm41AppendInput(out,cap,buttonNames[i]);
        first=false;
    }
    if(!buttons)std::snprintf(out,cap,"INPUT ?");
}
// Fixed physical-width timeline. Three display pixels per host observation:
// two colored pixels, one gutter, numbered each ten frames.
struct XM41FrameCardLayout {
    static const int PIXELS_PER_FRAME=3;
    static const int HEAD=20,ROW=26,TAIL=4;
    XM30TimelineWrap wrap;
    int height=0;
};
static inline XM41FrameCardLayout xm41FrameCardLayout(
        const XM30Event& e,const XM30FrameData& f,int cardWidth){
    XM41FrameCardLayout out{};
    unsigned long long nativeFinish=e.inputFrame;
    if(e.closed || e.noBox)nativeFinish=std::max(nativeFinish,e.endFrame);
    if(!e.closed && !e.noBox)
        nativeFinish=std::max(nativeFinish,e.hitboxLastFrame);
    if(e.attackObserved)
        nativeFinish=std::max(nativeFinish,
            e.attackEnded?e.attackEndFrame:e.attackLastFrame);
    if(e.despawnFrame)nativeFinish=std::max(nativeFinish,e.despawnFrame);
    if(e.projectileBase)
        nativeFinish=std::max(nativeFinish,e.lastProjectileFrame);
    for(int i=0;i<e.contactCount;++i)
        nativeFinish=std::max(nativeFinish,e.contactFrames[i]);
    out.wrap.start=e.inputWallFrame;
    out.wrap.finish=std::max(e.lastWallFrame,
        f.wallForNative(e.player,nativeFinish,out.wrap.start));
    for(int i=0;i<e.contactCount;++i)
        out.wrap.finish=std::max(out.wrap.finish,e.contactWallFrames[i]);
    if(out.wrap.finish<out.wrap.start)out.wrap.finish=out.wrap.start;
    out.wrap.ticksPerRow=(unsigned)std::max(1,
        std::max(1,cardWidth-17)/XM41FrameCardLayout::PIXELS_PER_FRAME);
    out.height=XM41FrameCardLayout::HEAD+
        (int)out.wrap.rows()*XM41FrameCardLayout::ROW+
        XM41FrameCardLayout::TAIL;
    return out;
}
static inline void xm41DrawFrameCard(SDL_Renderer* r,const SDL_Rect& card,
                                       const XM30Event& e,const XM30FrameData& f){
    if(!r || card.w<105 || card.h<49)return;
    xm39RoundedCard(r,card);
    const int left=card.x+7,right=card.x+card.w-7;
    const int x0=left+2;
    const XM41FrameCardLayout layout=xm41FrameCardLayout(e,f,card.w);
    const XM30TimelineWrap& t=layout.wrap;
    const unsigned long long startWall=t.start,finishWall=t.finish;
    const int HEAD=XM41FrameCardLayout::HEAD;
    const int ROW=XM41FrameCardLayout::ROW;
    const int STEP=XM41FrameCardLayout::PIXELS_PER_FRAME;
    static const int HIT_Y=13,MOVE_Y=19,BAR_H=3;
    char title[128];xm41InputTitle(e,title,sizeof(title));
    xm32ColoredText(r,title,left,card.y+5,1,183,218,253);
    SDL_SetRenderDrawColor(r,53,78,110,255);
    SDL_RenderDrawLine(r,left,card.y+17,right,card.y+17);
    auto nativeWall=[&](unsigned long long native)->unsigned long long{
        return f.wallForNative(e.player,native,startWall);
    };
    auto tick=[&](int x,int y,int h,Uint8 red,Uint8 green,Uint8 blue){
        SDL_SetRenderDrawColor(r,red,green,blue,245);
        const SDL_Rect bar={x,y,1,h};
        SDL_RenderFillRect(r,&bar);
    };
    const unsigned long long activeEnd=(e.closed||e.noBox)?
        e.endFrame:e.hitboxLastFrame;
    const unsigned long long actionEnd=e.attackEnded?
        e.attackEndFrame:e.attackLastFrame;
    const unsigned rows=t.rows();
    const unsigned maxRows=card.h>=HEAD+ROW?
        (unsigned)((card.h-HEAD)/ROW):0u;
    for(unsigned row=0;row<rows && row<maxRows;++row){
        const int y=card.y+HEAD+(int)row*ROW;
        const unsigned long long base=startWall+
            (unsigned long long)row*t.ticksPerRow;
        const unsigned long long last=std::min(
            finishWall,base+(unsigned long long)t.ticksPerRow-1u);
        for(unsigned long long at=base;at<=last;++at){
            const XM30WallTick sample=f.wallTick(e.player,at);
            const unsigned long long native=sample.wall?sample.native:
                e.inputFrame+(at-startWall);
            const bool held=xm30VisualHeld(sample,e,at,native);
            const int x=x0+(int)(at-base)*STEP;
            const unsigned long long number=at-startWall+1u;
            const bool ten=number%10u==0u,five=number%5u==0u;
            // Every frame has an individual notch, with 5/10 emphasis.
            tick(x+1,y+(ten?8:(five?9:10)),ten?4:(five?3:1),
                 ten?225:(five?151:81),
                 ten?235:(five?169:106),
                 ten?246:(five?198:135));
            if(number==1u || ten){
                char label[16];std::snprintf(label,sizeof(label),"%llu",number);
                const int w=(int)std::strlen(label)*6;
                const int labelX=std::max(left,std::min(x+1-w/2,right-w));
                xm32ColoredText(r,label,labelX,y,1,159,181,204);
            }
            Uint8 red=113,green=133,blue=160;
            if(held){red=43;green=48;blue=57;}
            else if(!e.noBox && native>=e.firstFrame && native<=activeEnd){
                red=93;green=224;blue=135;
            }else if(e.source==1 && e.spawnFrame &&
                     native>=e.spawnFrame &&
                     (!e.despawnFrame || native<e.despawnFrame)){
                if(!e.noBox && native>activeEnd){
                    red=86;green=157;blue=204;
                }else{red=253;green=204;blue=96;}
            }
            for(int w=0;w<STEP-1;++w)
                tick(x+w,y+HIT_Y,BAR_H,red,green,blue);
            if(e.source==1 && !held && native>=e.inputFrame){
                const unsigned long long j=native-e.inputFrame;
                if(j<1024 && (e.motionBits[j/64]&(1ull<<(j%64))))
                    tick(x+1,y+HIT_Y+BAR_H,1,86,193,243);
            }
            for(int w=0;w<STEP-1;++w){
                if(held)tick(x+w,y+MOVE_Y,BAR_H,43,48,57);
                else if(e.attackObserved && native>=e.attackStartFrame &&
                        native<=actionEnd)
                    tick(x+w,y+MOVE_Y,BAR_H,236,150,99);
                else tick(x+w,y+MOVE_Y,BAR_H,74,88,111);
            }
        }
    }
    // Isolated red confirmations on the hit lane, never a red strip
    // cutting through both hit and action bars.
    for(int i=0;i<e.contactCount && i<XM30Event::CONTACTS;++i){
        const unsigned long long at=e.contactWallFrames[i]?
            e.contactWallFrames[i]:nativeWall(e.contactFrames[i]);
        if(at<startWall || at>finishWall)continue;
        const unsigned row=t.row(at);
        if(row>=maxRows)continue;
        const int x=x0+(int)t.column(at)*STEP;
        const int y=card.y+HEAD+(int)row*ROW;
        tick(x+1,y+HIT_Y-3,3,249,57,66);
        tick(x,y+HIT_Y,2,255,126,126);
        tick(x+1,y+HIT_Y,2,249,57,66);
    }
}
static inline void xm23DrawText(SDL_Renderer* r,const char* text,int x,int y){
    xm23DrawTextSized(r,text,x,y,10000,2);
}
// Stage 87.217: the external FRAME DATA panel remains SDL UI. Floating
// S:##F popups are composited in the native 384x224 game pixel layer
// (vid_sdl2.cpp::xm87217NativeFloat) and are NOT drawn in this overlay.
static inline void xm30Overlay(SDL_Renderer* r,const SDL_Rect& dest,int sw,int sh){
    XM30FrameData& f=xm23State.frameData;
    if(!f.enabled || !xm31ExternalPane || sw<=0 || sh<=0)return;
    // Stage26 game rectangle stays inside the first 960 logical units.
    // Reserve 240 units to its right; the old game's 720x540 view is unchanged.
    const int panelX=dest.x+dest.w+12;
    const int panelRight=XM31_BASE_W+XM31_EXTRA_W-12;
    const int panelW=std::min(230,panelRight-panelX);
    if(panelW>40) {
        const int top=7;
        SDL_Rect panel={panelX,top,panelW,XM31_BASE_H-14};
        SDL_SetRenderDrawColor(r,4,8,20,165);SDL_RenderFillRect(r,&panel);
        SDL_SetRenderDrawColor(r,92,147,219,210);SDL_RenderDrawRect(r,&panel);
        xm23DrawTextSized(r,"FRAME DATA",panel.x+8,top+8,panelW-16,2);
        xm32ColoredText(r,"S",panel.x+7,top+27,1,255,218,84);
        xm32ColoredText(r,":START  ",panel.x+13,top+27,1,179,199,231);
        xm32ColoredText(r,"A",panel.x+61,top+27,1,102,236,142);
        xm32ColoredText(r,":ACTIVE",panel.x+67,top+27,1,179,199,231);
        // Source cards grow vertically as stable 1px host ticks wrap.
        // Retain as many NEWEST complete cards as actually fit; long
        // supers never rescale, overwrite, or erase their first row.
        const int available=std::max(0,panel.h-85);
        int first=f.count,used=0;
        const int cardWidth=panelW-8;
        for(int i=f.count-1;i>=0;--i){
            const int h=xm41FrameCardLayout(f.history[i],f,cardWidth).height;
            if(used+h+(used?3:0)>available)break;
            used+=h+(used?3:0);
            first=i;
        }
        if(first==f.count && f.count>0)first=f.count-1;
        int cardY=top+52;
        for(int i=first;i<f.count;++i){
            const XM30Event& e=f.history[i];
            const int h=xm41FrameCardLayout(e,f,cardWidth).height;
            SDL_Rect card={panel.x+4,cardY,cardWidth,
                           std::min(h,std::max(0,available-(cardY-top-52)))};
            if(card.h>=50)xm41DrawFrameCard(r,card,e,f);
            cardY+=h+3;
        }
        xm23DrawTextSized(r,"QC QD DP HC DQ  PR=PROJECTILE",panel.x+8,top+panel.h-26,panelW-14,1);
        xm23DrawTextSized(r,"NEWEST AT BOTTOM",panel.x+8,top+panel.h-13,panelW-14,1);
    }

}
static inline void xm23Overlay(SDL_Renderer* renderer, const SDL_Rect& gameDest,
                              int sourceWidth,int sourceHeight) {
    if(!renderer || !xm23State.combat) return;
    SDL_BlendMode oldBlend=SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer,&oldBlend);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    if(xm23State.hitboxes && sourceWidth>0 && sourceHeight>0 && !xm23State.menuOpen) {
        for(int i=0;i<xm23State.count && i<192;i++) {
            const XM23Box& b=xm23State.box[i];
            if(b.w<=0 || b.h<=0)continue;
            SDL_Rect r={gameDest.x+b.x*gameDest.w/sourceWidth,
                gameDest.y+b.y*gameDest.h/sourceHeight,
                b.w*gameDest.w/sourceWidth,b.h*gameDest.h/sourceHeight};
            if(r.w<1)r.w=1;
            if(r.h<1)r.h=1;
            Uint8 red=0,green=0,blue=0;
            switch(b.kind) {
                case 0:red=110;green=110;blue=255;break; // hurt
                case 1:red=255;break;                      // attack
                case 2:green=255;break;                    // push
                case 3:red=255;green=220;break;            // throwable
                case 4:green=255;blue=255;break;           // projectile hurt
                default:red=255;green=60;blue=255;break;   // projectile attack
            }
            SDL_SetRenderDrawColor(renderer,red,green,blue,45);
            SDL_RenderFillRect(renderer,&r);
            SDL_SetRenderDrawColor(renderer,red,green,blue,205);
            SDL_RenderDrawRect(renderer,&r);
        }
    }
    if(xm23State.dataDisplay && !xm23State.menuOpen) {
        SDL_Rect back={gameDest.x+6,gameDest.y+gameDest.h-35,gameDest.w-12,30};
        SDL_SetRenderDrawColor(renderer,0,0,0,145);
        SDL_RenderFillRect(renderer,&back);
        char msg[190];
        std::snprintf(msg,sizeof(msg),"P1 HP:%03d  METER:%d  COMBO:%d",xm23State.health[0],xm23State.meter[0],xm23State.combo[0]);
        xm23DrawText(renderer,msg,back.x+5,back.y+3);
        std::snprintf(msg,sizeof(msg),"P2 HP:%03d  METER:%d  COMBO:%d",xm23State.health[1],xm23State.meter[1],xm23State.combo[1]);
        xm23DrawText(renderer,msg,back.x+5,back.y+16);
    }
    xm30Overlay(renderer,gameDest,sourceWidth,sourceHeight);
    if(xm23State.menuOpen) {
        SDL_Rect bg={gameDest.x+gameDest.w/8,gameDest.y+gameDest.h/12,
                     gameDest.w*3/4,gameDest.h*5/6};
        SDL_SetRenderDrawColor(renderer,0,0,0,232);
        SDL_RenderFillRect(renderer,&bg);
        SDL_SetRenderDrawColor(renderer,65,110,225,255);
        SDL_RenderDrawRect(renderer,&bg);
        const int margin=std::max(6,bg.w/35);
        const int x=bg.x+margin, w=bg.w-2*margin;
        const int rowHeight=std::max(12,(bg.h-48)/11);
        xm23DrawTextSized(renderer,"TRAINING OPTIONS",x,bg.y+7,w,2);
        const char* items[11]={"HITBOXES","DISPLAY DATA","INFINITE METER",
                              "DIZZY","DAMAGE MODE","DUMMY HP 1","DUMMY HP 2",
                              "PLAYER SELECT","RETURN","MAIN MENU","FRAME DATA"};
        for(int i=0;i<11;i++) {
            const int yy=bg.y+25+i*rowHeight;
            // Native focus convention: selected text at full brightness,
            // every other row darkened. No focus box, bar, or arrow glyph.
            const bool selected=(xm23State.selected==i);
            const char* val=i==0?(xm23State.hitboxes?"ON":"OFF"):
                i==1?(xm23State.dataDisplay?"ON":"OFF"):
                i==2?(xm23State.infiniteMeter?"ON":"OFF"):
                i==3?(xm23State.dizzyAllowed?"ON":"OFF"):
                i==4?(xm87102DamageMode?"KO ENABLED":"REGENERATE"):
                i==7?(xm14InputTarget?"P2":"P1"):
                i==10?(xm23State.frameData.enabled?"ON":"OFF"):"";
            char lineText[110];
            if(i==5 || i==6)
                std::snprintf(lineText,sizeof(lineText),"%s %u%%",
                              items[i],XM87105HP::state().percent((unsigned)(i-5)));
            else
                std::snprintf(lineText,sizeof(lineText),"%s %s",items[i],val);
            const int fontScale=rowHeight>=16?2:1;
            if(!XMGlyph::left(renderer,lineText,x+5,yy+2,w-10,fontScale,selected?255:82))
                xm23DrawTextSized(renderer,lineText,x+5,yy+2,w-10,fontScale);
        }
        xm23DrawTextSized(renderer,"UP/DOWN  A SELECT  B/ESC BACK",x,bg.y+bg.h-17,w,1);
    }
    SDL_SetRenderDrawBlendMode(renderer,oldBlend);
}
#endif
