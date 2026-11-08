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
#include <cstring> // bounded native-neutral actor snapshot for catastrophic dash recovery

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
static inline void setLong(std::uint8_t* r,unsigned a,std::uint32_t v){
    setWord(r,a,v>>16u);setWord(r,a+2u,v);
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
// Authentic pre-dash neutral controller; no invented idle script pointers.
struct Neutral {
    bool valid=false;
    unsigned identity=~0u,frame=0,step06=0,step08=0,timer=0,anim=0,mode=0,action=0,air=0;
    std::uint32_t script=0,next=0;
};
struct Actor {
    Neutral neutral;
    bool ready=false,tracking=false;
    // TEST 87.226: last stage-plausible native X (87.225 capture leapt
    // from ~0x02B00000 to 0x7041D600 before an endless 37f dash loop).
    bool safeValid=false,recoveryActive=false,neutralSnapshotValid=false;
    unsigned mode=0,identity=~0u,entryFrame=0,dashAge=0;
    // 87.232: old unconditional 36f watchdog killed an actually moving
    // native dash at capture frame 6570.
    unsigned stalledFrames=0,scriptLoopsAtOrigin=0;
    std::uint32_t lastScript=0;
    unsigned safeFrame=0,lastRecoveryFrame=~0u,quietFrames=0;
    unsigned suppressedFrames=0,snapshotFrame=0,snapshotRestores=0;
    std::uint32_t lastX=0,entryX=0,safeX=0;
    // Only the actor's 0x000..0x0FF movement/controller/animation slice.
    // Health/meter live outside this slice and must NOT rewind. Snapshot
    // only an actual safe native idle from this same fighter.
    std::uint8_t neutralBytes[0x100u]={};
};
struct Event {
    bool entered=false,patched=false,moving=false;
    unsigned mode=0,identity=0,timerBefore=0,timerAfter=0;
};
struct Controller {
    Actor actor[2];
    unsigned lastFrame=~0u;
    bool haveRuleMode=false,lastTournament=false;
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
    // Only a demonstrably broken dash may use this controller restoration.
    // Script, action, pose and mode all come from this fighter's own
    // previously captured native neutral, never an invented idle pointer.
    static void restoreNeutral(std::uint8_t* r,unsigned p,Actor& a,bool restoreX){
        const Neutral& n=a.neutral;
        setWord(r,p+0x06u,n.step06);
        setWord(r,p+0x08u,n.step08);
        setLong(r,p+0x14u,0u);
        setLong(r,p+0x18u,0u);
        if(restoreX && a.safeValid)setLong(r,p+0x0cu,a.safeX);
        setLong(r,p+0x34u,n.script);
        setLong(r,p+0x38u,n.next);
        setWord(r,p+0x3eu,n.timer?n.timer:1u);
        setWord(r,p+0x42u,n.anim);
        setWord(r,p+0x60u,n.action);
        setWord(r,p+0xa0u,n.mode);
        setWord(r,p+0xaau,n.air);
        a.ready=false;a.tracking=false;a.dashAge=0;
        a.stalledFrames=0;a.scriptLoopsAtOrigin=0;a.lastScript=0;
    }
    static bool restoreNativeNeutral(std::uint8_t* r,unsigned p,Actor& a,
                                     unsigned frame){
        if(!a.neutralSnapshotValid || !a.safeValid ||
           frame<a.snapshotFrame || frame-a.snapshotFrame>600u ||
           word(r,p+0x52u)!=a.neutral.identity)return false;
        // The 87.226 fix restored only 9 selected fields; native 68000
        // reasserted a bad 0x704E7800 X + 0x1400 dash for 497 frames.
        // Recover the *original CPU-owned* idle controller slice instead,
        // including any related dash/animation phase variables. Avoid any
        // snapshot/rewind of health, meter, or opponent state.
        std::memcpy(r+p,a.neutralBytes,sizeof(a.neutralBytes));
        setLong(r,p+0x0cu,a.safeX);
        setLong(r,p+0x14u,0u);
        setLong(r,p+0x18u,0u);
        a.ready=false;a.tracking=false;a.dashAge=0u;
        a.stalledFrames=0;a.scriptLoopsAtOrigin=0;a.lastScript=0;
        ++a.snapshotRestores;
        return true;
    }
    Event sample(std::uint8_t* r,unsigned p,unsigned frame,unsigned side){
        Event e;
        Actor& a=actor[side];
        const unsigned identity=word(r,p+0x52u);
        const unsigned mode=word(r,p+0xa0u);
        const unsigned active=word(r,p);
        const unsigned control=word(r,p+0x06u);
        const std::uint32_t x=longword(r,p+0x0cu);
        // Original record must be in player control, not hitstun, tag,
        // guard-stun or a DHC host-owned controller transaction.
        if(!active || identity>0x24u || (identity&1u)){
            a=Actor{};return e;
        }
        if(a.neutral.valid && a.neutral.identity!=identity)a=Actor{};
        // Save the most recent physically plausible stage coordinate;
        // the 87.225 flying-screen dash had X≈0x70000000, or >28k
        // world pixels, whereas legitimate observed combat was <0x03000000.
        const bool saneX=x>=0x00010000u && x<0x10000000u;
        if(saneX && !a.recoveryActive && control==0u){
            a.safeValid=true;a.safeX=x;a.safeFrame=frame;
        }
        const bool brokenX=!saneX && x>=0x10000000u;
        const bool canRecover=a.neutral.valid &&
            a.neutral.identity==identity && a.neutral.script && a.neutral.next;
        // The 87.221 36-frame watchdog exited 12 consecutive native dashes
        // in the 87.225 log. Each re-entered within 1f; suppressing further
        // dash state is needed until the native CPU stays OUT of dash for
        // three consecutive full frames.
        if(a.recoveryActive){
            if(!ground(mode)){
                if(++a.quietFrames>=3u){
                    a.recoveryActive=false;
                    a.lastRecoveryFrame=~0u;
                    a.quietFrames=0u;
                    a.suppressedFrames=0u;
                    a.snapshotRestores=0u;
                }
            }else if(!control && canRecover){
                ++a.suppressedFrames;
                // Escalate only AFTER several consecutive reentries and
                // only after confirming that we have the actual ROM-native
                // neutral controller snapshot for this SAME fighter.
                // Retry at 16-frame intervals if the ROM still regenerates
                // the corrupted position. This is not a normal dash cancel.
                const bool recoverSnapshot=a.suppressedFrames>=4u &&
                    ((a.suppressedFrames-4u)%16u)==0u &&
                    restoreNativeNeutral(r,p,a,frame);
                if(!recoverSnapshot)restoreNeutral(r,p,a,true);
                if(log && (recoverSnapshot || a.suppressedFrames==1u ||
                           a.suppressedFrames%64u==0u)){
                    std::fprintf(log,"%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\t%s\n",
                        frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                        a.neutral.script,a.neutral.timer,a.neutral.timer,
                        recoverSnapshot?"FS_DASH_NATIVE_CONTROLLER_RESTORED":
                          "FS_DASH_REENTRY_SUPPRESSED");
                    std::fflush(log);
                }
                return e;
            }
        }
        if(ground(mode) && !control && canRecover &&
           a.safeValid && frame>=a.safeFrame && frame-a.safeFrame<=600u &&
           brokenX){
            a.recoveryActive=true;a.quietFrames=0u;
            a.suppressedFrames=0u;
            a.snapshotRestores=0u;
            a.lastRecoveryFrame=frame;
            restoreNeutral(r,p,a,true);
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_DASH_INVALID_WORLD_X_RECOVERED\n",
                    frame,side+1u,identity,mode,x,0u,
                    a.neutral.script,a.neutral.timer,a.neutral.timer);
                std::fflush(log);
            }
            return e;
        }
        // A second ROM-ground dash immediately after watchdog recovery is
        // the characteristic FS loop, not a fresh independently queued dash.
        if(ground(mode) && !control && canRecover &&
           a.lastRecoveryFrame!=~0u && frame>=a.lastRecoveryFrame &&
           frame-a.lastRecoveryFrame<=5u){
            a.recoveryActive=true;a.quietFrames=0u;
            a.suppressedFrames=0u;
            a.snapshotRestores=0u;
            restoreNeutral(r,p,a,a.safeValid);
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_DASH_REPEAT_AFTER_WATCHDOG\n",
                    frame,side+1u,identity,mode,x,0u,
                    a.neutral.script,a.neutral.timer,a.neutral.timer);
                std::fflush(log);
            }
            return e;
        }
        // Retain ROM-owned neutral animation and action from this character.
        if(!control && (mode&0xff00u)==0u &&
           (word(r,p+0xaau)>>8u)==0u && longword(r,p+0x14u)==0u){
            Neutral& n=a.neutral;
            n.valid=true;n.identity=identity;n.frame=frame;
            n.step06=word(r,p+0x06u);n.step08=word(r,p+0x08u);
            n.timer=word(r,p+0x3eu);n.anim=word(r,p+0x42u);
            n.mode=mode;n.action=word(r,p+0x60u);n.air=word(r,p+0xaau);
            n.script=longword(r,p+0x34u);n.next=longword(r,p+0x38u);
            if(saneX && n.script && n.next){
                std::memcpy(a.neutralBytes,r+p,sizeof(a.neutralBytes));
                a.neutralSnapshotValid=true;
                a.snapshotFrame=frame;
            }
        }
        if(control || !dash(mode)){
            a.ready=false;a.tracking=false;a.dashAge=0;
            a.stalledFrames=0;a.scriptLoopsAtOrigin=0;a.lastScript=0;
            return e;
        }
        e.mode=mode;e.identity=identity;
        const std::uint32_t vx=longword(r,p+0x14u);
        e.entered=!a.ready || a.mode!=mode || a.identity!=identity;
        const std::uint32_t script=longword(r,p+0x34u);
        if(e.entered){
            a.tracking=true;a.entryFrame=frame;a.entryX=x;
            a.dashAge=0;
            a.stalledFrames=0;a.scriptLoopsAtOrigin=0;
            a.lastScript=script;
        }else{
            const bool changedX=x!=a.lastX;
            if(changedX){
                a.tracking=false;
                e.moving=true;
                a.stalledFrames=0;
            }else if(a.stalledFrames<1000u)++a.stalledFrames;
            // Script wraps are only suspicious if the actor also loops
            // back near its starting world coordinate. A continuously
            // travelling fighter can legitimately wrap animation frames.
            const std::int64_t relative=
                std::int64_t(std::int32_t(x))-
                std::int64_t(std::int32_t(a.entryX));
            const bool returnedToOrigin=relative>=-0x00100000LL &&
                                        relative<= 0x00100000LL;
            if(script<a.lastScript && returnedToOrigin &&
               a.scriptLoopsAtOrigin<1000u)++a.scriptLoopsAtOrigin;
            a.lastScript=script;
        }
        // A valid 37+ frame native return dash must finish naturally.
        // Exit only a demonstrably motionless or repeated-origin loop.
        const bool confirmedDashLoop=a.stalledFrames>=36u ||
            (a.scriptLoopsAtOrigin>=2u && a.dashAge>=36u);
        if(ground(mode) && confirmedDashLoop && canRecover &&
           frame>=a.neutral.frame && frame-a.neutral.frame<=600u){
            const Neutral n=a.neutral;
            // Do not preempt a normal native ROM dash sooner than 36f.
            restoreNeutral(r,p,a,false);
            a.lastRecoveryFrame=frame;
            if(log){
                std::fprintf(log,"%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_DASH_LOOP_EXIT\n",
                    frame,side+1u,identity,mode,x,vx,n.script,n.timer,n.timer);
                std::fflush(log);
            }
            return e;
        }
        if(a.dashAge<1000u)++a.dashAge;
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
    // An emulation/Classic/Tournament transition is a session boundary.
    // Keep 87.220 zero-wait startup and 87.221 dash recovery in BOTH
    // modes. A stale native neutral script never crosses a mode switch.
    void frame(unsigned frame,std::uint8_t* r,bool xmvsf,bool tournament=true){
        if(lastFrame==frame && haveRuleMode && lastTournament==tournament)return;
        const bool rewound=lastFrame!=~0u && frame<lastFrame;
        if(rewound)reset(); // replay/snapshot or fresh DrvInit
        lastFrame=frame;
        if(!xmvsf||!r||byte(r,0x5000u)!=8u){
            reset();haveRuleMode=false;return;
        }
        init();
        if(!haveRuleMode || tournament!=lastTournament || rewound){
            reset();haveRuleMode=true;lastTournament=tournament;
        }
        sample(r,0x4000u,frame,0u);
        sample(r,0x4400u,frame,1u);
        if(log && (frame%120u)==0u)std::fflush(log);
    }
};
static inline Controller& state(){static Controller c;return c;}
} // XM87220Dash
#endif
