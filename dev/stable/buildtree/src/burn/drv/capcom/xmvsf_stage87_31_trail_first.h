#ifndef XMVSF_STAGE87_31_TRAIL_FIRST_H
#define XMVSF_STAGE87_31_TRAIL_FIRST_H
// Stage87.31: source-qualified trail pass-through BEFORE the noir classifier.
// Existing 87.28 body/outline path stays entirely intact for other objects.
// No ROM writes, no global palette edits, no CPS priority/order changes.
#include <cstdint>
#include <cstdlib>
namespace XM8731 {
inline bool Enabled(){
    static const bool enabled=[](){
        const char* s=std::getenv("XM8731_TRAIL_FIRST");
        return !(s && s[0]=='0' && s[1]=='\0');
    }();
    return enabled;
}
inline bool ExactWriter(const XM8717::Source& source,unsigned pal){
    return source.owner==0u &&
           ((pal==21u && source.a0==0xFF9C00u) ||
            (pal==22u && (source.a0==0xFF9CC0u || source.a0==0xFF9D80u)));
}
inline bool Route(const std::uint16_t* obj,unsigned n,unsigned frame,
                  unsigned priority,unsigned pal,const XM8717::Source& source){
    if(!Enabled() || priority!=6u || (pal!=21u && pal!=22u))return false;
    // Known producer wins immediately, even when the other two producers
    // disappear on the first/last frame of an animation. In particular, do
    // NOT require 87.22/23 gray to have been selected first.
    if(XM8717::Recent(source,frame))return ExactWriter(source,pal);
    // During the ROM's buffered source handoff a tag can be absent. Accept
    // only the independently observed dense triple group, not a loose RGB
    // threshold. Reject known independent FF6xxx effects even if stale.
    if(source.valid && source.a0>=0xFF6000u && source.a0<0xFF8000u)return false;
    return XM8723::ThreeEchoesPresent(obj,n,frame);
}
inline unsigned RedPalette(const std::uint32_t* native,std::uint32_t* output){
    unsigned changed=0;
    for(unsigned p=0;p<16u;++p){
        output[p]=native[p];
        if(p==0u)continue; // hardware transparent pen
        const auto c=xm8657Unpack(native[p]);
        const unsigned peak=c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b);
        if(!peak)continue;
        const unsigned red=peak<112u?112u:peak;
        output[p]=xm8657Pack(native[p],red,red/16u,red/32u);
        ++changed;
    }
    return changed;
}
} // namespace XM8731
#endif
