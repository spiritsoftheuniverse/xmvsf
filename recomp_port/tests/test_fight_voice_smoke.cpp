// 87.487: syntax-check SDL3 Fight stream including explicit tail halt.
#include "../xmvsf_menu_audio.h"
#include <type_traits>
#include <utility>
using port::xmvsf_home::MenuAudio;
static_assert(sizeof(MenuAudio) > 0);
static_assert(std::is_same_v<
    decltype(std::declval<MenuAudio &>().FinishFight()), void>);
int main() { return 0; }
