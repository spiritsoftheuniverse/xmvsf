#ifndef XMVSF_STAGE87_20_SECONDARY_OWNERSHIP_H
#define XMVSF_STAGE87_20_SECONDARY_OWNERSHIP_H
// Stage87.20: independently sourced body echoes and verified standalone effect.
// Based on actual producer -> draw capture, not old palette grade labels.
// The three echo sources reproduce 298/307 directly sourced P2 fighter tile IDs.
// Their 9 unmatched tiles stay under the existing RGB-first decision.
// A separate priority-6 source FF6830/palette 6 has no directly sourced
// fighter tile matches in the recorded combat sequence.
#include "xmvsf_stage87_18_source_color.h"
#include <cstdlib>
#include <cstdint>
namespace XM8720 {
enum Decision { Unchanged=0, FighterEchoGray=1, IndependentFxRGB=2 };
inline bool Enabled(){
    static const bool enabled=[](){
        const char* v=std::getenv("XM8720_SECONDARY_OWNER");
        return !v || !(v[0]=='0' && v[1]=='\0');
    }();
    return enabled;
}
inline bool KnownBodyTile(const XM8718::Identity& ids,unsigned tile){
    for(const auto& actor:ids.actors){
        const auto it=actor.knownTiles.find(tile);
        if(it!=actor.knownTiles.end() && it->second.frames>=2u)return true;
    }
    return false;
}
inline Decision Classify(const XM8717::Source& source,
                        const XM8718::Identity& ids,
                        unsigned frame,unsigned priority,unsigned palette,unsigned tile){
    if(!Enabled() || priority!=6u || !XM8717::Recent(source,frame) || source.owner!=0u)
        return Unchanged;
    // The trace establishes that these secondary objects make afterimages or
    // copies of the real P2 body. Tile identity prevents attached FX from
    // inheriting the gray decision, even when emitted by the same writer.
    if(source.a0==0xFF9C00u || source.a0==0xFF9CC0u || source.a0==0xFF9D80u)
        return KnownBodyTile(ids,tile)?FighterEchoGray:Unchanged;
    // Only this exact non-body source+palette combination was witnessed
    // incorrectly grayscaled (240 observations, 44 distinct body-vetoed tiles).
    if(source.a0==0xFF6830u && palette==6u && !KnownBodyTile(ids,tile))
        return IndependentFxRGB;
    return Unchanged;
}
}
#endif
