#ifndef XMVSF_STAGE87_137_DHC_INPUT_HISTORY_H
#define XMVSF_STAGE87_137_DHC_INPUT_HISTORY_H
// Stage 87.137: exact mapped-input history for future reserve-super parser.
// It observes the existing post-gate SDL virtual 1P/2P controls: no new reads,
// no buffered DELIVERY to the ROM, no simulated buttons, no new input latency.
// Direction codes 0:left 1:right 2:up 3:down (Stage13 source).
// Attack codes 0x80..0x85 are LP/MP/HP/LK/MK/HK. Coin and Start excluded.
#include <cstdint>
namespace XM87137Input {
enum { CAPACITY=48u };
struct Sample {
    unsigned frame=0u;
    std::uint8_t direction=0u,buttons=0u,pressed=0u;
    bool valid=false;
};
struct History {
    Sample ring[CAPACITY];
    unsigned latest=0u,count=0u,lastFrame=0u;
    std::uint8_t direction=0u,buttons=0u;
    void clear(){
        latest=count=lastFrame=0u;direction=buttons=0u;
        for(unsigned i=0;i<CAPACITY;++i)ring[i]=Sample();
    }
    void mapped(unsigned sub,bool down,unsigned frame){
        if(!(sub<4u || (sub>=0x80u && sub<=0x85u)))return;
        if(count>0u && frame<lastFrame)clear(); // save-state/frame rewind
        if(!count || frame!=lastFrame){
            latest=count ? (latest+1u)%CAPACITY : 0u;
            if(count<CAPACITY)++count;
            lastFrame=frame;
            ring[latest]=Sample();
            ring[latest].frame=frame;
            ring[latest].direction=direction;
            ring[latest].buttons=buttons;
            ring[latest].valid=true;
        }
        Sample& s=ring[latest];
        if(sub<4u){
            const std::uint8_t bit=std::uint8_t(1u<<sub);
            if(down)direction=std::uint8_t(direction|bit);
            else direction=std::uint8_t(direction & ~bit);
            s.direction=direction;
        }else{
            const std::uint8_t bit=std::uint8_t(1u<<(sub-0x80u));
            if(down && !(buttons&bit))s.pressed=std::uint8_t(s.pressed|bit);
            if(down)buttons=std::uint8_t(buttons|bit);
            else buttons=std::uint8_t(buttons & ~bit);
            s.buttons=buttons;
        }
    }
    bool at(unsigned ago,Sample& out)const{
        if(ago>=count)return false;
        const unsigned i=(latest+CAPACITY-(ago%CAPACITY))%CAPACITY;
        out=ring[i];return out.valid;
    }
};
struct State {
    History player[2];
    void mapped(unsigned p,unsigned sub,int value,unsigned frame){
        if(p<2u)player[p].mapped(sub,value!=0,frame);
    }
    bool sample(unsigned p,unsigned ago,Sample& out)const{
        return p<2u&&player[p].at(ago,out);
    }
    void clear(){player[0].clear();player[1].clear();}
};
inline State& state(){static State s;return s;}
} // namespace XM87137Input
#endif
