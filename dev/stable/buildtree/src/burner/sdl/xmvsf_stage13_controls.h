#include "xmvsf_stage25_4_input_handoff.h" /* XMVSF_STAGE25_4_INPUT_HANDOFF */
#include "xmvsf_stage25_6_training_guard.h" /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
// XMVSF Stage 13: explicit two-player SDL joystick mapping, independent of
// device enumeration order. Used by boot menu and existing SDL input backend.
#ifndef XMVSF_STAGE13_CONTROLS_H
#define XMVSF_STAGE13_CONTROLS_H
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Six CPS2 attacks, two three-button macros, Coin/Start, and four directions.
#define XM13_ACTIONS 14
enum { XM13_LP=0,XM13_MP,XM13_HP,XM13_LK,XM13_MK,XM13_HK,
       XM13_3P,XM13_3K,XM13_COIN,XM13_START,XM13_LEFT,XM13_RIGHT,XM13_UP,XM13_DOWN };
#define XM13_DEADZONE 9000
struct XM13_Binding { char type; int index; }; // B: button, A: positive axis, K: SDL scancode, N: none
struct XM13_Player {
    int active;
    char guid[33];
    int occurrence; // 0-based ordinal among identical GUIDs; -1 for keyboard
    XM13_Binding binding[XM13_ACTIONS]; // LP MP HP LK MK HK 3P 3K COIN START LEFT RIGHT UP DOWN
};
struct XM13_Settings { XM13_Player player[2]; };

