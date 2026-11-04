#ifndef XMVSF_STAGE87_23_ECHO_MONO_H
#define XMVSF_STAGE87_23_ECHO_MONO_H
// Stage87.23: afterimage-only monochrome fallback derived from the full
// combat provenance session. The three Zangief afterimage actors concurrently
// produce large palette-21 / palette-22 groups across the sampled sequence.
// This is NOT a global ban on those colors: standalone palette-21 FX such as
// Magneto's Tempest must remain RGB, as do known FF6xxx effect producers.
#include "xmvsf_stage87_17_provenance.h"
#include <cstdint>
#include <cstdlib>

namespace XM8723 {
inline bool Enabled() {
    static const bool enabled=[](){
        const char* p=std::getenv("XM8723_ECHO_MONO");
        return !(p && p[0]=='0' && p[1]=='\0');
    }();
    return enabled;
}
inline unsigned Word(const std::uint16_t* obj,unsigned off){
    return unsigned(BURN_ENDIAN_SWAP_INT16(obj[off]));
}
// The published 87.19 capture: all 338 frames of the afterimage triplet
// contained >=12 priority-6 palette-21 sprites AND >=24 palette-22 sprites.
// Crucially, this doesn't match palette-21 effects by themselves.
inline bool ThreeEchoesPresent(const std::uint16_t* obj,unsigned n,unsigned frame) {
    struct Cache {const std::uint16_t* obj=nullptr;unsigned n=0,frame=~0u;bool result=false;};
    static Cache cache;
    if(cache.frame==frame && cache.obj==obj && cache.n==n)return cache.result;
    unsigned twentyOne=0,twentyTwo=0;
    if(obj){
        for(unsigned i=0;i<n && i<1024u;i++){
            if((Word(obj,i*4u)>>13)!=6u)continue;
            const unsigned p=Word(obj,i*4u+3u)&31u;
            if(p==21u)++twentyOne;
            else if(p==22u)++twentyTwo;
        }
    }
    cache.obj=obj;cache.n=n;cache.frame=frame;
    cache.result=twentyOne>=12u && twentyTwo>=24u;
    return cache.result;
}
inline bool GrayEcho(const std::uint16_t* obj,unsigned n,unsigned frame,
                     unsigned priority,unsigned pal,const XM8717::Source& source) {
    if(!Enabled() || priority!=6u || (pal!=21u && pal!=22u) ||
       !ThreeEchoesPresent(obj,n,frame))return false;
    if(XM8717::Recent(source,frame)){
        // Preserve independent special FX even if they coexist with Zangief's
        // shadows (Magneto's Tempest can also use palette 21).
        if(source.a0>=0xFF6000u && source.a0<0xFF8000u)return false;
        // Stage87.19 afterimage structures live in the secondary actor pool.
        if(source.a0>=0xFF9000u && source.a0<=0xFF9FFFu)return true;
        // A body animation can occasionally write directly from the active
        // actor during the first piledriver/grab frame.
        return source.owner==1u || source.owner==2u;
    }
    // Early sprite-buffer frames can lack a fresh source tag. Only accept
    // this fallback when the characteristic THREE-echo palette triplet exists.
    return true;
}
} // namespace XM8723
#endif
