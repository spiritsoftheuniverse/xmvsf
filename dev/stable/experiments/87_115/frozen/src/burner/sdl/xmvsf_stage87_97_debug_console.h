#ifndef XMVSF_STAGE87_97_DEBUG_CONSOLE_H
#define XMVSF_STAGE87_97_DEBUG_CONSOLE_H
// Stage87.97: host instrumentation, read-only CPS2 observations; no ROM/RAM edits.
// Shift+D toggles on gameplay SDL event queue; D is blocked as game input only
// while the chord is held. SDL event processing time is NOT physical latency.
#include <SDL.h>
#include "xmvsf_stage87_98_latency_trace.h" // XM87.98 timestamps
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
namespace XM8797Debug {
static const char* const READY="XM87.97 DEBUG_CONSOLE_READY";
static inline double ms(Uint64 a, Uint64 b){
    const Uint64 f=SDL_GetPerformanceFrequency();
    return f&&b>=a?double(b-a)*1000.0/double(f):0.0;
}
struct Rolling {
    double last=0.0, mean=0.0, peak=0.0;
    unsigned long long samples=0;
    void add(double v) {last=v; ++samples;mean+=(v-mean)*0.05;peak=std::max(peak*0.995,v);}
};
struct State {
    bool visible=false;
    bool hotkeyDown=false;
    Uint64 lastEventQpc=0, lastFrameBegin=0, lastInputEnd=0;
    Uint64 frameBeginQpc=0, inputBeginQpc=0, emuBeginQpc=0, paintBeginQpc=0, presentBeginQpc=0;
    Uint64 controllerPollQpc=0, lastControllerPollQpc=0;
    Uint64 mappedEdges=0;
    unsigned lastMappedPlayer=0, lastMappedSub=0;
    bool lastMappedButtons[2][8]={{false}};
    Uint64 eventCount=0, toggles=0, frames=0, presents=0, frameSkips=0;
    std::FILE* csv=nullptr;bool csvChecked=false;
    ~State(){if(csv)std::fclose(csv);}
    void ensureLog(){
        if(csvChecked)return;
        csvChecked=true;
        const char* name=std::getenv("XM8797_DEBUG_LOG_FILE");
        if(name&&*name){csv=std::fopen(name,"wb");if(csv)std::fputs("frame\tvisible\tframe_ms\tframe_avg_ms\tinterval_ms\tinputmake_ms\tvidframe_ms\tpaint_ms\tpresentcall_ms\teventtoinput_unpaired_ms\tpadpoll_ms\tpadpoll_to_map_ms\tbutton_edges\tevent_count\n",csv);}
    }
    Uint64 eventBeforeFrame=0; // monotonically increasing event counter snapshot
    unsigned lastEventSDLms=0, lastEventType=0;
    Rolling eventToInput, inputMake, emulator, paint, presentCall, frameInterval, frameTotal;
    Rolling controllerPollCost, controllerPollToMapped;
    bool eventMatch=false;
    void onEvent(const SDL_Event& e) {
        if(e.type==SDL_KEYDOWN && e.key.keysym.scancode==SDL_SCANCODE_D &&
           (e.key.keysym.mod&KMOD_SHIFT)!=0){
            if(!e.key.repeat && !hotkeyDown){visible=!visible;++toggles;}
            hotkeyDown=true;
            return;
        }
        if(e.type==SDL_KEYUP && e.key.keysym.scancode==SDL_SCANCODE_D){hotkeyDown=false;return;}
        switch(e.type){
        case SDL_KEYDOWN:case SDL_KEYUP:
        case SDL_CONTROLLERBUTTONDOWN:case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERAXISMOTION:
        case SDL_JOYBUTTONDOWN:case SDL_JOYBUTTONUP:case SDL_JOYAXISMOTION:
            lastEventQpc=SDL_GetPerformanceCounter();
            lastEventSDLms=e.common.timestamp;
            lastEventType=e.type;
            ++eventCount;
            break;
        default:break;
        }
    }
    bool consumeHotkey(const SDL_Event& e){
        if(e.type==SDL_KEYDOWN && e.key.keysym.scancode==SDL_SCANCODE_D && (e.key.keysym.mod&KMOD_SHIFT)){onEvent(e);return true;}
        if(e.type==SDL_KEYUP && e.key.keysym.scancode==SDL_SCANCODE_D && hotkeyDown){onEvent(e);return true;}
        onEvent(e);return false;
    }
    void frameStart(){
        ensureLog();
        const Uint64 now=SDL_GetPerformanceCounter();
        if(lastFrameBegin)frameInterval.add(ms(lastFrameBegin,now));
        lastFrameBegin=now;frameBeginQpc=now;eventBeforeFrame=eventCount;
        inputBeginQpc=0;emuBeginQpc=0;paintBeginQpc=0;
        ++frames;
    }
    void beforeInput(){inputBeginQpc=SDL_GetPerformanceCounter();
        // Host event-to-InputMake is UNPAIRED; only show fresh event processing.
        if(lastEventQpc && lastEventQpc<=inputBeginQpc && ms(lastEventQpc,inputBeginQpc)<250.0){
            eventToInput.add(ms(lastEventQpc,inputBeginQpc));eventMatch=true;
        }else eventMatch=false;
    }
    void afterInput(){lastInputEnd=SDL_GetPerformanceCounter();if(inputBeginQpc)inputMake.add(ms(inputBeginQpc,lastInputEnd));}
    void beforeControllerPoll(){controllerPollQpc=SDL_GetPerformanceCounter();}
    void afterControllerPoll(){
        if(controllerPollQpc){controllerPollCost.add(ms(controllerPollQpc,SDL_GetPerformanceCounter()));
            lastControllerPollQpc=SDL_GetPerformanceCounter();}
    }
    void mappedInput(unsigned player,unsigned sub,int value){
        if(player>=2||sub<0x80u||sub>0x87u)return;
        bool& prior=lastMappedButtons[player][sub-0x80u];
        if(value&&!prior){
            ++mappedEdges;lastMappedPlayer=player+1;lastMappedSub=sub;
            const Uint64 now=SDL_GetPerformanceCounter();
            if(lastControllerPollQpc&&now>=lastControllerPollQpc&&ms(lastControllerPollQpc,now)<250.0)
                controllerPollToMapped.add(ms(lastControllerPollQpc,now));
        }
        prior=value!=0;
    }
    void beforeEmu(){emuBeginQpc=SDL_GetPerformanceCounter();}
    void afterEmu(){if(emuBeginQpc)emulator.add(ms(emuBeginQpc,SDL_GetPerformanceCounter()));}
    void beforePaint(){paintBeginQpc=SDL_GetPerformanceCounter();}
    void beforePresent(){const Uint64 now=SDL_GetPerformanceCounter();if(paintBeginQpc)paint.add(ms(paintBeginQpc,now));presentBeginQpc=now;}
    void afterPresent(){if(presentBeginQpc){presentCall.add(ms(presentBeginQpc,SDL_GetPerformanceCounter()));++presents;}}
    void endFrame(){
        if(frameBeginQpc)frameTotal.add(ms(frameBeginQpc,SDL_GetPerformanceCounter()));
        if(csv && (frames%60u)==0u){
            std::fprintf(csv,"%llu\t%u\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%llu\t%llu\n",
             (unsigned long long)frames,visible?1u:0u,frameTotal.last,frameTotal.mean,frameInterval.last,inputMake.last,emulator.last,paint.last,presentCall.last,eventToInput.last,controllerPollCost.last,controllerPollToMapped.last,(unsigned long long)mappedEdges,(unsigned long long)eventCount);
            std::fflush(csv);
        }
    }
};
// Function-local static defined in inline function: one program-wide instance
// under the C++11 ODR across run/input/video translation units.
inline State& state(){static State s;return s;}
inline bool blockChordD(){
    const Uint8* k=SDL_GetKeyboardState(NULL);
    return k && k[SDL_SCANCODE_D] && (state().hotkeyDown || k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]);
}
inline unsigned b8(const std::uint8_t* ram,unsigned cpu){return ram[(cpu&65535u)^1u];}
inline unsigned b16(const std::uint8_t* ram,unsigned cpu){return (b8(ram,cpu)<<8)|b8(ram,cpu+1);}
inline void draw(SDL_Renderer* renderer, SDL_Texture* font, const std::uint8_t* ram,
                 int status, int emulatedFrames, int virtualFps, bool runAhead,
                 void (*print)(SDL_Renderer*,const char*,Uint32,Uint32)) {
    State& st=state();
    if(!st.visible||!renderer||!font||!print)return;
    int w=0,h=0;SDL_GetRendererOutputSize(renderer,&w,&h);
    if(w<520||h<280)return;
    const int width=std::min(850,w-16), height=std::min(394,h-16);
    SDL_BlendMode oldBlend=SDL_BLENDMODE_NONE;SDL_GetRenderDrawBlendMode(renderer,&oldBlend);
    Uint8 cr=0,cg=0,cb=0,ca=0;SDL_GetRenderDrawColor(renderer,&cr,&cg,&cb,&ca);
    Uint8 fr=255,fg=255,fb=255;SDL_GetTextureColorMod(font,&fr,&fg,&fb);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,8,15,27,232);
    SDL_Rect bg={8,8,width,height};SDL_RenderFillRect(renderer,&bg);
    SDL_SetRenderDrawColor(renderer,64,151,219,255);SDL_RenderDrawRect(renderer,&bg);
    SDL_SetTextureColorMod(font,107,213,255);
    print(renderer,"XMVSF ENGINE DIAGNOSTICS 87.97  | SHIFT+D CLOSE",20,18);
    SDL_SetTextureColorMod(font,229,237,248);
    char line[256];unsigned y=35;
    #define XM97_LINE(...) do{std::snprintf(line,sizeof(line),__VA_ARGS__);print(renderer,line,20,y);y+=15;}while(0)
    const double hz=st.frameInterval.mean>0.0?1000.0/st.frameInterval.mean:0.0;
    const double target=virtualFps?100000.0/virtualFps:0.0; // FBNeo nAppVirtualFps fixed-point x100
    const char* mode=std::getenv("XM8755_TOURNAMENT_MODE");
    XM97_LINE("PHASE %02X  MODE %s  EMU %d  PRESENTS %llu  RUNAHEAD %s",unsigned(status<0?255:status),mode&&mode[0]=='1'?"TOURNAMENT":"CLASSIC",emulatedFrames,(unsigned long long)st.presents,runAhead?"ON":"OFF");
    XM97_LINE("FRAME RATE %.2f Hz  TARGET %.3f ms  FRAME INTERVAL %.3f ms",hz,target,st.frameInterval.last);
    XM97_LINE("FRAME MAIN %6.3f ms (avg %6.3f, peak %6.3f)",st.frameTotal.last,st.frameTotal.mean,st.frameTotal.peak);
    XM97_LINE("INPUT MAKE %6.3f ms (avg %6.3f) | VIDFRAME %6.3f ms",st.inputMake.last,st.inputMake.mean,st.emulator.last);
    XM97_LINE("PAINT/COPY %6.3f ms | SDL PRESENT CALL %6.3f ms",st.paint.last,st.presentCall.last);
    XM97_LINE("SDL PAD POLL %.3f ms | POLL -> MAPPED EDGE %.3f ms",st.controllerPollCost.last,st.controllerPollToMapped.last);
    XM97_LINE("INPUT EDGES %llu  LAST P%u CODE %02X",(unsigned long long)st.mappedEdges,st.lastMappedPlayer,st.lastMappedSub);
    const XM8798Latency::State& lat=XM8798Latency::state();
    XM97_LINE("DF %s | CPS DRAW %.3f ms | OBJECT %.3f ms PASSES %u",XM8798Latency::df()?"ON":"OFF",lat.lastDrawMs,lat.lastObjectMs,lat.objectPasses);
    XM97_LINE("LAST INPUT CANDIDATE: P%u %02X -> ACTION %04X, %u FRAMES %.3f ms",lat.lastResponsePlayer,lat.lastResponseSub,lat.lastResponseAction,lat.lastResponseFrames,lat.lastSampleToActionMs);
    XM97_LINE("INPUT SAMPLE -> SDL PRESENT RETURN %.3f ms (NOT PHOTONS)",lat.lastSampleToSubmitMs);
    XM97_LINE("LAST EVENT -> INPUT SAMPLE %6.3f ms [%s, UNPAIRED]",st.eventToInput.last,st.eventMatch?"FRESH":"STALE");
    XM97_LINE("EVENT COUNT %llu   EVENT TYPE 0x%04X   SDL TIMESTAMP %u",(unsigned long long)st.eventCount,st.lastEventType,st.lastEventSDLms);
    y+=7;SDL_SetTextureColorMod(font,107,213,255);XM97_LINE("CPS-2 NATIVE FIGHTER OBSERVATIONS  (READ ONLY)");SDL_SetTextureColorMod(font,229,237,248);
    if(ram&&status==8){
      for(unsigned p=0;p<2;++p){
        const unsigned base=p?0x4400u:0x4000u;
        XM97_LINE("P%u ACTION %04X SUB %04X HP %u  X %u Y %u",p+1,b16(ram,base+0x60u),b16(ram,base+6u),b16(ram,base+0x210u),b16(ram,base+0x0eu),b16(ram,base+0x10u));
        XM97_LINE("   STUN LEFT %u | DIZZY %02X | BOX-GATE %02X | INVUL %04X",
             b8(ram,base+0xb8u),b8(ram,base+0x135u),b8(ram,base+0x83u),b16(ram,base+0xf6u));
        XM97_LINE("   ATTACK BOX %04X  DEFENSE BOX %04X  CONTACT %02X  HIT-LATCH %02X",
             b16(ram,base+0x70u),b16(ram,base+0x74u),b8(ram,base+0x87u),b8(ram,base+0x84u));
      }
    }else XM97_LINE("Fighter diagnostics available in combat (phase 08).");
    y+=7;SDL_SetTextureColorMod(font,107,213,255);XM97_LINE("MEASUREMENT BOUNDARIES / WARNINGS");SDL_SetTextureColorMod(font,229,237,248);
    XM97_LINE("EVENT -> INPUT uses host dequeue time, not physical button press.");
    XM97_LINE("PAD POLL -> MAPPED EDGE is internal host time, not input latency.");
    XM97_LINE("PRESENT is call duration, NOT photon/display latency or queue age.");
    XM97_LINE("Input->visual causal latency requires per-button native ROM acknowledgement.");
    XM97_LINE("Debug overlay costs GPU/CPU time; close for baseline measurements.");
    #undef XM97_LINE
    SDL_SetTextureColorMod(font,fr,fg,fb);
    SDL_SetRenderDrawBlendMode(renderer,oldBlend);SDL_SetRenderDrawColor(renderer,cr,cg,cb,ca);
}
} // namespace XM8797Debug
#endif
