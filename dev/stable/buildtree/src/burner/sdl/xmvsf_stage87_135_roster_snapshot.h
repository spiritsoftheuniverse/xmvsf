#ifndef XMVSF_STAGE87_135_ROSTER_SNAPSHOT_H
#define XMVSF_STAGE87_135_ROSTER_SNAPSHOT_H
// Stage 87.135: preserve native X-Men vs Street Fighter team/point identity when
// loading a first-live-frame Training snapshot. File-based interception works
// on a *copy* of the FB1/FS1 state: it does not edit the automatic match save.
// Do not interpret the actor activity word at +0 as a character ID. The real
// native fighter ID is a 16-bit word at fighter-object offset +0x52.
#include "burnint.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
namespace XM87135 {
static const unsigned SLOTS[4]={0x4000u,0x4400u,0x4800u,0x4c00u};
static const char* CACHE_DIR="config/games/xmvsfu_training_rosters";
static const char* WORKING="config/games/xmvsfu_training_roster_loaded.fs";
static inline unsigned u32(const unsigned char* b){
 return (unsigned)b[0]|((unsigned)b[1]<<8)|((unsigned)b[2]<<16)|((unsigned)b[3]<<24);
}
static inline unsigned word(const unsigned char* b,unsigned o){
 return (unsigned)b[o]|((unsigned)b[o+1]<<8);
}
struct Team { unsigned id[4]={0,0,0,0}; };
static inline bool validId(unsigned id){return id<=0x40u && (id&1u)==0u;}
static inline bool readIds(const unsigned char* ram, Team& t){
 for(unsigned i=0;i<4;++i){t.id[i]=word(ram,SLOTS[i]+0x52u);if(!validId(t.id[i]))return false;}
 return true;
}
static inline bool fromCPU(Team& t){
 cpu_core_config* c=GetCpuCoreConfig(0);
 if(!c||!c->open||!c->close||!c->read)return false;
 c->open(0);
 const unsigned phase=c->read(0xFF5000u)&255u;
 const unsigned p0=(c->read(0xFFA014u)&255u)<<24 | (c->read(0xFFA015u)&255u)<<16 |
                   (c->read(0xFFA016u)&255u)<<8 | (c->read(0xFFA017u)&255u);
 for(unsigned i=0;i<4u;++i){
  const unsigned a=0xFF0000u+SLOTS[i]+0x52u;
  t.id[i]=((c->read(a)&255u)<<8)|(c->read(a+1u)&255u);
 }
 const bool actors=(c->read(0xFF4000u)&255u)!=0 && (c->read(0xFF4400u)&255u)!=0;
 c->close();
 if(phase!=8u || p0!=0x00FF4000u || !actors)return false;
 for(unsigned i=0;i<4u;++i)if(!validId(t.id[i]))return false;
 return true;
}
static inline bool fileRead(const char* path,std::vector<unsigned char>& out){
 FILE* f=std::fopen(path,"rb");if(!f)return false;
 if(std::fseek(f,0,SEEK_END)!=0){std::fclose(f);return false;}
 const long sz=std::ftell(f);
 if(sz<96 || sz>2*1024*1024){std::fclose(f);return false;}
 std::rewind(f);out.resize((size_t)sz);
 const bool ok=std::fread(out.data(),1,out.size(),f)==out.size();
 std::fclose(f);return ok;
}
static inline bool fileWrite(const char* path,const std::vector<unsigned char>& b){
 FILE* f=std::fopen(path,"wb");if(!f)return false;
 const bool ok=std::fwrite(b.data(),1,b.size(),f)==b.size() && std::fflush(f)==0;
 if(std::fclose(f)!=0)return false;
 return ok;
}
// This project's statec.cpp writes an UNCOMPRESSED FB1/FS1 variable buffer.
// Find "CpsRamFF" by its native variable hash (0x10518621), not by block order.
static inline bool findRam(std::vector<unsigned char>& b,unsigned char*& ram){
 ram=nullptr;
 if(b.size()<100 || std::memcmp(b.data(),"FB1 FS1 ",8)!=0)return false;
 const unsigned clen=u32(b.data()+8), n=u32(b.data()+24);
 if(clen<0x40u || n<12u || 76ull+n>b.size() || 12ull+clen>b.size())return false;
 size_t p=76u;
 if(u32(b.data()+p)!=0xa55a0000u)return false;
 p+=8;
 const size_t end=76u+n;
 while(p+12<=end){
  if(u32(b.data()+p)!=0xa55a0001u)return false;
  const unsigned len=u32(b.data()+p+4),hash=u32(b.data()+p+8);
  p+=12;
  if((size_t)len>end-p)return false;
  if(hash==0x10518621u){
   if(len!=0x10000u)return false;
   ram=b.data()+p;return true;
  }
  p+=len;
 }
 return false;
}
static inline std::string cacheName(const Team& t){
 char b[180];std::snprintf(b,sizeof(b),"%s/roster_%02X_%02X_%02X_%02X.fs",
   CACHE_DIR,t.id[0],t.id[2],t.id[1],t.id[3]);return std::string(b);
}
static inline void cacheFirstFrame(const char* saved){
 std::vector<unsigned char> state;unsigned char* ram=nullptr;Team ids;
 if(!fileRead(saved,state)||!findRam(state,ram)||!readIds(ram,ids))return;
 #ifdef _WIN32
 _mkdir(CACHE_DIR);
 #else
 mkdir(CACHE_DIR,0755);
 #endif
 const std::string dst=cacheName(ids);
 if(!fileWrite(dst.c_str(),state))
  std::fprintf(stderr,"XM87.135 ROSTER_CACHE_FAILED path=%s\n",dst.c_str());
 else
  std::fprintf(stderr,"XM87.135 ROSTER_CACHED point=%02X,%02X reserve=%02X,%02X\n",
    ids.id[0],ids.id[1],ids.id[2],ids.id[3]);
}
static inline bool sideMatches(const Team& now,const Team& was,unsigned p,unsigned r){
 return (now.id[p]==was.id[p] && now.id[r]==was.id[r]) ||
        (now.id[p]==was.id[r] && now.id[r]==was.id[p]);
}
static inline bool identical(const Team& a,const Team& b){
 for(unsigned i=0;i<4;++i)if(a.id[i]!=b.id[i])return false;
 return true;
}
static inline void copySpan(unsigned char* dst,const unsigned char* original,unsigned base,unsigned len){
 std::memcpy(dst+base,original+base,len);
}
// For native tags the ROM swaps which character occupies point/reserve.
// Use both correctly initialized 0x400-byte CHARACTER records from the same
// first-frame state; restore ROLE/neutral-frame fields from the original slots.
// This is not intended for importing an arbitrary foreign fighter ID.
static inline void swapSide(unsigned char* ram,unsigned a,unsigned b){
 unsigned char originalA[0x400],originalB[0x400];
 std::memcpy(originalA,ram+a,0x400);
 std::memcpy(originalB,ram+b,0x400);
 std::memcpy(ram+a,originalB,0x400);
 std::memcpy(ram+b,originalA,0x400);
 // Only native slot activity/role and world position/velocity are
 // restored from the old point/bench addresses. +0x06/+0x08 are the
 // character's live animation/reaction and native step, not position.
 // Copying +0x00..+0x1F wholesale substituted Cyclops' idle animation
 // state for Wolverine's after a synthesized roster swap; the incoming
 // point could remain invisible until a movement generated a new pose.
 copySpan(ram+a,originalA,0x000,0x006);
 copySpan(ram+b,originalB,0x000,0x006);
 copySpan(ram+a,originalA,0x00c,0x014);
 copySpan(ram+b,originalB,0x00c,0x014);
 // The native +0x4B facing flag maps to RAW saved-RAM byte +0x4A
 // (CPS2 68000 byte lanes are XOR 1). The surrounding +0x4B..+0x4F
 // raw bytes are not proven slot-role state: keep them with the
 // incoming character instead of restoring Cyclops-owned display data.
 ram[a+0x04au]=(unsigned char)((ram[a+0x04au]&~1u)|(originalA[0x04au]&1u));
 ram[b+0x04au]=(unsigned char)((ram[b+0x04au]&~1u)|(originalB[0x04au]&1u));
 // Keep the point slot's neutral action WORD, not the old fighter's
 // character-dependent animation/action substates at +0x62..+0x6B.
 copySpan(ram+a,originalA,0x060,0x002);
 copySpan(ram+b,originalB,0x060,0x002);
 copySpan(ram+a,originalA,0x200,0x010);
 copySpan(ram+b,originalB,0x200,0x010);
 copySpan(ram+a,originalA,0x220,0x008);
 copySpan(ram+b,originalB,0x220,0x008);
}
// Called immediately before BurnStateLoad, with the *live* roster captured.
// Finds a full first-frame snapshot of the same two team members; when tagging
// changed who's on point, makes a separate, bounded scratch copy before load.
static inline bool prepare(const char* primary,const Team& now,std::string& loadPath){
 std::vector<unsigned char> src;unsigned char* ram=nullptr;Team was;
 // The current match has priority. An old cached copy of this roster
 // might belong to an earlier match, stage, or build.
 const std::string exact=cacheName(now);
 const char* candidate[2]={primary,exact.c_str()};
 for(unsigned attempt=0;attempt<2;++attempt){
  src.clear();ram=nullptr;
  if(!fileRead(candidate[attempt],src)||!findRam(src,ram)||!readIds(ram,was))continue;
  if(identical(now,was)){loadPath=candidate[attempt];return true;}
 }
 // Next seek a first-frame snapshot of these SAME fighters in either tag order.
 for(unsigned mask=0;mask<4u;++mask){
  Team key=now;
  if(mask&1u)std::swap(key.id[0],key.id[2]);
  if(mask&2u)std::swap(key.id[1],key.id[3]);
  const std::string from=cacheName(key);
  const char* sources[2]={primary,from.c_str()};
  for(unsigned j=0;j<2u;++j){
   src.clear();ram=nullptr;
   if(!fileRead(sources[j],src)||!findRam(src,ram)||!readIds(ram,was))continue;
   if(!sideMatches(now,was,0,2)||!sideMatches(now,was,1,3))continue;
   const unsigned swaps=(now.id[0]!=was.id[0]?1u:0u) | (now.id[1]!=was.id[1]?2u:0u);
   if(swaps&1u)swapSide(ram,0x4000u,0x4800u);
   if(swaps&2u)swapSide(ram,0x4400u,0x4c00u);
   Team result;
   if(!readIds(ram,result)||!identical(now,result))continue;
   if(!fileWrite(WORKING,src))return false;
   loadPath=WORKING;
   std::fprintf(stderr,"XM87.135 MATCH_STATE_ROSTER_INTERCEPT point=%02X,%02X reserve=%02X,%02X tags=%u\n",
       now.id[0],now.id[1],now.id[2],now.id[3],swaps);
   return true;
  }
 }
 std::fprintf(stderr,"XM87.135 ROSTER_REFUSED: no first-frame state for live team %02X/%02X vs %02X/%02X; unchanged\n",
     now.id[0],now.id[2],now.id[1],now.id[3]);
 return false;
}
} // namespace XM87135
#endif
