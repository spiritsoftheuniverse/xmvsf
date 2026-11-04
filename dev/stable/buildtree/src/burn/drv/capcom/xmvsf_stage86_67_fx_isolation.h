#ifndef XMVSF_STAGE86_67_FX_ISOLATION_H
#define XMVSF_STAGE86_67_FX_ISOLATION_H
// Stage86.67: keep known chromatic FX pens without dyeing entire sprite objects.
// Data: Stage86.65 user-supplied sprite_inventory.tsv, 19,100 observations.
// The overlapping tile IDs were observed as both (P2) FX and (P1) fighter;
// this is a risk signal, NOT a declaration that the tile contains no FX.
// Only tile IDs (never whole 256-tile families) participate in matching.
#include <cstdint>
#include <algorithm>
#include <iterator>
#include "xmvsf_stage86_57_palette.h"
static inline bool xm8667InventorySharedTile(unsigned tile){
    static const unsigned shared[]={
    484u, 14703u, 19147u, 27367u, 27368u, 27370u, 27372u,
    27373u, 27374u, 27376u, 27385u, 27388u, 27392u, 27393u,
    27397u, 27413u, 27428u, 27433u, 27449u, 131640u, 133159u,
    133315u, 135596u, 135612u, 137067u, 171062u, 171069u, 171074u,
    171152u, 171158u, 172122u, 172127u, 172133u, 172140u, 172184u,
    172234u, 172240u, 172256u, 172258u, 172504u, 172509u, 172525u,
    172527u, 172989u, 172996u, 173000u, 173003u, 173129u, 173132u,
    173150u, 173151u, 173152u, 173176u, 173182u, 173188u, 173196u,
    173200u, 173205u, 173455u, 173464u, 173468u, 173473u, 173478u,
    173488u, 173491u, 173498u, 173503u, 173507u, 173509u, 173517u,
    173534u, 173536u, 174177u, 174188u, 174195u, 174204u, 174209u,
    174214u, 174224u, 174231u, 174260u, 174280u, 174285u, 174288u,
    174290u, 174306u, 203870u, 229100u, 229361u, 230811u,
    };
    return std::binary_search(std::begin(shared),std::end(shared),tile);
}
// Conservative palette-pen accents for mixed body/effect artwork.
// Ignore costume midtones and dark outline pens; a pen must resemble a
// luminous attack effect. Distinguish magenta/pale-green Magneto glow, while
// retaining vivid red/blue/cyan/yellow effects on the same classification path.
static inline bool xm8667ChromaticEffectPen(std::uint32_t col){
    const auto c=xm8657Unpack(col);
    const unsigned r=c.r,g=c.g,b=c.b;
    const unsigned hi=std::max(r,std::max(g,b));
    const unsigned lo=std::min(r,std::min(g,b));
    if(hi<170u || hi-lo<65u)return false;
    const bool magenta=r>=180u && b>=170u && r>g+45u && b>g+40u;
    const bool paleGreen=g>=185u && g>r+25u && g>b+20u && r>=65u;
    const bool neonGreen=g>=215u && g>r+65u && g>b+55u;
    const bool red=r>=220u && r>g+95u && r>b+90u;
    const bool blue=b>=220u && b>r+85u && b>g+65u;
    const bool cyan=b>=185u && g>=180u && b>r+65u && g>r+65u;
    const bool yellow=r>=215u && g>=185u && b+95u<r && b+85u<g;
    return magenta || paleGreen || neonGreen || red || blue || cyan || yellow;
}
static inline unsigned xm8667FilteredPalette(const std::uint32_t* rgb,
                                               const std::uint32_t* ink,
                                               std::uint32_t* dst){
    unsigned colored=0;
    for(unsigned pen=0;pen<16u;++pen){
        const bool keep=xm8667ChromaticEffectPen(rgb[pen]);
        dst[pen]=keep?rgb[pen]:ink[pen];
        colored+=unsigned(keep);
    }
    return colored;
}
#endif
