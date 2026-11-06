#ifndef XMVSF_STAGE87_191_SUPER_LIFETIME_GATE_H
#define XMVSF_STAGE87_191_SUPER_LIFETIME_GATE_H
// 87.191: one authoritative DHC availability condition.
// The ORIGINAL 68000 ROM has already entered a native 2E super root.
// Remain eligible throughout that original super's lifetime, from its
// start until its actual action/controller EXIT. A script, action substep,
// air/ground animation, A0 low byte, or +4E timer NEVER changes availability.
// A fresh human command is still required; a held gesture from startup
// must never be replayed later. Native C0/CA consumed-punch refusal,
// Ryu->Ken roster guard, 68000 controller-8 outcome, and abort remain
// independent safety/correctness checks in the existing handoff.
#include "xmvsf_stage87_189_native_super_family.h"
namespace XM87191Lifetime {
static inline bool isOpen(const XM87189Family::Tracker& native,
                         unsigned frame,
                         const XM87184Life::Sample& actual) {
    return native.live && native.recognizes() &&
           native.lastFrame==frame && frame>native.beginFrame &&
           native.fighter==actual.fighter &&
           native.originalD6==actual.d6 &&
           XM87189Family::Tracker::originalSuperController(actual);
}
static inline bool canArm(const XM87189Family::Tracker& native,
                          unsigned now,unsigned selectedAt,
                          const XM87184Life::Sample& actual) {
    return isOpen(native,now,actual) && selectedAt==now;
}
} // namespace XM87191Lifetime
#endif