static void xm13Default(XM13_Settings* s) {
    memset(s,0,sizeof(*s));
    for(int p=0;p<2;p++) {
        s->player[p].occurrence=0;
        for(int a=0;a<XM13_ACTIONS;a++) {
            s->player[p].binding[a].type='N';
            s->player[p].binding[a].index=-1;
        }
    }
}
static int xm13ValidGuid(const char* g) {
    if (strlen(g)!=32) return 0;
    for(int i=0;i<32;i++) if(!((g[i]>='0'&&g[i]<='9') || (g[i]>='a'&&g[i]<='f') || (g[i]>='A'&&g[i]<='F'))) return 0;
    return 1;
}
static inline bool xm13IsKeyboard(const XM13_Player* p) {
    return p && p->active && strcmp(p->guid,"KEYBOARD")==0;
}
static int xm13Load(XM13_Settings* s) {
    xm13Default(s);
    FILE* f=fopen("xmvsf_controller_map.ini","r");
    if(!f)return 0;
    char line[1024]={};
    if(!fgets(line,sizeof(line),f)){fclose(f);return 0;}
    // The original eight-action file remains loadable. Its Coin/Start
    // entries migrate to indexes 8/9, leaving 3P/3K and directions empty.
    const bool oldFormat=strncmp(line,"XMVSF_CONTROLLERS_1",19)==0;
    if(!oldFormat && strncmp(line,"XMVSF_CONTROLLERS_2",19)!=0){fclose(f);return 0;}
    const int oldIndices[8]={0,1,2,3,4,5,8,9};
    for(int p=0;p<2;p++){
        if(!fgets(line,sizeof(line),f))break;
        char* tok=strtok(line," \t\r\n");
        int number=0;
        if(!tok || sscanf(tok,"P%d",&number)!=1 || number!=p+1)continue;
        tok=strtok(NULL," \t\r\n");
        if(!tok)continue;
        char guid[40]={};snprintf(guid,sizeof(guid),"%s",tok);
        const bool keyboard=!strcmp(guid,"KEYBOARD") && !oldFormat;
        if(!keyboard && (!xm13ValidGuid(guid) ||
           !strcmp(guid,"00000000000000000000000000000000")))continue;
        tok=strtok(NULL," \t\r\n");
        if(!tok)continue;
        const int occurrence=atoi(tok);
        if((keyboard && occurrence!=-1) ||
           (!keyboard && (occurrence<0||occurrence>7)))continue;
        XM13_Player* player=&s->player[p];
        snprintf(player->guid,sizeof(player->guid),"%s",guid);
        player->occurrence=occurrence;
        player->active=1;
        for(int a=0;a<(oldFormat?8:XM13_ACTIONS);a++){
            tok=strtok(NULL," \t\r\n");
            char type='N';int ix=-1;
            if(!tok || sscanf(tok,"%c:%d",&type,&ix)!=2 ||
               (type!='B'&&type!='A'&&type!='K'&&type!='N') ||
               ix < -1 || ix >= (type=='K'?SDL_NUM_SCANCODES:128) ||
               (oldFormat && type=='K')){
                player->active=0;break;
            }
            const int idx=oldFormat?oldIndices[a]:a;
            player->binding[idx].type=type;
            player->binding[idx].index=ix;
        }
    }
    fclose(f);
    return 1;
}
static int xm13Save(const XM13_Settings* s) {
    FILE* f=fopen("xmvsf_controller_map.ini.tmp","w");
    if(!f)return 0;
    fputs("XMVSF_CONTROLLERS_2\n",f);
    for(int p=0;p<2;p++) {
        const XM13_Player* v=&s->player[p];
        fprintf(f,"P%d %s %d",p+1,v->active?v->guid:"00000000000000000000000000000000",v->occurrence);
        for(int a=0;a<XM13_ACTIONS;a++)fprintf(f," %c:%d",v->binding[a].type,v->binding[a].index);
        fputc('\n',f);
    }
    if(fclose(f)!=0) {remove("xmvsf_controller_map.ini.tmp");return 0;}
    // Windows rename() cannot replace an existing file. Preserve backup first.
    remove("xmvsf_controller_map.ini.bak");
    rename("xmvsf_controller_map.ini","xmvsf_controller_map.ini.bak");
    if(rename("xmvsf_controller_map.ini.tmp","xmvsf_controller_map.ini")!=0) {
        rename("xmvsf_controller_map.ini.bak","xmvsf_controller_map.ini"); return 0;
    }
    return 1;
}
static void xm13GuidForDevice(int device,char out[33]) {
    SDL_JoystickGUID g=SDL_JoystickGetDeviceGUID(device);
    SDL_JoystickGetGUIDString(g,out,33);
}
static int xm13DeviceOrdinal(int device) {
    char current[33];xm13GuidForDevice(device,current);
    int count=0;
    for(int i=0;i<device;i++) {
        char other[33];xm13GuidForDevice(i,other);
        if(strcmp(current,other)==0)count++;
    }
    return count;
}
static int xm13FindDevice(const XM13_Player* p) {
    if(!p->active)return -1;
    int ordinal=0;
    for(int i=0;i<SDL_NumJoysticks();i++) {
        char candidate[33];xm13GuidForDevice(i,candidate);
        if(strcmp(p->guid,candidate)==0) {
            if(ordinal==p->occurrence)return i;
            ordinal++;
        }
    }
    return -1; // disconnected devices must NOT control somebody else's player
}
// The label is read live from SDL (which queries the OS driver), never
// persisted as an identity. GUID + occurrence remains the assignment key.
static void xm13DeviceDisplayName(int device,char* out,size_t capacity) {
    if(!out || capacity==0)return;
    out[0]=0;
    if(device<0 || device>=SDL_NumJoysticks())return;
    const char* name=SDL_JoystickNameForIndex(device);
    if((!name || !*name) && SDL_IsGameController(device))
        name=SDL_GameControllerNameForIndex(device);
    if(!name || !*name)name="UNKNOWN CONTROLLER";
    // Keep an intelligible SDL/OS name even for drivers with punctuation
    // outside the menu bitmap glyph atlas. Never display a made-up pad ID.
    size_t i=0;
    for(;*name && i+1<capacity;++name){
        const unsigned char ch=(unsigned char)*name;
        if(ch>=32 && ch<127)out[i++]=(char)ch;
        else if(i && out[i-1]!=' ')out[i++]=' ';
    }
    out[i]=0;
}
static void xm13SetDefaultButtons(XM13_Player* p,int device){
    // Only actual CPS2 buttons get default assignments. Three-punch and
    // three-kick macros intentionally begin unbound.
    const SDL_GameControllerButton names[8]={
        SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START
    };
    const int slots[8]={XM13_LP,XM13_MP,XM13_HP,XM13_LK,XM13_MK,XM13_HK,XM13_COIN,XM13_START};
    const int fallback[8]={2,3,4,0,1,5,6,7};
    SDL_GameController* gc=SDL_IsGameController(device)?SDL_GameControllerOpen(device):NULL;
    for(int a=0;a<8;a++){
        XM13_Binding* b=&p->binding[slots[a]];
        b->type='B';b->index=fallback[a];
        if(gc){
            const SDL_GameControllerButtonBind input=SDL_GameControllerGetBindForButton(gc,names[a]);
            if(input.bindType==SDL_CONTROLLER_BINDTYPE_BUTTON)b->index=input.value.button;
            else if(input.bindType==SDL_CONTROLLER_BINDTYPE_AXIS && input.value.axis>=0){
                b->type='A';b->index=input.value.axis;
            }
        }
    }
    if(gc)SDL_GameControllerClose(gc);
}
static int xm13AssignKeyboard(XM13_Settings* settings,int player){
    if(!settings || player<0 || player>1)return 0;
    XM13_Settings updated=*settings;
    XM13_Player* p=&updated.player[player];
    const bool alreadyKeyboard=xm13IsKeyboard(p);
    p->active=1;
    snprintf(p->guid,sizeof(p->guid),"KEYBOARD");
    p->occurrence=-1;
    if(!alreadyKeyboard){
        for(int i=0;i<XM13_ACTIONS;i++){p->binding[i].type='N';p->binding[i].index=-1;}
        // Two non-overlapping starter layouts, freely editable in Options.
        const int keys1[XM13_ACTIONS]={
            SDL_SCANCODE_J,SDL_SCANCODE_K,SDL_SCANCODE_L,
            SDL_SCANCODE_U,SDL_SCANCODE_I,SDL_SCANCODE_O,
            SDL_SCANCODE_H,SDL_SCANCODE_Y,SDL_SCANCODE_5,SDL_SCANCODE_RETURN,
            SDL_SCANCODE_A,SDL_SCANCODE_D,SDL_SCANCODE_W,SDL_SCANCODE_S
        };
        const int keys2[XM13_ACTIONS]={
            SDL_SCANCODE_KP_1,SDL_SCANCODE_KP_2,SDL_SCANCODE_KP_3,
            SDL_SCANCODE_KP_4,SDL_SCANCODE_KP_5,SDL_SCANCODE_KP_6,
            SDL_SCANCODE_KP_7,SDL_SCANCODE_KP_8,SDL_SCANCODE_KP_0,SDL_SCANCODE_KP_ENTER,
            SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN
        };
        const int* defaults=player==0?keys1:keys2;
        for(int i=0;i<XM13_ACTIONS;i++){p->binding[i].type='K';p->binding[i].index=defaults[i];}
    }
    if(!xm13Save(&updated))return 0;
    *settings=updated;
    return 1;
}
static int xm13Assign(XM13_Settings* settings,int player,int device) {
    if(player<0||player>1||device<0||device>=SDL_NumJoysticks())return 0;
    char guid[33];xm13GuidForDevice(device,guid);
    if(!xm13ValidGuid(guid))return 0;
    int occ=xm13DeviceOrdinal(device);
    const int other=1-player;
    // Reassigning a pad owned by the opposite slot swaps the COMPLETE
    // player mappings (device identity AND customized button bindings).
    // If the destination was empty, the previous owner becomes unassigned.
    // Commit to the active configuration only after the atomic ini save.
    XM13_Settings updated=*settings;
    if(updated.player[other].active && !strcmp(updated.player[other].guid,guid)
       && updated.player[other].occurrence==occ) {
        const XM13_Player prior=updated.player[player];
        updated.player[player]=updated.player[other];
        updated.player[other]=prior;
    } else {
        XM13_Player* p=&updated.player[player];
        p->active=1;snprintf(p->guid,sizeof(p->guid),"%s",guid);p->occurrence=occ;
        xm13SetDefaultButtons(p,device);
    }
    if(!xm13Save(&updated))return 0;
    *settings=updated;
    return 1;
}
extern int xm24ResultInputCapture; /* XMVSF_STAGE24_3_LIVE_RESULTS */
static int xm13BindingPressed(const XM13_Binding& binding,SDL_Joystick* joy){
    if(binding.type=='K'){
        const Uint8* keys=SDL_GetKeyboardState(nullptr);
        return keys && binding.index>=0 && binding.index<SDL_NUM_SCANCODES &&
               keys[binding.index]!=0;
    }
    if(!joy)return 0;
    if(binding.type=='B' && binding.index>=0 &&
       binding.index<SDL_JoystickNumButtons(joy))
        return SDL_JoystickGetButton(joy,binding.index)!=0;
    if(binding.type=='A' && binding.index>=0 &&
       binding.index<SDL_JoystickNumAxes(joy))
        return SDL_JoystickGetAxis(joy,binding.index)>16000;
    return 0;
}
static int xm13ReadInput(const XM13_Settings* s,int player,int sub,
                         SDL_Joystick** raw,SDL_GameController** gc,int count){
    if(xm24ResultInputCapture)return 0;
    if(xm25InputHandoffBlock())return 0;
    extern int xm24VersusMode;
    if(xm24VersusMode && sub==0x86)return 0;
    if(player<0 || player>1 || !s->player[player].active)return -1;
    const XM13_Player* p=&s->player[player];
    const int device=xm13IsKeyboard(p)?-1:xm13FindDevice(p);
    SDL_Joystick* joy=device>=0 && device<count && raw[device] &&
        SDL_JoystickGetAttached(raw[device])?raw[device]:nullptr;
    SDL_GameController* pad=joy?gc[device]:nullptr;
    if(sub>=0 && sub<=3){
        const int idx=XM13_LEFT+sub;
        const bool bound=xm13BindingPressed(p->binding[idx],joy)!=0;
        if(!joy)return bound;
        const int dx=SDL_JoystickNumAxes(joy)>0?SDL_JoystickGetAxis(joy,0):0;
        const int dy=SDL_JoystickNumAxes(joy)>1?SDL_JoystickGetAxis(joy,1):0;
        int dpad=0;
        if(pad){
            const SDL_GameControllerButton button=(SDL_GameControllerButton)
                (sub==0?SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                 sub==1?SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                 sub==2?SDL_CONTROLLER_BUTTON_DPAD_UP:SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            dpad=SDL_GameControllerGetButton(pad,button)!=0;
        }
        if(SDL_JoystickNumHats(joy)>0){
            const Uint8 hat=SDL_JoystickGetHat(joy,0);
            const int mask=sub==0?SDL_HAT_LEFT:sub==1?SDL_HAT_RIGHT:
                           sub==2?SDL_HAT_UP:SDL_HAT_DOWN;
            dpad=dpad || (hat&mask)!=0;
        }
        const bool axis=sub==0?dx<-XM13_DEADZONE:
                        sub==1?dx>XM13_DEADZONE:
                        sub==2?dy<-XM13_DEADZONE:dy>XM13_DEADZONE;
        return bound || dpad || axis;
    }
    if(sub>=0x80 && sub<=0x87){
        const int rawAction=sub-0x80;
        const int action=rawAction<6?rawAction:
                         rawAction==6?XM13_COIN:XM13_START;
        const int pressed=xm13BindingPressed(p->binding[action],joy);
        if(rawAction<3)
            return pressed || xm13BindingPressed(p->binding[XM13_3P],joy);
        if(rawAction<6)
            return pressed || xm13BindingPressed(p->binding[XM13_3K],joy);
        return pressed;
    }
    return 0;
}
#endif
