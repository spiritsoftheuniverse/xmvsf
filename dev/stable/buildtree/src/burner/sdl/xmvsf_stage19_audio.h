// XMVSF Stage19.3: original FBNeo SDL game-audio output remains authoritative.
// The menu owns a TEMPORARY separate SDL device, CLOSED before game startup.
// Presentation PCM is mixed after the existing FBNeo audio callback produces
// its output. We never replace or pause the FBNeo game audio callback.
#ifndef XMVSF_STAGE19_AUDIO_H
#define XMVSF_STAGE19_AUDIO_H
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>

struct XM19Sound {
    SDL_AudioSpec spec;
    Uint8* samples;
    Uint32 byteLength, frames;
    double cursor;
};
struct XM19Audio {
    SDL_AudioDeviceID menuDevice;
    XM19Sound music,cursorSound,confirmSound;
    float gain,target;
    Uint32 cursorIndex,confirmIndex;
    int cursorActive,confirmActive,combatFadeStarted,inGame;
};
static XM19Audio xm19={};
// FBNeo's original SDL legacy device is opened by SDLSoundInit().
extern int nAudSampleRate[8];

static void xm19AudioAssetPath(const char* name,char* output,size_t count) {
    char* base=SDL_GetBasePath();
    if(base) {snprintf(output,count,"%sassets/xmvsf/%s",base,name);SDL_free(base);}
    else snprintf(output,count,"assets/xmvsf/%s",name);
}
static int xm19AudioLoad(XM19Sound* sound,const char* name) {
    char path[1024];xm19AudioAssetPath(name,path,sizeof(path));
    SDL_AudioSpec spec={};Uint8* data=nullptr;Uint32 len=0;
    if(!SDL_LoadWAV(path,&spec,&data,&len)){
        fprintf(stderr,"XMVSF audio: cannot open %s: %s\n",path,SDL_GetError());return 0;
    }
    if(spec.freq!=48000||spec.format!=AUDIO_S16SYS||spec.channels!=2||len<4||(len%4)) {
        fprintf(stderr,"XMVSF audio: PCM16 stereo 48000 required: %s\n",path);
        SDL_FreeWAV(data);return 0;
    }
    sound->spec=spec;sound->samples=data;sound->byteLength=len;sound->frames=len/4;sound->cursor=0;
    return 1;
}
static inline float xm19PCM(const XM19Sound* s,Uint32 frame,unsigned ch) {
    const int16_t* pcm=reinterpret_cast<const int16_t*>(s->samples);
    return pcm[frame*2+ch];
}
// Called from either the menu-only callback (48000 Hz), or AFTER the genuine
// FBNeo SDL callback has filled the legacy game-audio output buffer.
static void xm19MixPCM(Uint8* stream,int bytes,int outputHz) {
    if(!xm19.music.samples||bytes<4||outputHz<=0)return;
    int16_t* out=reinterpret_cast<int16_t*>(stream);
    const unsigned frames=unsigned(bytes)/4;
    XM19Audio* a=&xm19;
    const Uint32 fade=a->music.frames>1024?240:0;
    const double stride=48000.0/double(outputHz);
    for(unsigned i=0;i<frames;i++){
        const float gainStep=1.f/7200.f;
        const float delta=a->target-a->gain;
        if(delta>gainStep)a->gain+=gainStep;
        else if(delta < -gainStep)a->gain-=gainStep;
        else a->gain=a->target;
        const Uint32 frame=Uint32(a->music.cursor);
        const Uint32 next=frame+1<a->music.frames?frame+1:0;
        const float frac=float(a->music.cursor-double(frame));
        for(unsigned ch=0;ch<2;ch++){
            float music=xm19PCM(&a->music,frame,ch)*(1.f-frac)+xm19PCM(&a->music,next,ch)*frac;
            if(fade && a->music.cursor>=double(a->music.frames-fade)){
                const double k=a->music.cursor-double(a->music.frames-fade);
                const Uint32 tpos=Uint32(k);
                const float tfrac=float(k-double(tpos));
                const Uint32 tnext=tpos+1<a->music.frames?tpos+1:tpos;
                const float intro=xm19PCM(&a->music,tpos,ch)*(1.f-tfrac)+xm19PCM(&a->music,tnext,ch)*tfrac;
                const float weight=float((k+1.0)/double(fade));
                music=music*(1.f-weight)+intro*weight;
            }
            float mix=float(out[i*2+ch])+music*a->gain;
            if(a->cursorActive&&a->cursorIndex<a->cursorSound.frames)
                mix+=0.82f*xm19PCM(&a->cursorSound,a->cursorIndex,ch);
            if(a->confirmActive&&a->confirmIndex<a->confirmSound.frames)
                mix+=0.90f*xm19PCM(&a->confirmSound,a->confirmIndex,ch);
            if(mix>32767.f)mix=32767.f;
            if(mix< -32768.f)mix=-32768.f;
            out[i*2+ch]=int16_t(mix);
        }
        a->music.cursor+=stride;
        if(a->music.cursor>=a->music.frames){
            const double reset=double(a->music.frames-fade);
            do{a->music.cursor-=reset;}while(a->music.cursor>=a->music.frames);
        }
        if(a->cursorActive&&++a->cursorIndex>=a->cursorSound.frames)a->cursorActive=0;
        if(a->confirmActive&&++a->confirmIndex>=a->confirmSound.frames)a->confirmActive=0;
    }
}
static void SDLCALL xm19MenuAudioCallback(void*,Uint8* stream,int length){
    memset(stream,0,length);
    xm19MixPCM(stream,length,48000);
}
// Exported function linked from main.cpp into aud_sdl.cpp.
// This does NOT alter the core ring play position or its frame pacing.
void xm19MixPresentation(Uint8* stream,int length){
    if(!xm19.inGame)return;
    xm19MixPCM(stream,length,nAudSampleRate[0]>0?nAudSampleRate[0]:48000);
}
static void xm19AudioShutdown(){
    if(xm19.menuDevice){SDL_CloseAudioDevice(xm19.menuDevice);xm19.menuDevice=0;}
    if(xm19.music.samples)SDL_FreeWAV(xm19.music.samples);
    if(xm19.cursorSound.samples)SDL_FreeWAV(xm19.cursorSound.samples);
    if(xm19.confirmSound.samples)SDL_FreeWAV(xm19.confirmSound.samples);
    memset(&xm19,0,sizeof(xm19));
}
static int xm19AudioStart(){
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){
        fprintf(stderr,"XMVSF menu audio: SDL unavailable: %s\n",SDL_GetError());return 0;
    }
    if(!xm19AudioLoad(&xm19.music,"xmvsfMenu.wav")||
       !xm19AudioLoad(&xm19.cursorSound,"menu_movement1.wav")||
       !xm19AudioLoad(&xm19.confirmSound,"menu_selection1.wav")){
        xm19AudioShutdown();return 0;
    }
    SDL_AudioSpec want={};want.freq=48000;want.format=AUDIO_S16SYS;
    want.channels=2;want.samples=1024;want.callback=xm19MenuAudioCallback;
    SDL_AudioSpec got={};
    xm19.menuDevice=SDL_OpenAudioDevice(nullptr,0,&want,&got,0);
    if(!xm19.menuDevice){fprintf(stderr,"XMVSF menu audio: open failed: %s\n",SDL_GetError());xm19AudioShutdown();return 0;}
    if(got.freq!=48000||got.format!=AUDIO_S16SYS||got.channels!=2){
        fprintf(stderr,"XMVSF menu audio: unexpected obtained format\n");xm19AudioShutdown();return 0;
    }
    xm19.gain=0.5f;xm19.target=0.5f;
    SDL_PauseAudioDevice(xm19.menuDevice,0);
    fprintf(stderr,"XMVSF audio: menu-only SDL device started at 48000Hz\n");
    return 1;
}
static inline void xm19AudioMovement(){
    if(!xm19.menuDevice)return;
    SDL_LockAudioDevice(xm19.menuDevice);
    xm19.cursorIndex=0;xm19.cursorActive=1;
    SDL_UnlockAudioDevice(xm19.menuDevice);
}
static inline void xm19AudioConfirm(){
    if(!xm19.menuDevice)return;
    SDL_LockAudioDevice(xm19.menuDevice);
    xm19.confirmIndex=0;xm19.confirmActive=1;
    SDL_UnlockAudioDevice(xm19.menuDevice);
}
static inline void xm19AudioEnterGame(){
    if(xm19.menuDevice){
        SDL_LockAudioDevice(xm19.menuDevice);
        xm19.target=1.f;
        SDL_UnlockAudioDevice(xm19.menuDevice);
        // Key fix: release this device BEFORE FBNeo calls its original
        // SDL_OpenAudio. Never leave a second device competing for pacing.
        SDL_CloseAudioDevice(xm19.menuDevice);
        xm19.menuDevice=0;
    }
    xm19.inGame=1;
    xm19.combatFadeStarted=0;
    fprintf(stderr,"XMVSF audio: menu device closed; FBNeo opens original game audio next\n");
}
void xm19AudioOnCombat(){
    if(!xm19.inGame)return;
    SDL_LockAudio();
    if(!xm19.combatFadeStarted){xm19.combatFadeStarted=1;xm19.target=0.f;}
    SDL_UnlockAudio();
}
#endif
