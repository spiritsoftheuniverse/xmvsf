#ifndef XMVSF_STAGE20_VS_EARLY_PHASE_H
#define XMVSF_STAGE20_VS_EARLY_PHASE_H
// 87.322: versus-song cue follows the ROM's first PRE-MATCH screen
// handoff (step 04 -> intermediate 05/06/07), never the combat screen.
// Last character/speed choice reaching 07 is NOT the versus screen.
// A direct 04 -> 08 transition is retained as a no-intermediate fallback
// for ROM revisions that skip step 06; it cannot fire after combat.
// No writes to emulated RAM, inputs or ROM.
namespace XM20VsEarly {
enum Event { NONE=0, VS_SCREEN_EDGE=1, MATCH_SETUP_EDGE=2 };
struct Gate {
    unsigned previousStep=0xffffffffu;
    bool sawChoosing=false;
    bool sawActiveChooser=false;
    bool fired=false;
    Event observe(unsigned step,unsigned p1,unsigned p2){
        if(step==4u){
            sawChoosing=true;
            if((p1>=1u&&p1<7u)||(p2>=1u&&p2<7u))
                sawActiveChooser=true;
        }
        Event answer=NONE;
        // The native 04 -> 05/06/07 screen handoff, not the last button
        // acceptance, is the first frame of the versus presentation.
        // Jumping directly to 08 can happen after restored snapshots.
        if(!fired && sawChoosing && previousStep==4u){
            if(step>=5u && step<8u)answer=VS_SCREEN_EDGE;
            else if(step==8u)answer=MATCH_SETUP_EDGE;
            if(answer!=NONE)fired=true;
        }
        previousStep=step;
        return answer;
    }
};
}
#endif
