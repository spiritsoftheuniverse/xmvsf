#ifndef XMVSF_STAGE87_30_COMPLETE_TRAIL_RED_H
#define XMVSF_STAGE87_30_COMPLETE_TRAIL_RED_H
// Stage87.30: fix the full Zangief secondary 21/22 echo run; source-qualified
// when tagged, exact simultaneous triplet when provenance is absent. Apply
// after Stage87.22/23 gray and BEFORE native CPS object draw. No ROM changes.
// Requires the already-included Stage87.17, Stage87.23 and Stage86.57 headers.
#include <cstdint>
#include <cstdlib>
namespace XM8730 {
inline bool Enabled(){
    static const bool enabled=[](){
        const char* s=std::getenv("XM8730_RED_TRAILS");
        return !(s && s[0]=='0' && s[1]=='\0');
    }();
    return enabled;
}
inline bool Target(bool echoGray,const std::uint16_t* obj,unsigned count,
                   unsigned frame,unsigned priority,unsigned pal,
                   const XM8717::Source& s){
    if(!Enabled() || !echoGray || priority!=6u ||
       (pal!=21u && pal!=22u) ||
       !XM8723::ThreeEchoesPresent(obj,count,frame))return false;
    if(XM8717::Recent(s,frame)) {
        // The recorded FF9C trail writers are owner=0. Refuse other tagged
        // sources, especially the independent FF6xxx effects and body actors.
        if(s.owner!=0u)return false;
        return (pal==21u && s.a0==0xFF9C00u) ||
               (pal==22u && (s.a0==0xFF9CC0u || s.a0==0xFF9D80u));
    }
    // Stage87.23 itself used this same exact dense 21+22 triplet to make
    // untagged first/transition frames gray. Reverse THAT decision only.
    return true;
}
inline unsigned Make(const std::uint32_t* original,
                     const std::uint32_t* previouslyGraded,
                     std::uint32_t* out){
    unsigned changed=0;
    for(unsigned pen=0;pen<16u;++pen){
        out[pen]=previouslyGraded[pen];
        if(!pen)continue;
        const auto c=xm8657Unpack(original[pen]);
        const unsigned peak=(c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b));
        // Preserve original pen intensity, but DO NOT require its hue to be
        // blue. The old IsBlue test discarded most of the trail ramp.
        const unsigned red=peak && peak<64u ? 64u : peak;
        out[pen]=xm8657Pack(original[pen],red,red/12u,red/24u);
        ++changed;
    }
    return changed;
}
} // namespace XM8730
#endif
