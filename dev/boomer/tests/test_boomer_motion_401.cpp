// TEST 87.401: SDL-independent stage geometry and projectile behavior.
// Pure C++ standard assertions; run locally via RUN_TEST_BOOMER_401.bat.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_401_boomer_motion.h"
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_404_boomer_layers.h"
#include <cassert>
#include <cstdio>
#include <algorithm>
int main(){
    using namespace XM87401Motion;
    static_assert(RightCornerScreenX==244,"right gun moved 34px left");
    static_assert(LeftCornerScreenX==104,"mirrored left gun moved 2px left");
    static_assert(RotorScreenMargin==8,"rotors cover full stage view");
    static_assert(RotorMinSeparation==88,"all three rotors must fit with wide clearance");
    static_assert(RotorGunGap==38,"rotors need blaster clearance");
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
    // 87.405: side-specific world-space adjustments preserve camera lock.
    assert(gunWorldX(true)==104);
    assert(gunWorldX(false)==628);
    assert(screenX(gunWorldX(true),0)==104);
    assert(screenX(gunWorldX(false),384)==244);
    assert(screenX(gunWorldX(true),192)==-88);
    assert(screenX(gunWorldX(false),192)==436);
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
    // Up-facing shot/bullet cycles are permitted ONLY in the central
    // vertical third, based on the animated gun's physical sprite center.
    assert(!inMiddleVerticalBand(-10,30));
    assert(!inMiddleVerticalBand(55,38)); // center=74
    assert(inMiddleVerticalBand(56,38));  // center=75
    assert(inMiddleVerticalBand(110,38)); // center=129
    assert(inMiddleVerticalBand(130,38)); // center=149
    assert(!inMiddleVerticalBand(131,38)); // center=150
    assert(!inMiddleVerticalBand(224,38));
    assert(!verticalFlip(0) && verticalFlip(1) && !verticalFlip(2));
    // Real 3-slot separation feasibility for cameras from both corners,
    // INCLUDING the two adjusted stationary gun world positions.
    // Search the same screen-wide thirds and use the same production
    // rectangle-clearance predicate. No giant 150..220 rotor cluster.
    constexpr int rotorW=24, gunW=32;
    constexpr int cameraCases[]={0,24,48,72,96,144,192,240,288,336,360,384};
    for(int cam : cameraCases){
        int rotorXs[3]={-10000,-10000,-10000};
        const int lo=RotorScreenMargin;
        const int hi=StageScreenW-RotorScreenMargin-rotorW;
        const int span=hi-lo+1;
        const auto clearOfGuns=[&](int candidate){
            // Must stay away from both guns for the ENTIRE camera pan:
            // one gun follows 100% camera X, rotors follow 60%.
            for(int futureCam : {cam,0,StageWorldW-StageScreenW}){
                const int projected=rotorProjectedX(candidate,cam,futureCam);
                if(!horizontallySeparated(projected,rotorW,
                    screenX(gunWorldX(true),futureCam),gunW,RotorGunGap))
                    return false;
                if(!horizontallySeparated(projected,rotorW,
                    screenX(gunWorldX(false),futureCam),gunW,RotorGunGap))
                    return false;
            }
            return true;
        };
        for(int slot=0;slot<3;++slot){
            const int laneLo=lo+span*slot/3;
            const int laneHi=std::max(laneLo,lo+span*(slot+1)/3-1);
            bool found=false;
            for(int x=laneLo;x<=laneHi;++x){
                if(!clearOfGuns(x))continue;
                bool separated=true;
                for(int j=0;j<slot;++j)
                    if(!horizontallySeparated(x,rotorW,
                             rotorXs[j],rotorW,
                             std::max(42,RotorMinSeparation-rotorW)))
                        separated=false;
                if(!separated)continue;
                rotorXs[slot]=x;found=true;break;
            }
            // When a gun occupies a zone, the actual spawner searches
            // the rest of the viewport instead of stacking on the gun.
            if(!found){
                for(int x=lo;x<=hi && !found;++x){
                    if(!clearOfGuns(x))continue;
                    bool separated=true;
                    for(int j=0;j<slot;++j)
                        if(!horizontallySeparated(x,rotorW,
                             rotorXs[j],rotorW,
                             std::max(42,RotorMinSeparation-rotorW)))
                            separated=false;
                    if(!separated)continue;
                    rotorXs[slot]=x;found=true;
                }
            }
            assert(found && "Three non-overlapping rotor spawns must fit");
        }
        for(int a=0;a<3;++a)
            for(int b=a+1;b<3;++b)
                assert(horizontallySeparated(rotorXs[a],rotorW,
                   rotorXs[b],rotorW,std::max(42,RotorMinSeparation-rotorW)));
    }
    std::puts("TEST 87.405 BOOMER CORNERS, MIDDLE-UP AND 3-ROTOR SPREAD PASS");
    return 0;
}
