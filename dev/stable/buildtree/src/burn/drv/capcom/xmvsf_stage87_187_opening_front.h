#ifndef XMVSF_STAGE87_187_OPENING_FRONT_H
#define XMVSF_STAGE87_187_OPENING_FRONT_H
// 87.187 original 87.186 proof: old startup command selected at 2524 or
// 2565 was armed at attack 2544 or 2625 without a new button press.
// Keep current-frame ATTACK and known RECOVERY route intact.
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87187Front {
enum Disposition { KEEP=0, STARTUP_NOT_ARMABLE=1,
    STALE_ATTACK_INPUT=2 };
static inline Disposition check(const XM87184Life::Sample& s,
        unsigned now,unsigned selectedAt) {
    const XM87184Life::Phase phase=XM87184Life::identify(s);
    if(phase==XM87184Life::STARTUP)return STARTUP_NOT_ARMABLE;
    if(phase==XM87184Life::ATTACK) {
        if(selectedAt!=now)return STALE_ATTACK_INPUT;
        // 87.187: original Shinkuu ATTACK legitimately uses 2A02.
        // Do not infer a universal veto from one wrong native dispatch.
    }
    return KEEP;
}
static inline const char* reason(Disposition d) {
    switch(d) {
    case STARTUP_NOT_ARMABLE:
        return "DHC_187_STARTUP_INPUT_CLOSED_NO_DEFERRED_TAG";
    case STALE_ATTACK_INPUT:
        return "DHC_187_STALE_ATTACK_REQUEST_EXPIRED_NO_PHYSICAL_EDGE";
    default:return "DHC_187_FRESH_INPUT_KEEP";
    }
}
static inline const char* wrongAction(unsigned action) {
    switch(action) {
    case 0x0002u:return "DHC_187_ROM_0002_WRONG_ACTION_ABORT_NO_ROLLBACK";
    case 0x2C02u:return "DHC_187_ROM_2C02_WRONG_ACTION_ABORT_NO_ROLLBACK";
    case 0x2C04u:return "DHC_187_ROM_2C04_WRONG_ACTION_ABORT_NO_ROLLBACK";
    default:return "DHC_187_ROM_OTHER_WRONG_ACTION_ABORT_NO_ROLLBACK";
    }
}
} // namespace XM87187Front
#endif
