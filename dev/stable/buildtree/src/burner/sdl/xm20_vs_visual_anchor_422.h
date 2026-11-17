#ifndef XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
#define XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
// TEST 87.422. Native F6 capture at F=00002004 ST=02 STEP=0008,
// last exact FF5000 00->02 write at F=00001969 (WRPC=00F1E4).
// The user selected this visible-stage position, 35 real emulation
// frames after entering 02. This is the CURRENT CAPTURED PLACEMENT,
// not a claim that 02->04 is when the wall begins moving in all runs.
// No CPU writes, fixed-CPU-cycle delays, stage IDs, or renderer reads.
namespace XM87422VisualAnchor {
static constexpr unsigned DelayFrom02=35u;
struct Gate {
    unsigned lastPhase=~0u;
    unsigned phase02Entry=0u;
    bool armed=false;
    bool fired=false;
    bool observe(unsigned phase,unsigned frame){
        if(fired)return false;
        if(phase==2u && lastPhase==0u){
            phase02Entry=frame;
            armed=true;
        }else if(phase!=2u){
            armed=false;
        }
        lastPhase=phase;
        if(!armed || frame<phase02Entry ||
           frame-phase02Entry<DelayFrom02)return false;
        fired=true;
        return true;
    }
};
}
#endif
