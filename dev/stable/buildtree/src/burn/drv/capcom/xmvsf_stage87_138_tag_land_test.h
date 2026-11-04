#ifndef XMVSF_STAGE87_138_TAG_LAND_TEST_H
#define XMVSF_STAGE87_138_TAG_LAND_TEST_H
// XM87.138: opt-in native TAG LANDING prototype. Demonstrates one piece of
// opponent-relative DHC using an actual native point/reserve swap.
// No synthetic tag, no ROM patch, no super launch, no new input route.
// Normal play is 100% unchanged unless XM87138_TAG_LAND_TEST=1.
// Scope: Stage 05 only (measured world bounds 192..768), with verified tag
// identity swap and airborne incoming point. Never moves either opponent.
// If tag ownership changes only after landing, log it and do NOT teleport.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87138Tag {
static inline unsigned b(const std::uint8_t* r,unsigned a){return r[(a&65535u)^1u];}
static inline unsigned w(const std::uint8_t* r,unsigned a){return (b(r,a)<<8)|b(r,a+1u);}
static inline std::uint32_t d(const std::uint8_t* r,unsigned a){return (std::uint32_t(w(r,a))<<16)|w(r,a+2u);}
static inline void sb(std::uint8_t* r,unsigned a,unsigned v){r[(a&65535u)^1u]=std::uint8_t(v);}
static inline void sw(std::uint8_t* r,unsigned a,unsigned v){sb(r,a,v>>8);sb(r,a+1u,v);}
static inline bool idOk(unsigned x){return x<=0x40u && (x&1u)==0;}
struct Actor {unsigned id=0,x=0,y=0,activity=0;};
static inline Actor actor(const std::uint8_t* r,unsigned a) {
    Actor x;x.id=w(r,a+0x52u);x.x=w(r,a+0x0Cu);
    x.y=w(r,a+0x10u);x.activity=w(r,a);return x;
}
struct State {
    bool initialized=false,enabled=false,seeded[2]={false,false};
    bool jumping[2]={false,false};
    unsigned jumpId[2]={0u,0u};
    bool dimStage()const{return enabled && (jumping[0]||jumping[1]);}
    unsigned previousFrame=~0u,swaps=0,positioned=0;
    Actor previousPoint[2],previousReserve[2];
    std::FILE* log=nullptr;
    ~State(){if(log)std::fclose(log);}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* on=std::getenv("XM87138_TAG_LAND_TEST");
        enabled=on && std::strcmp(on,"1")==0;
        if(!enabled)return;
        const char* name=std::getenv("XM87138_TAG_LAND_LOG");
        if(name && *name){log=std::fopen(name,"wb");
            if(log)std::fputs("frame\tside\tfromPoint\ttoPoint\topponentX\toldPointX\tincomingX\tincomingY\ttargetX\tdecision\n",log);}
    }
    void clear(){
        seeded[0]=seeded[1]=false;
        jumping[0]=jumping[1]=false;
    }
    // Called at the safe CPS2 end-of-real-frame boundary with word-swapped
    // native CpsRamFF. It is an explicit experiment, NOT yet a DHC trigger.
    void frame(std::uint8_t* r,unsigned frame,const char* driver,bool runahead){
        init();if(!enabled||!r||runahead||frame==previousFrame)return;
        previousFrame=frame;
        if(!driver||std::strncmp(driver,"xmvsf",5)!=0 ||
           b(r,0x5000u)!=8u || (b(r,0x5113u)&15u)!=5u ||
           d(r,0xA014u)!=0x00FF4000u){clear();return;}
        for(unsigned side=0;side<2u;++side){
            const unsigned point=0x4000u+side*0x400u;
            const unsigned reserve=0x4800u+side*0x400u;
            const unsigned opponent=0x4000u+(side^1u)*0x400u;
            const Actor now=actor(r,point),bench=actor(r,reserve);
            const Actor foe=actor(r,opponent);
            // The experimental visual follows the real native tag's airborne
            // period, NOT a guessed 20/40-frame cinematic timer.
            if(jumping[side] && (now.id!=jumpId[side] || now.y>=472u))
                jumping[side]=false;
            const bool valid=idOk(now.id)&&idOk(bench.id)&&idOk(foe.id) &&
                now.id!=bench.id && now.activity && foe.activity;
            if(seeded[side] && valid &&
               previousPoint[side].id==bench.id &&
               previousReserve[side].id==now.id &&
               previousPoint[side].id!=now.id) {
                ++swaps;
                int target=-1;
                const bool onLeft=previousPoint[side].x<=foe.x;
                const int requested=int(foe.x)+(onLeft?-48:48);
                const bool bounds=requested>=192 && requested<=768;
                const bool airborne=now.y<472u; // observed Stage87.120 native landing Y
                const bool xValid=now.x>=192u && now.x<=768u &&
                                  foe.x>=192u && foe.x<=768u;
                const char* decision="REFUSED";
                if(bounds && airborne && xValid){
                    target=requested;
                    sw(r,point+0x0Cu,unsigned(target)); // preserve fractional 16-bit X
                    const unsigned oldFacing=b(r,point+0x4Bu);
                    sb(r,point+0x4Bu,(oldFacing&~1u)|(onLeft?0u:1u));
                    ++positioned;
                    jumping[side]=true;jumpId[side]=now.id;
                    decision="POSITIONED_IN_NATIVE_TAG_JUMP";
                }else if(!airborne)decision="TAG_SWAP_AT_OR_AFTER_LANDING";
                else if(!bounds)decision="TARGET_OUT_OF_STAGE05_BOUNDS";
                else decision="UNTRUSTED_ACTOR_POSITION";
                if(log){
                    std::fprintf(log,"%u\t%u\t%04X\t%04X\t%u\t%u\t%u\t%u\t%d\t%s\n",
                        frame,side+1u,previousPoint[side].id,now.id,
                        foe.x,previousPoint[side].x,now.x,now.y,target,decision);
                    std::fflush(log);
                }
            }
            previousPoint[side]=now;previousReserve[side]=bench;
            seeded[side]=valid;
        }
    }
};
static inline State& state(){static State s;return s;}
}
#endif
