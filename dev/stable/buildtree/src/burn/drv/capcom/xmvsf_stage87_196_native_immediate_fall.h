#ifndef XMVSF_STAGE87_196_NATIVE_IMMEDIATE_FALL_H
#define XMVSF_STAGE87_196_NATIVE_IMMEDIATE_FALL_H
// 87.196: User 87.195.zip confirms five separate native Ken entries
// hold 2A08 (script D567E..D56EE) for exactly 67 *real frames*
// before the 68000 transfers to 0212/0600/D3D76, begins gravity,
// then reaches actual ground contact. That 67-frame native tag-lag
// animation is unnecessary for DHC. This is NOT a skip-by-timer:
// enter the same ROM-confirmed post-tag fall controller directly.
//
// This policy does not identify arbitrary character fall scripts.
// Expand only after the original ROM trace proves another fighter.
namespace XM87196Fall {
struct Entry {
    bool valid=false;
    unsigned action=0u,selector=0u,script=0u,mode=0u,d6=0u;
    unsigned verticalVelocity=0u,gravity=0u;
};
static inline Entry choose(unsigned fighter,unsigned incomingAction,
                           unsigned d6,unsigned y,unsigned floor,
                           bool enabled,unsigned resolvedFallScript){
    Entry p;
    // Native post-swap Ken action 2A0C/2A0E, before the kick has
    // played. Both 2A frames were already proven in 87.149/87.195.
    if(!enabled || fighter!=0x14u ||
       (incomingAction!=0x2A0Cu && incomingAction!=0x2A0Eu) ||
       d6!=0x10u || y>=floor || floor<128u || floor>768u ||
       resolvedFallScript!=0x0D3D76u)return p;
    p.valid=true;
    p.action=0x02u; p.selector=0x12u;
    p.script=0x0D3D76u; p.mode=0x0600u; p.d6=0x12u;
    // 87.195 native trace 1712: on first 0212 frame +1C=0,
    // +20=00006000; next 1713 +1C=00006000. This is the
    // ORIGINAL ROM gravity, not an invented jump or timeout.
    p.verticalVelocity=0u;p.gravity=0x00006000u;
    return p;
}
} // namespace XM87196Fall
#endif
