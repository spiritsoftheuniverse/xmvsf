#include "xmvsf_stage25_4_input_handoff.h" /* XMVSF_STAGE25_4_INPUT_HANDOFF */
#include "xmvsf_stage25_6_training_guard.h" /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
// XMVSF Stage 13: explicit two-player SDL joystick mapping, independent of
// device enumeration order. Used by boot menu and existing SDL input backend.
#ifndef XMVSF_STAGE13_CONTROLS_H
#define XMVSF_STAGE13_CONTROLS_H
#include <SDL.h>
#include "xmvsf_stage249_controller_policy.h"
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
    char deviceName[96]; // system name, used only if the hardware GUID changes
    int nameOccurrence; // separate identical-name controllers by their ordinal
    XM13_Binding binding[XM13_ACTIONS]; // LP MP HP LK MK HK 3P 3K COIN START LEFT RIGHT UP DOWN
};
#define XM13_MAX_PROFILES 24
struct XM13_Settings {
    XM13_Player player[2];
    XM13_Player profiles[XM13_MAX_PROFILES]; // remembers devices no longer assigned
    int profileCount;
};

static void xm13Default(XM13_Settings* s) {
    memset(s,0,sizeof(*s));
    for(int p=0;p<2;p++) {
        s->player[p].occurrence=0;
        s->player[p].nameOccurrence=-1;
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
// A saved hardware name is a recovery key, not a replacement for the
// precise GUID. The ordinal disambiguates multiple devices of one name.
// Legacy controller maps are imported and upgraded without losing bindings.
static void xm13DeviceDisplayName(int device,char* out,size_t capacity);
static int xm13NameOrdinal(int device);
static int xm13FindDevice(const XM13_Player* p);
static int xm13Save(const XM13_Settings* s);

static inline bool xm13HasName(const XM13_Player* p) {
    return p && p->deviceName[0]!=0 && strcmp(p->deviceName,"UNKNOWN CONTROLLER")!=0;
}
static bool xm13SameDevice(const XM13_Player* saved,const XM13_Player* requested) {
    if(!saved || !requested || !saved->active || !requested->active)return false;
    if(!strcmp(saved->guid,requested->guid) &&
       saved->occurrence==requested->occurrence)return true;
    return xm13HasName(saved) && xm13HasName(requested) &&
           !strcmp(saved->deviceName,requested->deviceName) &&
           saved->nameOccurrence==requested->nameOccurrence;
}
static void xm13RememberProfile(XM13_Settings* s,const XM13_Player* p) {
    if(!s || !p || !p->active || xm13IsKeyboard(p))return;
    int index=-1;
    // Prefer a precise hardware identity; fall back to name+ordinal
    // when Windows/SDL presents the same pad through a different driver.
    for(int i=0;i<s->profileCount;i++) {
        if(!strcmp(s->profiles[i].guid,p->guid) &&
           s->profiles[i].occurrence==p->occurrence){index=i;break;}
    }
    if(index<0 && xm13HasName(p))for(int i=0;i<s->profileCount;i++) {
        if(xm13HasName(&s->profiles[i]) &&
           !strcmp(s->profiles[i].deviceName,p->deviceName) &&
           s->profiles[i].nameOccurrence==p->nameOccurrence){index=i;break;}
    }
    if(index<0){
        if(s->profileCount>=XM13_MAX_PROFILES)return;
        index=s->profileCount++;
    }
    s->profiles[index]=*p;
}
static void xm13NameToHex(const char* name,char* dst,size_t capacity) {
    static const char alphabet[]="0123456789ABCDEF";
    if(!dst || !capacity)return;
    size_t i=0;
    if(name)for(;*name && i+2<capacity;++name){
        const unsigned char ch=(unsigned char)*name;
        dst[i++]=alphabet[ch>>4];dst[i++]=alphabet[ch&15];
    }
    if(!i && capacity>1)dst[i++]='-';
    dst[i]=0;
}
static int xm13FromHex(char c){
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
static bool xm13HexToName(const char* hex,char* out,size_t size){
    if(!out || !size || !hex)return false;
    out[0]=0;
    if(!strcmp(hex,"-"))return true;
    const size_t length=strlen(hex);
    if(length%2 || length/2>=size)return false;
    for(size_t i=0;i<length/2;i++){
        const int high=xm13FromHex(hex[i*2]);
        const int low=xm13FromHex(hex[i*2+1]);
        if(high<0 || low<0)return false;
        const unsigned char value=(unsigned char)((high<<4)|low);
        if(value<32 || value>=127)return false;
        out[i]=(char)value;
    }
    out[length/2]=0;
    return true;
}
static bool xm13ParseRecord(char* line,XM13_Player* dst,int version){
    if(!line || !dst)return false;
    XM13_Player parsed={};
    parsed.nameOccurrence=-1;
    for(int a=0;a<XM13_ACTIONS;a++){
        parsed.binding[a].type='N';parsed.binding[a].index=-1;
    }
    char* tok=strtok(line," \t\r\n"); // initial P1/P2/D label
    if(!tok)return false;
    tok=strtok(nullptr," \t\r\n");
    if(!tok || strlen(tok)>=sizeof(parsed.guid))return false;
    snprintf(parsed.guid,sizeof(parsed.guid),"%s",tok);
    if(strcmp(parsed.guid,"KEYBOARD") &&
       (!xm13ValidGuid(parsed.guid) ||
       !strcmp(parsed.guid,"00000000000000000000000000000000")))return false;
    if(version==1 && !strcmp(parsed.guid,"KEYBOARD"))return false;
    tok=strtok(nullptr," \t\r\n");
    if(!tok)return false;
    char extra=0;int ordinal=0;
    if(sscanf(tok,"%d%c",&ordinal,&extra)!=1)return false;
    if(!strcmp(parsed.guid,"KEYBOARD")){
        if(ordinal!=-1)return false;
    }else if(ordinal<0 || ordinal>7)return false;
    parsed.occurrence=ordinal;
    parsed.active=1;
    if(version>=3){
        tok=strtok(nullptr," \t\r\n");
        if(!tok || !xm13HexToName(tok,parsed.deviceName,sizeof(parsed.deviceName)))return false;
        tok=strtok(nullptr," \t\r\n");
        if(!tok || sscanf(tok,"%d%c",&parsed.nameOccurrence,&extra)!=1 ||
           parsed.nameOccurrence<-1 || parsed.nameOccurrence>7)return false;
    }
    const int oldSlots[8]={0,1,2,3,4,5,8,9};
    for(int a=0;a<(version==1?8:XM13_ACTIONS);a++){
        tok=strtok(nullptr," \t\r\n");
        char kind='N';int index=-1;char trailing=0;
        if(!tok || sscanf(tok,"%c:%d%c",&kind,&index,&trailing)!=2 ||
           (kind!='B' && kind!='A' && kind!='K' && kind!='N') ||
           (version==1 && kind=='K') ||
           index < -1 || index >= (kind=='K'?SDL_NUM_SCANCODES:128))return false;
        const int slot=version==1?oldSlots[a]:a;
        parsed.binding[slot].type=kind;
        parsed.binding[slot].index=index;
    }
    *dst=parsed;
    return true;
}
static int xm13Load(XM13_Settings* s){
    xm13Default(s);
    FILE* f=fopen("xmvsf_controller_map.ini","r");
    if(!f)return 0;
    char line[1024]={};
    if(!fgets(line,sizeof(line),f)){fclose(f);return 0;}
    const int version=strncmp(line,"XMVSF_CONTROLLERS_1",19)==0?1:
                      strncmp(line,"XMVSF_CONTROLLERS_2",19)==0?2:
                      strncmp(line,"XMVSF_CONTROLLERS_3",19)==0?3:0;
    if(!version){fclose(f);return 0;}
    for(int p=0;p<2;p++){
        if(!fgets(line,sizeof(line),f))break;
        char expected[3]={'P',(char)('1'+p),0};
        if(strncmp(line,expected,2)!=0 || (line[2]!=' ' && line[2]!='\t'))continue;
        xm13ParseRecord(line,&s->player[p],version);
    }
    if(version>=3)while(s->profileCount<XM13_MAX_PROFILES &&
                         fgets(line,sizeof(line),f)){
        if(strncmp(line,"D ",2)!=0)continue;
        XM13_Player profile={};
        if(xm13ParseRecord(line,&profile,version))
            xm13RememberProfile(s,&profile);
    }
    fclose(f);
    bool recoveredName=false;
    // Old map migration: fill in system names for any attached saved GUID.
    // Offline devices retain their original GUID until the name is known.
    for(int p=0;p<2;p++){
        XM13_Player* player=&s->player[p];
        if(!player->active || xm13IsKeyboard(player) || xm13HasName(player))continue;
        const int found=xm13FindDevice(player);
        if(found<0)continue;
        xm13DeviceDisplayName(found,player->deviceName,sizeof(player->deviceName));
        player->nameOccurrence=xm13NameOrdinal(found);
        if(xm13HasName(player))recoveredName=true;
    }
    // One-time upgrade when an old mapping and its hardware are present.
    if(recoveredName || version<3){
        XM13_Settings snapshot=*s;
        for(int p=0;p<2;p++)xm13RememberProfile(&snapshot,&snapshot.player[p]);
        if(recoveredName && xm13Save(&snapshot))*s=snapshot;
    }
    return 1;
}
static void xm13WriteRecord(FILE* f,const char* marker,const XM13_Player* p){
    char nameHex[sizeof(p->deviceName)*2+2]={};
    xm13NameToHex(p->deviceName,nameHex,sizeof(nameHex));
    fprintf(f,"%s %s %d %s %d",marker,p->active?p->guid:
        "00000000000000000000000000000000",p->occurrence,
        nameHex,p->nameOccurrence);
    for(int a=0;a<XM13_ACTIONS;a++)
        fprintf(f," %c:%d",p->binding[a].type,p->binding[a].index);
    fputc('\n',f);
}
static int xm13Save(const XM13_Settings* s){
    if(!s)return 0;
    XM13_Settings snapshot=*s;
    for(int p=0;p<2;p++)xm13RememberProfile(&snapshot,&snapshot.player[p]);
    FILE* f=fopen("xmvsf_controller_map.ini.tmp","w");
    if(!f)return 0;
    fputs("XMVSF_CONTROLLERS_3\n",f);
    for(int p=0;p<2;p++){
        char label[3]={'P',(char)('1'+p),0};
        xm13WriteRecord(f,label,&snapshot.player[p]);
    }
    for(int i=0;i<snapshot.profileCount;i++)
        xm13WriteRecord(f,"D",&snapshot.profiles[i]);
    if(fclose(f)!=0){remove("xmvsf_controller_map.ini.tmp");return 0;}
    remove("xmvsf_controller_map.ini.bak");
    rename("xmvsf_controller_map.ini","xmvsf_controller_map.ini.bak");
    if(rename("xmvsf_controller_map.ini.tmp","xmvsf_controller_map.ini")!=0){
        rename("xmvsf_controller_map.ini.bak","xmvsf_controller_map.ini");return 0;
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
    if(!p || !p->active || xm13IsKeyboard(p))return -1;
    // Exact GUID + occurrence wins whenever it is available.
    int guidOrdinal=0;
    for(int i=0;i<SDL_NumJoysticks();i++) {
        char guid[33];xm13GuidForDevice(i,guid);
        if(strcmp(p->guid,guid)==0) {
            if(guidOrdinal==p->occurrence)return i;
            ++guidOrdinal;
        }
    }
    // Driver updates can change GUID. Restore the saved controls if the
    // system-reported name and ordinal still match this physical device.
    if(xm13HasName(p) && p->nameOccurrence>=0) {
        int nameOrdinal=0;
        for(int i=0;i<SDL_NumJoysticks();i++){
            char current[96]={};
            xm13DeviceDisplayName(i,current,sizeof(current));
            if(strcmp(p->deviceName,current)==0){
                if(nameOrdinal==p->nameOccurrence)return i;
                ++nameOrdinal;
            }
        }
    }
    return -1;
}
// TEST 87.249: the boot menu receives controller events whether or not
// that hardware has a saved mapping. Native gameplay must honor the actual
// pad that launched the match. This is an EPHEMERAL session override: never
// rewrite the user's persisted controller profile simply for pressing A.
struct XM249SessionPad {
    bool selected=false;
    SDL_JoystickID instance=-1;
    char guid[33]={};
    char name[96]={};
};
extern XM249SessionPad xm249SessionPad[2];
static inline int xm249FindSessionDevice(int player) {
    if(player<0 || player>1 || !xm249SessionPad[player].selected)return -1;
    XM249SessionPad& owner=xm249SessionPad[player];
    const int count=SDL_NumJoysticks();
    // A live instance always beats saved GUID/ordinal: DS4Windows can
    // expose a physical DS4 AND a virtual XInput pad simultaneously.
    for(int i=0;i<count;++i)
        if(SDL_JoystickGetDeviceInstanceID(i)==owner.instance)return i;
    const SDL_JoystickID other=xm249SessionPad[1-player].instance;
    for(int i=0;i<count;++i) {
        if(SDL_JoystickGetDeviceInstanceID(i)==other)continue;
        char guid[33]={};
        xm13GuidForDevice(i,guid);
        if(owner.guid[0] && !strcmp(owner.guid,guid)) {
            owner.instance=SDL_JoystickGetDeviceInstanceID(i);
            return i;
        }
        if(owner.name[0]){
            char name[96]={};
            xm13DeviceDisplayName(i,name,sizeof(name));
            if(!strcmp(owner.name,name)) {
                owner.instance=SDL_JoystickGetDeviceInstanceID(i);
                return i;
            }
        }
    }
    return -1;
}

// Runtime-only ownership survives SDL reindexing when two pads have an
// identical GUID/name and the first one is unplugged. Never persist SDL
// instance IDs: they change across reboot and Bluetooth reconnection.
struct XM248RuntimeOwner {
    bool configured=false;
    char guid[33]={};
    int occurrence=0;
    char deviceName[96]={};
    int nameOccurrence=-1;
    SDL_JoystickID instance=-1;
};
extern XM248RuntimeOwner xm248RuntimeOwner[2];

static inline int xm248FindAssignedDevice(const XM13_Settings* settings,int player) {
    if(!settings || player<0 || player>1)return -1;
    const XM13_Player& p=settings->player[player];
    // The face button that started the match explicitly chose this pad.
    // It must work even when options.ini is unassigned or names a Fighter Pi.
    const int fromMenu=xm249FindSessionDevice(player);
    if(fromMenu>=0)return fromMenu;
    XM248RuntimeOwner& saved=xm248RuntimeOwner[player];
    if(!p.active || xm13IsKeyboard(&p)){
        // Switching this slot to keyboard/unassigned immediately releases
        // its old physical instance for the other player.
        saved=XM248RuntimeOwner{};
        return -1;
    }
    const bool same=saved.configured &&
        saved.occurrence==p.occurrence &&
        saved.nameOccurrence==p.nameOccurrence &&
        strcmp(saved.guid,p.guid)==0 &&
        strcmp(saved.deviceName,p.deviceName)==0;
    if(!same){
        saved=XM248RuntimeOwner{};
        saved.configured=true;
        snprintf(saved.guid,sizeof(saved.guid),"%s",p.guid);
        saved.occurrence=p.occurrence;
        snprintf(saved.deviceName,sizeof(saved.deviceName),"%s",p.deviceName);
        saved.nameOccurrence=p.nameOccurrence;
    }
    // First prefer the existing physical device even if its SDL index or
    // ordinal changed because a different but identical pad disconnected.
    const int count=SDL_NumJoysticks();
    if(saved.instance>=0)for(int i=0;i<count;++i)
        if(SDL_JoystickGetDeviceInstanceID(i)==saved.instance)return i;
    const int other=1-player;
    const SDL_JoystickID otherInstance=xm248RuntimeOwner[other].instance;
    const bool otherLive=otherInstance>=0 &&
        [&](){
            for(int i=0;i<count;++i)
                if(SDL_JoystickGetDeviceInstanceID(i)==otherInstance)return true;
            return false;
        }();
    auto unclaimed=[&](int index)->bool{
        return index>=0 && index<count &&
            !(otherLive && SDL_JoystickGetDeviceInstanceID(index)==otherInstance);
    };
    // Precise saved identity is still the preferred first-connection rule.
    const int exact=xm13FindDevice(&p);
    if(unclaimed(exact)){
        saved.instance=SDL_JoystickGetDeviceInstanceID(exact);
        return exact;
    }
    // On reconnect the ordinal of two identical devices may swap. Prefer
    // the unclaimed matching hardware instead of stealing the other
    // player's currently attached instance.
    for(int i=0;i<count;++i){
        if(!unclaimed(i))continue;
        char guid[33];xm13GuidForDevice(i,guid);
        if(strcmp(p.guid,guid)==0){
            saved.instance=SDL_JoystickGetDeviceInstanceID(i);
            return i;
        }
        if(xm13HasName(&p)){
            char name[96]={};xm13DeviceDisplayName(i,name,sizeof(name));
            if(strcmp(p.deviceName,name)==0){
                saved.instance=SDL_JoystickGetDeviceInstanceID(i);
                return i;
            }
        }
    }
    return -1;
}

// Read the exact name reported through SDL/OS. Display it live and persist
// a sanitized copy as a fallback profile key; GUID remains the first choice.
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
static int xm13NameOrdinal(int device){
    if(device<0 || device>=SDL_NumJoysticks())return -1;
    char candidate[96]={};
    xm13DeviceDisplayName(device,candidate,sizeof(candidate));
    if(!candidate[0] || !strcmp(candidate,"UNKNOWN CONTROLLER"))return -1;
    int ordinal=0;
    for(int i=0;i<device;i++){
        char prior[96]={};
        xm13DeviceDisplayName(i,prior,sizeof(prior));
        if(!strcmp(candidate,prior))++ordinal;
    }
    return ordinal;
}
static XM13_Player xm13IdentityForDevice(int device){
    XM13_Player result={};
    if(device<0 || device>=SDL_NumJoysticks())return result;
    result.active=1;
    xm13GuidForDevice(device,result.guid);
    result.occurrence=xm13DeviceOrdinal(device);
    xm13DeviceDisplayName(device,result.deviceName,sizeof(result.deviceName));
    result.nameOccurrence=xm13NameOrdinal(device);
    return result;
}
// Capture exactly the SDL instance which generated a menu confirm.
// Controller-event and raw-joystick-event IDs are both SDL instance IDs.
static inline bool xm249SelectSessionPad(int player,SDL_JoystickID id) {
    if(player<0 || player>1 || id<0)return false;
    const int count=SDL_NumJoysticks();
    int found=-1;
    for(int i=0;i<count;++i)
        if(SDL_JoystickGetDeviceInstanceID(i)==id){found=i;break;}
    if(found<0)return false;
    // A native arcade board (e.g. Fighter Pi) already works through the
    // legacy raw joystick path; leave that path and its button order alone.
    // This automatic menu->game selection is for SDL GameControllers
    // (Xbox/XInput and DS4Windows virtual gamepads), whose menu accepts
    // semantic A but whose game route previously required a saved profile.
    if(!SDL_IsGameController(found))return false;
    XM249SessionPad& p=xm249SessionPad[player];
    p=XM249SessionPad{};
    p.selected=true;p.instance=id;
    xm13GuidForDevice(found,p.guid);
    xm13DeviceDisplayName(found,p.name,sizeof(p.name));
    fprintf(stderr,
        "XM87.249 MENU_CONTROLLER_SELECTED p=%d instance=%d device=%d name=%s\n",
        player+1,(int)id,found,p.name);
    return true;
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
    if(!alreadyKeyboard)xm13RememberProfile(&updated,p);
    p->deviceName[0]=0;
    p->nameOccurrence=-1;
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
static int xm13Assign(XM13_Settings* settings,int player,int device){
    if(!settings || player<0 || player>1 || device<0 ||
       device>=SDL_NumJoysticks())return 0;
    const XM13_Player wanted=xm13IdentityForDevice(device);
    if(!xm13ValidGuid(wanted.guid))return 0;
    XM13_Settings updated=*settings;
    const int other=1-player;
    // Swaps retain customized mappings for BOTH users, not only pad IDs.
    if(xm13SameDevice(&updated.player[other],&wanted)){
        const XM13_Player previous=updated.player[player];
        updated.player[player]=updated.player[other];
        updated.player[other]=previous;
        // A device can come back with a new GUID after a driver change.
        snprintf(updated.player[player].guid,sizeof(updated.player[player].guid),"%s",wanted.guid);
        updated.player[player].occurrence=wanted.occurrence;
        snprintf(updated.player[player].deviceName,sizeof(updated.player[player].deviceName),
                 "%s",wanted.deviceName);
        updated.player[player].nameOccurrence=wanted.nameOccurrence;
    }else{
        XM13_Player* target=&updated.player[player];
        const bool keepBindings=xm13SameDevice(target,&wanted);
        if(!keepBindings){
            // Retain the outgoing device's profile even if it is unplugged.
            xm13RememberProfile(&updated,target);
            int saved=-1;
            for(int i=0;i<updated.profileCount;i++){
                if(!strcmp(updated.profiles[i].guid,wanted.guid) &&
                   updated.profiles[i].occurrence==wanted.occurrence){
                    saved=i;break;
                }
            }
            if(saved<0 && xm13HasName(&wanted))for(int i=0;i<updated.profileCount;i++){
                if(xm13HasName(&updated.profiles[i]) &&
                   !strcmp(updated.profiles[i].deviceName,wanted.deviceName) &&
                   updated.profiles[i].nameOccurrence==wanted.nameOccurrence){
                    saved=i;break;
                }
            }
            if(saved>=0)*target=updated.profiles[saved];
            else {
                XM13_Player clean={};
                for(int a=0;a<XM13_ACTIONS;a++){
                    clean.binding[a].type='N';clean.binding[a].index=-1;
                }
                *target=clean;
                xm13SetDefaultButtons(target,device);
            }
        }
        target->active=1;
        snprintf(target->guid,sizeof(target->guid),"%s",wanted.guid);
        target->occurrence=wanted.occurrence;
        snprintf(target->deviceName,sizeof(target->deviceName),"%s",wanted.deviceName);
        target->nameOccurrence=wanted.nameOccurrence;
    }
    xm13RememberProfile(&updated,&updated.player[player]);
    if(!xm13Save(&updated))return 0;
    *settings=updated;
    return 1;
}
// TEST 87.248: shared identity-based SDL handle reconciliation for the
// boot, Training, Arcade and Versus menus. An SDL enumeration index is
// NOT stable across disconnection; its instance ID is. Keep saved user
// mappings separate from these ephemeral runtime handles.
static inline bool xm248ReconcileMenuPad(
        int device, SDL_Joystick*& stick, SDL_GameController*& pad,
        SDL_JoystickID& instance) {
    const SDL_JoystickID wanted=(device>=0 && device<SDL_NumJoysticks())?
        SDL_JoystickGetDeviceInstanceID(device):-1;
    const bool live=stick && SDL_JoystickGetAttached(stick) &&
        instance==wanted && SDL_JoystickInstanceID(stick)==wanted;
    bool changed=false;
    if(!live){
        if(pad){SDL_GameControllerClose(pad);pad=nullptr;changed=true;}
        if(stick){SDL_JoystickClose(stick);stick=nullptr;changed=true;}
        instance=-1;
    }else if(pad && (!SDL_GameControllerGetAttached(pad) ||
                     !SDL_IsGameController(device))){
        SDL_GameControllerClose(pad);pad=nullptr;changed=true;
    }
    if(wanted<0)return changed;
    if(!stick){
        SDL_Joystick* candidate=SDL_JoystickOpen(device);
        if(!candidate)return changed; // failed open: next poll will retry
        if(!SDL_JoystickGetAttached(candidate)){
            SDL_JoystickClose(candidate);return changed;
        }
        stick=candidate;
        instance=SDL_JoystickInstanceID(candidate);
        changed=true;
    }
    if(stick && !pad && SDL_IsGameController(device)){
        pad=SDL_GameControllerOpen(device);
        if(pad)changed=true;
    }
    return changed;
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
// These SDL GameController buttons are SEMANTIC, unlike a joystick's
// device-specific raw button numbers. Xbox XInput and DS4Windows' virtual
// Xbox device can use completely different raw indices. For a menu-selected
// pad with no matching saved profile, use the same semantic layout that
// navigated the boot UI (but DO NOT alter custom saved bindings).
static inline int xm249DefaultGameAction(SDL_GameController* pad,
                                          SDL_Joystick* joy,int action) {
    if(!joy || !SDL_JoystickGetAttached(joy))return 0;
    const SDL_GameControllerButton names[8]={
        SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START
    };
    const int fallback[8]={2,3,4,0,1,5,6,7};
    if(action<0 || action>=8)return 0;
    if(pad && SDL_GameControllerGetAttached(pad))
        return SDL_GameControllerGetButton(pad,names[action])!=0;
    // Non-GameController SDL joysticks retain the existing defaults.
    return fallback[action]<SDL_JoystickNumButtons(joy) &&
           SDL_JoystickGetButton(joy,fallback[action])!=0;
}
static int xm13ReadInput(const XM13_Settings* s,int player,int sub,
                         SDL_Joystick** raw,SDL_GameController** gc,int count){
    if(xm24ResultInputCapture)return 0;
    if(xm25InputHandoffBlock())return 0;
    extern int xm24VersusMode;
    if(xm24VersusMode && sub==0x86)return 0;
    if(!s || player<0 || player>1)return -1;
    const XM13_Player* p=&s->player[player];
    const bool savedActive=p->active!=0;
    const bool fromMenu=xm249SessionPad[player].selected;
    if(!XM249Policy::needsVirtualMap(savedActive,fromMenu))return -1;
    const int device=xm248FindAssignedDevice(s,player);
    SDL_Joystick* joy=device>=0 && device<count && raw[device] &&
        SDL_JoystickGetAttached(raw[device])?raw[device]:nullptr;
    SDL_GameController* pad=joy && gc?gc[device]:nullptr;
    // A saved Fighter Pi profile is not a valid mapping for an Xbox pad
    // which actually selected Arcade. Use standardized GC buttons if the
    // current device differs from the saved profile. Same-device profiles
    // retain every user's custom binding.
    const bool useSessionDefaults=joy && XM249Policy::useSemanticPad(
        fromMenu,savedActive,xm13IsKeyboard(p),xm13FindDevice(p)==device);
    const bool allowSaved=!useSessionDefaults || xm13IsKeyboard(p);
    if(sub>=0 && sub<=3){
        const int idx=XM13_LEFT+sub;
        const bool bound=allowSaved &&
            xm13BindingPressed(p->binding[idx],joy)!=0;
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
        const int saved=allowSaved &&
            xm13BindingPressed(p->binding[action],joy);
        const int semantic=useSessionDefaults &&
            xm249DefaultGameAction(pad,joy,rawAction);
        const int pressed=XM249Policy::deliver(saved!=0,semantic!=0,
                                               useSessionDefaults);
        if(rawAction<3)
            return pressed ||
                (allowSaved && xm13BindingPressed(p->binding[XM13_3P],joy));
        if(rawAction<6)
            return pressed ||
                (allowSaved && xm13BindingPressed(p->binding[XM13_3K],joy));
        return pressed;
    }
    return 0;
}
#endif
