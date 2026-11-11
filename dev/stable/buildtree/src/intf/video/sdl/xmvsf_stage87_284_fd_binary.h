#ifndef XMVSF_STAGE87_284_FD_BINARY_H
#define XMVSF_STAGE87_284_FD_BINARY_H
// Final Destination: ffplay-style bounded frame queue and SDL2 streaming textures.
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
};
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
   if(roots[clip].empty()&&!missing[clip]){
    roots[clip]=findRoot(clip);
    if(roots[clip].empty()){
     missing[clip]=true;
     std::fprintf(stderr,
      "XM87.283 FD_STREAM_MISSING clip=%u expected=_art/video/%s/frame_00000.png\n",
      clip+1u,NAMES[clip]);
    }
   }
   if(missing[clip]){SDL_Delay(10);continue;}
   const std::string filename=roots[clip]+path(clip,next);
   SDL_Surface* decoded=IMG_Load(filename.c_str());
   if(!decoded){
    std::fprintf(stderr,"XM87.283 FD_STREAM_DECODE_FAIL clip=%u frame=%u %s\n",
      clip+1u,next,IMG_GetError());
    // A corrupt/missing individual PNG must never create an infinite
    // decode/retry loop or fill the launch log every 20 ms.
    ++next;continue;
   }
   // SDL pixel conversion is also on the worker, never on the game thread.
   SDL_Surface* rgba=SDL_ConvertSurfaceFormat(decoded,SDL_PIXELFORMAT_RGBA32,0);
   SDL_FreeSurface(decoded);
   if(!rgba){
    std::fprintf(stderr,"XM87.283 FD_STREAM_CONVERT_FAIL clip=%u frame=%u %s\n",
      clip+1u,next,SDL_GetError());
    ++next;continue;
   }
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
  return 0;
 }
 bool start(){
  if(thread)return true;
  mutex=SDL_CreateMutex();
  if(!mutex)return false;
  thread=SDL_CreateThread(run,"XMFD PNG decoder",this);
  if(!thread){SDL_DestroyMutex(mutex);mutex=nullptr;return false;}
  std::fprintf(stderr,
   "XM87.283 FD_STREAM_START worker=1 queue=3 reusableTextures=2 png=1\n");
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
    tex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                           SDL_TEXTUREACCESS_STREAMING,w,h);
    if(!tex){
     std::fprintf(stderr,"XM87.283 FD_STREAM_ALLOC_FAIL %s\n",SDL_GetError());
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
     "XM87.283 FD_STREAM_TEXTURES_READY count=2 dimensions=%dx%d\n",w,h);
  }
  const int target=1-active;
  void* pixels=nullptr;int pitch=0;
  if(SDL_LockTexture(texture[target],nullptr,&pixels,&pitch)!=0){
   std::fprintf(stderr,"XM87.283 FD_STREAM_LOCK_FAIL %s\n",SDL_GetError());
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
   SDL_FreeSurface(prepared.pixels);
   if(ok){shownClip=(int)upcomingClip;shownIndex=0;}
  }
 }
 void tick(SDL_Renderer* renderer,std::uint64_t frame,unsigned status){
  if(!decoder.thread)decoder.start();
  if(status<8u){if(phase!=PRE_MATCH)resetMatch();return;}
  if(phase==PRE_MATCH){
   if(status!=8u)return;
   phase=INITIAL;matchStart=segmentStart=lastFrame=frame;clip=0u;
   std::fprintf(stderr,"XM87.283 FD_STREAM_MATCH_START frame=%llu wait=600f hold=4\n",
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
   std::fprintf(stderr,"XM87.283 FD_STREAM_END clip=%u frames=%u\n",
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
    SDL_FreeSurface(decoded.pixels);
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
