#ifndef XMVSF_STAGE87_178_TAG_LATCH_RECOVERY_H
#define XMVSF_STAGE87_178_TAG_LATCH_RECOVERY_H
// 87.178: restore only the DHC-owned controller +259 latch on abort.
// No actor action/mode/script history rewrites. Conservative: the ROM has
// consumed +256, tag 3200 was NOT selected, owner+reserve are unchanged,
// and this DHC set +259=08 from a captured original zero.
namespace XM87178Recovery {
static inline bool staleHostTag259(unsigned phase,bool enabled,bool early,
    unsigned point,unsigned reserve,unsigned outgoing,unsigned incoming,
    unsigned action,unsigned d6,unsigned controller256,
    unsigned controller259,unsigned original259){
    return enabled && early && phase==2u &&
        point==0x12u && reserve==0x14u &&
        outgoing==point && incoming==reserve &&
        // 87.180: the 87.179 F9 trace confirms that controller-8
        // may be consumed into ROM action 2C02 or 2C04 (D6=00).
        // The older D6=0A-only guard left OUR +259=08 stuck there,
        // making later controller inputs unreliable. Clear ONLY that
        // host-owned latch; never rewind the native wrong action.
        action!=0x3200u &&
        (d6==0x0Au ||
         (d6==0x00u && (action==0x2C02u || action==0x2C04u))) &&
        controller256==0u && controller259==0x08u &&
        original259==0u;
}
} // namespace XM87178Recovery
#endif
