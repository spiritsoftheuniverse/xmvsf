#ifndef XMVSF_STAGE87_195_TAG_LATCH_GUARD_H
#define XMVSF_STAGE87_195_TAG_LATCH_GUARD_H
// 87.195: 87.193(2).zip frame 5703:
// outgoing Ryu entered native action 2C26, controller256=00,
// controller259=08, captured controller259=00. The earlier 87.178
// recovery guard did NOT classify this changed action as safe to clean,
// even though 259=08 was written by OUR immediately prior controller8
// handoff. This leaves a stale host-owned tag latch in a real 2C actor.
//
// Cleanup only one exact failed DHC controller transaction. Never undo
// the ROM's actual action, fighter ownership, animation or script.
// Ordinary tags, a real 3200, another actor or an unconsumed controller
// still belong to the original game.
namespace XM87195Latch {
static inline bool shouldReleaseWrongAction(
        unsigned phase,unsigned point,unsigned reserve,
        unsigned outgoing,unsigned incoming,
        unsigned action,unsigned originalAction,
        unsigned controller256,unsigned controller259,
        unsigned captured259){
    return phase==2u && outgoing && incoming &&
        point==outgoing && reserve==incoming &&
        action!=0x3200u && action!=originalAction &&
        controller256==0u && controller259==8u &&
        captured259==0u;
}
}
#endif
