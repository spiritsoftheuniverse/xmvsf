// 87.468: compile the real SDL3 pixel-reveal header before Ninja.
#include "../xmvsf_select_reveal.h"
static_assert(sizeof(port::xmvsf_home::SelectReveal) > 0);
int main() {
    port::xmvsf_home::SelectReveal reveal;
    return reveal.Active() ? 1 : 0;
}
