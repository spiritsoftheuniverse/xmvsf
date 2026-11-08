#ifndef XMVSF_STAGE87_220_ZERO_DASH_STARTUP_H
#define XMVSF_STAGE87_220_ZERO_DASH_STARTUP_H
// TEST 87.220: eliminate *waiting* in native XMVSF dash animation startup.
// Original 87.47 68000/RAM observations: +A0 high byte 14 = ground dash,
// 16 = possible reverse ground dash, 3C = air dash (0137B2 dispatch).
// Original animation: +34 current ROM frame, +3E live frame countdown.
// Do NOT write 0 to +3E: the native frame runner expects a positive count;
// one means advance at its NEXT native update, with zero extra held frames.
// This is deliberately limited to the first, still-stationary part of
// a real native dash. It never changes X/Y, speed, action, input, or ROM.
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace XM87220Dash {
static inline unsigned byte(const std::uint8_t* r,unsigned a){
    return r[(a&0xffffu)^1u];
}
static inline unsigned word(const std::uint8_t* r,unsigned a){
    return (byte(r,a)<<8u)|byte(r,a+1u);
}
static inline std::uint32_t longword(const std::uint8_t* r,unsigned a){
    return (std::uint32_t(word(r,a))<<16u)|word(r,a+2u);
}
static inline void setWord(std::uint8_t* r,unsigned a,unsigned v){
    r[(a&0xffffu)^1u]=std::uint8_t(v>>8u);
    r[((a+1u)&0xffffu)^1u]=std::uint8_t(v);
}
static inline bool ground(unsigned action){
    return (action&0xff00u)==0x1400u || (action&0xff00u)==0x1600u;
}
static inline bool air(unsigned action){return (action&0xff00u)==0x3c00u;}
static inline bool dash(unsigned action){return ground(action)||air(action);}
static inline bool timerMayAdvance(unsigned timer){
    // 0000 can be a native sentinel; high-bit durations (7FFF) can
    // deliberately hold a frame. Never turn either into a dash skip.
    return timer>1u && timer<0x1000u;
}
struct Actor {
    bool ready=false,tracking=false;
    unsigned mode=0,identity=~0u,entryFrame=0;
    std::uint32_t lastX=0,entryX=0;
};
struct Event {
    bool entered=false,patched=false,moving=false;
    unsigned mode=0,identity=0,timerBefore=0,timerAfter=0;
};
struct Controller {
    Actor actor[2];
    unsigned lastFrame=~0u;
    std::FILE* log=nullptr;
    bool initialized=false;
    ~Controller(){if(log)std::fclose(log);}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* filename=std::getenv("XM87220_DASH_LOG_FILE");
        if(filename && *filename) {
            log=std::fopen(filename,"ab");
            if(log)std::fputs("frame\tplayer\tfighter\tmode\tentry\tx\tvx\tframe_ptr\ttimer_before\ttimer_after\tdecision\n",log);
        }
    }
    void reset(){actor[0]=Actor{};actor[1]=Actor{};}
    Event sample(std::uint8_t* r,unsigned p,unsigned frame,unsigned side){
        Event e;
        Actor& a=actor[side];
        const unsigned identity=word(r,p+0x52u);
        const unsigned mode=word(r,p+0xa0u);
        const unsigned active=word(r,p);
        const unsigned control=word(r,p+0x06u);
        // Original record must be in player control, not hitstun, tag,
        // guard-stun or a DHC host-owned controller transaction.
        if(!active || identity>0x24u || (identity&1u) || control ||
           !dash(mode)){
            a=Actor{};
            return e;
        }
        e.mode=mode;e.identity=identity;
        const std::uint32_t x=longword(r,p+0x0cu);
        const std::uint32_t vx=longword(r,p+0x14u);
        e.entered=!a.ready || a.mode!=mode || a.identity!=identity;
        if(e.entered){
            a.tracking=true;a.entryFrame=frame;a.entryX=x;
        }else if(x!=a.lastX || x!=a.entryX){
            a.tracking=false;
            e.moving=true;
        }
        // For airborne dash only touch the ENTRY animation frame.
        // Ground dash may use several original wind-up frames; advance
        // each held frame until actual X movement starts, then disengage.
        const bool waiting=a.tracking &&
            (e.entered || ground(mode)) &&
            (air(mode) || vx==0u);
        e.timerBefore=word(r,p+0x3eu);
        e.timerAfter=e.timerBefore;
        if(waiting && timerMayAdvance(e.timerBefore)){
            setWord(r,p+0x3eu,1u);
            e.timerAfter=1u;
            e.patched=true;
        }
        a.ready=true;a.mode=mode;a.identity=identity;a.lastX=x;
        if(log && (e.entered || e.patched || e.moving)){
            std::fprintf(log,"%u\t%u\t%04X\t%04X\t%u\t%08X\t%08X\t%08X\t%u\t%u\t%s\n",
                frame,side+1u,identity,mode,e.entered?1u:0u,x,vx,
                longword(r,p+0x34u),e.timerBefore,e.timerAfter,
                e.patched?"NO_STARTUP_WAIT":e.moving?"MOTION_LIVE":"NATIVE_UNCHANGED");
        }
        return e;
    }
    void frame(unsigned frame,std::uint8_t* r,bool xmvsf){
        if(lastFrame==frame)return;
        if(lastFrame!=~0u && frame<lastFrame)reset(); // replay/snapshot reset
        lastFrame=frame;
        if(!xmvsf||!r||byte(r,0x5000u)!=8u){reset();return;}
        init();
        sample(r,0x4000u,frame,0u);
        sample(r,0x4400u,frame,1u);
        if(log && (frame%120u)==0u)std::fflush(log);
    }
};
static inline Controller& state(){static Controller c;return c;}
} // XM87220Dash
#endif
