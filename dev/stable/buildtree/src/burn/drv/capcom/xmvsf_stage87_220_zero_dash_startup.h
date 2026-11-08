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
#include "xmvsf_stage87_233_airborne_entry.h" // 87.233 original 68k pre-ground entry gate

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
static inline bool plausibleWorldX(std::uint32_t x){
    const std::int64_t s=std::int64_t(std::int32_t(x));
    // A small negative world X is valid for an offscreen entrant.
    // Reject multi-thousand-pixel PC 0109B4 corruptions (e.g. B27C0700).
    return s>=-0x02000000LL && s<0x10000000LL;
}
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
    XM87233AirReturn::Tracker airReturn; // independent of dash watchdog
    bool ready=false,tracking=false;
    // TEST 87.226: last stage-plausible native X (87.225 capture leapt
    // from ~0x02B00000 to 0x7041D600 before an endless 37f dash loop).
    bool safeValid=false,recoveryActive=false,neutralSnapshotValid=false;
    // 87.232: distinguish CPU-scripted offscreen entry from a voluntary
    // dash. The scripted entry needs its original frame staging, but an
    // ordinary player dash retains the 87.220 zero-wait modification.
    bool autoReturn=false,returnAnchorValid=false;
    unsigned failedReturnFrames=0,advancingReturnFrames=0,returnMode=0;
    std::uint32_t returnAnchor=0;
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
    // Grounding is sourced from the ORIGINAL fighter's +254 floor, never
    // from an invented Y or a copied opponent controller. +AA's low byte
    // can contain other native state; clear only the documented air byte.
    static bool settleReturnOnNativeFloor(std::uint8_t* r,unsigned p){
        const unsigned floor=word(r,p+0x254u);
        if(!floor || floor>=2048u)return false;
        setWord(r,p+0x10u,floor);
        setWord(r,p+0x12u,0u);
        setLong(r,p+0x1cu,0u);
        setWord(r,p+0xaau,word(r,p+0xaau)&0x00ffu);
        return true;
    }
    // A real visible opponent gives us a grounded world-space reference
    // without guessing the level camera or the stage's absolute left/right
    // limits. 384 CPS2 native horizontal pixels plus 64 of sprite clearance
    // puts a returning fighter definitively outside the opponent's view.
    // No valid opponent, native floor or established prior side => fail
    // closed and retain the older safe controller restoration.
    static bool nativeOffscreenAnchor(const std::uint8_t* r,unsigned p,
                                       const Actor& a,std::uint32_t& anchor){
        if(!a.safeValid || !word(r,p+0x254u) ||
           word(r,p+0x254u)>=2048u)return false;
        const unsigned other=p==0x4000u?0x4400u:0x4000u;
        if(!word(r,other) || word(r,other+0x52u)>0x24u)return false;
        const std::uint32_t otherX=longword(r,other+0x0cu);
        if(!plausibleWorldX(otherX))return false;
        const std::int64_t me=std::int64_t(std::int32_t(a.safeX));
        const std::int64_t them=std::int64_t(std::int32_t(otherX));
        if(me==them)return false;
        const std::int64_t nativeScreen=384LL*65536LL;
        const std::int64_t clearance=64LL*65536LL;
        const std::int64_t candidate=them+
            (me<them?-(nativeScreen+clearance):(nativeScreen+clearance));
        if(candidate<-0x02000000LL || candidate>=0x10000000LL)return false;
        anchor=std::uint32_t(std::int32_t(candidate));
        return true;
    }
    // Already offscreen, but with a SANE native world X: preserve the ROM's
    // chosen entry X exactly. We only need to stop the sky-fall and protect
    // the scripted startup interval from the voluntary zero-wait patch.
    static bool outsideOpponentView(const std::uint8_t* r,unsigned p,
                                    std::uint32_t x){
        const unsigned other=p==0x4000u?0x4400u:0x4000u;
        if(!word(r,other) || word(r,other+0x52u)>0x24u)return false;
        const std::uint32_t otherX=longword(r,other+0x0cu);
        if(!plausibleWorldX(x) || !plausibleWorldX(otherX))return false;
        const std::int64_t delta=std::int64_t(std::int32_t(x))-
            std::int64_t(std::int32_t(otherX));
        return delta>=0x01c00000LL || delta<=-0x01c00000LL;
    }
    // A native ground-dash mode and its ROM script remain in place: the
    // engine, not a host-generated velocity, supplies actual dash motion.
    static bool stageNativeGroundEntry(std::uint8_t* r,unsigned p,Actor& a){
        std::uint32_t anchor=0;
        if(!nativeOffscreenAnchor(r,p,a,anchor))return false;
        if(!settleReturnOnNativeFloor(r,p))return false;
        setLong(r,p+0x0cu,anchor);
        a.returnAnchor=anchor;a.returnAnchorValid=true;
        a.returnMode=word(r,p+0xa0u);
        a.autoReturn=true;
        a.ready=false;a.tracking=false;a.dashAge=0u;
        a.stalledFrames=0u;a.scriptLoopsAtOrigin=0u;a.lastScript=0u;
        return true;
    }
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
        const bool saneX=plausibleWorldX(x) && x!=0u;
        if(saneX && !a.recoveryActive && !a.autoReturn && control==0u){
            a.safeValid=true;a.safeX=x;a.safeFrame=frame;
        }
        const bool brokenX=!plausibleWorldX(x);
        const bool canRecover=a.neutral.valid &&
            a.neutral.identity==identity && a.neutral.script && a.neutral.next;
        // 87.233: an offscreen airborne fighter cannot reach 87.232's
        // ground-mode interceptor until the ORIGINAL 68k has put that
        // fighter on the floor. Stage +10/+1C/+AA first, without inventing
        // a +A0 mode, attack script, horizontal velocity or world X.
        // Actual native 1400/1600 dispatch will become the same 87.232
        // autoReturn path on a subsequent committed frame.
        const unsigned other=p==0x4000u?0x4400u:0x4000u;
        XM87233AirReturn::Native airState;
        airState.combat=true; // Controller::frame already checks battle 08.
        airState.active=active!=0u;
        airState.opponentActive=word(r,other)!=0u &&
            word(r,other+0x52u)<=0x24u &&
            !(word(r,other+0x52u)&1u);
        airState.allowed=!a.recoveryActive && !a.autoReturn;
        airState.fighter=identity;airState.control=control;
        airState.action=word(r,p+0x60u);airState.mode=mode;
        airState.y=word(r,p+0x10u);
        airState.floor=word(r,p+0x254u);
        airState.air=word(r,p+0xaau);
        airState.x=x;airState.opponentX=longword(r,other+0x0cu);
        const XM87233AirReturn::Decision entry=a.airReturn.observe(airState);
        if(entry==XM87233AirReturn::GROUND_BEFORE_NATIVE_DASH ||
           entry==XM87233AirReturn::REGROUND_BEFORE_NATIVE_DASH){
            if(!settleReturnOnNativeFloor(r,p))a.airReturn.reset();
            else{
                if(log){
                    std::fprintf(log,
                        "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\t%s\n",
                        frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                        longword(r,p+0x34u),word(r,p+0x3eu),word(r,p+0x3eu),
                        entry==XM87233AirReturn::GROUND_BEFORE_NATIVE_DASH?
                        "FS_AIR_OFFSCREEN_NATIVE_GROUND_PREP":
                        "FS_AIR_OFFSCREEN_NATIVE_GROUND_REASSERTED");
                    std::fflush(log);
                }
                return e;
            }
        }else if(entry==XM87233AirReturn::NATIVE_GROUND_DASH_READY ||
                 entry==XM87233AirReturn::NATIVE_GROUND_DASH_BAD_X){
            const bool bad=entry==XM87233AirReturn::NATIVE_GROUND_DASH_BAD_X;
            const std::uint32_t anchored=bad?a.airReturn.anchor:x;
            if(settleReturnOnNativeFloor(r,p)){
                if(bad)setLong(r,p+0x0cu,anchored);
                a.autoReturn=true;a.returnAnchorValid=true;
                a.returnAnchor=anchored;a.returnMode=mode;
                a.failedReturnFrames=0u;a.advancingReturnFrames=0u;
                a.ready=false;a.tracking=false;a.dashAge=0u;
                a.stalledFrames=0u;a.scriptLoopsAtOrigin=0u;a.lastScript=0u;
                if(log){
                    std::fprintf(log,
                        "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\t%s\n",
                        frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                        longword(r,p+0x34u),word(r,p+0x3eu),word(r,p+0x3eu),
                        bad?"FS_AIR_TO_NATIVE_DASH_CORRUPT_X_RECOVERED":
                            "FS_AIR_TO_NATIVE_GROUND_DASH");
                    std::fflush(log);
                }
                return e; // 87.232 original entry countdown still intact.
            }
        }else if(entry==XM87233AirReturn::WAIT_FOR_NATIVE_DASH){
            // Never teach the ordinary neutral snapshot system a partially
            // converted airborne controller state during return staging.
            return e;
        }else if(entry==XM87233AirReturn::NATIVE_DASH_NOT_OBSERVED ||
                 entry==XM87233AirReturn::RETURN_ABORTED){
            if(log){
                std::fprintf(log,
                    "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\t%s\n",
                    frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                    longword(r,p+0x34u),word(r,p+0x3eu),word(r,p+0x3eu),
                    entry==XM87233AirReturn::NATIVE_DASH_NOT_OBSERVED?
                        "FS_AIR_NATIVE_GROUND_DASH_MISSING":
                        "FS_AIR_RETURN_CANCELLED");
                std::fflush(log);
            }
        }
        // Recover CPU-scripted offscreen entry BEFORE generic native-neutral
        // watchdog recovery. The 87.231 original-CPU trace observed PPC
        // 0109B4 write X=B27C0700 and PPC 01284A set native mode 1400.
        // Do NOT shorten the original entry frame timer: that may be the
        // actual anchor/ground staging interval.
        if(a.autoReturn){
            if(!ground(mode) || control || mode!=a.returnMode){
                a.autoReturn=false;
                a.failedReturnFrames=0u;a.advancingReturnFrames=0u;
                a.returnAnchorValid=false;
            }else if(brokenX && a.returnAnchorValid){
                if(++a.failedReturnFrames<=4u &&
                   settleReturnOnNativeFloor(r,p)){
                    setLong(r,p+0x0cu,a.returnAnchor);
                    if(log)std::fprintf(log,
                        "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_NATIVE_ENTRY_68K_REASSERTED_X\n",
                        frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                        longword(r,p+0x34u),word(r,p+0x3eu),
                        word(r,p+0x3eu));
                    return e;
                }
                // Four failed native retries mean 0109B4 is STILL
                // rebuilding the invalid coordinate. Do not silently
                // keep the actor in an invented host-driven dash.
                a.autoReturn=false;a.recoveryActive=true;
                a.suppressedFrames=0u;a.quietFrames=0u;
                restoreNeutral(r,p,a,true);
                settleReturnOnNativeFloor(r,p);
                setLong(r,p+0x0cu,a.returnAnchor);
                if(log)std::fprintf(log,
                    "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_NATIVE_ENTRY_UNRESOLVED_68K_SOURCE\n",
                    frame,side+1u,identity,mode,x,0u,
                    a.neutral.script,a.neutral.timer,a.neutral.timer);
                return e;
            }else if(saneX && ++a.advancingReturnFrames>=2u){
                a.failedReturnFrames=0u;
            }
        }
        if(!a.autoReturn && !a.recoveryActive && ground(mode) && !control &&
           saneX && outsideOpponentView(r,p,x) &&
           word(r,p+0x254u)>0u && word(r,p+0x254u)<2048u &&
           (word(r,p+0x10u)!=word(r,p+0x254u) ||
            (word(r,p+0xaau)&0xff00u)!=0u) &&
           settleReturnOnNativeFloor(r,p)){
            // The original 68000 already supplied a valid offscreen X.
            // Do not rewrite it or substitute an invented native script.
            a.autoReturn=true;a.returnAnchorValid=true;
            a.returnAnchor=x;a.returnMode=mode;
            a.failedReturnFrames=0u;a.advancingReturnFrames=0u;
            a.ready=false;a.tracking=false;a.dashAge=0u;
            a.stalledFrames=0u;a.scriptLoopsAtOrigin=0u;a.lastScript=0u;
            if(log)std::fprintf(log,
                "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_NATIVE_VALID_OFFSCREEN_GROUNDED\n",
                frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                longword(r,p+0x34u),word(r,p+0x3eu),
                word(r,p+0x3eu));
            return e;
        }
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
                    a.returnAnchorValid=false;
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
                // Once native 0109B4 has corrupted X, do not make a
                // recovered actor drop out of the SKY or pop back into
                // visible middle-stage space. Keep the grounded offscreen
                // anchor while original CPU controller retries continue.
                if(a.returnAnchorValid){
                    settleReturnOnNativeFloor(r,p);
                    setLong(r,p+0x0cu,a.returnAnchor);
                }
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
            // Prefer the native actor's own automatic dash controller
            // with a world-space offscreen start and original delay.
            if(stageNativeGroundEntry(r,p,a)){
                a.failedReturnFrames=0u;a.advancingReturnFrames=0u;
                if(log)std::fprintf(log,
                    "%u\t%u\t%04X\t%04X\t0\t%08X\t%08X\t%08X\t%u\t%u\tFS_NATIVE_GROUND_OFFSCREEN_ENTRY\n",
                    frame,side+1u,identity,mode,x,longword(r,p+0x14u),
                    longword(r,p+0x34u),word(r,p+0x3eu),
                    word(r,p+0x3eu));
                return e;
            }
            a.recoveryActive=true;a.quietFrames=0u;
            a.suppressedFrames=0u;
            a.snapshotRestores=0u;
            a.lastRecoveryFrame=frame;
            restoreNeutral(r,p,a,true);
            settleReturnOnNativeFloor(r,p);
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
        if(waiting && !a.autoReturn && timerMayAdvance(e.timerBefore)){
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
