#ifndef XMVSF_STAGE86_60_NOIR_H
#define XMVSF_STAGE86_60_NOIR_H
// Stage86.60: recover the original Double Feature deep-noir *tone curve*.
// The Stage86.59 linear reduction preserved contrast but didn't produce the
// user's requested deep-dark scenery.  Preserve 35-second spectral timing,
// but reduce chroma; whites can NEVER bloom because noir peaks at 64/255.
#include "xmvsf_stage86_59_grade.h"
#include <cstdint>
static inline unsigned xm8660NoirLuma(unsigned y) {
    unsigned dark=0;
    if(y<=48u)dark=y/24u;
    else if(y<=184u)dark=2u+(y-48u)*61u/136u;
    else dark=63u+(y-184u)*25u/71u;
    return (dark*73u+50u)/100u; // at full white -> 64/255
}
static inline std::uint32_t xm8660Background(std::uint32_t from,XM8657Spectral spectral){
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8660NoirLuma(xm8657Luma(c.r,c.g,c.b));
    // Reduce saturation of the Stage86.29 cycle without removing hue motion.
    constexpr double chroma=0.28;
    return xm8657Pack(from,
        xm8657Clamp(double(y)*(1.0+chroma*(spectral.r-1.0))),
        xm8657Clamp(double(y)*(1.0+chroma*(spectral.g-1.0))),
        xm8657Clamp(double(y)*(1.0+chroma*(spectral.b-1.0))));
}
// Costume table reference from the local XMvSF USA program ROM at 0x1145A8.
// Row 7 has the distinctive red/purple normal and blue/purple alternate
// candidate.  A sprite palette is tinted ONLY if its real RGB pens match a
// ROM stock costume row. No color guessing from screen coordinates, priority
// or dominant hues; reject all other fighters and effects. This candidate
// still needs live confirmation as Magneto (not assumed proven by screenshots).
static inline unsigned xm8660RomWord(const unsigned char* rom,unsigned at){
    return unsigned(rom[at]) | (unsigned(rom[at+1])<<8);
}
static inline bool xm8660MagnetoDirectory(const unsigned char* rom,unsigned size){
    if(!rom || size<=0x115740u)return false;
    const unsigned at=0x1145a8u+7u*4u;
    const unsigned ptr=(xm8660RomWord(rom,at)<<16)|xm8660RomWord(rom,at+2u);
    return ptr==0x1156bcu; // explicit ROM-version guard
}
static inline unsigned xm8660MagnetoCostume(const std::uint32_t* pal,
                 const unsigned char* rom,unsigned size){
    if(!pal || !xm8660MagnetoDirectory(rom,size))return 0u;
    unsigned matches[2]={0u,0u},errors[2]={0u,0u};
    for(unsigned costume=0;costume<2;costume++){
        for(unsigned pen=1;pen<15;pen++){
            const unsigned rgb=xm8660RomWord(rom,0x1156bcu+costume*0x60u+pen*2u)&0xfffu;
            const auto actual=xm8657Unpack(pal[pen]);
            const unsigned ref[3]={((rgb>>8)&15u)*17u,((rgb>>4)&15u)*17u,(rgb&15u)*17u};
            const unsigned got[3]={actual.r,actual.g,actual.b};
            unsigned maximum=0u;
            for(unsigned c=0;c<3;c++){
                const unsigned delta=got[c]>ref[c]?got[c]-ref[c]:ref[c]-got[c];
                if(delta>maximum)maximum=delta;
            }
            if(maximum<=24u)matches[costume]++;
            errors[costume]+=maximum;
        }
    }
    // Matches must be strongly specific. The two ROM costume rows share some
    // colors, so choose the better match only after the >=10-pen gate.
    if(matches[0]<10u && matches[1]<10u)return 0u;
    if(matches[0]==matches[1])return errors[0]<=errors[1]?1u:2u;
    return matches[0]>matches[1]?1u:2u;  // 1=normal, 2=alt
}
static inline std::uint32_t xm8660MagnetoTint(std::uint32_t from,unsigned costume){
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    if(y<=XM8659_FIGHTER_BLACK_POINT)return xm8657Pack(from,0u,0u,0u);
    if(costume==1u) // subdued crimson, retains gray-shaped light and dark
        return xm8657Pack(from,xm8657Clamp(y*1.16),xm8657Clamp(y*0.70),xm8657Clamp(y*0.77));
    if(costume==2u) // cold blue, retain full grayscale modeling
        return xm8657Pack(from,xm8657Clamp(y*0.70),xm8657Clamp(y*0.84),xm8657Clamp(y*1.17));
    return xm8659FighterInk(from);
}
#endif
