#ifndef XMVSF_STAGE87_282_FD_FRAMES_H
#define XMVSF_STAGE87_282_FD_FRAMES_H
// Final Destination: native 700x400 pre-coalesced PNG frame sequences.
// One SDL worker decodes source PNGs ONCE into a short bounded queue.
// The main SDL render thread uploads at most ONE frame per emulated
// paint to immutable GPU textures. Playback never decodes, composites,
// rescales or uploads a frame that has already been cached.
// Source folders checked on final-destination branch:
// fd01/frame_00000..00192.png (193)
// fd02/frame_00000..00199.png (200)
// fd03/frame_00000..00395.png (396)
// No GIF or WIC runtime paths, no new library or build step.
// Timing: one PNG for four native match frames (quarter-speed),
// ten-second initial delay; 1s fade at each end; 5s clip gaps.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace XMFDVideo {
static const std::uint64_t FPS=60u,WAIT_INITIAL=10u*FPS,GAP=5u*FPS,FADE=FPS,FRAME_HOLD=4u;
static const unsigned CLIPS=3u;
static const unsigned FRAME_COUNT[CLIPS]={193u,200u,396u};
static const char* const CLIP_NAME[CLIPS]={"fd01","fd02","fd03"};

static std::string findDirectory(unsigned clip){
 if(clip>=CLIPS)return {};
 std::vector<std::string> roots={"_art/video/","../_art/video/","../../_art/video/"};
 char* base=SDL_GetBasePath();
 if(base){
  const std::string prefix(base);SDL_free(base);
  roots.push_back(prefix+"_art/video/");
  roots.push_back(prefix+"../_art/video/");
  roots.push_back(prefix+"../../_art/video/");
 }
 for(const auto& root:roots){
  const std::string dir=root+CLIP_NAME[clip]+"/";
  SDL_RWops* first=SDL_RWFromFile((dir+"frame_00000.png").c_str(),"rb");
  if(first){SDL_RWclose(first);return dir;}
 }
 return {};
}
static std::string framePath(const std::string& dir,unsigned index){
 char filename[40];
 std::snprintf(filename,sizeof(filename),"frame_%05u.png",index);
 return dir+filename;
}
struct Item {
 unsigned clip=0u,index=0u;
 SDL_Surface* surface=nullptr;
};
struct FrameCache {
 SDL_mutex* mutex=nullptr;
 SDL_Thread* thread=nullptr;
 bool quit=false,complete=false;
 std::deque<Item> pending;
 std::vector<SDL_Texture*> gpu[CLIPS];
 bool missing[CLIPS]={false,false,false};
 unsigned uploaded[CLIPS]={0u,0u,0u};
 FrameCache(){
  for(unsigned i=0;i<CLIPS;++i)gpu[i].assign(FRAME_COUNT[i],nullptr);
 }
 // The only thread that does IMG_Load. Never create/destroy GPU textures here.
 static int SDLCALL run(void* opaque){
  FrameCache& self=*(FrameCache*)opaque;
  for(unsigned clip=0u;clip<CLIPS;++clip){
   SDL_LockMutex(self.mutex);
   bool stop=self.quit;
   SDL_UnlockMutex(self.mutex);
   if(stop)break;
   const std::string dir=findDirectory(clip);
   if(dir.empty()){
    SDL_LockMutex(self.mutex);self.missing[clip]=true;SDL_UnlockMutex(self.mutex);
    std::fprintf(stderr,
     "XM87.282 FD_FRAMES_MISSING clip=%u expected=_art/video/%s/frame_00000.png\n",
     clip+1u,CLIP_NAME[clip]);
    continue;
   }
   std::fprintf(stderr,"XM87.282 FD_FRAMES_DECODE_START clip=%u dir=%s frames=%u\n",
                clip+1u,dir.c_str(),FRAME_COUNT[clip]);
   bool failed=false;
   for(unsigned i=0u;i<FRAME_COUNT[clip];++i){
    // The queue is bounded to four decoded images (~4.3 MiB at 700x400).
    // The worker is never permitted to accumulate all 789 CPU surfaces.
    for(;;){
     SDL_LockMutex(self.mutex);
     const bool stopping=self.quit;
     const bool room=self.pending.size()<4u;
     SDL_UnlockMutex(self.mutex);
     if(stopping){failed=true;break;}
     if(room)break;
     SDL_Delay(3);
    }
    if(failed)break;
    const std::string file=framePath(dir,i);
    SDL_Surface* surface=IMG_Load(file.c_str());
    if(!surface){
     std::fprintf(stderr,
      "XM87.282 FD_FRAME_LOAD_FAIL clip=%u frame=%u path=%s error=%s\n",
      clip+1u,i,file.c_str(),IMG_GetError());
     SDL_LockMutex(self.mutex);self.missing[clip]=true;SDL_UnlockMutex(self.mutex);
     failed=true;break;
    }
    SDL_LockMutex(self.mutex);
    if(self.quit){SDL_UnlockMutex(self.mutex);SDL_FreeSurface(surface);failed=true;break;}
    Item item;item.clip=clip;item.index=i;item.surface=surface;
    self.pending.push_back(item);
    SDL_UnlockMutex(self.mutex);
   }
   if(!failed)std::fprintf(stderr,
     "XM87.282 FD_FRAMES_DECODE_ENQUEUED clip=%u frames=%u\n",
     clip+1u,FRAME_COUNT[clip]);
  }
  SDL_LockMutex(self.mutex);self.complete=true;SDL_UnlockMutex(self.mutex);
  return 0;
 }
 bool start(){
  if(thread)return true;
  if(!mutex)mutex=SDL_CreateMutex();
  if(!mutex){
   std::fprintf(stderr,"XM87.282 FD_FRAME_MUTEX_FAIL %s\n",SDL_GetError());return false;
  }
  quit=false;complete=false;
  thread=SDL_CreateThread(run,"XMFD PNG preload",this);
  if(!thread){
   std::fprintf(stderr,"XM87.282 FD_FRAME_THREAD_FAIL %s\n",SDL_GetError());
   SDL_DestroyMutex(mutex);mutex=nullptr;return false;
  }
  std::fprintf(stderr,
   "XM87.282 FD_FRAMES_START png=1 gif=0 fullResolution=1 quarterSpeed=1 uploadMaxPerPaint=1 frames=193+200+396\n");
  return true;
 }
 void pump(SDL_Renderer* renderer){
  if(!mutex || !renderer)return;
  Item item;bool any=false;
  SDL_LockMutex(mutex);
  if(!pending.empty()){
   item=pending.front();pending.pop_front();any=true;
  }
  SDL_UnlockMutex(mutex);
  if(!any)return;
  // GPU interaction and SDL_CreateTextureFromSurface are render-thread ONLY.
  SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,item.surface);
  SDL_FreeSurface(item.surface);
  if(!texture){
   std::fprintf(stderr,"XM87.282 FD_FRAME_GPU_FAIL clip=%u frame=%u %s\n",
     item.clip+1u,item.index,SDL_GetError());
   return;
  }
  SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
  if(item.clip<CLIPS && item.index<gpu[item.clip].size()){
   gpu[item.clip][item.index]=texture;
   ++uploaded[item.clip];
   if(uploaded[item.clip]==FRAME_COUNT[item.clip])
    std::fprintf(stderr,"XM87.282 FD_FRAMES_GPU_READY clip=%u frames=%u\n",
       item.clip+1u,uploaded[item.clip]);
  }else SDL_DestroyTexture(texture);
 }
 SDL_Texture* get(unsigned clip,unsigned index)const{
  if(clip>=CLIPS||index>=gpu[clip].size())return nullptr;
  return gpu[clip][index];
 }
 void shutdown(){
  if(thread){
   SDL_LockMutex(mutex);quit=true;SDL_UnlockMutex(mutex);
   SDL_WaitThread(thread,nullptr);thread=nullptr;
  }
  if(mutex){
   SDL_LockMutex(mutex);
   for(Item& item:pending)if(item.surface)SDL_FreeSurface(item.surface);
   pending.clear();
   SDL_UnlockMutex(mutex);
   SDL_DestroyMutex(mutex);mutex=nullptr;
  }
  for(unsigned c=0u;c<CLIPS;++c){
   for(SDL_Texture*& tex:gpu[c]){
    if(tex)SDL_DestroyTexture(tex);
    tex=nullptr;
   }
   uploaded[c]=0u;missing[c]=false;
  }
  quit=false;complete=false;
 }
};
struct Sequence {
 enum Phase{PRE_MATCH,INITIAL,PLAY,GAP_PHASE};
 FrameCache frames;
 Phase phase=PRE_MATCH;
 std::uint64_t matchStart=0,segmentStart=0,lastFrame=0;
 unsigned clip=0u,selected=0u;
 Uint8 alpha=0u;
 SDL_Texture* displayed=nullptr;

