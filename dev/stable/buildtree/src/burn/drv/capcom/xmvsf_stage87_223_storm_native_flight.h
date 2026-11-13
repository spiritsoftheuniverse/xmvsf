#ifndef XMVSF_STAGE87_223_STORM_NATIVE_FLIGHT_H
#define XMVSF_STAGE87_223_STORM_NATIVE_FLIGHT_H
// 87.223: observe genuine 68000 Storm air-HP flying-screen mechanics.
// The source is native POST-CPU actor RAM, BEFORE 87.86's TEST writer.
// Captures reference VX/VY, ROM reaction action, reaction INDEX (+E0),
// air/ground/substate, and searches the decrypted 68000 opcode buffer for
// candidate A6-relative writes. The opcode search does NOT claim dynamic
// execution proof: this requires a subsequent native PC/write trace.
// Gameplay only requests the observed motion; it must never memcpy a
// character-specific ROM animation/script/controller into a different actor.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87223Flight {
static inline unsigned b(const std::uint8_t*r,unsigned a){return r[(a&0xffffu)^1u];}
static inline unsigned w(const std::uint8_t*r,unsigned a){return (b(r,a)<<8u)|b(r,a+1u);}
static inline std::uint32_t d(const std::uint8_t*r,unsigned a){
    return (std::uint32_t(w(r,a))<<16u)|w(r,a+2u);
}
static inline std::int64_t signed32(std::uint32_t v){
    return (v&0x80000000u)?std::int64_t(v)-0x100000000LL:std::int64_t(v);
}
static inline std::int64_t mag(std::int64_t v){return v<0?-v:v;}
// Earlier native Storm HK->HP flight capture (87.80), NOT the Cyclops slam.
// These are a fallback until real Storm HP + reaction/index is measured
// together; 87.223 observer prints every measured candidate for validation.
static constexpr std::uint32_t NativeObservedVX=0x0010AA00u;
static constexpr std::uint32_t NativeObservedVY=0x0000B000u;
// Storm 1A04/5DAA -> native 1C18/0230 in the existing 87.86
// original-ROM impact capture. Keep this exact native pair as a fallback
// until we can observe a verified Storm HP event in the new TEST session.
static constexpr unsigned NativeStormFSAction=0x1C18u;
static constexpr unsigned NativeStormFSIndex=0x0230u;
static constexpr unsigned MaximumFlightBoostsBeforeLanding=1u;
struct Motion {
    bool valid=false;
    std::uint32_t vx=NativeObservedVX,vy=NativeObservedVY,gravity=0;
    unsigned action=0,profile=0,substate=0,air=0,gate82=0,gate83=0;
    unsigned fighter=0,attack=0,anim=0,frame=0;
};
struct Source {
    unsigned hpFrame=~0u,pressFrame=~0u;
    unsigned priorButtons=0,priorAction=0,priorVictimAction=0,priorVictimIndex=0;
    bool priorReaction=false;
};
struct Tracker {
    Source source[2];Motion observed;
    unsigned lastFrame=~0u;
    bool initialized=false,candidateScanDone=false;
    std::FILE *log=nullptr,*opcodes=nullptr;
    ~Tracker(){
        if(log)std::fclose(log);
        if(opcodes)std::fclose(opcodes);
    }
    void init(){
        if(initialized)return;
        initialized=true;
        const char*path=std::getenv("XM87223_STORM_FLIGHT_LOG_FILE");
        if(path&&*path){
            log=std::fopen(path,"wb");
            if(log)std::fputs(
          "frame\tside\tmode\tstorm_action\tstorm_anim\thp_edge_age"
          "\tvictim_id\tvictim_reaction\tstep06\tstep08\tvictim_action"
          "\tflight_index_E0\tgate82\tgate83\tairAA\tx\ty"
          "\tfloor254\tvx14\tvy1C\tgravity20\tdecision\n",log);
        }
        path=std::getenv("XM87223_M68K_CANDIDATES_FILE");
        if(path&&*path){
            opcodes=std::fopen(path,"wb");
            if(opcodes)
            std::fputs("pc\topcode\topcode_format\tactor_offset\timmediate\torder\n",opcodes);
        }
    }
    void reset(){
        source[0]=Source{};
        source[1]=Source{};
        observed=Motion{};
    }
    static bool nativeStormHP(unsigned id,unsigned bank,unsigned action){
        return id==0x0004u && bank==0x0022u &&
               (action==0x1804u || action==0x1a04u);
    }
    static bool motionFromStormHP(unsigned action,unsigned profile,unsigned reaction,
                                 unsigned substate,std::uint32_t vx){
        return reaction==0x000cu && substate==2u &&
               action==NativeStormFSAction && profile==NativeStormFSIndex &&
               mag(signed32(vx))>=0x000c0000LL &&
               mag(signed32(vx))<=0x00300000LL;
    }
    // 68000 MOVE.[B/W/L] #imm,(d16,A6) patterns. This is a source-code
    // *candidate* scan, not proof that a particular opcode executed.
    // The host stores opcode words either BE or word swapped.
    void scanOpcodeCandidates(const std::uint8_t*code,unsigned size){
        if(candidateScanDone || !code || size<8u || !opcodes)return;
        candidateScanDone=true;
        unsigned emitted=0;
        for(unsigned swap=0;swap<2u;++swap)for(unsigned pc=0;pc+8u<size;pc+=2u){
            const auto rd=[&](unsigned at)->unsigned{
                return swap ? (unsigned(code[at+1u])<<8u)|code[at] :
                              (unsigned(code[at])<<8u)|code[at+1u];
            };
            const unsigned op=rd(pc);
            if(op!=0x2d7cu && op!=0x3d7cu && op!=0x1d7cu)continue;
            const bool wide=op==0x2d7cu;
            const unsigned disp=rd(pc+(wide?6u:4u));
            if(disp!=0x0014u && disp!=0x001cu && disp!=0x0020u &&
               disp!=0x0060u && disp!=0x00e0u && disp!=0x0082u &&
               disp!=0x0083u && disp!=0x0134u && disp!=0x0135u)continue;
            const std::uint32_t imm=wide ? (std::uint32_t(rd(pc+2u))<<16u)|rd(pc+4u)
                                        : rd(pc+2u);
            std::fprintf(opcodes,"%06X\t%04X\tMOVE_%s_IMMEDIATE_TO_A6\t%04X\t%08X\t%s\n",
                pc,op,wide?"L":op==0x3d7cu?"W":"B",disp,imm,swap?"WORD_SWAP":"BE");
            if(++emitted>=1024u)break;
        }
        std::fflush(opcodes);
    }
    static bool sameMotion(const Motion&a,const Motion&b){
        return a.valid&&b.valid&&a.vx==b.vx&&a.vy==b.vy&&
               a.action==b.action&&a.profile==b.profile;
    }
    void frame(unsigned tick,const std::uint8_t*ram,const std::uint8_t*code,
               unsigned codeSize,bool xmvsf,bool tournament,
               unsigned p1Buttons,unsigned p2Buttons,
               bool suppressObservations=false){
        if(lastFrame==tick)return;
        if(lastFrame!=~0u && tick<lastFrame)reset();
        lastFrame=tick;
        if(!xmvsf||!ram||b(ram,0x5000u)!=8u){reset();return;}
        // 87.347: preserve native Storm flight learning/physics, but no
        // diagnostic file or static 68000 opcode scan during DHC test.
        if(!suppressObservations){
            init();
            scanOpcodeCandidates(code,codeSize);
        }
        const unsigned buttons[2]={p1Buttons,p2Buttons};
        for(unsigned side=0;side<2u;++side){
            const unsigned a=side?0x4400u:0x4000u,v=side?0x4000u:0x4400u;
            Source&s=source[side];
            const unsigned action=w(ram,a+0x60u);
            const bool hp=nativeStormHP(w(ram,a+0x52u),w(ram,a+0x40u),action);
            // The original CPU can enter HP several frames after the
            // physical button edge. Remember that edge, then track the
            // victim's reaction for 24 frames even if Storm's pose advances.
            const unsigned bnow=buttons[side];
            if((bnow&4u) && !(s.priorButtons&4u))s.pressFrame=tick;
            s.priorButtons=bnow;
            if(hp && s.pressFrame!=~0u && tick>=s.pressFrame &&
               tick-s.pressFrame<=12u && s.hpFrame==~0u)
                s.hpFrame=s.pressFrame;
            const unsigned age=(s.hpFrame!=~0u && tick>=s.hpFrame)?
                tick-s.hpFrame:~0u;
            if(age>24u){s.hpFrame=~0u;s.priorReaction=false;}
            if(!hp && age>24u){s.priorAction=action;continue;}

            const unsigned va=w(ram,v+0x60u),index=w(ram,v+0xe0u),
                 reaction=w(ram,v+0x06u),step=w(ram,v+0x08u);
            const std::uint32_t vx=d(ram,v+0x14u),vy=d(ram,v+0x1cu);
            const bool genuine=age<=24u &&
                motionFromStormHP(va,index,reaction,step,vx);
            const bool onset=genuine && (!s.priorReaction ||
                va!=s.priorVictimAction || index!=s.priorVictimIndex);
            if(onset){
                Motion native;
                native.valid=true;native.vx=(std::uint32_t)mag(signed32(vx));
                native.vy=vy;native.gravity=d(ram,v+0x20u);
                native.action=va;native.profile=index;
                native.substate=step;native.air=w(ram,v+0xaau);
                native.gate82=b(ram,v+0x82u);native.gate83=b(ram,v+0x83u);
                native.fighter=w(ram,v+0x52u);native.attack=action;
                native.anim=w(ram,a+0x42u);native.frame=tick;
                // Later source samples can refine an initial soft-hit
                // observation, but never overwrite a proven native Storm
                // sample with generic Tournament host-written state.
                if(!observed.valid || !sameMotion(observed,native))
                    observed=native;
            }
            if(!suppressObservations && log &&
               (onset || (genuine && ((tick-s.hpFrame)%4u==0u)))){
                std::fprintf(log,
                    "%u\t%u\t%s\t%04X\t%04X\t%u\t%04X\t%04X\t%04X\t%04X"
                    "\t%04X\t%04X\t%02X\t%02X\t%04X\t%08X\t%u\t%u"
                    "\t%08X\t%08X\t%08X\t%s\n",
                    tick,side+1u,tournament?"TOURNAMENT":"CLASSIC",
                    action,w(ram,a+0x42u),age,w(ram,v+0x52u),reaction,reaction,step,
                    va,index,b(ram,v+0x82u),b(ram,v+0x83u),
                    w(ram,v+0xaau),d(ram,v+0x0cu),w(ram,v+0x10u),
                    w(ram,v+0x254u),vx,vy,d(ram,v+0x20u),
                    onset?"NATIVE_STORM_HP_REFERENCE":"NATIVE_STORM_HP_CONTINUING");
                std::fflush(log);
            }
            s.priorReaction=genuine;s.priorAction=action;
            s.priorVictimAction=va;s.priorVictimIndex=index;
        }
    }
    unsigned action()const{
        return observed.valid && observed.action==NativeStormFSAction &&
            observed.profile==NativeStormFSIndex?
            observed.action:NativeStormFSAction;
    }
    unsigned index()const{
        return observed.valid && observed.action==NativeStormFSAction &&
            observed.profile==NativeStormFSIndex?
            observed.profile:NativeStormFSIndex;
    }
    bool learned()const{return observed.valid;}
    std::uint32_t speedX()const{
        const std::int64_t v=observed.valid?mag(signed32(observed.vx)):0;
        return v>=0x00060000LL && v<=0x00300000LL?
            (std::uint32_t)v:NativeObservedVX;
    }
    std::uint32_t speedY()const{
        const std::int64_t v=observed.valid?signed32(observed.vy):-1;
        return v>=0 && v<=0x00040000LL ? observed.vy : NativeObservedVY;
    }
};
static inline Tracker& state(){static Tracker instance;return instance;}
static inline unsigned flightVX(){return state().speedX();}
static inline unsigned flightVY(){return state().speedY();}
static inline unsigned flightAction(){return state().action();}
static inline unsigned flightIndex(){return state().index();}
static inline bool flightLearned(){return state().learned();}
} // namespace XM87223Flight
#endif
