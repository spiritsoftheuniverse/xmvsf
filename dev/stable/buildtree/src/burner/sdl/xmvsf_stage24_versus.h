#ifndef XMVSF_STAGE24_VERSUS_H
#define XMVSF_STAGE24_VERSUS_H
// Native Versus session state: NEVER serialized in a CPS-2 .fs save state.
// No guessed score RAM writes. Explicit result winner and lifetime wins survive state reloads.
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include "xmvsf_stage25_sets.h" /* XMVSF_STAGE25_SETS */

extern int xm24VersusMode;
extern int xm14TrainingMode; /* XMVSF_STAGE25_8_ARCADE_MENU */
enum XM24Command { XM24_NONE=0, XM24_REMATCH=1, XM24_RESELECT=2, XM24_BACK_MENU=3, XM24_TO_ARCADE=4, XM24_TO_TRAINING=5, XM24_REBOOT_MAIN=6 }; /* XMVSF_STAGE25_5_SCOREBOARD_RESET */ /* XMVSF_STAGE25_SETS */
struct XM24Session {
    bool active=false, resultMenu=false, matchSaved=false, seenCombat=false, awarded=false;
    bool triedVsCapture=false, triedCombatCapture=false;
    int wins[2]={0,0};
    int selected=0, winner=0, awaitingResult=0, command=0;
    unsigned long frames=0, liveFrames=0, afterKO=0, outOfCombat=0;
    uint32_t lastMenuNavigateTick=0;
    int lastHp[2]={0,0};
    bool mouseDown=false;
};
extern XM24Session xm24Session;
extern int xm24ResultInputCapture; /* XMVSF_STAGE24_3_LIVE_RESULTS */

