#ifndef XMVSF_STAGE87_352_CYCLOPS_SOB_CLEANUP_H
#define XMVSF_STAGE87_352_CYCLOPS_SOB_CLEANUP_H
// Original native CPS2 helper cleanup after an ACTUAL DHC out of
// Cyclops' Super Optic Blast (character 02, Hyper D6 02).
// Original helper records carry their maker at +52, owner slot at +54.
// Never classify by sprite palette, touch opponent FX, or rewrite ROM.
#include <cstdint>
namespace XM87352CyclopsFx {
static inline unsigned b(const std::uint8_t* r,unsigned a){return r[(a&65535u)^1u];}
static inline unsigned w(const std::uint8_t* r,unsigned a){return (b(r,a)<<8u)|b(r,a+1u);}
static inline unsigned l(const std::uint8_t* r,unsigned a){return (w(r,a)<<16u)|w(r,a+2u);}
static inline void sb(std::uint8_t* r,unsigned a,unsigned v){r[(a&65535u)^1u]=std::uint8_t(v);}
static inline bool valid(unsigned a){return a>=0xFF5000u && a<=0xFFFE00u;}
struct Tracked {unsigned base=0u;bool live=false;};
struct Side {
 bool armed=false,committed=false;
 unsigned at=0u,point=0u,reserve=0u,n=0u;
 Tracked fx[12];
 void reset(){*this=Side();}
 unsigned capture(const std::uint8_t* ram){
  if(!armed||!ram)return 0u;
  unsigned newCount=0u;
  for(unsigned slot=1u;slot<=12u;++slot){
   const unsigned ptr=l(ram,0xFFE3D8u-4u*slot);
   if(!valid(ptr))break;
   if(w(ram,ptr+0x52u)!=0x02u ||
      l(ram,ptr+0x54u)!=0xFF0000u+point ||
      !b(ram,ptr+0x03u))continue;
   bool known=false;
   for(unsigned i=0u;i<n;++i)if(fx[i].live&&fx[i].base==ptr)known=true;
   if(!known && n<12u){fx[n].base=ptr;fx[n].live=true;++n;++newCount;}
  }
  return newCount;
 }
 void begin(const std::uint8_t* ram,unsigned frame,unsigned owner,
            unsigned other){
  reset();armed=true;at=frame;point=owner;reserve=other;
  capture(ram);
 }
 // Call once per real emulated frame. 'nativeSwap' is based on the
 // ORIGINAL M68K actor identities, not on a host's tag prediction.
 unsigned update(std::uint8_t* ram,unsigned frame,bool nativeSwap,
                 bool nativeFailed,bool sourceBackOnPoint){
  if(!armed||!ram)return 0u;
  if(frame<at || nativeFailed || sourceBackOnPoint){reset();return 0u;}
  if(nativeSwap)committed=true;
  if(!committed){
   if(frame-at>20u)reset();
   else capture(ram);
   return 0u;
  }
  unsigned linked[12]={};unsigned ln=0u;
  for(unsigned slot=1u;slot<=12u;++slot){
   const unsigned ptr=l(ram,0xFFE3D8u-4u*slot);
   if(!valid(ptr))break;
   linked[ln++]=ptr;
  }
  // A vanished link invalidates the old identity; a reused address
  // must never inherit a cleanup scheduled by the previous Hyper.
  for(unsigned i=0u;i<n;++i)if(fx[i].live){
   bool found=false;
   for(unsigned j=0u;j<ln;++j)if(fx[i].base==linked[j])found=true;
   if(!found)fx[i].live=false;
  }
  if(frame-at<210u)return 0u; // allow 3.5 seconds for normal native FX
  unsigned retired=0u;
  for(unsigned i=0u;i<n;++i)if(fx[i].live){
   const unsigned base=fx[i].base;
   const unsigned maker=w(ram,base+0x52u);
   const unsigned owner=l(ram,base+0x54u);
   // Require STILL active, same maker, and one of the original
   // fighter's two owned slots. Do not touch incoming/other FX.
   if(maker==0x02u &&
      (owner==0xFF0000u+point || owner==0xFF0000u+reserve) &&
      b(ram,base+0x03u)){
    sb(ram,base+0x03u,0u);
    sb(ram,base+0x70u,0u);sb(ram,base+0x71u,0u);
    sb(ram,base+0x72u,0u);sb(ram,base+0x73u,0u);
    sb(ram,base+0x82u,0u);
    ++retired;
   }
  }
  reset();return retired;
 }
};
struct State {Side side[2];void reset(){side[0].reset();side[1].reset();}};
static inline State& state(){static State s;return s;}
} // namespace XM87352CyclopsFx
#endif
