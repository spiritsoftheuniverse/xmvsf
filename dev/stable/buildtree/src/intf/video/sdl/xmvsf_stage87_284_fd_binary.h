#ifndef XMVSF_STAGE87_284_FD_BINARY_H
#define XMVSF_STAGE87_284_FD_BINARY_H
// Final Destination 87.284: offline seekable BGRA32 binary frame packs.
// Native pack format is XMFDBIN1: magic[8], six LE32 fields (width,
// height, count, format=1 for BGRA8, rowBytes, reserved), then 32-bit
// pixels in full coalesced top-to-bottom frames. The worker reads the
// binary pack directly into a 3-frame queue. Source PNGs are a fallback.
// The game's main/render thread never decodes images or opens video files.
// Only the worker calls IMG_Load; only the render thread touches SDL textures.
// Two reusable streaming GPU textures; never allocate a texture per PNG.
// No GPU work except a changed frame, and no render-thread waiting for decoding.
// PNGs are full coalesced frames, not GIF patches; retain 4-frame hold at 60 Hz.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
namespace XMFDVideo {
static const std::uint64_t FPS=60u,WAIT_INITIAL=600u,GAP=300u,FADE=60u,HOLD=4u;
static const unsigned CLIPS=3u,COUNT[CLIPS]={193u,200u,396u};
static const char* const NAMES[CLIPS]={"fd01","fd02","fd03"};
static std::string path(unsigned clip,unsigned index){
 char suffix[48];
 std::snprintf(suffix,sizeof(suffix),"/frame_%05u.png",index);
 return std::string(NAMES[clip])+suffix;
}
static std::string findRoot(unsigned clip){
 std::vector<std::string> roots={"_art/video/","../_art/video/","../../_art/video/"};
 char* base=SDL_GetBasePath();
 if(base){
  const std::string p(base);SDL_free(base);
  roots.push_back(p+"_art/video/");
  roots.push_back(p+"../_art/video/");
  roots.push_back(p+"../../_art/video/");
 }
 for(const std::string& root:roots){
  SDL_RWops* rw=SDL_RWFromFile((root+path(clip,0u)).c_str(),"rb");
  if(rw){SDL_RWclose(rw);return root;}
 }
 return {};
}
struct Frame {
 unsigned clip=0u,index=0u,generation=0u;
 SDL_Surface* pixels=nullptr;
 bool shared=false; // Cached memory; caller must not free the surface.
};
static std::string findPack(unsigned clip){
 std::vector<std::string> roots={"_art/video/","../_art/video/","../../_art/video/"};
 char* base=SDL_GetBasePath();
 if(base){
  const std::string p(base);SDL_free(base);
  roots.push_back(p+"_art/video/");
  roots.push_back(p+"../_art/video/");
  roots.push_back(p+"../../_art/video/");
 }
 for(const std::string& root:roots){
  const std::string filename=root+NAMES[clip]+".fdb";
  SDL_RWops* rw=SDL_RWFromFile(filename.c_str(),"rb");
  if(rw){SDL_RWclose(rw);return filename;}
 }
 return {};
}
static unsigned u32le(const unsigned char* p){
 return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);
}
struct PackIndex { std::uint64_t offset=0; unsigned size=0; };
static std::uint64_t u64le(const unsigned char* p){
 return (std::uint64_t)u32le(p)|((std::uint64_t)u32le(p+4)<<32);
}
static SDL_RWops* openPack(unsigned clip,int& w,int& h,bool& compressed,std::vector<PackIndex>& index){
 const std::string filename=findPack(clip);
 if(filename.empty())return nullptr;
 SDL_RWops* rw=SDL_RWFromFile(filename.c_str(),"rb");
 if(!rw)return nullptr;
 unsigned char hdr[32]={};
 const bool header=SDL_RWread(rw,hdr,1u,32u)==32u;
 const bool v1=header && std::memcmp(hdr,"XMFDBIN1",8u)==0 && u32le(hdr+20)==1u;
 const bool v2=header && std::memcmp(hdr,"XMFDBIN2",8u)==0 && u32le(hdr+20)==2u;
 const Sint64 fileSize=SDL_RWseek(rw,0,RW_SEEK_END);
 bool valid=(v1||v2) &&
    u32le(hdr+8)>=1u && u32le(hdr+8)<=8192u &&
    u32le(hdr+12)>=1u && u32le(hdr+12)<=8192u &&
    u32le(hdr+16)==COUNT[clip] &&
    u32le(hdr+24)==u32le(hdr+8)*4u && u32le(hdr+28)==0u;
 if(valid && v1)
  valid=fileSize==(Sint64)(32u+(std::uint64_t)COUNT[clip]*
           (std::uint64_t)u32le(hdr+24)*(std::uint64_t)u32le(hdr+12));
 if(valid && v2){
  const std::uint64_t start=32u+(std::uint64_t)COUNT[clip]*16u;
  valid=fileSize>=(Sint64)start && SDL_RWseek(rw,32,RW_SEEK_SET)==32;
  if(valid){
   index.resize(COUNT[clip]);
   std::uint64_t previous=start;
   for(unsigned i=0;i<COUNT[clip];++i){
    unsigned char entry[16];
    if(SDL_RWread(rw,entry,1u,16u)!=16u){valid=false;break;}
    const std::uint64_t offset=u64le(entry);
    const unsigned size=u32le(entry+8);
    if(offset<previous||size<8u||size>67108864u||
       offset>(std::uint64_t)fileSize||
       (std::uint64_t)size>(std::uint64_t)fileSize-offset||
       u32le(entry+12)!=0u){valid=false;break;}
    index[i].offset=offset;index[i].size=size;
    previous=offset+(std::uint64_t)size;
   }
  }
 }
 compressed=v2;
 if(!valid){
  std::fprintf(stderr,"XM87.284 FD_BINARY_INVALID clip=%u path=%s; PNG fallback\n",
       clip+1u,filename.c_str());
  SDL_RWclose(rw);return nullptr;
 }
 w=(int)u32le(hdr+8);h=(int)u32le(hdr+12);
 std::fprintf(stderr,
  "XM87.284 FD_BINARY_READY clip=%u frames=%u size=%dx%d path=%s compressed=%u\n",
  clip+1u,COUNT[clip],w,h,filename.c_str(),compressed?1u:0u);
 return rw;
}
// Dedicated low-priority selection-screen preloader. Only this worker writes
// the persistent surfaces, and they remain owned until video.close().
struct Preloader {
 SDL_mutex* mutex=nullptr;
 SDL_Thread* thread=nullptr;
 bool stop=false,enabled=false;
 unsigned loaded=0u;
 std::vector<SDL_Surface*> frames[CLIPS];
 static int SDLCALL run(void* context){
  Preloader& p=*(Preloader*)context;
  SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
  std::string roots[CLIPS];
  SDL_RWops* packs[CLIPS]={nullptr,nullptr,nullptr};
  bool compressed[CLIPS]={false,false,false};
  std::vector<PackIndex> indexes[CLIPS];
  int widths[CLIPS]={0,0,0},heights[CLIPS]={0,0,0};
  bool checked[CLIPS]={false,false,false};
  unsigned clip=0u,frame=0u;
  while(clip<CLIPS){
   SDL_LockMutex(p.mutex);
   const bool quit=p.stop,active=p.enabled;
   SDL_UnlockMutex(p.mutex);
   if(quit)break;
   if(!active){SDL_Delay(8);continue;}
   if(frame>=COUNT[clip]){++clip;frame=0u;continue;}
   if(!checked[clip]){
    checked[clip]=true;
    packs[clip]=openPack(clip,widths[clip],heights[clip],compressed[clip],indexes[clip]);
    if(!packs[clip])roots[clip]=findRoot(clip);
    if(!packs[clip] && roots[clip].empty()){
     std::fprintf(stderr,"XM87.286 FD_PRELOAD_MISSING clip=%u\n",clip+1u);
     ++clip;frame=0u;continue;
    }
   }
   SDL_Surface* surface=nullptr;
   if(packs[clip] && compressed[clip]){
    const PackIndex& entry=indexes[clip][frame];
    std::vector<unsigned char> png(entry.size);
    if(SDL_RWseek(packs[clip],(Sint64)entry.offset,RW_SEEK_SET)==(Sint64)entry.offset &&
       SDL_RWread(packs[clip],png.data(),1u,entry.size)==entry.size){
     SDL_RWops* memory=SDL_RWFromConstMem(png.data(),(int)entry.size);
     if(memory){
      SDL_Surface* source=IMG_Load_RW(memory,1);
      if(source){
       surface=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_BGRA32,0);
       SDL_FreeSurface(source);
       if(surface && (surface->w!=widths[clip]||surface->h!=heights[clip])){
        SDL_FreeSurface(surface);surface=nullptr;
       }
      }
     }
    }
   }else if(packs[clip]){
    const int w=widths[clip],h=heights[clip];
    surface=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_BGRA32);
    if(surface){
     const Sint64 offset=32+(Sint64)frame*(Sint64)w*(Sint64)h*4;
     bool good=SDL_RWseek(packs[clip],offset,RW_SEEK_SET)==offset;
     if(good && surface->pitch==w*4){
      const size_t bytes=(size_t)w*(size_t)h*4u;
      good=SDL_RWread(packs[clip],surface->pixels,1u,bytes)==bytes;
     }else if(good){
      for(int y=0;good && y<h;++y){
       unsigned char* row=(unsigned char*)surface->pixels+(size_t)y*surface->pitch;
       good=SDL_RWread(packs[clip],row,1u,(size_t)w*4u)==(size_t)w*4u;
      }
     }
     if(!good){SDL_FreeSurface(surface);surface=nullptr;}
    }
   }else{
    const std::string filename=roots[clip]+path(clip,frame);
    SDL_Surface* source=IMG_Load(filename.c_str());
    if(source){
     surface=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_BGRA32,0);
     SDL_FreeSurface(source);
    }
   }
   if(surface){
    SDL_LockMutex(p.mutex);
    p.frames[clip][frame]=surface;
    ++p.loaded;
    const unsigned total=COUNT[0]+COUNT[1]+COUNT[2];
    if(p.loaded%100u==0u || p.loaded==total)
     std::fprintf(stderr,"XM87.286 FD_PRELOAD_PROGRESS %u/%u\n",p.loaded,total);
    SDL_UnlockMutex(p.mutex);
   }else{
    std::fprintf(stderr,"XM87.286 FD_PRELOAD_DECODE_FAIL clip=%u frame=%u\n",clip+1u,frame);
   }
   ++frame;
   SDL_Delay(1); // bounded menu/selection CPU competition
  }
  for(unsigned c=0u;c<CLIPS;++c)if(packs[c])SDL_RWclose(packs[c]);
  if(clip==CLIPS)
   std::fprintf(stderr,"XM87.286 FD_PRELOAD_FINISHED frames=%u/%u\n",
                p.loaded,COUNT[0]+COUNT[1]+COUNT[2]);
  return 0;
 }
 void setEnabled(bool on){
  if(on && !mutex){
   mutex=SDL_CreateMutex();
   if(!mutex)return;
   for(unsigned c=0u;c<CLIPS;++c)frames[c].resize(COUNT[c],nullptr);
   thread=SDL_CreateThread(run,"XMFD predecode",this);
   if(!thread){
    SDL_DestroyMutex(mutex);mutex=nullptr;
    return;
   }
   std::fprintf(stderr,"XM87.286 FD_PRELOAD_START lowPriority=1 total=%u\n",
                COUNT[0]+COUNT[1]+COUNT[2]);
  }
  if(mutex){
   SDL_LockMutex(mutex);
   enabled=on;
   SDL_UnlockMutex(mutex);
  }
 }
 bool peek(unsigned clip,unsigned index,Frame& result){
  if(!mutex||clip>=CLIPS||index>=COUNT[clip])return false;
  SDL_LockMutex(mutex);
  SDL_Surface* pixels=frames[clip][index];
  SDL_UnlockMutex(mutex);
  if(!pixels)return false;
  result.clip=clip;result.index=index;result.pixels=pixels;result.shared=true;
  return true;
 }
 bool contains(unsigned clip,unsigned index){
  Frame unused;
  return peek(clip,index,unused);
 }
 bool complete(){
  if(!mutex)return false;
  SDL_LockMutex(mutex);
  const bool ready=loaded==COUNT[0]+COUNT[1]+COUNT[2];
  SDL_UnlockMutex(mutex);
  return ready;
 }
 void shutdown(){
  if(!mutex)return;
  SDL_LockMutex(mutex);stop=true;SDL_UnlockMutex(mutex);
  if(thread){SDL_WaitThread(thread,nullptr);thread=nullptr;}
  for(unsigned c=0u;c<CLIPS;++c){
   for(SDL_Surface* f:frames[c])if(f)SDL_FreeSurface(f);
   frames[c].clear();
  }
  SDL_DestroyMutex(mutex);mutex=nullptr;
  stop=enabled=false;loaded=0u;
 }
};
inline Preloader& preload(){static Preloader instance;return instance;}
struct Decoder {
 SDL_mutex* mutex=nullptr;
 SDL_Thread* thread=nullptr;
 bool stop=false;
 bool requested=false;
 unsigned clip=0u,index=0u,generation=1u;
 std::deque<Frame> ready;
 static int SDLCALL run(void* ctx){
  Decoder& d=*(Decoder*)ctx;
  std::string roots[CLIPS];
  bool missing[CLIPS]={false,false,false};
  bool checked[CLIPS]={false,false,false};
  SDL_RWops* pack[CLIPS]={nullptr,nullptr,nullptr};
  int width[CLIPS]={0,0,0},height[CLIPS]={0,0,0};
  bool compressed[CLIPS]={false,false,false};
  std::vector<PackIndex> indexes[CLIPS];
  unsigned loadedGeneration=0u,next=0u;
  for(;;){
   SDL_LockMutex(d.mutex);
   const bool quit=d.stop;
   const bool active=d.requested;
   const unsigned clip=d.clip,index=d.index,generation=d.generation;
   const size_t queued=d.ready.size();
   SDL_UnlockMutex(d.mutex);
   if(quit)break;
   if(!active||queued>=3u){SDL_Delay(2);continue;}
   if(generation!=loadedGeneration){
    loadedGeneration=generation;
    next=index;
   }
   if(next>=COUNT[clip]){SDL_Delay(2);continue;}
   if(preload().contains(clip,next)){++next;continue;}
   if(!checked[clip]){
    checked[clip]=true;
    pack[clip]=openPack(clip,width[clip],height[clip],compressed[clip],indexes[clip]);
    if(!pack[clip]){
     roots[clip]=findRoot(clip);
     missing[clip]=roots[clip].empty();
     if(missing[clip])
      std::fprintf(stderr,
       "XM87.284 FD_BINARY_AND_PNG_MISSING clip=%u expected=_art/video/%s.fdb\n",
       clip+1u,NAMES[clip]);
     else
      std::fprintf(stderr,
       "XM87.284 FD_BINARY_ABSENT clip=%u PNG fallback; run tools/pack_fd_video.ps1\n",
       clip+1u);
    }
   }
   if(missing[clip]){SDL_Delay(10);continue;}
   SDL_Surface* rgba=nullptr;
   if(pack[clip] && compressed[clip]){
    const PackIndex& item=indexes[clip][next];
    std::vector<unsigned char> png(item.size);
    const bool good=SDL_RWseek(pack[clip],(Sint64)item.offset,RW_SEEK_SET)==(Sint64)item.offset &&
       SDL_RWread(pack[clip],png.data(),1u,item.size)==item.size;
    if(good){
     SDL_RWops* memory=SDL_RWFromConstMem(png.data(),(int)item.size);
     if(memory){
      SDL_Surface* decoded=IMG_Load_RW(memory,1);
      if(decoded){rgba=SDL_ConvertSurfaceFormat(decoded,SDL_PIXELFORMAT_BGRA32,0);
       SDL_FreeSurface(decoded);
       if(rgba && (rgba->w!=width[clip]||rgba->h!=height[clip])){
        SDL_FreeSurface(rgba);rgba=nullptr;
       }
      }
     }
    }
    if(!rgba)std::fprintf(stderr,"XM87.284 FD_BINARY_PNG_FAIL clip=%u frame=%u\\n",clip+1u,next);
   }else if(pack[clip]){
    const int w=width[clip],h=height[clip];
    rgba=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_BGRA32);
    if(!rgba){
     std::fprintf(stderr,
      "XM87.284 FD_BINARY_ALLOC_FAIL clip=%u frame=%u %s\n",
       clip+1u,next,SDL_GetError());
     ++next;continue;
    }
    const Sint64 offset=32+(Sint64)next*(Sint64)w*(Sint64)h*4;
    bool good=SDL_RWseek(pack[clip],offset,RW_SEEK_SET)==offset;
    // Native 32-bit surfaces normally have tightly packed rows:
    // one sequential read, not 400 small IO calls per video frame.
    const size_t bytes=(size_t)w*(size_t)h*4u;
    if(good && rgba->pitch==w*4){
     good=SDL_RWread(pack[clip],rgba->pixels,1u,bytes)==bytes;
    }else if(good){
     for(int y=0;good && y<h;++y){
      unsigned char* row=(unsigned char*)rgba->pixels+(size_t)y*rgba->pitch;
      good=SDL_RWread(pack[clip],row,1u,(size_t)w*4u)==(size_t)w*4u;
     }
    }
    if(!good){
     std::fprintf(stderr,
       "XM87.284 FD_BINARY_READ_FAIL clip=%u frame=%u\n",
       clip+1u,next);
     SDL_FreeSurface(rgba);rgba=nullptr;
    }
   }else{
    const std::string filename=roots[clip]+path(clip,next);
    SDL_Surface* png=IMG_Load(filename.c_str());
    if(!png){
     std::fprintf(stderr,
      "XM87.284 FD_PNG_DECODE_FAIL clip=%u frame=%u %s\n",
       clip+1u,next,IMG_GetError());
     ++next;continue;
    }
    rgba=SDL_ConvertSurfaceFormat(png,SDL_PIXELFORMAT_BGRA32,0);
    SDL_FreeSurface(png);
   }
   if(!rgba){++next;continue;}
   SDL_LockMutex(d.mutex);
   if(d.stop||d.generation!=generation){
    SDL_UnlockMutex(d.mutex);
    SDL_FreeSurface(rgba);
    continue;
   }
   Frame frame;frame.clip=clip;frame.index=next;frame.generation=generation;
   frame.pixels=rgba;d.ready.push_back(frame);
   SDL_UnlockMutex(d.mutex);
   ++next;
  }
  for(unsigned i=0u;i<CLIPS;++i)
   if(pack[i])SDL_RWclose(pack[i]);
  return 0;
 }
 bool start(){
  if(thread)return true;
  mutex=SDL_CreateMutex();
  if(!mutex)return false;
  thread=SDL_CreateThread(run,"XMFD binary video",this);
  if(!thread){SDL_DestroyMutex(mutex);mutex=nullptr;return false;}
  std::fprintf(stderr,
   "XM87.284 FD_BINARY_STREAM_START worker=1 queue=3 reusableTextures=2 binary=preferred png=fallback\n");
  return true;
 }
 void request(unsigned c,unsigned i){
  if(!mutex||c>=CLIPS)return;
  i=std::min(i,COUNT[c]-1u);
  SDL_LockMutex(mutex);
  // A sequential frame advance consumes the queue; do not flush it.
  // A clip switch or a seek outside the next 3 decoded frames resets it.
  const bool seek=!requested||clip!=c||i<index||i>index+3u;
  if(seek){
   ++generation;
   for(Frame& f:ready)SDL_FreeSurface(f.pixels);
   ready.clear();
  }
  requested=true;clip=c;index=i;
  SDL_UnlockMutex(mutex);
 }
 bool take(unsigned c,unsigned i,Frame& out){
  if(preload().peek(c,i,out))return true;
  if(!mutex)return false;
  SDL_LockMutex(mutex);
  // Drop late decoded frames without ever blocking game presentation.
  while(!ready.empty() && ready.front().clip==c &&
        ready.front().index<i){
   SDL_FreeSurface(ready.front().pixels);
   ready.pop_front();
  }
  bool found=false;
  if(!ready.empty() && ready.front().clip==c &&
     ready.front().index==i){
   out=ready.front();ready.pop_front();found=true;
  }
  SDL_UnlockMutex(mutex);
  return found;
 }
 void shutdown(){
  if(!mutex)return;
  SDL_LockMutex(mutex);stop=true;SDL_UnlockMutex(mutex);
  if(thread){SDL_WaitThread(thread,nullptr);thread=nullptr;}
  for(Frame& f:ready)SDL_FreeSurface(f.pixels);
  ready.clear();
  SDL_DestroyMutex(mutex);mutex=nullptr;
  stop=false;requested=false;clip=index=0u;generation=1u;
 }
};
struct Sequence {
 enum Phase{PRE_MATCH,INITIAL,PLAY,GAP_PHASE};
 Decoder decoder;
 SDL_Texture* texture[2]={nullptr,nullptr};
 int active=0,texW=0,texH=0;
 Phase phase=PRE_MATCH;
 std::uint64_t matchStart=0,segmentStart=0,lastFrame=0;
 unsigned clip=0u,selected=0u;
 int shownClip=-1,shownIndex=-1;
 Uint8 alpha=0u;
 bool visible=false;
 void close(){
  decoder.shutdown();
  preload().shutdown();
  for(SDL_Texture*& tex:texture){if(tex)SDL_DestroyTexture(tex);tex=nullptr;}
  active=texW=texH=0;resetMatch();
 }
 void resetMatch(){
  phase=PRE_MATCH;matchStart=segmentStart=lastFrame=0u;
  clip=selected=0u;alpha=0u;visible=false;
  shownClip=shownIndex=-1;
 }
 bool upload(SDL_Renderer* renderer,SDL_Surface* frame){
  if(!renderer||!frame)return false;
  const int w=frame->w,h=frame->h;
  if(w<1||h<1)return false;
  if(!texture[0]||w!=texW||h!=texH){
   for(SDL_Texture*& tex:texture){if(tex)SDL_DestroyTexture(tex);tex=nullptr;}
   texW=w;texH=h;active=0;
   for(SDL_Texture*& tex:texture){
    tex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_BGRA32,
                           SDL_TEXTUREACCESS_STREAMING,w,h);
    if(!tex){
     std::fprintf(stderr,"XM87.284 FD_BINARY_STREAM_ALLOC_FAIL %s\n",SDL_GetError());
     for(SDL_Texture*& allocated:texture){
      if(allocated)SDL_DestroyTexture(allocated);
      allocated=nullptr;
     }
     texW=texH=0;
     return false;
    }
    SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex,SDL_ScaleModeLinear);
   }
   std::fprintf(stderr,
     "XM87.284 FD_BINARY_STREAM_TEXTURES_READY count=2 dimensions=%dx%d\n",w,h);
  }
  const int target=1-active;
  void* pixels=nullptr;int pitch=0;
  if(SDL_LockTexture(texture[target],nullptr,&pixels,&pitch)!=0){
   std::fprintf(stderr,"XM87.284 FD_BINARY_STREAM_LOCK_FAIL %s\n",SDL_GetError());
   return false;
  }
  const int rowBytes=w*4;
  for(int y=0;y<h;++y)
   std::memcpy((Uint8*)pixels+(size_t)y*pitch,
               (const Uint8*)frame->pixels+(size_t)y*frame->pitch,
               (size_t)rowBytes);
  SDL_UnlockTexture(texture[target]);
  active=target;
  return true;
 }
 // Upload the first ready frame during the existing idle/fade gap rather
 // than creating GPU textures on the first visible animation frame.
 void prewarm(SDL_Renderer* renderer,unsigned upcomingClip){
  decoder.request(upcomingClip,0u);
  if(shownClip==(int)upcomingClip && shownIndex==0)return;
  Frame prepared;
  if(decoder.take(upcomingClip,0u,prepared)){
   const bool ok=upload(renderer,prepared.pixels);
   if(!prepared.shared)SDL_FreeSurface(prepared.pixels);
   if(ok){shownClip=(int)upcomingClip;shownIndex=0;}
  }
 }
 void tick(SDL_Renderer* renderer,std::uint64_t frame,unsigned status){
  if(!decoder.thread)decoder.start();
  if(status<8u){if(phase!=PRE_MATCH)resetMatch();return;}
  if(phase==PRE_MATCH){
   if(status!=8u)return;
   phase=INITIAL;matchStart=segmentStart=lastFrame=frame;clip=0u;
   std::fprintf(stderr,"XM87.284 FD_BINARY_STREAM_MATCH_START frame=%llu wait=600f hold=4\n",
     (unsigned long long)frame);
  }
  if(frame<lastFrame){resetMatch();return;}
  lastFrame=frame;
  if(phase==INITIAL){
   prewarm(renderer,0u); // decode and upload during the initial ten seconds
   if(frame-matchStart<WAIT_INITIAL)return;
   segmentStart=matchStart+WAIT_INITIAL;phase=PLAY;
  }
  if(phase==GAP_PHASE){
   prewarm(renderer,clip); // prefetch/upload next clip during five-second gap
   if(frame-segmentStart<GAP)return;
   segmentStart+=GAP;phase=PLAY;
  }
  if(phase!=PLAY)return;
  const std::uint64_t elapsed=frame-segmentStart;
  const std::uint64_t duration=(std::uint64_t)COUNT[clip]*HOLD;
  if(elapsed>=duration){
   std::fprintf(stderr,"XM87.284 FD_BINARY_STREAM_END clip=%u frames=%u\n",
      clip+1u,COUNT[clip]);
   phase=GAP_PHASE;segmentStart=frame;clip=(clip+1u)%CLIPS;
   visible=false;alpha=0u;
   decoder.request(clip,0u);
   return;
  }
  selected=(unsigned)(elapsed/HOLD);
  decoder.request(clip,selected);
  if(shownClip!=(int)clip||shownIndex!=(int)selected){
   Frame decoded;
   if(decoder.take(clip,selected,decoded)){
    const bool ok=upload(renderer,decoded.pixels);
    if(!decoded.shared)SDL_FreeSurface(decoded.pixels);
    if(ok){shownClip=(int)clip;shownIndex=(int)selected;}
   }
  }
  // On a late decode, retain the previous frame without waiting; never
  // show a frame from a different clip or before the new clip begins.
  visible=texture[active] && shownClip==(int)clip;
  if(!visible){alpha=0u;return;}
  const std::uint64_t in=std::min(FADE,elapsed);
  const std::uint64_t out=std::min(FADE,duration-elapsed);
  alpha=(Uint8)(std::min(in,out)*255u/FADE);
 }
 void draw(SDL_Renderer* renderer,const SDL_Rect& view){
  if(!renderer||!visible||alpha==0u||!texture[active])return;
  SDL_SetTextureAlphaMod(texture[active],alpha);
  SDL_RenderCopy(renderer,texture[active],nullptr,&view);
 }
};
} // namespace XMFDVideo
#endif
