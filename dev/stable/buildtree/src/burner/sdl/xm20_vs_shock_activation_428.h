#ifndef XMVSF_STAGE20_VS_SHOCK_ACTIVATION_428_H
#define XMVSF_STAGE20_VS_SHOCK_ACTIVATION_428_H
// TEST 87.428: screenshot F1475 ST06, FF5000 EDGE 04>06 AT F1452,
// actual M68K writer PPC 00F2BE. Shock reveal begins 23 frames into
// native phase 06. This is INDEPENDENT of the earlier song at 02+27.
// Do not use the song-to-shock hold interval for the captured path.
namespace XM87428ShockActivation {
static constexpr unsigned FramesInto06=23u;
struct Gate {
    unsigned previousPhase=~0u;
    unsigned phase06Entry=0u;
    bool armed=false;
    bool fired=false;
    bool observe(unsigned phase,unsigned frame) {
        if(fired)return false;
        if(phase==6u && previousPhase==4u){
            phase06Entry=frame;
            armed=true;
        }else if(phase!=6u){
            armed=false;
        }
        previousPhase=phase;
        if(!armed || frame<phase06Entry ||
           frame-phase06Entry<FramesInto06)return false;
        fired=true;
        return true;
    }
};
}
#endif
