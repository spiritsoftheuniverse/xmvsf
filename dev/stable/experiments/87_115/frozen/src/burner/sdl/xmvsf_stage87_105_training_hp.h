#ifndef XMVSF_STAGE87_105_TRAINING_HP_H
#define XMVSF_STAGE87_105_TRAINING_HP_H
// XM87.107: both presets belong to P2's two individual dummy characters.
// P1 must never receive a preset write. A dummy keeps its own preset
// regardless of which P2 RAM slot the native tag handoff places it in.
namespace XM87105HP {
static const unsigned MAX_HP=144u;
static inline unsigned p2Base(unsigned slot){return slot<2u?0xFF4400u+0x800u*slot:0u;}
struct State {
    unsigned tick[2]={10u,10u}; // P2 Dummy 1, P2 Dummy 2
    bool pending[2]={true,true};
    bool needsSlot[2]={true,true};
    unsigned identity[2]={0u,0u};
    bool known[2]={false,false};
    unsigned lastIdentity[2]={0u,0u};
    bool observed[2]={false,false};
    unsigned slotOwner[2]={0u,1u};
    bool combat=false;
    static unsigned amount(unsigned t){return (MAX_HP*t+5u)/10u;}
    unsigned target(unsigned dummy) const{return dummy<2u?amount(tick[dummy]):0u;}
    unsigned percent(unsigned dummy) const{return dummy<2u?tick[dummy]*10u:0u;}
    unsigned activeDummy() const{return slotOwner[0];}
    unsigned activeTarget() const{return target(activeDummy());}
    void change(unsigned dummy,int direction){
        if(dummy>=2u||direction==0)return;
        tick[dummy]=direction>0?(tick[dummy]==10u?1u:tick[dummy]+1u):
                                 (tick[dummy]==1u?10u:tick[dummy]-1u);
        pending[dummy]=true;
    }
    void queueAll(){pending[0]=pending[1]=true;needsSlot[0]=needsSlot[1]=true;}
    void seeCombat(bool live){
        if(!live){combat=false;known[0]=known[1]=false;
            observed[0]=observed[1]=false;
            slotOwner[0]=0u;slotOwner[1]=1u;return;}
        if(!combat){queueAll();combat=true;}
    }
    // Character index +0x53 is observed from P2 fighter RAM, never P1.
    // First valid point character is Dummy 1, first valid reserve is Dummy 2.
    // Later, match the character index rather than its current RAM slot.
    // This also handles reserve initialization only when it first tags in.
    unsigned observe(unsigned slot,unsigned charId,bool valid){
        if(slot>=2u)return 2u;
        if(!valid){observed[slot]=false;return slotOwner[slot];}
        unsigned owner=slotOwner[slot];
        if(known[0] && identity[0]==charId &&
           (!known[1]||identity[1]!=charId))owner=0u;
        else if(known[1] && identity[1]==charId &&
                (!known[0]||identity[0]!=charId))owner=1u;
        else if(!known[slot]){
            // Initial roster; prefer the native initial P2 point/bench order.
            owner=slot;
            if(known[1u-slot] && identity[1u-slot]==charId){
                // Same-character teams cannot be distinguished by ID alone;
                // retain physical-slot ownership until an additional tag cue exists.
                owner=slot;
            }
            identity[owner]=charId;known[owner]=true;
        }else if(known[0] && !known[1] && identity[0]!=charId){
            owner=1u;identity[1]=charId;known[1]=true;
        }else if(known[1] && !known[0] && identity[1]!=charId){
            owner=0u;identity[0]=charId;known[0]=true;
        }
        const bool changed=!observed[slot] || lastIdentity[slot]!=charId || slotOwner[slot]!=owner;
        lastIdentity[slot]=charId;observed[slot]=true;slotOwner[slot]=owner;
        if(changed)needsSlot[slot]=true;
        return owner;
    }
    bool takeSlot(unsigned slot,unsigned& dummy){
        if(slot>=2u||!observed[slot])return false;
        dummy=slotOwner[slot];
        if(!pending[dummy]&&!needsSlot[slot])return false;
        pending[dummy]=false;needsSlot[slot]=false;return true;
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87105HP
#endif
