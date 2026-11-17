#ifndef XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
#define XMVSF_STAGE20_VS_VISUAL_ANCHOR_422_H
// TEST 87.426. Exact new user F6 capture: frame 1678 in phase 02,
// preceding 00->02 write at frame 1651 (PPC 00F1E4): offset 27.
// Start the VS song and the existing 150f HUD-delay at that position.
// No CPU writes, stage IDs, renderer reads or 8/9 key changes.
namespace XM87422VisualAnchor {
static constexpr unsigned DelayFrom02=27u;
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
