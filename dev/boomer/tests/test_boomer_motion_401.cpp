// TEST 87.401: SDL-independent stage geometry and projectile behavior.
// Pure C++ standard assertions; run locally via RUN_TEST_BOOMER_401.bat.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_401_boomer_motion.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace XM87401Motion;
    static_assert(DefaultRightX==277,"right Triblaster X");
    static_assert(MirroredLeftX==105,"left Triblaster X");
    static_assert(RotorXMin==150 && RotorXMax==220,"rotor spawn range");
    static_assert(NearFramesPerPixel==2u,"wall 30px/second at 60fps");
    static_assert(RotorFramesPerPixel==3u,"rotor between C=5 and B=2");
    static_assert(ProjectileFramesPerPixel==1u,"projectiles 2x wall speed");
    // Actual B wall frame parity, not (now-born)/2 rounding.
    assert(wallShift(101,101)==0);
    assert(wallShift(101,102)==1);
    assert(wallShift(101,103)==1);
    assert(wallShift(101,104)==2);
    assert(rotorShift(101,103)==0);
    assert(rotorShift(101,105)==1);
    // Source attachment points from the default-side GIF artwork.
    assert(offsetX(false,40,9,8)==8);
    assert(offsetX(true,40,9,8)==23);
    assert(offsetX(false,40,6,27)==27);
    assert(offsetX(true,40,6,27)==7);
    assert(offsetY(false,70,9,26)==26);
    assert(offsetY(true,70,9,26)==35);
    assert(offsetY(false,70,5,47)==47);
    assert(offsetY(true,70,5,47)==18);
    // Right default fires LEFT; mirrored left side fires RIGHT.
    assert(projectileDx(false,true,60)==-60);
    assert(projectileDx(true,true,60)==60);
    assert(projectileDx(false,false,60)==0);
    // Shot rides the B scrolling elevator while moving horizontally.
    assert(projectileDy(false,false,100,160)==30);
    // Downward bullet travels +60 of its own speed plus B's +30.
    assert(projectileDy(false,true,100,160)==90);
    // Vertically mirrored cycle reverses bullet, still +30 from B.
    assert(projectileDy(true,true,100,160)==-30);
    assert(!verticalFlip(0)&&verticalFlip(1)&&
           !verticalFlip(2)&&verticalFlip(3));
    std::puts("TEST 87.401 BOOMER MOTION PASS");
    return 0;
}