 void close(){frames.shutdown();resetMatch();}
 void resetMatch(){
  // Keep the GPU cache across rematches and temporary stage exits.
  phase=PRE_MATCH;matchStart=segmentStart=lastFrame=0;
  clip=0u;selected=0u;alpha=0u;displayed=nullptr;
 }
 bool showing()const{return phase==PLAY && displayed && alpha>0u;}
 void tick(SDL_Renderer* renderer,std::uint64_t frame,unsigned status){
  alpha=0u;displayed=nullptr;
  // Decode/preload asynchronously before the fight as well as during it.
  // GPU upload budget: at most one PNG per paint, never one per video tick.
  if(frames.start())frames.pump(renderer);
  if(status<8u){if(phase!=PRE_MATCH)resetMatch();return;}
  if(phase==PRE_MATCH){
   if(status!=8u)return;
   phase=INITIAL;clip=0u;matchStart=segmentStart=lastFrame=frame;
   std::fprintf(stderr,
    "XM87.282 FD_FRAMES_MATCH_START frame=%llu wait=600f frameHold=4\n",
    (unsigned long long)frame);
  }
  if(frame<lastFrame){resetMatch();return;}
  lastFrame=frame;
  if(phase==INITIAL){
   if(frame-matchStart<WAIT_INITIAL)return;
   segmentStart=matchStart+WAIT_INITIAL;phase=PLAY;
  }
  if(phase==GAP_PHASE){
   if(frame-segmentStart<GAP)return;
   segmentStart+=GAP;phase=PLAY;
  }
  if(phase!=PLAY)return;
  const std::uint64_t elapsed=frame-segmentStart;
  const std::uint64_t duration=(std::uint64_t)FRAME_COUNT[clip]*FRAME_HOLD;
  if(elapsed>=duration){
   std::fprintf(stderr,"XM87.282 FD_FRAMES_END clip=%u frames=%u duration=%llu gap=300f\n",
    clip+1u,FRAME_COUNT[clip],(unsigned long long)duration);
   phase=GAP_PHASE;segmentStart=frame;clip=(clip+1u)%CLIPS;return;
  }
  selected=(unsigned)(elapsed/FRAME_HOLD);
  displayed=frames.get(clip,selected);
  if(!displayed)return; // starfield remains visible if PNG is not cached yet
  const std::uint64_t fin=std::min(FADE,elapsed);
  const std::uint64_t fout=std::min(FADE,duration-elapsed);
  alpha=(Uint8)(std::min(fin,fout)*255u/FADE);
 }
 void draw(SDL_Renderer* renderer,const SDL_Rect& view){
  if(!renderer || !showing())return;
  SDL_SetTextureAlphaMod(displayed,alpha);
  SDL_RenderCopy(renderer,displayed,nullptr,&view);
 }
};
} // namespace XMFDVideo
#endif
