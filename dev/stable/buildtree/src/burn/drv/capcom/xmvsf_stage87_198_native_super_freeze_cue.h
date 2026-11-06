#ifndef XMVSF_STAGE87_198_NATIVE_SUPER_FREEZE_CUE_H
#define XMVSF_STAGE87_198_NATIVE_SUPER_FREEZE_CUE_H
// 87.198: the 87.197 capture shows the host's first accepted Ken 2E
// action is still a SUPER STARTUP (no offensive box). The original
// ROM does not place the FIRST attacking hitbox until:
//   Ken 2E00: root D6722 at 10553 -> hit70=0064 at 10600 (+47f)
//   Ken 2E02: root D6976 at 2326 -> hit70=0050 at 2378 (+52f)
//   2E02 repeats at 11049->11102 (+53f), 11891->11943 (+52f),
//   17490->17542 (+52f). No guessed timer or per-move script list.
//
// Releasing at 2E action selection makes the opponent move again
// during the long original startup. Release at a live original ROM
// OFFENSIVE BOX while the same confirmed incoming super owns Ken.
// The test-only actor hold must not overwrite the opponent's first
// real hit reaction/damage on the trigger frame.
namespace XM87198Cue {
static inline bool realKenSuperOffense(unsigned fighter,unsigned action,
                                      unsigned mode,unsigned step,
                                      unsigned offensiveBox){
    return fighter==0x14u && (action&0xFF00u)==0x2E00u &&
           (mode&0xFF00u)==0x2A00u &&
           step>=4u && (step&1u)==0u &&
           offensiveBox>0u && offensiveBox<0x8000u;
}
} // namespace XM87198Cue
#endif
