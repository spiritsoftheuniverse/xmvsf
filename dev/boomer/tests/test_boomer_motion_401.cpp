// TEST 87.401: SDL-independent stage geometry and projectile behavior.
// Pure C++ standard assertions; run locally via RUN_TEST_BOOMER_401.bat.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_401_boomer_motion.h"
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_404_boomer_layers.h"
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
    assert(rotorShift(99,101)==0);
    assert(rotorShift(99,102)==1);
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
    // 87.404: camera extrema must place guns near the SCREEN corners,
    // NOT at x=105/277 at the center of a 768-wide stage.
    assert(gunWorldX(true)==106);
    assert(gunWorldX(false)==662);
    assert(screenX(gunWorldX(true),0)==106);
    assert(screenX(gunWorldX(false),384)==278);
    assert(screenX(gunWorldX(true),192)==-86);
    assert(screenX(gunWorldX(false),192)==470);
    // Horizontal scroll is exactly worldX minus native camera X.
    assert(screenX(gunWorldX(true),80)-screenX(gunWorldX(true),100)==20);
    // A jump's native camera-Y movement must move sprites exactly as wall B.
    const int stageTop100=224-471+256-100;
    const int stageTop80=224-471+256-80;
    assert(stageTop80-stageTop100==wallCameraYOffset(100,80));
    assert(wallCameraYOffset(100,100)==0);
    assert(wallCameraYOffset(100,110)==-10);
    // 1-9 independently filter Boomer sources; 0 always shows all A/B/C
    // but hides actors, HUD, and projectiles for F6 scenery-only capture.
    for(int layer=0;layer<9;++layer){
        const bool keep=layer==XM87404Layers::DistantC||
                        layer==XM87404Layers::WallB||
                        layer==XM87404Layers::PlatformA;
        assert(XM87404Layers::shown(layer,true,true)==keep);
        assert(XM87404Layers::shown(layer,false,true)==keep);
        assert(XM87404Layers::shown(layer,true,false));
        assert(!XM87404Layers::shown(layer,false,false));
    }
    assert(!XM87404Layers::shown(-1,true,false));
    assert(!XM87404Layers::shown(9,true,false));
    std::puts("TEST 87.404 BOOMER CAMERA/CORNER/LAYER MOTION PASS");
    return 0;
}
