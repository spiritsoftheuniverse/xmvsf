#ifndef XMVSF_STAGE87_22_ECHO_SOURCE_H
#define XMVSF_STAGE87_22_ECHO_SOURCE_H
// Source-based afterimage grayscale. In the Stage87.19 trace, FF9C00,
// FF9CC0 and FF9D80 emit Zangief-style afterimages in palettes 21/22.
// Tile whitelists are incomplete at animation boundaries; only source+palette
// provenance is used. Keep original RGB for all other producer sources.
#include "xmvsf_stage87_17_provenance.h"
#include <cstdlib>
namespace XM8722 {
inline bool Enabled(){
    static const bool enabled=[](){const char* s=std::getenv("XM8722_ECHO_SOURCE");
        return !s || !(s[0]=='0' && s[1]=='\0');}();
    return enabled;
}
inline bool Echo(const XM8717::Source& s,unsigned frame,unsigned pri,unsigned pal){
    if(!Enabled() || pri!=6u || !XM8717::Recent(s,frame) || s.owner!=0u)return false;
    return (s.a0==0xFF9C00u && pal==21u) ||
           ((s.a0==0xFF9CC0u || s.a0==0xFF9D80u) && pal==22u);
}
}
#endif