static inline void xm24Log(const char* event) {
    FILE* f=std::fopen("xmvsf_stage24_versus.log","a");
    if(!f)return;
    std::fprintf(f,"frame=%lu event=%s wins=%d:%d winner=%d live=%lu afterKO=%lu outside=%lu saved=%d\n",
        xm24Session.frames,event,xm24Session.wins[0],xm24Session.wins[1],xm24Session.winner,
        xm24Session.liveFrames,xm24Session.afterKO,xm24Session.outOfCombat,(int)xm24Session.matchSaved);
    std::fclose(f);
}
static inline const char* xm24SelectPath() { return "config/games/xmvsfu_training_select.fs"; }
static inline const char* xm24FightPath() { return "config/games/xmvsfu_versus_match.fs"; }
static inline bool xm24StateFileValid(const char* path) {
    FILE* f=std::fopen(path,"rb"); if(!f)return false;
    char magic[4]={};
    const bool ok=std::fread(magic,1,4,f)==4 && std::memcmp(magic,"FB1 ",4)==0;
    std::fclose(f);return ok;
}
static inline void xm24ResetMatchTracking() {
    xm24ResultInputCapture=0; /* XMVSF_STAGE24_3_LIVE_RESULTS */
    xm24Session.resultMenu=false;
    xm24Session.seenCombat=false;
    xm24Session.awarded=false;
    xm24Session.winner=0;
    xm24Session.awaitingResult=0;
    xm24Session.command=0;
    xm24Session.liveFrames=0;
    xm24Session.afterKO=0;
    xm24Session.outOfCombat=0;
    xm24Session.lastHp[0]=xm24Session.lastHp[1]=0;
    xm24Session.selected=0;
    xm24Session.triedVsCapture=false;
    xm24Session.triedCombatCapture=false;
}
static inline void xm24StartSession() {
    xm24ResultInputCapture=0; /* XMVSF_STAGE24_3_LIVE_RESULTS */
    xm24Session=XM24Session{};
    xm24Session.active=xm24VersusMode!=0;
    if(xm24Session.active)xm25StartNewSet(); /* XMVSF_STAGE25_SETS */
    if(xm24Session.active)xm24Log("VERSUS_START_SCORE_ZERO");
}
static inline void xm24ShowResult(int winner) {
    if(!xm24Session.active || xm24Session.resultMenu)return;
    xm24Session.winner=(winner>=1 && winner<=2)?winner:0;
    // Credit exactly once, only if winning team is known. Timeouts can be assigned manually.
    if(xm24Session.winner && !xm24Session.awarded) {
        ++xm24Session.wins[xm24Session.winner-1];
        xm24Session.awarded=true;
        xm25OnAward(xm24Session.wins[0],xm24Session.wins[1]); /* XMVSF_STAGE25_SETS */
    }
    xm24Session.resultMenu=true;
    xm24ResultInputCapture=1; /* XMVSF_STAGE24_3_LIVE_RESULTS */
    xm24Session.selected=0;
    xm24Session.lastMenuNavigateTick=0;
    xm24Log(xm24Session.winner?"RESULT_MENU_WIN_CREDITED":"RESULT_MENU_WINNER_UNKNOWN"); /* XMVSF_STAGE24_2_POST_TALLY_UI */
}
static inline bool xm24HandleEvent(const SDL_Event& e) {
    if(!xm24VersusMode)return false;
    // F9 is a safety fallback if the ROM's point-tally phase cannot be recognized.
    if(e.type==SDL_KEYDOWN && !e.key.repeat && e.key.keysym.sym==SDLK_F9) {
        if(!xm24Session.resultMenu && xm24Session.seenCombat) xm24ShowResult(xm24Session.winner);
        return true;
    }
    // The Versus quick menu now handles mapped Coin and keyboard 5 together
    // in xm243PollLiveMenu(), avoiding duplicate toggles on one keypress.
    if(!xm24Session.resultMenu) {
        // Exit any live match to Versus results with Escape (no winner credited).
        if(e.type==SDL_KEYDOWN && !e.key.repeat && e.key.keysym.sym==SDLK_ESCAPE){
            xm24ShowResult(0);return true;
        }
        return false;
    }
    if(xm25State.mainMenu) {
        xm24Session.command=XM24_REBOOT_MAIN; /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
        if(e.type==SDL_KEYDOWN && !e.key.repeat){
            SDL_Keycode k=e.key.keysym.sym;
            if(xm25State.controlsView){if(k==SDLK_ESCAPE || k==SDLK_RETURN)xm25State.controlsView=false;return true;}
            if(k==SDLK_ESCAPE){xm25State.mainMenu=false;xm25State.lobby=true;return true;}
            if(k==SDLK_UP){xm25State.mainRow=(xm25State.mainRow+3)%4;xm25MenuSound(false);} /* XMVSF_STAGE25_2_LEDGER_RESULTS */
            if(k==SDLK_DOWN){xm25State.mainRow=(xm25State.mainRow+1)%4;xm25MenuSound(false);}
            if(k==SDLK_RETURN){
                xm25MenuSound(true);
                if(xm25State.mainRow==0)xm24Session.command=XM24_TO_ARCADE;
                else if(xm25State.mainRow==1)xm24Session.command=XM24_TO_TRAINING;
                else if(xm25State.mainRow==2){xm25State.mainMenu=false;xm25State.lobby=true;}
                else xm25State.controlsView=true;
            }
        }
        return true;
    }
    if(xm25State.quickMenu) {
        if(e.type==SDL_KEYDOWN && !e.key.repeat){
            SDL_Keycode k=e.key.keysym.sym;
            if(k==SDLK_ESCAPE){xm25State.quickMenu=false;xm24Session.resultMenu=false;xm24ResultInputCapture=0;return true;}
            if(k==SDLK_UP || k==SDLK_DOWN){xm24Session.selected=1-xm24Session.selected;xm25MenuSound(false);}
            if(k==SDLK_RETURN || k==SDLK_SPACE){xm25MenuSound(true);
                xm24Session.command=xm24Session.selected==0?XM24_BACK_MENU:XM24_RESELECT;}
        }
        return true;
    } /* XMVSF_STAGE25_1_POLISH */
    if(xm25State.lobby) {
        if(e.type==SDL_TEXTINPUT && xm25State.focus>=0){xm25EditName(xm25State.focus,e.text.text);return true;}
        if(e.type==SDL_KEYDOWN && !e.key.repeat){
            SDL_Keycode k=e.key.keysym.sym;
            if(k==SDLK_BACKSPACE && xm25State.focus>=0){xm25Backspace(xm25State.focus);return true;}
            if(k==SDLK_ESCAPE){xm25State.ledgerView=false;xm25State.setupRow=6;return true;}
            if(xm25State.ledgerView){
                if(k==SDLK_LEFT || k==SDLK_UP)xm25State.page=std::max(0,xm25State.page-1);
                if(k==SDLK_RIGHT || k==SDLK_DOWN)xm25State.page=std::min(xm255MaxPage(),xm25State.page+1); /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
                if(k==SDLK_RETURN)xm25State.ledgerView=false;
            }else{
                if(k==SDLK_UP){xm25State.setupRow=(xm25State.setupRow+6)%7;xm25MenuSound(false);}
                if(k==SDLK_DOWN){xm25State.setupRow=(xm25State.setupRow+1)%7;xm25MenuSound(false);}
                if(k==SDLK_RETURN) {
                    xm25MenuSound(true);
                    if(xm25State.setupRow<5){xm25State.modeIndex=xm25State.setupRow;xm24Session.command=XM24_RESELECT;}
                    else if(xm25State.setupRow==5){xm25LoadLedger();xm25State.ledgerView=true;xm25State.page=0;}
                    else {xm24Session.command=XM24_REBOOT_MAIN;} /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
                }
            }
        }
        if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT){
            // Mouse logical coordinates for a fullscreen game frame.
            int mx=e.button.x,my=e.button.y;
            if(xm25NameMouse(mx,my))return true;
        }
        return true;
    } /* XMVSF_STAGE25_SETS */
    int movement=0;bool accept=false;
    if(e.type==SDL_KEYDOWN && !e.key.repeat) {
        if(e.key.keysym.sym==SDLK_UP || e.key.keysym.sym==SDLK_LEFT)movement=-1;
        if(e.key.keysym.sym==SDLK_DOWN || e.key.keysym.sym==SDLK_RIGHT)movement=1;
        if(e.key.keysym.sym==SDLK_RETURN || e.key.keysym.sym==SDLK_SPACE)accept=true;
        // For timeout or ambiguous results only: user can assign the actual winner.
        if(e.key.keysym.sym==SDLK_1 && !xm24Session.awarded) {
            xm24Session.winner=1;++xm24Session.wins[0];xm24Session.awarded=true;xm25OnAward(xm24Session.wins[0],xm24Session.wins[1]);xm24Log("MANUAL_P1_WIN");
        }
        if(e.key.keysym.sym==SDLK_2 && !xm24Session.awarded) {
            xm24Session.winner=2;++xm24Session.wins[1];xm24Session.awarded=true;xm25OnAward(xm24Session.wins[0],xm24Session.wins[1]);xm24Log("MANUAL_P2_WIN");
        }
    } /* physical controller buttons are handled by XMVSF_STAGE24_3_LIVE_RESULTS polling */
    if(movement) {
        xm25MenuSound(false); /* XMVSF_STAGE25_2_LEDGER_RESULTS */
        // GameController and Joystick APIs may both report one physical D-pad
        // action. Accept one navigation event, never two toggles per press.
        const uint32_t now=SDL_GetTicks();
        if(!xm24Session.lastMenuNavigateTick || now-xm24Session.lastMenuNavigateTick>=120) {
            xm24Session.selected=(xm24Session.selected+1)%3; /* XMVSF_STAGE25_SETS */
            xm24Session.lastMenuNavigateTick=now?now:1;
        }
    }
    if(accept && !xm24Session.command) {
        xm25MenuSound(true);
        xm24Session.command=xm24Session.selected==0?XM24_REMATCH:(xm24Session.selected==1?XM24_RESELECT:XM24_BACK_MENU);
        xm24Log(xm24Session.command==XM24_REMATCH?"REQUEST_REMATCH":"REQUEST_CHARACTER_SELECT");
    }
    // Consume gameplay input while result menu is open, but let window-close pass through.
    return e.type==SDL_KEYDOWN || e.type==SDL_KEYUP || e.type==SDL_CONTROLLERBUTTONDOWN ||
           e.type==SDL_CONTROLLERBUTTONUP || e.type==SDL_JOYBUTTONDOWN ||
           e.type==SDL_JOYBUTTONUP || e.type==SDL_JOYHATMOTION;
}

