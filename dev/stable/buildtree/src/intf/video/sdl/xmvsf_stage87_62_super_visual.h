#ifndef XMVSF_STAGE87_62_SUPER_VISUAL_H
#define XMVSF_STAGE87_62_SUPER_VISUAL_H
// Stage87.62: EXPERIMENTAL render-only super-candidate envelope for DF Wily.
// Not a global semantic super signal. 0x2A is shared by other actions;
// false positive lighting bursts are expected and must be reviewed.
// Both Classic and Tournament can use this read-only observation.
#include <cstdint>
#include <cstdio>
namespace XM8762 {
struct VisualProbe {
    bool ready[2]={false,false};
    bool contact[2]={false,false};
    bool lastActive=false;
    unsigned char prevAction[2]={0,0};
    std::uint64_t lastFrame=~std::uint64_t(0);
    bool active=false;
    static unsigned Read(const unsigned char* r,unsigned i){return r[i^1u];}
    void Reset(){
        for(unsigned i=0;i<2;++i){ready[i]=false;contact[i]=false;prevAction[i]=0;}
        lastActive=active=false; lastFrame=~std::uint64_t(0);
    }
    bool Observe(const unsigned char* ram,int phase,std::uint64_t frame){
        if(!ram||phase!=8){
            if(active)std::fprintf(stderr,"XM87.62 DF_SUPER_CANDIDATE_END frame=%llu reason=phase\n",(unsigned long long)frame);
            Reset();return false;
        }
        if(lastFrame==frame)return active;
        lastFrame=frame;
        unsigned char action[2];
        bool valid[2];
        for(unsigned i=0;i<2;++i){
            const unsigned base=0x4000u+i*0x400u;
            action[i]=static_cast<unsigned char>(Read(ram,base+0xA0u));
            valid[i]=ram[base]!=0;
        }
        bool any=false;
        const unsigned context=ram[0x53E8u]; // recorded raw host word-swapped offset
        for(unsigned i=0;i<2;++i){
            const unsigned j=i^1u;
            if(!valid[i]){contact[i]=false;ready[i]=false;continue;}
            const bool pairedEntry=action[i]==0x32u && valid[j] && action[j]==0x34u &&
                (context==0x04u || (ready[i] && prevAction[i]==0x2Au));
            if(pairedEntry)contact[i]=true;
            if(action[i]!=0x32u)contact[i]=false;
            // 0x2A is intentionally a broad *test* candidate. 0x32 requires
            // paired entry evidence, then persists through its full lifetime.
            if(action[i]==0x2Au || (action[i]==0x32u && contact[i]))any=true;
            prevAction[i]=action[i];ready[i]=true;
        }
        active=any;
        if(active!=lastActive){
            std::fprintf(stderr,"XM87.62 DF_SUPER_CANDIDATE_%s frame=%llu p1=%02X p2=%02X context=%02X brightness=%u hue_period_ms=%u\n",
                active?"BEGIN":"END",(unsigned long long)frame,
                unsigned(action[0]),unsigned(action[1]),context,
                active?80u:30u,active?3500u:35000u);
            lastActive=active;
        }
        return active;
    }
};
static inline VisualProbe& State(){static VisualProbe p;return p;}
} // XM8762
#endif
