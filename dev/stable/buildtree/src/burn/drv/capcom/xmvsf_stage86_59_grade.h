#ifndef XMVSF_STAGE86_59_GRADE_H
#define XMVSF_STAGE86_59_GRADE_H
// Native Double Feature color grading. Only used while XM86.57 palette grading is active.
// Stage86.29 Numpad-5 fighter ink (96) is faithfully restored, not a five-tone quantizer.
#include "xmvsf_stage86_57_palette.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

static constexpr unsigned XM8659_BG_PERCENT=30u;
static constexpr unsigned XM8659_FIGHTER_BLACK_POINT=96u;
static inline std::uint32_t xm8659Background(std::uint32_t from,XM8657Spectral s){
    const auto c=xm8657Unpack(from);
    const double y=double(xm8657Luma(c.r,c.g,c.b))*0.30;
    return xm8657Pack(from,xm8657Clamp(y*s.r),xm8657Clamp(y*s.g),xm8657Clamp(y*s.b));
}
static inline std::uint32_t xm8659FighterInk(std::uint32_t from){
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    const unsigned ink=(y<=XM8659_FIGHTER_BLACK_POINT)?0u:y;
    return xm8657Pack(from,ink,ink,ink);
}
// Stage86.29 established an exact FX rule signature: priority,palette,tile/256.
// The default proven priority-6 palette 16..23 remains in the caller.
// Optional signatures ONLY; a screenshot alone cannot identify CPS2 object IDs.
struct XM8659FXRule {unsigned priority,palette,family;};
static inline bool xm8659EffectFamily(unsigned priority,unsigned pal,unsigned tile){
    struct Rules {
        XM8659FXRule entries[256]{};
        unsigned count=0;
        Rules(){
            const char* path=std::getenv("XM8659_FX_RULES_FILE");
            if(!path || !*path)return;
            FILE* file=std::fopen(path,"rb");
            if(!file){std::fprintf(stderr,"XM86.59 FX_RULES cannot_open=%s\n",path);return;}
            char line[192];
            while(std::fgets(line,sizeof(line),file)){
                if(line[0]=='#'||line[0]==';'||line[0]=='\n')continue;
                unsigned p=0,k=0,f=0;char end=0;
                if(std::sscanf(line," %u , %u , %u %c",&p,&k,&f,&end)!=3)continue;
                if(p<=7u && k<32u && f<16384u && count<256u)
                    entries[count++]={p,k,f};
            }
            std::fclose(file);
            std::fprintf(stderr,"XM86.59 FX_RULES exact_family_count=%u\n",count);
        }
    };
    static const Rules rules{};
    for(unsigned i=0;i<rules.count;++i){
        const auto &r=rules.entries[i];
        if(r.priority==priority && r.palette==pal && r.family==tile/256u)return true;
    }
    return false;
}
#endif
