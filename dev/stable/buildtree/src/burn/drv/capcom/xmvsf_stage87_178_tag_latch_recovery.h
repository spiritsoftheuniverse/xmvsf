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
        action!=0x3200u && d6==0x0Au &&
        controller256==0u && controller259==0x08u &&
        original259==0u;
}
} // namespace XM87178Recovery
#endif
