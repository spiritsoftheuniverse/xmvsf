#ifndef XMVSF_STAGE87_18_SOURCE_COLOR_H
#define XMVSF_STAGE87_18_SOURCE_COLOR_H
// Stage87.18: classify fighter body from its real sprite writer source PLUS
// the actor's dominant body palette, not from palette alone and not from
// screen proximity. Direct actor writers may also emit attached effects;
// therefore foreign palette artwork is never blindly forced grayscale.
#include "xmvsf_stage87_17_provenance.h"
#include <cstdint>
#include <unordered_map>
#include <cstring>
namespace XM8718 {
extern bool enabled;
enum Decision { Unchanged=0, FighterGray=1 };
struct TileEvidence {unsigned frames=0, lastFrame=~0u;};
struct Actor {
  int primary=-1, candidate=-1;
  unsigned stableFrames=0, lastPrimaryFrame=~0u;
  std::unordered_map<unsigned,TileEvidence> knownTiles;
};
struct Identity {
  unsigned lastFrame=~0u;
  Actor actors[2];
  void Reset(){lastFrame=~0u;actors[0]=Actor();actors[1]=Actor();}
  // Word is stored in the native 68000 byte order by CPS2 object RAM.
  static unsigned Word(const std::uint16_t* obj,unsigned off){
    return unsigned(BURN_ENDIAN_SWAP_INT16(obj[off]));
  }
  static unsigned Tile(const std::uint16_t* obj,unsigned i){
    return Word(obj,i*4u+2u) | ((Word(obj,i*4u+1u)&0x6000u)<<3);
  }
  void Observe(const std::uint16_t* obj,unsigned n,
               const XM8717::Source* sources,unsigned frame){
    if(lastFrame==frame)return;
    if(lastFrame!=~0u && (frame<lastFrame || frame-lastFrame>90u))Reset();
    lastFrame=frame;
    if(!obj || !sources)return;
    unsigned counts[2][32]{};
    for(unsigned i=0;i<n && i<1024u;++i){
      const auto& s=sources[i];
      if(!XM8717::Recent(s,frame) || s.owner<1u || s.owner>2u)continue;
      if((Word(obj,i*4u)>>13)!=6u)continue;
      ++counts[s.owner-1u][Word(obj,i*4u+3u)&31u];
    }
    for(unsigned a=0;a<2u;++a){
      unsigned sum=0,bestCount=0;int best=-1;
      for(unsigned p=0;p<32u;++p){
        sum+=counts[a][p];
        if(counts[a][p]>bestCount){bestCount=counts[a][p];best=int(p);}
      }
      Actor& act=actors[a];
      if(best>=0 && bestCount>=4u && bestCount*5u>=sum*3u){
        if(act.candidate==best)++act.stableFrames;
        else{act.candidate=best;act.stableFrames=1u;}
        if(act.primary<0 && act.stableFrames>=2u)act.primary=best;
        else if(act.primary!=best && act.primary>=0 && act.stableFrames>=10u &&
                act.lastPrimaryFrame!=~0u && frame>act.lastPrimaryFrame+60u){
          act.primary=best;
          act.knownTiles.clear(); // tag switch: don't carry former fighter art
        }
      } else {act.candidate=-1;act.stableFrames=0;}
      if(act.primary>=0 && counts[a][act.primary])act.lastPrimaryFrame=frame;
    }
    // Learn only source-confirmed normal-body palette artwork, never FX and
    // never the former renderer's guessed 'fighter' classifications.
    for(unsigned i=0;i<n && i<1024u;++i){
      const auto& s=sources[i];
      if(!XM8717::Recent(s,frame) || s.owner<1u || s.owner>2u)continue;
      if((Word(obj,i*4u)>>13)!=6u)continue;
      const unsigned a=s.owner-1u,pal=Word(obj,i*4u+3u)&31u;
      Actor& act=actors[a];
      if(act.primary<0 || pal!=unsigned(act.primary))continue;
      if(act.knownTiles.size()>8192u)act.knownTiles.clear();
      TileEvidence& e=act.knownTiles[Tile(obj,i)];
      if(e.lastFrame!=frame){e.lastFrame=frame;if(e.frames<65535u)++e.frames;}
    }
  }
  Decision Classify(const XM8717::Source& s,unsigned currentFrame,
                    unsigned priority,unsigned palette,unsigned tile,bool gameplay)const{
    if(!gameplay || priority!=6u || !XM8717::Recent(s,currentFrame) ||
       s.owner<1u || s.owner>2u)return Unchanged;
    const Actor& act=actors[s.owner-1u];
    if(act.primary>=0 && unsigned(act.primary)==palette)return FighterGray;
    auto it=act.knownTiles.find(tile);
    if(it!=act.knownTiles.end() && it->second.frames>=2u)return FighterGray;
    return Unchanged;
  }
};
inline Identity& State(){static Identity state;return state;}
}
#endif
