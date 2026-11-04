#ifndef XMVSF_STAGE87_102_FLIGHT_IMPACT_H
#define XMVSF_STAGE87_102_FLIGHT_IMPACT_H
// XM87.104: flying-screen sound from the REAL Cyclops down+MP downward
// 16.16 velocity signature, not a hit animation, landing, or HUD state.
// Reference: victim FFxx1C=00132A00 (+19.1640625), 1C18 / 0230 / 000C.
// This is strictly an observer: ROM, CPS2 fighter data, and gameplay unchanged.
#include <cstdint>
namespace XM87102Impact {
static const std::uint32_t CYCLOPS_DOWN_MP_VY=0x00132A00u;
struct Fighter {
    bool signatureActive=false;
    void reset(){signatureActive=false;}
    bool tick(bool combat,unsigned action,unsigned profile,unsigned y,
              unsigned reaction,std::uint32_t nativeVy){
        (void)y;
        if(!combat){reset();return false;}
        const bool active=action==0x1C18u && profile==0x0230u &&
            reaction==0x000Cu && nativeVy==CYCLOPS_DOWN_MP_VY;
        const bool onset=active && !signatureActive;
        signatureActive=active;
        return onset;
    }
};
struct State {
    Fighter fighter[2];
    unsigned lastTick=~0u;
    void reset(){fighter[0].reset();fighter[1].reset();lastTick=~0u;}
};
inline State& state(){static State s;return s;}
} // namespace XM87102Impact
#endif
