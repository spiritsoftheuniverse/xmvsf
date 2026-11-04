#ifndef XMVSF_STAGE87_71_SELECT_STREAM_H
#define XMVSF_STAGE87_71_SELECT_STREAM_H
// Stage87.71 Training-only native framebuffer movement recorder.
// NEVER performs GPU readback, encoding, file IO, or waits for disk on game thread.
// No event/frame limit. Fixed maximum queued memory; overflow is diagnosed.
// Output: lossless native framebuffer batches (XM8771B1), index TSV, health TSV.
#include <SDL.h>
#include <deque>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <utility>
#include <algorithm>
namespace XM8771 {
struct Event { uint32_t id,frame;uint8_t player,dir; };
struct Frame {
 uint32_t id,frame,inputFrame,player,dir,delta,w,h,bpp,pitch;
 std::vector<uint8_t> bytes;
};
struct Capture {
 bool prev[2][4]; uint32_t nextId,dropPending,dropQueued,failedWrites,written;
 std::deque<Event> pending; std::deque<Frame> queue;
 SDL_mutex* mutex;SDL_cond* cond;SDL_Thread* thread;
 bool initialized,stop,started;std::string dir;
 Capture():nextId(0),dropPending(0),dropQueued(0),failedWrites(0),written(0),
     mutex(NULL),cond(NULL),thread(NULL),initialized(false),stop(false),started(false) {
  std::memset(prev,0,sizeof(prev));
 }
 ~Capture(){Shutdown();}
 static void Write32(std::FILE*f,uint32_t x){uint8_t b[4];for(int i=0;i<4;++i)b[i]=(uint8_t)(x>>(8*i));std::fwrite(b,1,4,f);}
 static int Entry(void* ctx){return ((Capture*)ctx)->Writer();}
 int Writer(){
  std::FILE* idx=NULL;std::FILE* batch=NULL;uint32_t batchId=0,batchItems=0;
  char path[1600];std::snprintf(path,sizeof(path),"%s/xm8771_screen_events.tsv",dir.c_str());
  idx=std::fopen(path,"wb");
  if(idx)std::fputs("capture\tinput_frame\tpaint_frame\tplayer\tdirection\tdelta\tbatch\twidth\theight\tbpp\tpitch\tbytes\n",idx);
  else {++failedWrites;std::fprintf(stderr,"XM87.71 CAPTURE_INDEX_OPEN_FAILED %s\n",path);}
  for(;;){
   Frame f;bool have=false;
   SDL_LockMutex(mutex);
   while(queue.empty()&&!stop)SDL_CondWaitTimeout(cond,mutex,500);
   if(!queue.empty()){f=std::move(queue.front());queue.pop_front();have=true;}
   const bool done=stop&&!have;
   SDL_UnlockMutex(mutex);
   if(done)break;
   if(!have){if(batch)std::fflush(batch);if(idx)std::fflush(idx);continue;}
   if(!batch || batchItems>=32){
    if(batch)std::fclose(batch);
    std::snprintf(path,sizeof(path),"%s/xm8771_frames_%06u.bin",dir.c_str(),batchId++);
    batch=std::fopen(path,"wb");batchItems=0;
    if(batch)std::fwrite("XM8771B1",1,8,batch);
    else {++failedWrites;std::fprintf(stderr,"XM87.71 BATCH_OPEN_FAILED %s\n",path);}
   }
   const uint32_t bytes=(uint32_t)f.bytes.size();
   bool wrote=false;
   if(batch && bytes){
    const uint32_t fields[]={f.id,f.frame,f.inputFrame,f.player,f.dir,f.delta,f.w,f.h,f.bpp,f.pitch,bytes};
    for(unsigned j=0;j<sizeof(fields)/sizeof(fields[0]);++j)Write32(batch,fields[j]);
    wrote=std::fwrite(f.bytes.data(),1,bytes,batch)==bytes;
   }
   if(wrote){
    if(idx)std::fprintf(idx,"%u\t%u\t%u\t%u\t%u\t%u\t%06u\t%u\t%u\t%u\t%u\t%u\n",
       f.id,f.inputFrame,f.frame,f.player,f.dir,f.delta,batchId-1,f.w,f.h,f.bpp,f.pitch,bytes);
    ++written;
   }else ++failedWrites;
   if(++batchItems==32){if(batch)std::fflush(batch);if(idx)std::fflush(idx);}
  }
  if(batch)std::fclose(batch);
  if(idx)std::fclose(idx);
  std::snprintf(path,sizeof(path),"%s/xm8771_capture_health.tsv",dir.c_str());
  if(std::FILE*health=std::fopen(path,"wb")){
   std::fprintf(health,"captured\tpending_overflow\tqueue_overflow\twrite_failures\n%u\t%u\t%u\t%u\n",written,dropPending,dropQueued,failedWrites);
   std::fclose(health);
  }
  std::fprintf(stderr,"XM87.71 CAPTURE_FINISHED frames=%u pending_overflow=%u queue_overflow=%u io_errors=%u\n",written,dropPending,dropQueued,failedWrites);
  return 0;
 }
 void Start(){
  if(initialized)return;initialized=true;
  const char*root=std::getenv("XM8766_SELECTION_TRACE");
  if(!root||!*root)return;
  dir=root;
  mutex=SDL_CreateMutex();cond=SDL_CreateCond();
  if(!mutex||!cond){std::fprintf(stderr,"XM87.71 CAPTURE_SYNC_FAILED %s\n",SDL_GetError());return;}
  thread=SDL_CreateThread(&Capture::Entry,"xm8771_capture_writer",this);
  if(!thread){std::fprintf(stderr,"XM87.71 CAPTURE_THREAD_FAILED %s\n",SDL_GetError());return;}
  started=true;
  std::fprintf(stderr,"XM87.71 BATCH_CAPTURE_READY cpu_framebuffer=1 gpu_readbacks=0 batch_size=32 queue=128 hard_frame_limit=0\n");
 }
 void Input(unsigned slot,unsigned code,bool down,unsigned frame,bool selecting){
  if(slot>1||code>3)return;
  if(!selecting){prev[slot][code]=false;return;}
  const bool edge=down&&!prev[slot][code];prev[slot][code]=down;
  if(!edge)return;
  if(pending.size()>=128){++dropPending;return;}
  const Event e={++nextId,frame,(uint8_t)slot,(uint8_t)code};pending.push_back(e);
 }
 void RecordCPU(const void*source,unsigned w,unsigned h,unsigned pitch,unsigned bpp,unsigned frame,bool training,bool selecting){
  if(!training||!selecting||pending.empty()||!source)return;
  Start();if(!started)return;
  // Always let the existing native-frame compositor present uninterrupted.
  // One snapshot at most per rendered frame, independent of number of inputs.
  while(!pending.empty() && frame>pending.front().frame+13)pending.pop_front();
  if(pending.empty())return;
  Event event={0,0,0,0};unsigned delta=0;bool take=false;
  for(std::deque<Event>::const_iterator it=pending.begin();it!=pending.end();++it){
   if(frame<it->frame)continue;
   const unsigned d=frame-it->frame;
   if(d==0||d==1||d==3||d==6||d==12){event=*it;delta=d;take=true;break;}
  }
  if(!take||w<120||h<96||w>1024||h>768||bpp<2||bpp>4||pitch<w*bpp||pitch>8192)return;
  // The only per-capture work in the emulator thread is a bounded memcpy.
  // Max 128 pending images; on overload capture continues with a health counter.
  SDL_LockMutex(mutex);
  if(queue.size()>=128){++dropQueued;SDL_UnlockMutex(mutex);return;}
  SDL_UnlockMutex(mutex);
  Frame snap;snap.id=event.id;snap.frame=frame;snap.inputFrame=event.frame;
  snap.player=event.player+1;snap.dir=event.dir;snap.delta=delta;
  snap.w=w;snap.h=h;snap.pitch=pitch;snap.bpp=bpp;
  snap.bytes.resize((size_t)h*pitch);
  std::memcpy(snap.bytes.data(),source,(size_t)h*pitch);
  SDL_LockMutex(mutex);
  if(queue.size()<128){queue.push_back(std::move(snap));SDL_CondSignal(cond);}
  else ++dropQueued;
  SDL_UnlockMutex(mutex);
 }
 void Shutdown(){
  if(thread){SDL_LockMutex(mutex);stop=true;SDL_CondSignal(cond);SDL_UnlockMutex(mutex);
   SDL_WaitThread(thread,NULL);thread=NULL;}
  if(cond){SDL_DestroyCond(cond);cond=NULL;}
  if(mutex){SDL_DestroyMutex(mutex);mutex=NULL;}
  started=false;
 }
};
static inline Capture& State(){static Capture s;return s;}
}
#endif
