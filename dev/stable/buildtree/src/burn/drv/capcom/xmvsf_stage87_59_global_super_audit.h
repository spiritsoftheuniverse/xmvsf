#ifndef XMVSF_STAGE87_59_GLOBAL_SUPER_AUDIT_H
#define XMVSF_STAGE87_59_GLOBAL_SUPER_AUDIT_H
// Read-only GLOBAL super research, independent of Classic/Tournament and DF.
// This emits observations, NEVER a confirmed super begin/end gameplay signal.
// +A0 is a fighter action, +A1 tends to track the previous action, +AA is
// SUPERJUMP (not a super combo). Contact pair 2A->32 / opponent ->34 is only
// a candidate. Global/entity data from FF4800..FF53FF is included to research
// projectile ownership, command state and move-specific lifetimes.
// Opt-in variables: XM8759_GLOBAL_AUDIT_FILE, XM8759_GLOBAL_RAM_FILE.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
namespace XM8759 {
struct Fighter {
    unsigned a0,a1,a2,a4,aa,ac,valid;
};
static inline unsigned Byte(const unsigned char* ram,unsigned off){return ram[off^1u];}
static inline Fighter Read(const unsigned char* ram,unsigned p){
    const unsigned b=0x4000u+0x400u*p;
    Fighter s={Byte(ram,b+0xA0u),Byte(ram,b+0xA1u),
        Byte(ram,b+0xA2u),Byte(ram,b+0xA4u),
        Byte(ram,b+0xAAu),Byte(ram,b+0xACu),
        (unsigned)(ram[b]!=0)};
    return s;
}
struct Probe {
    bool initialized,ready[2];
    FILE *events,*globals;
    unsigned lastFrame,frames;
    Fighter prev[2];
    Probe():initialized(false),events(nullptr),globals(nullptr),lastFrame(~0u),frames(0u){
        ready[0]=ready[1]=false;
        std::memset(prev,0,sizeof(prev));
    }
    ~Probe(){if(events)std::fclose(events);if(globals)std::fclose(globals);}
    void Init(){
        if(initialized)return;
        initialized=true;
        const char* path=std::getenv("XM8759_GLOBAL_AUDIT_FILE");
        if(!path||!*path)return;
        events=std::fopen(path,"wb");
        if(!events){std::fprintf(stderr,"XM87.59 GLOBAL_SUPER_OPEN_FAILED path=%s\n",path);return;}
        std::fputs("frame\tphase\tplayer\tevent\ta0\tprev_a0\trom_previous_a1\ta2\ta4\taa_superjump\tac\tother_a0\tother_prev_a0\tcontact_candidate\n",events);
        const char* bin=std::getenv("XM8759_GLOBAL_RAM_FILE");
        if(bin&&*bin){
            globals=std::fopen(bin,"wb");
            if(globals)std::fwrite("XM8759G1",1,8,globals);
        }
        std::fprintf(stderr,"XM87.59 GLOBAL_SUPER_AUDIT_READY events=%s globals=%d mode_independent=1 gameplay_changes=0\n",path,int(globals!=nullptr));
    }
    void Observe(const unsigned char* ram,unsigned frame,int phase,bool firstPass){
        if(!ram||!firstPass||frame==lastFrame)return;
        lastFrame=frame;Init();
        if(!events||frames>=18000u)return;
        if(phase!=8){ready[0]=ready[1]=false;return;}
        const Fighter curr[2]={Read(ram,0u),Read(ram,1u)};
        if(globals){
            const std::uint32_t h[2]={std::uint32_t(frame),std::uint32_t(phase)};
            std::fwrite(h,sizeof(h[0]),2,globals);
            std::fwrite(ram+0x4800u,1,0xC00u,globals);
        }
        for(unsigned p=0;p<2u;++p){
            const unsigned q=1u-p;
            const Fighter& x=curr[p];const Fighter& o=curr[q];
            if(!x.valid){ready[p]=false;continue;}
            const Fighter& a=prev[p];
            const bool start=ready[p]&&ready[q]&&a.a0==0x2Au&&x.a0==0x32u&&
                o.a0==0x34u&&prev[q].a0!=0x34u;
            const bool end=ready[p]&&a.a0==0x32u&&x.a0!=0x32u;
            const bool change=!ready[p]||a.a0!=x.a0||a.a1!=x.a1||
                a.a2!=x.a2||a.a4!=x.a4||a.aa!=x.aa||a.ac!=x.ac;
            if(!change&&!start&&!end)continue;
            const char* event=start?"CONTACT_PAIR_CANDIDATE":end?"CONTACT_ACTION_EXIT":
               (!ready[p]?"FIRST":"ACTION_FIELD_CHANGE");
            std::fprintf(events,"%u\t%d\t%u\t%s\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%u\n",
                frame,phase,p+1u,event,x.a0,ready[p]?a.a0:x.a0,
                x.a1,x.a2,x.a4,x.aa,x.ac,o.a0,ready[q]?prev[q].a0:o.a0,
                start?1u:0u);
            if(start||end)std::fprintf(stderr,
                "XM87.59 %s frame=%u p=%u from=%02X to=%02X rom_previous=%02X NOT_A_VERIFIED_SUPER\n",
                event,frame,p+1u,a.a0,x.a0,x.a1);
        }
        prev[0]=curr[0];prev[1]=curr[1];
        ready[0]=curr[0].valid!=0;ready[1]=curr[1].valid!=0;
        ++frames;
        if((frames%120u)==0u){std::fflush(events);if(globals)std::fflush(globals);}
    }
};
static inline Probe& State(){static Probe p;return p;}
static inline void Observe(const unsigned char* raw,unsigned f,int phase,bool pass){
    State().Observe(raw,f,phase,pass);
}
} // namespace XM8759
#endif
