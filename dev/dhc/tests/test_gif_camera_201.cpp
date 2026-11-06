// XM87.201 pure-math unit proof. Requires only C++11, no SDL or ROM.
#include "../../stable/buildtree/src/intf/video/sdl/xmvsf_stage87_201_gif_camera.h"
#include <cassert>
using namespace XM87201GifCamera;
static bool same(Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h;}
int main(){
    assert(worldCameraX(0)==-64);
    assert(worldCameraX(384)==317);
    assert(worldCameraX(192)==127);
    // Natural 1:1 entire-world picture.
    assert(same(crop(765,494,0,-14),Rect{0,0,384,224}));
    assert(same(crop(765,494,384,256),Rect{381,270,384,224}));
    assert(same(crop(765,494,192,121),Rect{191,135,384,224}));
    // 2x resolution must preserve the same camera/world alignment.
    assert(same(crop(1530,988,0,-14),Rect{0,0,768,448}));
    assert(same(crop(1530,988,384,256),Rect{762,540,768,448}));
    // Excess source width/height is centered; registration is still monotonic.
    const Rect topLeft=crop(1800,988,0,-14), bottomRight=crop(1800,988,384,256);
    assert(topLeft.x>=0 && topLeft.y>=0);
    assert(bottomRight.x>topLeft.x && bottomRight.y>topLeft.y);
    assert(bottomRight.x+bottomRight.w<=1800);
    assert(bottomRight.y+bottomRight.h<=988);
    const Rect tallLeft=crop(765,1200,0,-14), tallRight=crop(765,1200,384,256);
    assert(tallLeft.y>0 && tallRight.y>tallLeft.y);
    assert(tallRight.y+tallRight.h<=1200);
    // Clamp camera to true native limits, never infer extra stage travel.
    assert(same(crop(765,494,-1000,-1000),crop(765,494,0,-14)));
    assert(same(crop(765,494,9999,9999),crop(765,494,384,256)));
    assert(crop(0,494,0,0).w==0);
    return 0;
}
