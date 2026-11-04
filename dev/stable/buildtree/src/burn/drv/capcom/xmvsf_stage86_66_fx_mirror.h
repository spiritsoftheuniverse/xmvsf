#ifndef XMVSF_STAGE86_66_FX_MIRROR_H
#define XMVSF_STAGE86_66_FX_MIRROR_H
// Stage86.67 correction to 86.66: exact tile matching ONLY. A 256-tile
// family contains both fighter bodies and effects in the supplied trace;
// promotion after three seed tiles was demonstrably too broad.
#include <unordered_set>
#include <cstdio>
struct XM8666FXMirror {
    std::unordered_set<unsigned> fxTiles;
    std::unordered_set<unsigned> fighterTiles;
    unsigned lastFrame=0,seen=0,mirrored=0;
    bool initialized=false;
    void advance(unsigned frame){
        if(initialized && frame<lastFrame){
            fxTiles.clear();fighterTiles.clear();seen=mirrored=0;
            std::fprintf(stderr,"XM86.67 FX_RESET frame_rewind=%u->%u\n",lastFrame,frame);
        }
        initialized=true;lastFrame=frame;
    }
    void observe(unsigned priority,unsigned palette,unsigned tile,unsigned frame){
        advance(frame);
        if(priority!=6u || palette<16u || palette>23u || fxTiles.size()>=8192u)return;
        if(fxTiles.insert(tile).second && ++seen<=16u)
            std::fprintf(stderr,"XM86.67 FX_EXACT_SEED tile=%u palette=%u\n",tile,palette);
    }
    void observeFighter(unsigned priority,unsigned tile,unsigned frame){
        advance(frame);
        if(priority==6u && fighterTiles.size()<8192u)fighterTiles.insert(tile);
    }
    bool mixed(unsigned tile)const {return fighterTiles.count(tile)!=0;}
    bool mirroredFx(unsigned priority,unsigned palette,unsigned tile,unsigned frame){
        advance(frame);
        if(priority!=6u || palette>=16u || fxTiles.count(tile)==0u)return false;
        if(++mirrored<=16u)
            std::fprintf(stderr,"XM86.67 FX_P1_EXACT tile=%u palette=%u mixed=%u\n",tile,palette,unsigned(mixed(tile)));
        return true;
    }
};
static inline XM8666FXMirror& xm8666FxMirror(){
    static XM8666FXMirror state;
    return state;
}
#endif
