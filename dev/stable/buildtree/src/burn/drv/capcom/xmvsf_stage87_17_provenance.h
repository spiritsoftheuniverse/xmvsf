#ifndef XMVSF_STAGE87_17_PROVENANCE_H
#define XMVSF_STAGE87_17_PROVENANCE_H
// Stage87.17: opt-in trace, not a rendering rule. Source tags are attached to
// guest object-RAM slots and copied with sprites through the lagged OBJ list.
// Never infer a guaranteed body owner from a generic copy-routine PC.
#include <cstdint>
namespace XM8717 {
struct Source {
    std::uint32_t pc=0,a0=0,frame=0;
    unsigned char owner=0,valid=0;
};
extern Source slots[2][1024];
extern bool enabled;
inline unsigned char Owner(unsigned a0){
    if(a0==0xFF4000u || a0==0xFF4800u)return 1u;
    if(a0==0xFF4400u || a0==0xFF4C00u)return 2u;
    return 0u;
}
inline bool Recent(const Source& s,unsigned currentFrame){
    return s.valid && currentFrame>=s.frame && currentFrame-s.frame<=4u;
}
}
#endif