// Reuses the self-contained Stage23.1 pixel text renderer already used for Training.
static inline void xm24Overlay(SDL_Renderer* renderer,const SDL_Rect& view) {
    extern bool xm258ArcadeMenuOpen;
    if(xm258ArcadeMenuOpen && !xm24VersusMode && !xm14TrainingMode && renderer){
        SDL_BlendMode old=SDL_BLENDMODE_NONE;
        SDL_GetRenderDrawBlendMode(renderer,&old);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
        const int w=view.w*3/4,h=104;
        const int x=view.x+(view.w-w)/2,y=view.y+(view.h-h)/2;
        xm255Rect(renderer,x,y,w,h,xm255Rgb(7,13,32));
        xm255Rect(renderer,x,y,w,h,xm255Rgb(80,137,219),false);
        xm255Text(renderer,"ARCADE OPTIONS",x+12,y+10,w-24,2,xm255Rgb(213,233,255));
        xm255Rect(renderer,x+12,y+30,w-24,1,xm255Rgb(71,115,188));
        xm255Rect(renderer,x+12,y+40,w-24,25,xm255Rgb(31,59,105));
        xm255Rect(renderer,x+12,y+40,4,25,xm255Rgb(114,196,255));
        xm255Text(renderer,"> MAIN MENU",x+20,y+47,w-36,1,xm255Rgb(249,249,255));
        xm255Text(renderer,"A SELECT   B BACK   COIN CLOSE",x+11,y+77,w-22,1,xm255Rgb(154,185,222));
        SDL_SetRenderDrawBlendMode(renderer,old);
        return;
    }
    if(!xm24VersusMode || !xm24Session.resultMenu || !renderer)return;
    SDL_BlendMode old=SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(renderer,&old);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_Rect panel={view.x+view.w/12,view.y+view.h/22,view.w*10/12,view.h*20/22};
    xm255Rect(renderer,panel.x,panel.y,panel.w,panel.h,xm255Rgb(8,13,31));
    xm255Rect(renderer,panel.x,panel.y,panel.w,panel.h,xm255Rgb(83,135,218),false);
    const int x=panel.x+9,y=panel.y+8,w=panel.w-18;
    const XM255RGB fg=xm255Rgb(222,235,255),muted=xm255Rgb(153,178,208);
    if(xm25State.quickMenu){
        xm255Text(renderer,"VERSUS OPTIONS",x,y,w,1,fg);
        xm255Rect(renderer,x,y+13,w,1,xm255Rgb(64,108,179));
        const char* options[2]={"RETURN TO LOBBY","CHARACTER SELECT"};
        for(int i=0;i<2;++i){
            const int yy=y+36+i*37;
            const bool chosen=xm24Session.selected==i;
            xm255Rect(renderer,x,yy-5,w,29,chosen?xm255Rgb(34,58,99):xm255Rgb(17,28,50));
            if(chosen)xm255Rect(renderer,x,yy-5,4,29,xm255Rgb(112,192,255));
            xm255Text(renderer,chosen?">":" ",x+9,yy,w-14,1,fg);
            xm255Text(renderer,options[i],x+26,yy,w-31,1,fg);
        }
        xm255Text(renderer,"COIN CLOSE   UP/DOWN MOVE",x,y+155,w,1,muted);
        xm255Text(renderer,"A SELECT  B BACK",x,y+169,w,1,muted);
    }else if(xm25State.lobby){
        if(xm25State.ledgerView){
            xm255InGameScoreboard(renderer,x,y,w);
        }else{
            xm255Text(renderer,"VERSUS  -  CHOOSE MODE",x,y,w,1,fg);
            xm255Rect(renderer,x,y+13,w,1,xm255Rgb(64,108,179));
            const char* entries[7]={"FFA","FT3","FT5","FT10","FT20","SCOREBOARD","MAIN MENU"};
            for(int i=0;i<7;++i){
                const int yy=y+22+i*21;
                const bool chosen=xm25State.setupRow==i;
                xm255Rect(renderer,x,yy-3,w,18,chosen?xm255Rgb(34,58,99):xm255Rgb(14,24,43));
                if(chosen)xm255Rect(renderer,x,yy-3,3,18,xm255Rgb(106,185,255));
                xm255Text(renderer,chosen?">":" ",x+6,yy,16,1,fg);
                xm255Text(renderer,entries[i],x+24,yy,w-30,1,i<5?xm255ModeColor(xm25TargetAt(i)):fg);
            }
            char names[96];std::snprintf(names,sizeof(names),"P1 %.12s   P2 %.12s",xm25State.names[0],xm25State.names[1]);
            xm255Text(renderer,names,x,y+174,w,1,fg);
            xm255Text(renderer,"UP/DOWN MOVE  A SELECT  B BACK",x,y+186,w,1,muted);
        }
    }else if(xm25State.mainMenu){
        // An older build may leave this transient flag set; the dispatcher
        // immediately restarts instead of exposing nested main-menu navigation.
        xm255Text(renderer,"RETURNING TO MAIN MENU",x,y+65,w,1,fg);
    }else{
        xm255Text(renderer,"MATCH RESULTS",x,y,w,1,fg);
        xm255Text(renderer,xm25ModeName(xm25State.target),x+w-36,y,36,1,xm255ModeColor(xm25State.target));
        xm255Rect(renderer,x,y+13,w,1,xm255Rgb(64,108,179));
        if(xm25State.completed)xm255Text(renderer,"SET COMPLETE",x,y+19,w,1,xm255Rgb(119,220,159));
        xm255Text(renderer,xm25State.names[0],x,y+30,w,1,fg);
        xm255Text(renderer,xm25State.names[1],x,y+83,w,1,fg);
        const int count=xm25State.target?xm25State.target:std::max(5,std::max(xm24Session.wins[0],xm24Session.wins[1])+2);
        xm25StarRow(renderer,x+7,y+48,w-14,xm24Session.wins[0],count);
        xm25StarRow(renderer,x+7,y+101,w-14,xm24Session.wins[1],count);
        const char* options[3]={"REMATCH","CHARACTER SELECT","BACK TO LOBBY"};
        for(int i=0;i<3;++i){
            const int yy=y+128+i*18;
            const bool selected=xm24Session.selected==i;
            xm255Rect(renderer,x,yy-2,w,16,selected?xm255Rgb(37,63,111):xm255Rgb(13,23,40));
            if(selected)xm255Rect(renderer,x,yy-2,3,16,xm255Rgb(101,190,255));
            xm255Text(renderer,selected?">":" ",x+6,yy,18,1,fg);
            xm255Text(renderer,options[i],x+25,yy,w-30,1,fg);
        }
        if(!xm24Session.awarded)xm255Text(renderer,"1 P1 WIN  2 P2 WIN",x,y+184,w,1,muted);
        else xm255Text(renderer,"UP/DOWN MOVE  A SELECT  B BACK",x,y+184,w,1,muted);
    }
    SDL_SetRenderDrawBlendMode(renderer,old);
} /* XMVSF_STAGE25_5_SCOREBOARD_RESET */

#endif

/* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
