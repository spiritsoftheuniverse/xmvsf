#ifndef XMVSF_STAGE23_TRAINING_UI_H
#define XMVSF_STAGE23_TRAINING_UI_H
// Stage 23: UI state and SDL presentation only. No ROM writes here.
// Grounded in peon2/fbneo-training-mode XMVSF & Marvel hitbox profiles.
#include <SDL.h>
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
    bool previousA=false, previousB=false, previousUp=false, previousDown=false;
    bool wasOpen=false;
};
static inline void xm257CloseTrainingPad(XM257MenuPad& p){
    if(p.controller)SDL_GameControllerClose(p.controller);
    if(p.joystick)SDL_JoystickClose(p.joystick);
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
    SDL_JoystickUpdate();
    if(!p.wasOpen){
        XM13_Settings settings{};xm13Load(&settings);
        int id=xm13FindDevice(&settings.player[0]);
        if(id>=0){
            p.joystick=SDL_JoystickOpen(id);
            if(SDL_IsGameController(id))p.controller=SDL_GameControllerOpen(id);
        }
        p.wasOpen=true;
        // Seed previous state with any held buttons: no carried-over press.
        if(p.joystick){
            p.previousA=SDL_JoystickGetButton(p.joystick,0)!=0;
            p.previousB=SDL_JoystickGetButton(p.joystick,1)!=0;
        }
        return;
    }
    if(!p.joystick || !SDL_JoystickGetAttached(p.joystick))return;
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
struct XM39Mark {const char* label;unsigned long long at;int x=0;int labelX=0;};
static inline int xm39TimelineStride(unsigned long long span,int availablePixels){
    const int slots=std::max(1,availablePixels/3);
    return (int)std::max(1ull,(span+(unsigned long long)slots-1)/(unsigned long long)slots);
}
// Stage 41: two independently sampled timelines share exactly one time axis.
// Upper line is projectile spawn/active/travel; lower line is conservative
// return-to-neutral observation. The latter is NOT verified input permission.
static inline void xm41DrawFrameCard(SDL_Renderer* r,const SDL_Rect& card,
                                      const XM30Event& e,unsigned long long currentFrame){
    if(!r || card.w<105 || card.h<135)return;
    xm39RoundedCard(r,card);
    const int left=card.x+7,right=card.x+card.w-7;
    char caption[44];
    std::snprintf(caption,sizeof(caption),"P%d %s B%d %s",e.player+1,
        e.source==1?"PROJ":xm32MotionTag(e.motion),
        e.code>=0x80 && e.code<=0x85?e.code-0x80+1:0,
        e.noBox?"NO BOX":((e.source==1?e.despawnFrame!=0:e.closed)?"DONE":"LIVE"));
    xm32ColoredText(r,caption,left,card.y+6,1,159,199,255);
    SDL_SetRenderDrawColor(r,53,78,110,255);
    SDL_RenderDrawLine(r,left,card.y+19,right,card.y+19);

    unsigned long long finish=e.inputFrame;
    if(e.closed || e.noBox)finish=std::max(finish,e.endFrame);
    if(!e.closed && !e.noBox)finish=std::max(finish,e.source==1?
        std::min(currentFrame,e.inputFrame+600ull):currentFrame);
    if(e.neutralEstimate)finish=std::max(finish,e.neutralFrame);
    if(e.despawnFrame)finish=std::max(finish,e.despawnFrame);
    else if(e.projectileBase && e.positionKnown)
        finish=std::max(finish,std::min(currentFrame,e.inputFrame+600ull));
    // Pending recovery is an observation, not a hold-to-extend timer.
    // Keep it bounded if the actor never returns to observed idle.
    if(e.lockPending)finish=std::max(finish,
        std::min(currentFrame,e.inputFrame+(unsigned long long)XM30FrameData::MAX_IDLE_WAIT));
    // A still-pending entry uses its own ongoing elapsed frame range.
    if(finish<=e.inputFrame)finish=e.inputFrame+1;
    const unsigned long long span=finish-e.inputFrame+1;
    const int x0=left+2,x1=right-2;
    const int stride=xm39TimelineStride(span,x1-x0);
    auto px=[&](unsigned long long frame)->int{
        if(frame<e.inputFrame)frame=e.inputFrame;
        if(frame>finish)frame=finish;
        return std::min(x1,x0+(int)((frame-e.inputFrame)/(unsigned)stride)*3);
    };
    auto tick=[&](int x,int y,int h,Uint8 rr,Uint8 gg,Uint8 bb){
        SDL_SetRenderDrawColor(r,rr,gg,bb,245);
        SDL_Rect t={x,y,1,h};SDL_RenderFillRect(r,&t);
    };
    // Markers sit ABOVE both chart rows; connecting lines keep the precise
    // tick location visible even when captions need to be displaced.
    XM39Mark marks[5];int n=0;
    marks[n++]={"IN",e.inputFrame};
    if(e.source==1 && e.spawnFrame)marks[n++]={"SP",e.spawnFrame};
    if(!e.noBox)marks[n++]={"ON",e.firstFrame};
    // Projectile lifecycle ends at disappearance, not an artificial "OUT"
    // control phase. The rightmost tick is the object-lifetime boundary.
    // Fighter hitboxes still show OFF at last active frame.
    if(e.source==0 && !e.noBox && e.closed)marks[n++]={"OFF",e.endFrame};
    for(int k=0;k<n;++k){
        marks[k].x=px(marks[k].at);
        const int width=(int)std::strlen(marks[k].label)*6;
        int x=std::max(left,std::min(marks[k].x-width/2,right-width));
        if(k>0)x=std::max(x,marks[k-1].labelX+(int)std::strlen(marks[k-1].label)*6+3);
        marks[k].labelX=x;
    }
    if(n){
        const int overflow=marks[n-1].labelX+(int)std::strlen(marks[n-1].label)*6-right;
        if(overflow>0)for(int k=0;k<n;++k)marks[k].labelX=std::max(left,marks[k].labelX-overflow);
    }
    for(int k=0;k<n;++k){
        const int center=marks[k].labelX+(int)std::strlen(marks[k].label)*3;
        SDL_SetRenderDrawColor(r,77,101,139,255);
        SDL_RenderDrawLine(r,center,card.y+36,marks[k].x,card.y+43);
        xm32ColoredText(r,marks[k].label,marks[k].labelX,card.y+27,1,195,213,234);
        tick(marks[k].x,card.y+42,2,236,241,251);
    }
    // Primary hitbox timeline. Cyan underneath it represents actual tracked
    // projectile position changes, not assumed movement between frames.
    for(unsigned long long step=0;step<span;step+=(unsigned)stride){
        const unsigned long long at=e.inputFrame+step;
        Uint8 rr=113,gg=133,bb=160;
        const unsigned long long attackEnd=(e.closed||e.noBox)?e.endFrame:currentFrame;
        if(!e.noBox && at>=e.firstFrame && at<=attackEnd){rr=93;gg=224;bb=135;}
        else if(e.source==1 && e.spawnFrame && at>=e.spawnFrame &&
                (e.despawnFrame==0 || at<e.despawnFrame)) {
            // Yellow = travel-in before hitbox, blue = post-active travel.
            if(!e.noBox && at>attackEnd){rr=86;gg=157;bb=204;}
            else {rr=253;gg=204;bb=96;}
        }
        tick(px(at),card.y+46,7,rr,gg,bb);
        bool measuredMotion=false;
        if(e.source==1){
            // Collapse exact frame-motion evidence into the current pixel's
            // stride bucket. Never fill stationary gaps as if they moved.
            for(unsigned k=0;k<(unsigned)stride && step+k<1024;++k){
                const unsigned j=(unsigned)(step+k);
                if(e.motionBits[j/64]&(1ull<<(j%64))){measuredMotion=true;break;}
            }
        }
        if(measuredMotion)tick(px(at),card.y+60,5,86,193,243);
    }
    // A terminal border tick shows object removal, without a fake OUT phase.
    if(e.source==1 && e.despawnFrame)
        tick(px(e.despawnFrame),card.y+44,11,195,213,234);
    xm32ColoredText(r,"HIT",left,card.y+55,1,118,155,199);
    xm32ColoredText(r,"MOVE",right-25,card.y+55,1,86,193,243);
    SDL_SetRenderDrawColor(r,53,78,110,255);
    SDL_RenderDrawLine(r,left,card.y+74,right,card.y+74);
    // Synchronized lower lane. No guessed recovery time: for unsupported
    // moves this remains UNKNOWN rather than borrowing the projectile end.
    xm32ColoredText(r,"PLAYER ATTACK > IDLE",left,card.y+78,1,177,188,211);
    if(e.lockObserved){
        const unsigned long long end=e.neutralEstimate?e.neutralFrame:currentFrame;
        for(unsigned long long step=0;step<span;step+=(unsigned)stride){
            const unsigned long long at=e.inputFrame+step;
            if(at<=end)tick(px(at),card.y+92,7,236,150,99);
            else tick(px(at),card.y+92,7,74,88,111);
        }
        if(e.neutralEstimate)tick(px(e.neutralFrame),card.y+90,11,255,229,196);
    }else{
        xm32ColoredText(r,"UNKNOWN",left+2,card.y+91,1,157,169,185);
    }
    SDL_SetRenderDrawColor(r,53,78,110,255);
    SDL_RenderDrawLine(r,left,card.y+105,right,card.y+105);
    if(e.noBox)xm32ColoredText(r,"NO BOX",left,card.y+112,1,185,190,203);
    else{
        char val[24];
        xm32ColoredText(r,"S:",left,card.y+112,1,255,218,84);
        std::snprintf(val,sizeof(val),"%dF",e.startup);
        xm32ColoredText(r,val,left+14,card.y+112,1,255,207,182);
        xm32ColoredText(r,"A:",left+80,card.y+112,1,102,236,142);
        std::snprintf(val,sizeof(val),"%dF",e.active);
        xm32ColoredText(r,val,left+94,card.y+112,1,255,207,182);
    }
    char info[56];
    if(e.neutralEstimate){
        const unsigned long long duration=e.neutralFrame-e.inputFrame;
        std::snprintf(info,sizeof(info),"IDLE STATE %lluF",duration);
    }else if(e.recoveryUnresolved)std::snprintf(info,sizeof(info),"IDLE UNCONFIRMED");
    else if(e.lockObserved)std::snprintf(info,sizeof(info),"AWAIT IDLE STATE");
    else std::snprintf(info,sizeof(info),"IDLE UNKNOWN");
    xm32ColoredText(r,info,left,card.y+124,1,255,207,182);
    std::snprintf(info,sizeof(info),"1T=%dF",stride);
    xm32ColoredText(r,info,right-48,card.y+124,1,128,157,192);
}
static inline void xm23DrawText(SDL_Renderer* r,const char* text,int x,int y){
    xm23DrawTextSized(r,text,x,y,10000,2);
}
// Bold 5x7 pixel glyphs: black 1-pixel outline, white center, alpha fade.
static inline void xm30OutlinedText(SDL_Renderer* r,const char* str,int cx,int y,int px,Uint8 alpha){
    if(!r||!str)return;
    int len=(int)std::strlen(str);
    int x=cx-(len*6*px)/2;
    for(const char* t=str;*t;++t,x+=6*px){
        const unsigned char* glyph=xm23Glyph(*t);
        if(!glyph)continue;
        for(int row=0;row<7;++row)for(int col=0;col<5;++col){
            if(!(glyph[row]&(1u<<(4-col))))continue;
            const int xx=x+col*px,yy=y+row*px;
            SDL_SetRenderDrawColor(r,0,0,0,alpha);
            SDL_Rect border={xx-1,yy-1,px+2,px+2}; SDL_RenderFillRect(r,&border);
        }
    }
    x=cx-(len*6*px)/2;
    SDL_SetRenderDrawColor(r,255,255,255,alpha);
    for(const char* t=str;*t;++t,x+=6*px){
        const unsigned char* glyph=xm23Glyph(*t);
        if(!glyph)continue;
        for(int row=0;row<7;++row)for(int col=0;col<5;++col)
            if(glyph[row]&(1u<<(4-col))){
                SDL_Rect pixel={x+col*px,y+row*px,px,px}; SDL_RenderFillRect(r,&pixel);
            }
    }
}
// Panel is in its own logical column OUTSIDE gameDest. Popups remain over
// the game itself. One GPU renderer, one present, no readback or extra frames.
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
        // Stage39: each event has a bounded 1px-rounded card. All text is
        // outside the chart-line region; collision-prone marker captions are
        // nudged within the card and connected to their true frame ticks.
        const int rowHeight=146;
        const int visible=std::min(f.count,std::max(0,(panel.h-78)/rowHeight));
        const int first=f.count-visible;
        for(int i=first;i<f.count;++i){
            const XM30Event& e=f.history[i];
            const SDL_Rect card={panel.x+4,top+52+(i-first)*rowHeight,panelW-8,rowHeight-3};
            xm41DrawFrameCard(r,card,e,f.frame);
        }
        xm23DrawTextSized(r,"QC DP HC DQ  PR=PROJECTILE",panel.x+8,top+panel.h-26,panelW-14,1);
        xm23DrawTextSized(r,"NEWEST AT BOTTOM",panel.x+8,top+panel.h-13,panelW-14,1);
    }
    if(xm23State.menuOpen)return;
    for(int i=0;i<XM30FrameData::FLOATS;++i){
        const XM30Float& fl=f.floats[i];
        if(!fl.visible)continue;
        const int x=dest.x+(fl.x*dest.w)/sw;
        const int rise=(fl.age*40)/XM30FrameData::FLOAT_LIFE;
        const int y=dest.y+(fl.y*dest.h)/sh-rise-17;
        Uint8 alpha=fl.age<80?255:(Uint8)(255*(XM30FrameData::FLOAT_LIFE-fl.age)/40);
        char popup[32];std::snprintf(popup,sizeof(popup),"S:%dF",fl.startup);
        const int safeX=std::max(dest.x+25,std::min(x,dest.x+dest.w-25));
        const int safeY=std::max(dest.y+5,y);
        xm30OutlinedText(r,popup,safeX,safeY,2,alpha);
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
            SDL_Rect line={x,yy-1,w,std::max(13,rowHeight-2)};
            SDL_SetRenderDrawColor(renderer,
                xm23State.selected==i?34:14,xm23State.selected==i?62:26,
                xm23State.selected==i?112:49,255);
            SDL_RenderFillRect(renderer,&line);
            if(xm23State.selected==i){
                SDL_Rect stripe={x,yy-1,3,line.h};
                SDL_SetRenderDrawColor(renderer,100,189,255,255);SDL_RenderFillRect(renderer,&stripe);
            }
            const char* val=i==0?(xm23State.hitboxes?"ON":"OFF"):
                i==1?(xm23State.dataDisplay?"ON":"OFF"):
                i==2?(xm23State.infiniteMeter?"ON":"OFF"):
                i==3?(xm23State.dizzyAllowed?"ON":"OFF"):
                i==4?(xm87102DamageMode?"KO ENABLED":"REGENERATE"):
                i==7?(xm14InputTarget?"P2":"P1"):
                i==10?(xm23State.frameData.enabled?"ON":"OFF"):"";
            char lineText[110];
            if(i==5 || i==6)
                std::snprintf(lineText,sizeof(lineText),"%c %s %u%%",xm23State.selected==i?'>':' ',
                              items[i],XM87105HP::state().percent((unsigned)(i-5)));
            else
                std::snprintf(lineText,sizeof(lineText),"%c %s %s",xm23State.selected==i?'>':' ',items[i],val);
            xm23DrawTextSized(renderer,lineText,x+5,yy+2,w-10,rowHeight>=16?2:1);
        }
        xm23DrawTextSized(renderer,"UP/DOWN  A SELECT  B BACK",x,bg.y+bg.h-17,w,1);
    }
    SDL_SetRenderDrawBlendMode(renderer,oldBlend);
}
#endif
