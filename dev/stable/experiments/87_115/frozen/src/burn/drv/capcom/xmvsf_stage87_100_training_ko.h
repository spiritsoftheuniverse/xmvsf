#ifndef XMVSF_STAGE87_100_TRAINING_KO_H
#define XMVSF_STAGE87_100_TRAINING_KO_H
// XM87.101: training-only, opcode-fetch dual path clamp for lethal XMVSFU damage.
// Normal attack damage at $0155B0 SUB.W D5,($210,A6), BGT $0155F4;
// when lethal, $0155B6 BTST #5,($16,A0); $0155BC BNE $0155EE.
// Stage87.101 changes that BNE to BRA: use original $0155EE MOVE.W #1,HP.
// Other hit-type damage at $015B78 SUB.W D5,($210,A6), BGT $015B96.
// Original lethal path $015B7E: CLR.W ($210,A6), CLR.W ($21A,A6),
// MOVE.L hit source to KO global, MOVE.B side, ST KO flag.
// Patch the first two instructions with MOVE.W #1,($210,A6); BRA.S $15B96.
// A fighter stays alive with genuine native HP loss down to one HP; zero HP
// would also trip many unrelated native dead-player gates. Never alter ROM ZIP.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
namespace XM87100TrainingKO {
static constexpr unsigned NORMAL_OFFSET=0x155BCu;
static constexpr unsigned NORMAL_LENGTH=2u;
static constexpr std::uint8_t NORMAL_ORIGINAL[NORMAL_LENGTH]={0x66,0x30};
static constexpr std::uint8_t NORMAL_TRAINING[NORMAL_LENGTH]={0x60,0x30};
static constexpr unsigned OFFSET=0x15B7Eu;
static constexpr unsigned LENGTH=8u;
static constexpr std::uint8_t ORIGINAL[LENGTH]={0x42,0x6e,0x02,0x10,0x42,0x6e,0x02,0x1a};
static constexpr std::uint8_t TRAINING[LENGTH]={0x3d,0x7c,0x00,0x01,0x02,0x10,0x60,0x10};
enum class Layout{Unknown,Big,WordSwap};
inline bool bytesEqual(const std::uint8_t* p,const std::uint8_t* data,bool swapped){
 for(unsigned i=0;i<LENGTH;i+=2u){
  if(p[i]!=data[i+(swapped?1u:0u)]||p[i+1]!=data[i+(swapped?0u:1u)])return false;
 }
 return true;
}
inline void writeBytes(std::uint8_t* p,const std::uint8_t* data,bool swapped){
 for(unsigned i=0;i<LENGTH;i+=2u){
  p[i]=data[i+(swapped?1u:0u)];p[i+1]=data[i+(swapped?0u:1u)];
 }
}
struct Controller {
 bool installed=false, refused=false;
 Layout layout=Layout::Unknown;
 unsigned transitions=0;
 bool before(std::uint8_t* code,unsigned length,const char* driver,bool training){
  const bool supported=driver&&std::strcmp(driver,"xmvsfu")==0;
  if(!code||length<OFFSET+LENGTH||length<NORMAL_OFFSET+NORMAL_LENGTH){
   installed=false;
   if(!refused)std::fprintf(stderr,"XM87.101 TRAINING KO REFUSED: null/short CpsCode (%u)\n",length);
   refused=true;return false;
  }
  // Decode BOTH native branch sites before touching either. Mixed layouts,
  // unknown signatures, or partial restoration are not permitted.
  Layout detected=Layout::Unknown;
  bool primaryPatched=false,normalPatched=false;
  bool matched=false;
  for(Layout l:{Layout::Big,Layout::WordSwap}){
   const bool swap=l==Layout::WordSwap;
   const bool oldPrimary=bytesEqual(code+OFFSET,ORIGINAL,swap);
   const bool newPrimary=bytesEqual(code+OFFSET,TRAINING,swap);
   const bool oldNormal=(code[NORMAL_OFFSET]==NORMAL_ORIGINAL[swap?1:0]&&
       code[NORMAL_OFFSET+1]==NORMAL_ORIGINAL[swap?0:1]);
   const bool newNormal=(code[NORMAL_OFFSET]==NORMAL_TRAINING[swap?1:0]&&
       code[NORMAL_OFFSET+1]==NORMAL_TRAINING[swap?0:1]);
   if((oldPrimary||newPrimary)&&(oldNormal||newNormal)){
    detected=l;primaryPatched=newPrimary;normalPatched=newNormal;matched=true;break;
   }
  }
  if(!matched){
   if(!refused)std::fprintf(stderr,
     "XM87.101 TRAINING KO REFUSED signature mismatch normal @0155BC=%02X%02X other @015B7E=%02X%02X\n",
     code[NORMAL_OFFSET],code[NORMAL_OFFSET+1],code[OFFSET],code[OFFSET+1]);
   refused=true;installed=false;layout=Layout::Unknown;return false;
  }
  refused=false;layout=detected;
  const bool want=supported&&training;
  if(want!=primaryPatched||want!=normalPatched){
   std::uint8_t backupPrimary[LENGTH],backupNormal[NORMAL_LENGTH];
   std::memcpy(backupPrimary,code+OFFSET,LENGTH);
   std::memcpy(backupNormal,code+NORMAL_OFFSET,NORMAL_LENGTH);
   const bool swap=detected==Layout::WordSwap;
   writeBytes(code+OFFSET,want?TRAINING:ORIGINAL,swap);
   // Branch is a single 16-bit 68000 instruction; use the same byte layout.
   code[NORMAL_OFFSET]=(want?NORMAL_TRAINING:NORMAL_ORIGINAL)[swap?1:0];
   code[NORMAL_OFFSET+1]=(want?NORMAL_TRAINING:NORMAL_ORIGINAL)[swap?0:1];
   const std::uint8_t* expectNormal=want?NORMAL_TRAINING:NORMAL_ORIGINAL;
   if(!bytesEqual(code+OFFSET,want?TRAINING:ORIGINAL,swap)||
      code[NORMAL_OFFSET]!=expectNormal[swap?1:0]||
      code[NORMAL_OFFSET+1]!=expectNormal[swap?0:1]){
    std::memcpy(code+OFFSET,backupPrimary,LENGTH);
    std::memcpy(code+NORMAL_OFFSET,backupNormal,NORMAL_LENGTH);
    installed=primaryPatched&&normalPatched;return false;
   }
   ++transitions;
   std::fprintf(stderr,"XM87.101 Training dual KO paths %s normal@0155BC other@015B7E mode=%s driver=%s\n",
     want?"INSTALLED":"RESTORED",training?"Training":"Other",driver?driver:"unknown");
  }
  installed=want;
  return supported;
 }
};
inline Controller& state(){static Controller c;return c;}
} // namespace XM87100TrainingKO
#endif
