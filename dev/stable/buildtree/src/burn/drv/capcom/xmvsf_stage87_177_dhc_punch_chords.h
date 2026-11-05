#ifndef XMVSF_STAGE87_177_DHC_PUNCH_CHORDS_H
#define XMVSF_STAGE87_177_DHC_PUNCH_CHORDS_H
// Stage 87.177: DHC-only punch-chord recognition and 3P dash suppression.
// CPS2 physical bit order: LP=1, MP=2, HP=4, LK=8, MK=16, HK=32.
// NO ROM 68000 actor writes. Keep native Ken QCF+LP+MP (0x03) exactly as
// already verified, regardless of whether host requested 2P or 3P.
namespace XM87177Chords {
static inline unsigned popcount3(unsigned v) {
    v &= 7u;
    return (v&1u) + ((v>>1u)&1u) + ((v>>2u)&1u);
}
// Host queue acceptance only. Punches: any exact 2 or all 3; no kick bits.
// Kicks preserve 87.176's exact-two semantics, never turn 3K into superjump.
static inline bool valid(bool kicks,unsigned chord){
    if(chord & ~0x3Fu) return false;
    if(kicks) return (chord & 7u)==0u &&
        popcount3((chord>>3u)&7u)==2u;
    return (chord & 0x38u)==0u && popcount3(chord&7u)>=2u;
}
// While Ryu is in his verified Shinkuu Hadoken DHC window and BEFORE the
// host queue sees the chord (which happens after the 68000 frame), do not
// let simultaneous 3P invoke the original game's unrelated dash command.
// Leave 3P/normal dash in all other gameplay contexts untouched.
// The caller passes the P1 physical button mask; no history is fabricated.
static inline bool shieldPrequeue3P(bool enabled,bool earlyCancel,
          unsigned phase,bool nativeSuperCandidate,
          unsigned point,unsigned reserve,unsigned action,unsigned mode,
          unsigned p1PhysicalButtons){
    return enabled && earlyCancel && phase==0u && nativeSuperCandidate &&
        point==0x12u && reserve==0x14u && action==0x2E08u &&
        (mode==0x2A28u || mode==0x2A02u) &&
        (p1PhysicalButtons & 0x3Fu)==0x07u;
}
static inline unsigned nativeKenTwoPunchMask(){return 0x03u;}
} // namespace XM87177Chords
#endif
