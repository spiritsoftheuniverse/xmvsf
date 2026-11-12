#ifndef XMVSF_STAGE22_SELECTION_AUDIO_H
#define XMVSF_STAGE22_SELECTION_AUDIO_H
// XMVSF USA 961023: sound-CPU BGM-only muting before VS.
// Based on the CPS2 QSound cheat for XMVSF: CPU 1, $F026/$F027 = 0.
// Does not touch the main 68000 RAM, QSound output samples, input, or clock.
// When Stage20.1 VS detection fires, release the mute on ordinary stages.
// Final Destination keeps this music-only mute through combat; all native SFX remain.
#include <cstdio>

namespace xm22 {
static bool diagnosticWritten = false;
static unsigned int writes = 0;

static inline void muteSelectionBgm(bool driverReady, bool paused, bool versusReached, bool fdMusic=false) {
    if (!driverReady || paused || bBurnRunAheadFrame || (versusReached && !fdMusic)) return;
    cpu_core_config* sound = GetCpuCoreConfig(1);
    if (!sound || !sound->open || !sound->close || !sound->write) {
        if (!diagnosticWritten) {
            std::fprintf(stderr,"XM22: sound-CPU index 1 unavailable; music mute inactive.\n");
            diagnosticWritten = true;
        }
        return;
    }
    // Called only from RunFrame, after a real emulated frame has completed.
    sound->open(0);
    sound->write(0xF026, 0x00);
    sound->write(0xF027, 0x00);
    sound->close();
    ++writes;
    if (!diagnosticWritten) {
        std::fprintf(stderr,"XM22: enabling CPS2 sound-CPU music-only mute at F026/F027; QSound effects enabled.\n");
        diagnosticWritten = true;
    }
}
} // namespace xm22
#endif
