#ifndef XMVSF_STAGE20_VS_EARLY_PHASE_H
#define XMVSF_STAGE20_VS_EARLY_PHASE_H
// 87.313: pure read-only detector for the native chooser -> versus handoff.
// FF5180/FF51A0: 0x20-byte player choosing records; byte 0 == 7 when
// all character and speed choices are accepted by the ROM.
// FF5104: game step 4 choosing, 8 match setup.
// These are earlier than FF5000=02 (stage load / closing VS screen).
namespace XM20VsEarly {
enum Event { NONE=0, BOTH_CHOOSERS_DONE=1, MATCH_SETUP_EDGE=2 };
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
        if(!fired && sawChoosing){
            // A completed P1 alone must NEVER start VS while P2 selects.
            if(step==4u && sawActiveChooser && p1==7u && p2==7u)
                answer=BOTH_CHOOSERS_DONE;
            else if(previousStep==4u && step==8u)
                answer=MATCH_SETUP_EDGE;
            if(answer!=NONE)fired=true;
        }
        previousStep=step;
        return answer;
    }
};
}
#endif
