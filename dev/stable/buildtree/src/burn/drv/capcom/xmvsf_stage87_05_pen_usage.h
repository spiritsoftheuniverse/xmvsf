#ifndef XMVSF_STAGE87_05_PEN_USAGE_H
#define XMVSF_STAGE87_05_PEN_USAGE_H
// Exact decoded CPS2 tile art: 16x16 pixels, packed as 128 bytes of 4bpp.
// FBNeo's Ctv renderer considers pen 0 transparent. This reports actual
// palette indices used by artwork, never guesses from palette RGB alone.
#include <cstdint>
namespace XM8705 {
static inline std::uint16_t TilePenMask(const std::uint8_t* decoded){
    std::uint16_t mask=0;
    for(unsigned i=0;i<128u;++i){
        mask=std::uint16_t(mask | (1u << (decoded[i]&15u)) |
                                    (1u << (decoded[i]>>4)));
    }
    return std::uint16_t(mask & 0xfffeu); // zero is transparency
}
static inline unsigned SubTile(unsigned base,unsigned x,unsigned y){
    return (base & ~15u) + (y<<4) + ((base+x)&15u);
}
static inline std::uint64_t ObservationKey(unsigned tile,unsigned pal,
                      unsigned priority,unsigned width,unsigned height,unsigned grade){
    return (std::uint64_t(tile)&0x3ffffull) |
        ((std::uint64_t(pal)&31ull)<<18) | ((std::uint64_t(priority)&7ull)<<23) |
        ((std::uint64_t(width-1)&15ull)<<26) |
        ((std::uint64_t(height-1)&15ull)<<30) |
        ((std::uint64_t(grade)&15ull)<<34);
}
}
#endif
