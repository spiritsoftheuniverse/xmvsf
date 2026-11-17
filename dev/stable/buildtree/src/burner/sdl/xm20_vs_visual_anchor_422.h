#ifndef XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
#define XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
// TEST 87.424. Start song and HUD-delay clock at phase-02 FRAME ZERO,
// i.e. the first observed 00->02 transition (WRPC=00F1E4).
// The user's prior F6 sample was 35 frames into 02; this adjusts ONLY
// its requested offset to zero. No CPU writes, stage IDs or renderer reads.
namespace XM87422VisualAnchor {
static constexpr unsigned DelayFrom02=0u;
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
