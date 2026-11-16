#ifndef XMVSF_STAGE87_404_BOOMER_LAYERS_H
#define XMVSF_STAGE87_404_BOOMER_LAYERS_H
// Boomer's 1..9 layer audit, with key 0 reserved for complete scenery-only
// screenshot mode. Pure helper: no SDL dependency, ROM writes or filtering
// of the original native CPS2 object buffers.
namespace XM87404Layers {
enum Layer {
    DistantC=0,Rotors=1,WallB=2,Triblaster=3,Shot=4,Bullet=5,
    PlatformA=6,StunHUD=7,NativeFightersAndFlash=8
};
static inline bool shown(int layer,bool userEnabled,bool sceneryOnly){
    if(layer<0 || layer>8)return false;
    if(sceneryOnly)
        return layer==DistantC||layer==WallB||layer==PlatformA;
    return userEnabled;
}
}
#endif
