#ifndef XMVSF_STAGE87_02_FX_SOURCE_GATE_H
#define XMVSF_STAGE87_02_FX_SOURCE_GATE_H
// 87.02: Observing a palette-16..23 sprite does NOT prove it is an FX source.
// A cross-side transfer seed is accepted AFTER the Stage87.01 artwork veto,
// only when the object is still RGB and lacks fighter-artwork evidence.
// Uses exact tile identity, never 256-tile family guesses.
namespace XM8702 {
static inline bool AllowSource(unsigned priority,unsigned palette,bool finalRgb,
                                bool lightingBody,bool inventoryShared,
                                bool sessionFighter) {
    return priority==6u && palette>=16u && palette<=23u && finalRgb &&
           !lightingBody && !inventoryShared && !sessionFighter;
}
static inline bool AllowMirror(unsigned priority,unsigned palette,bool sessionFighter) {
    return priority==6u && palette<16u && !sessionFighter;
}
}
#endif
