#ifndef XMVSF_STAGE86_61_NOIR_H
#define XMVSF_STAGE86_61_NOIR_H
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
// Stage86.61: compare each live CPS-2 fighter palette against all 19
// *verified ROM directory* stock costume pairs, rather than rejecting
// Magneto when fewer than 10/14 stock pens match exactly.
// This is still palette attribution, not guessed fighter screen coordinates.
// Row 7 (ROM pointer 0x1156BC) is the red/blue candidate; other identities
// compete in the score, preventing accidental tint of a different fighter.
static inline unsigned xm8661RomWord(const unsigned char* rom,unsigned at){
    return unsigned(rom[at])|(unsigned(rom[at+1])<<8);
}
static inline unsigned xm8661RomPointer(const unsigned char* rom,unsigned row){
    const unsigned at=0x1145a8u+row*4u;
    return (xm8661RomWord(rom,at)<<16)|xm8661RomWord(rom,at+2u);
}
struct XM8661Match { unsigned costume=0, candidate=0, rival=0; };
static inline XM8661Match xm8661MagnetoCostume(const std::uint32_t* pal,
                 const unsigned char* rom,unsigned size){
    XM8661Match result{};
    if(!pal || !rom || size<0x1166c0u || xm8661RomPointer(rom,7u)!=0x1156bcu)
        return result;
    // Comparison in RGB444 space: remove 8-bit expansion rounding and
    // tolerate animations that update a fraction of the original pens.
    const unsigned expected[19]={0x114d7c,0x114e3c,0x114fbc,0x11531c,0x1153dc,
        0x11549c,0x11559c,0x1156bc,0x11577c,0x11589c,0x115a1c,0x115b9c,
        0x115c5c,0x115d5c,0x115fdc,0x11609c,0x11621c,0x11639c,0x1164bc};
    unsigned best=~0u, second=~0u, bestCostume=0;
    for(unsigned row=0;row<19;row++){
        const unsigned ptr=xm8661RomPointer(rom,row);
        if(ptr!=expected[row] || ptr+0x80u>=size)continue;
        for(unsigned costume=0;costume<2;costume++){
            unsigned score=0, closePens=0;
            for(unsigned pen=1;pen<=14;pen++){
                const auto p=xm8657Unpack(pal[pen]);
                const unsigned rgb=xm8661RomWord(rom,ptr+costume*0x60u+pen*2u)&0xfffu;
                const unsigned orig[3]={p.r,p.g,p.b};
                const unsigned stock[3]={((rgb>>8)&15u)*17u,((rgb>>4)&15u)*17u,(rgb&15u)*17u};
                unsigned maximum=0;
                for(unsigned channel=0;channel<3;channel++){
                    const unsigned delta=orig[channel]>stock[channel]?
                       orig[channel]-stock[channel]:stock[channel]-orig[channel];
                    if(delta>maximum)maximum=delta;
                }
                if(maximum<=36u)closePens++;
                // Ignore a few overprinted palette entries, not the entire bank.
                score += maximum>96u ? 96u : maximum;
            }
            // Select only when Magneto's own ROM row is a convincing winner.
            if(row==7u){
                if(score<best){best=score;bestCostume=costume+1u;}
            }else if(score<second)second=score;
            // Preserve both candidates for log diagnosis below.
            (void)closePens;
        }
    }
    result.candidate=best;
    result.rival=second;
    // Reject low-similarity palettes and near ties. Threshold is below the
    // former 10/14 exact requirement but still needs affirmative evidence.
    if(best<560u && best+60u<second)result.costume=bestCostume;
    return result;
}
static inline std::uint32_t xm8661MagnetoTint(std::uint32_t from,unsigned costume){
    const auto c=xm8657Unpack(from);
    const unsigned y=xm8657Luma(c.r,c.g,c.b);
    if(y<=XM8659_FIGHTER_BLACK_POINT)return xm8657Pack(from,0u,0u,0u);
    // The user requested unmistakably deep red or deep blue, not the
    // Stage86.60 subtle warm/cool grey. Retain the exact dark-ink cutoff.
    if(costume==1u)return xm8657Pack(from,xm8657Clamp(y*0.99),xm8657Clamp(y*0.20),xm8657Clamp(y*0.29));
    if(costume==2u)return xm8657Pack(from,xm8657Clamp(y*0.20),xm8657Clamp(y*0.34),xm8657Clamp(y*1.04));
    return xm8659FighterInk(from);
}
#endif
