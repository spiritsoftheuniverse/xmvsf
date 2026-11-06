#ifndef XMVSF_STAGE87_202_AIR_FOLLOWUP_REBOUND_H
#define XMVSF_STAGE87_202_AIR_FOLLOWUP_REBOUND_H
// 87.201 native PPC proof: twelve Ryu air 2E0E/step06 handoffs consume
// controller8, then 68000 PC 010392 selects 2E0C/step02 script D31A8.
// These are ORIGINAL native same-super selector rebounds, analogous to
// proven 2E0A->2E08 recovery. Never bridge 0212/0214, 0002, 2Cxx,
// another fighter, an unconsumed edge, or a later/stale tag latch.
namespace XM87202Air {
struct Evidence {
    unsigned phase=0u,point=0u,reserve=0u,outgoing=0u,incoming=0u;
    unsigned capturedAction=0u,capturedMode=0u,capturedStep=0u;
    unsigned capturedScript=0u,capturedD6=0u,captured259=0u;
    unsigned action=0u,mode=0u,step=0u,script=0u,d6=0u;
    unsigned controller256=0u,controller259=0u;
    unsigned started=0u,now=0u;
};
static inline bool exactNativeAirRebound(const Evidence& e){
    return e.phase==2u && e.point==0x12u && e.reserve==0x14u &&
        e.outgoing==e.point && e.incoming==e.reserve &&
        e.capturedAction==0x2E0Eu && e.capturedMode==0x2A1Cu &&
        e.capturedStep==6u && e.capturedD6==0x0Au &&
        e.capturedScript>=0x0D322Cu && e.capturedScript<=0x0D328Cu &&
        (e.capturedScript-0x0D322Cu)%16u==0u &&
        e.captured259==0u && e.action==0x2E0Cu &&
        e.mode==0x2A00u && e.step==2u &&
        e.script==0x0D31A8u && e.d6==0x0Au &&
        e.controller256==0u && e.controller259==8u &&
        e.now==e.started+1u;
}
} // namespace XM87202Air
#endif
