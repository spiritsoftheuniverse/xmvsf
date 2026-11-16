// 87.403: fast compile-only smoke check for the complete SDL_image GIF stage loader.
// Run with UCRT64 g++ -fsyntax-only before the expensive FBNeo Make pass.
// No SDL initialization, no runtime assets, no linking, no ROM required.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_401_boomer_enemies.h"
#include <type_traits>
static_assert(std::is_same<decltype(IMG_LoadAnimation("")),IMG_Animation*>::value,
              "SDL_image animation API must use IMG_Animation");
int main(){
    return 0;
}
