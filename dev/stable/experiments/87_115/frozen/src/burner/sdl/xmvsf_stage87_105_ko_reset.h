#ifndef XMVSF_STAGE87_105_KO_RESET_H
#define XMVSF_STAGE87_105_KO_RESET_H
// Recorded floor-entry transition 1C2A -> 1C34, with 1C34 -> 1C3E as
// fallback if entry was missed. The 87.102 KO capture observes this sequence
// in both lethal and ordinary knockdowns; ONLY a latched zero-HP fighter in
// Training / KO Enabled can initiate a snapshot reload.
namespace XM87105KO {
struct Detector {
    bool armed[2]={false,false};
    unsigned lastAction[2]={0xffffu,0xffffu};
    bool finished=false;
    void reset(){armed[0]=armed[1]=false;lastAction[0]=lastAction[1]=0xffffu;finished=false;}
    int observe(bool training,bool koEnabled,bool saved,unsigned phase,
                const unsigned hp[2],const unsigned action[2]){
        if(!training||!koEnabled||!saved){reset();return -1;}
        if(finished)return -1;
        int result=-1;
        for(unsigned i=0;i<2u;++i){
            // phase 08 is active combat; 0A is the real KO/round-ending phase.
            if((phase==8u||phase==10u) && hp[i]==0u)armed[i]=true;
            if(armed[i] && hp[i]==0u && phase==10u &&
                    ((lastAction[i]==0x1C2Au && action[i]==0x1C34u) ||
                     (lastAction[i]==0x1C34u && action[i]==0x1C3Eu)))
                result=(int)i;
            lastAction[i]=action[i];
        }
        if(result>=0)finished=true;
        return result;
    }
};
inline Detector& detector(){static Detector d;return d;}
} // XM87105KO
#endif
