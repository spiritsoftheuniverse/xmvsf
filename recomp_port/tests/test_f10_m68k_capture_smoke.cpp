// 87.472: compile-check exact native M68K F10 header without linking or a ROM.
#include "../../recomp/port/xmvsf_m68k_capture.h"
static_assert(sizeof(cps2recomp::xmvsf_f10::Capture) > 0);
int main() {
    return cps2recomp::xmvsf_f10::Capture::Get().Active() ? 1 : 0;
}
