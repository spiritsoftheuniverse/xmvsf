#ifndef XMVSF_STAGE87_281_FD_GIF_ASYNC_H
#define XMVSF_STAGE87_281_FD_GIF_ASYNC_H
// Final Destination backdrop: GIF stream is decoded OFF the game/render thread.
// WIC metadata, GIF disposal/composition and 384x224 downsampling belong to
// one SDL worker; SDL_UpdateTexture and SDL_RenderCopy stay on the render thread.
// The latter present to the existing hardware-accelerated SDL renderer.
// No all-frames preload, no per-game-frame GIF I/O, no new linked library.
// Playback is quarter-speed (each original GIF frame lasts 4x longer).
// Initial 10s, one-second fades and five-second clip gaps remain normal speed.
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#if defined(_WIN32) && __has_include(<wincodec.h>)
#define XMFD_ASYNC_WIC 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <propidl.h>
#else
#define XMFD_ASYNC_WIC 0
#endif

namespace XMFDVideo {
static const std::uint64_t FPS=60u,WAIT_INITIAL=10u*FPS,GAP=5u*FPS,FADE=FPS,SLOW=4u;
static const int WIDTH=384,HEIGHT=224;
static std::string filename(int clip){
 const char* n[]={"fd01.gif","fd02.gif","fd03.gif"};
 if(clip<0||clip>2)return {};
 const std::string name=n[clip];
 std::vector<std::string> bases={"_art/video/","../_art/video/","../../_art/video/"};
 char* root=SDL_GetBasePath();
 if(root){
  std::string prefix(root);SDL_free(root);
  bases.push_back(prefix+"_art/video/");
  bases.push_back(prefix+"../_art/video/");
  bases.push_back(prefix+"../../_art/video/");
 }
 for(const auto& b:bases){
  const std::string path=b+name;
  SDL_RWops* f=SDL_RWFromFile(path.c_str(),"rb");
  if(f){SDL_RWclose(f);return path;}
 }
 return {};
}
#if XMFD_ASYNC_WIC
template<class T>static void rel(T*& v){if(v){v->Release();v=nullptr;}}
static const GUID WicFactoryID={0xcacaf262,0x9370,0x4615,{0xa1,0x3b,0x9f,0x55,0x39,0xda,0x4c,0x0a}};
static const GUID WicFactoryIID={0xec5ec8a9,0xc395,0x4314,{0x9c,0x77,0x54,0xd7,0xa9,0x35,0xff,0x70}};
static const GUID WicBGRA={0x6fddc324,0x4e03,0x4bfe,{0xb1,0x85,0x3d,0x77,0x76,0x8d,0xc9,0x0f}};
static std::wstring wide(const std::string& s){
 const int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);
 if(n<2)return {};
 std::wstring w((size_t)n,L'\0');
 if(!MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],n))return {};
 w.resize((size_t)n-1u);return w;
}
static int metaInt(IWICMetadataQueryReader* r,const wchar_t* name,int fallback){
 if(!r)return fallback;
 PROPVARIANT v;PropVariantInit(&v);
 const HRESULT h=r->GetMetadataByName(name,&v);
 if(FAILED(h))return fallback;
 int out=fallback;
 switch(v.vt){
  case VT_UI1:out=v.bVal;break;case VT_UI2:out=v.uiVal;break;
  case VT_UI4:out=(int)v.ulVal;break;case VT_I2:out=v.iVal;break;
  case VT_I4:out=v.lVal;break;default:break;
 }
 PropVariantClear(&v);return out;
}
struct Frame {
 std::uint64_t start=0;
 unsigned duration=1,x=0,y=0,w=0,h=0;
 int disposal=0;
};
struct NativeDecoder {
 IWICImagingFactory* factory=nullptr;
 IWICBitmapDecoder* gif=nullptr;
 bool com=false,comAttempted=false;
 UINT width=0,height=0,count=0,indexed=0;
 int selected=-1,drawn=-1,previousDisposal=0;
 Frame previousRect;
 std::uint64_t duration=0;
 bool indexedAll=false;
 std::vector<Frame> timeline;
 std::vector<Uint8> canvas,restore,patch,small;

 bool setupCOM(){
  if(factory)return true;
  if(comAttempted)return false;
  comAttempted=true;
  HRESULT h=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  if(h==RPC_E_CHANGED_MODE)h=S_OK;
  else if(SUCCEEDED(h))com=true;
  if(FAILED(h))return false;
  h=CoCreateInstance(WicFactoryID,nullptr,CLSCTX_INPROC_SERVER,
                     WicFactoryIID,(void**)&factory);
  if(FAILED(h)||!factory){
   std::fprintf(stderr,"XM87.281 FD_GIF_WIC_CREATE_FAIL hr=%08lX\n",(unsigned long)h);
   return false;
  }
  return true;
 }
 void releaseClip(){
  rel(gif);
  selected=-1;drawn=-1;previousDisposal=0;previousRect=Frame{};
  width=height=count=indexed=0;duration=0;indexedAll=false;
  timeline.clear();canvas.clear();restore.clear();patch.clear();small.clear();
 }
 void close(){
  releaseClip();rel(factory);
  if(com)CoUninitialize();
  com=false;comAttempted=false;
 }
 bool open(int index){
  if(index==selected && gif)return true;
  releaseClip();
  if(!setupCOM())return false;
  const std::string path=filename(index);
  if(path.empty()){
   std::fprintf(stderr,"XM87.281 FD_GIF_MISSING clip=%d\n",index+1);return false;
  }
  const std::wstring name=wide(path);
  HRESULT hr=factory->CreateDecoderFromFilename(name.c_str(),nullptr,GENERIC_READ,
                               WICDecodeMetadataCacheOnDemand,&gif);
  if(FAILED(hr)||!gif){
   std::fprintf(stderr,"XM87.281 FD_GIF_OPEN_FAIL clip=%d hr=%08lX\n",index+1,(unsigned long)hr);
   releaseClip();return false;
  }
  hr=gif->GetFrameCount(&count);
  if(FAILED(hr)||count==0||count>40000u){releaseClip();return false;}
  IWICMetadataQueryReader* query=nullptr;
  gif->GetMetadataQueryReader(&query);
  width=(UINT)std::max(0,metaInt(query,L"/logscrdesc/Width",0));
  height=(UINT)std::max(0,metaInt(query,L"/logscrdesc/Height",0));
  rel(query);
  IWICBitmapFrameDecode* first=nullptr;
  if(SUCCEEDED(gif->GetFrame(0,&first))&&first){
   UINT fw=0,fh=0;first->GetSize(&fw,&fh);
   if(!width)width=fw;if(!height)height=fh;
  }
  rel(first);
  if(!width||!height||width>4096u||height>4096u){
   std::fprintf(stderr,"XM87.281 FD_GIF_INVALID_SIZE clip=%d w=%u h=%u\n",
                index+1,width,height);
   releaseClip();return false;
  }
  timeline.resize(count);
  canvas.resize((size_t)width*height*4u);
  small.resize((size_t)WIDTH*HEIGHT*4u);
  selected=index;
  std::fprintf(stderr,
   "XM87.281 FD_GIF_WORKER_OPEN clip=%d w=%u h=%u frames=%u compressedOnDemand=1\n",
   index+1,width,height,count);
  return true;
 }
 bool indexBatch(unsigned maxCount=120){
  if(indexedAll)return true;
  if(!gif)return false;
  for(unsigned n=0;n<maxCount && indexed<count;++n,++indexed){
   IWICBitmapFrameDecode* f=nullptr;
   const HRESULT hr=gif->GetFrame(indexed,&f);
   if(FAILED(hr)||!f){rel(f);return false;}
   Frame t;
   f->GetSize(&t.w,&t.h);
   IWICMetadataQueryReader* q=nullptr;
   f->GetMetadataQueryReader(&q);
   const int centiseconds=std::max(2,metaInt(q,L"/grctlext/Delay",10));
   t.duration=(unsigned)std::max(1,(centiseconds*60+50)/100);
   t.x=(unsigned)std::max(0,metaInt(q,L"/imgdesc/Left",0));
   t.y=(unsigned)std::max(0,metaInt(q,L"/imgdesc/Top",0));
   t.disposal=metaInt(q,L"/grctlext/Disposal",0);
   t.start=duration;
   duration+=t.duration;
   timeline[indexed]=t;
   rel(q);rel(f);
  }
  if(indexed==count){
   indexedAll=true;
   std::fprintf(stderr,
    "XM87.281 FD_GIF_WORKER_INDEXED clip=%d frames=%u originalFrames=%llu quarterSpeedFrames=%llu\n",
    selected+1,count,(unsigned long long)duration,(unsigned long long)(duration*SLOW));
  }
  return true;
 }
 void clearRect(const Frame& r){
  if(r.x>=width||r.y>=height)return;
  const unsigned xmax=std::min(width,(unsigned)std::min((std::uint64_t)width,(std::uint64_t)r.x+r.w));
  const unsigned ymax=std::min(height,(unsigned)std::min((std::uint64_t)height,(std::uint64_t)r.y+r.h));
  for(unsigned y=r.y;y<ymax;++y)
   for(unsigned x=r.x;x<xmax;++x){
    Uint8* p=canvas.data()+((size_t)y*width+x)*4u;
    p[0]=p[1]=p[2]=0u;p[3]=255u;
   }
 }
 bool composite(unsigned i){
  if(!gif||!indexedAll||i>=count)return false;
  const Frame& info=timeline[i];
  if(drawn>=0){
   if(previousDisposal==2)clearRect(previousRect);
   else if(previousDisposal==3 && restore.size()==canvas.size())canvas=restore;
  }
  if(info.disposal==3)restore=canvas;
  IWICBitmapFrameDecode* f=nullptr;
  HRESULT hr=gif->GetFrame(i,&f);
  if(FAILED(hr)||!f){rel(f);return false;}
  IWICFormatConverter* cv=nullptr;
  hr=factory->CreateFormatConverter(&cv);
  if(SUCCEEDED(hr))hr=cv->Initialize(f,WicBGRA,WICBitmapDitherTypeNone,
                                nullptr,0.,WICBitmapPaletteTypeCustom);
  const size_t bytes=(size_t)info.w*info.h*4u;
  if(SUCCEEDED(hr)&&info.w&&info.h&&bytes<=(size_t)4096*4096*4u){
   patch.resize(bytes);
   hr=cv->CopyPixels(nullptr,info.w*4u,(UINT)bytes,patch.data());
  }else hr=E_FAIL;
  rel(cv);rel(f);
  if(FAILED(hr))return false;
  if(info.x<width && info.y<height){
   const unsigned hlim=std::min(info.h,height-info.y);
   const unsigned wlim=std::min(info.w,width-info.x);
   for(unsigned py=0;py<hlim;++py){
    const Uint8* row=patch.data()+(size_t)py*info.w*4u;
    Uint8* dst=canvas.data()+((size_t)(info.y+py)*width+info.x)*4u;
    for(unsigned px=0;px<wlim;++px){
     const Uint8* q=row+(size_t)px*4u;Uint8* p=dst+(size_t)px*4u;
     const unsigned a=q[3];
     if(a==255u){p[0]=q[2];p[1]=q[1];p[2]=q[0];p[3]=255u;}
     else if(a){
      p[0]=(Uint8)((q[2]*a+p[0]*(255u-a)+127u)/255u);
      p[1]=(Uint8)((q[1]*a+p[1]*(255u-a)+127u)/255u);
      p[2]=(Uint8)((q[0]*a+p[2]*(255u-a)+127u)/255u);
      p[3]=255u;
     }
    }
   }
  }
  previousRect=info;previousDisposal=info.disposal;drawn=(int)i;
  return true;
 }
 int targetIndex(std::uint64_t sourceElapsed)const{
  if(!indexedAll||timeline.empty())return -1;
  const auto it=std::upper_bound(timeline.begin(),timeline.end(),sourceElapsed,
   [](std::uint64_t f,const Frame& m){return f<m.start;});
  if(it==timeline.begin())return 0;
  const size_t k=(size_t)(it-timeline.begin()-1);
  return (int)std::min<size_t>(k,timeline.size()-1);
 }
 bool composeToward(std::uint64_t sourceElapsed){
  const int target=targetIndex(sourceElapsed);
  if(target<0)return false;
  if(drawn>target){
   std::fill(canvas.begin(),canvas.end(),0u);
   drawn=-1;previousDisposal=0;restore.clear();
  }
  // This budget is on a dedicated worker, NEVER the game thread.
  for(unsigned n=0;n<3 && drawn<target;++n)
   if(!composite((unsigned)(drawn+1)))return false;
  return true;
 }
 void downsample(){
  unsigned cropW=width,cropH=height,ox=0,oy=0;
  if((std::uint64_t)width*HEIGHT>(std::uint64_t)height*WIDTH){
   cropW=(unsigned)std::max<std::uint64_t>(1,(std::uint64_t)height*WIDTH/HEIGHT);
   ox=(width-cropW)/2u;
  }else{
   cropH=(unsigned)std::max<std::uint64_t>(1,(std::uint64_t)width*HEIGHT/WIDTH);
   oy=(height-cropH)/2u;
  }
  for(int y=0;y<HEIGHT;++y){
   const size_t sy=oy+(size_t)y*cropH/HEIGHT;
   for(int x=0;x<WIDTH;++x){
    const size_t sx=ox+(size_t)x*cropW/WIDTH;
    const Uint8* p=canvas.data()+(sy*width+sx)*4u;
    Uint8* q=small.data()+((size_t)y*WIDTH+x)*4u;
    q[0]=p[0];q[1]=p[1];q[2]=p[2];q[3]=255u;
   }
  }
 }
};
#else
struct NativeDecoder {
 int selected=-1,drawn=-1;bool indexedAll=false;
 std::uint64_t duration=0;
 std::vector<Uint8> small;
 bool open(int){return false;}
 void releaseClip(){selected=-1;drawn=-1;indexedAll=false;duration=0;small.clear();}
 void close(){releaseClip();}
 bool indexBatch(unsigned=120){return false;}
 bool composeToward(std::uint64_t){return false;}
 void downsample(){}
};
#endif

// Main-thread commands are tiny. No WIC work, source-file I/O, full-image
// compositing, or GIF frame scanning may run from the render thread.
struct Worker {
 SDL_mutex* mutex=nullptr;
 SDL_Thread* thread=nullptr;
 bool stopping=false;
 unsigned generation=0,loadedGeneration=0,revision=0;
 int requested=-1,loadedClip=-1;
 std::uint64_t wantedSourceTime=0,clipDuration=0;
 bool indexed=false,failed=false,hasFrame=false;
 std::vector<Uint8> latest;
 static int SDLCALL entry(void* arg){
  Worker& w=*(Worker*)arg;
  NativeDecoder decoder;unsigned currentGen=~0u;
  int currentClip=-1,lastPublished=-1;
  for(;;){
   int requested=-1;unsigned gen=0;std::uint64_t at=0;bool quit=false;
   SDL_LockMutex(w.mutex);
   requested=w.requested;gen=w.generation;at=w.wantedSourceTime;
   quit=w.stopping;SDL_UnlockMutex(w.mutex);
   if(quit)break;
   if(gen!=currentGen){
    decoder.releaseClip();
    currentGen=gen;currentClip=requested;lastPublished=-1;
    SDL_LockMutex(w.mutex);
    w.loadedGeneration=gen;w.loadedClip=currentClip;
    w.clipDuration=0;w.indexed=false;w.failed=false;w.hasFrame=false;
    SDL_UnlockMutex(w.mutex);
    if(requested>=0){
     if(!decoder.open(requested)){
      SDL_LockMutex(w.mutex);
      if(w.generation==gen)w.failed=true;
      SDL_UnlockMutex(w.mutex);
     }
    }
   }
   if(currentClip<0){SDL_Delay(8);continue;}
   bool noDecoder=false;
   SDL_LockMutex(w.mutex);noDecoder=w.failed;SDL_UnlockMutex(w.mutex);
   if(noDecoder){SDL_Delay(12);continue;}
   if(!decoder.indexedAll){
    if(!decoder.indexBatch(100)){
     SDL_LockMutex(w.mutex);if(w.generation==gen)w.failed=true;SDL_UnlockMutex(w.mutex);
    }else if(decoder.indexedAll){
     SDL_LockMutex(w.mutex);
     if(w.generation==gen){
      w.clipDuration=decoder.duration;w.indexed=true;
     }
     SDL_UnlockMutex(w.mutex);
    }
    SDL_Delay(1);continue;
   }
   const int target=decoder.targetIndex(at);
   if(target<0){SDL_Delay(5);continue;}
   if(decoder.drawn==target && lastPublished==target){
    SDL_Delay(3);continue;
   }
   if(!decoder.composeToward(at)){
    SDL_LockMutex(w.mutex);if(w.generation==gen)w.failed=true;SDL_UnlockMutex(w.mutex);
    continue;
   }
   if(decoder.drawn==lastPublished){SDL_Delay(1);continue;}
   decoder.downsample();
   SDL_LockMutex(w.mutex);
   if(w.generation==gen){
    // Bound shared storage to exactly one 384x224 RGBA result.
    w.latest.swap(decoder.small);
    w.hasFrame=true;++w.revision;
    lastPublished=decoder.drawn;
   }
   SDL_UnlockMutex(w.mutex);
   // Swap hands buffer capacity back to worker; never allocate an
   // unbounded frame queue when the renderer or match is paused.
   if(decoder.small.size()!=(size_t)WIDTH*HEIGHT*4u)
    decoder.small.resize((size_t)WIDTH*HEIGHT*4u);
  }
  decoder.close();return 0;
 }
 bool start(){
  if(thread)return true;
  mutex=SDL_CreateMutex();
  if(!mutex)return false;
  thread=SDL_CreateThread(entry,"XMFD GIF decode",this);
  if(!thread){SDL_DestroyMutex(mutex);mutex=nullptr;return false;}
  std::fprintf(stderr,"XM87.281 FD_GIF_WORKER_STARTED async=1 quarterSpeed=1 gpuTexture=384x224\n");
  return true;
 }
 void command(int clip,std::uint64_t time){
  if(!mutex)return;
  SDL_LockMutex(mutex);
  if(requested!=clip){requested=clip;++generation;hasFrame=false;indexed=false;failed=false;}
  wantedSourceTime=time;
  SDL_UnlockMutex(mutex);
 }
 void query(int clip,std::uint64_t& duration,bool& ready,bool& problem,
            bool& available,std::vector<Uint8>& scratch,unsigned& seen){
  duration=0;ready=false;problem=false;available=false;
  if(!mutex)return;
  SDL_LockMutex(mutex);
  if(requested==clip && loadedClip==clip && loadedGeneration==generation){
   duration=clipDuration;ready=indexed;problem=failed;available=hasFrame;
   if(available && seen!=revision){
    scratch=latest;seen=revision;
   }
  }
  SDL_UnlockMutex(mutex);
 }
 void shutdown(){
  if(!thread)return;
  SDL_LockMutex(mutex);stopping=true;SDL_UnlockMutex(mutex);
  SDL_WaitThread(thread,nullptr);thread=nullptr;
  SDL_DestroyMutex(mutex);mutex=nullptr;
  latest.clear();stopping=false;requested=-1;
 }
};

struct Sequence {
 enum Phase{PRE_MATCH,INITIAL,PLAY,GAP_PHASE};
 Worker worker;
 SDL_Texture* texture=nullptr;
 Phase phase=PRE_MATCH;
 std::uint64_t matchStart=0,segmentStart=0,lastFrame=0;
 int clip=0;
 unsigned seen=0;
 Uint8 alpha=0;
 bool available=false;
 std::vector<Uint8> uploadScratch;
 void close(){
  worker.shutdown();
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;
  resetMatch();
 }
 void resetMatch(){
  worker.command(-1,0);
  phase=PRE_MATCH;matchStart=segmentStart=lastFrame=0;
  clip=0;alpha=0;available=false;seen=0;uploadScratch.clear();
 }
 void request(int index,std::uint64_t sourceTime){
  if(worker.start())worker.command(index,sourceTime);
 }
 void tick(SDL_Renderer* renderer,std::uint64_t frame,unsigned status){
  alpha=0;
  if(status<8u){if(phase!=PRE_MATCH)resetMatch();return;}
  if(phase==PRE_MATCH){
   if(status!=8u)return;
   phase=INITIAL;matchStart=frame;lastFrame=frame;clip=0;
   request(0,0); // 10s preload happens ONLY in the background
   std::fprintf(stderr,"XM87.281 FD_GIF_MATCH_START frame=%llu initial=600f slow=4x\n",
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
  // Quarter speed applies only to the original GIF frame clock.
  // Fade and gap clocks remain the user's requested real match time.
  request(clip,elapsed/SLOW);
  std::uint64_t duration=0;bool ready=false,failed=false,has=false;
  worker.query(clip,duration,ready,failed,has,uploadScratch,seen);
  if(failed){
   std::fprintf(stderr,"XM87.281 FD_GIF_SKIP clip=%d reason=decoder\n",clip+1);
   phase=GAP_PHASE;segmentStart=frame;clip=(clip+1)%3;
   request(clip,0);available=false;return;
  }
  if(!ready || duration==0)return;
  const std::uint64_t played=duration*SLOW;
  if(elapsed>=played){
   std::fprintf(stderr,
    "XM87.281 FD_GIF_END clip=%d slowedFrames=%llu nextAfter=300f\n",
    clip+1,(unsigned long long)played);
   phase=GAP_PHASE;segmentStart=frame;clip=(clip+1)%3;
   request(clip,0);available=false;return;
  }
  if(!texture){
   texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,
                 SDL_TEXTUREACCESS_STREAMING,WIDTH,HEIGHT);
   if(texture){
    SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
   }
  }
  // Upload exactly one small completed frame if worker has published a
  // NEW frame. SDL GPU objects are never touched from the worker thread.
  if(texture && has && !uploadScratch.empty()){
   if(SDL_UpdateTexture(texture,nullptr,uploadScratch.data(),WIDTH*4)!=0){
    std::fprintf(stderr,"XM87.281 FD_GIF_GPU_UPLOAD_FAIL %s\n",SDL_GetError());
   }else{
    uploadScratch.clear();available=true;
   }
  }
  if(available){
   const std::uint64_t in=std::min(FADE,elapsed);
   const std::uint64_t out=std::min(FADE,played-elapsed);
   alpha=(Uint8)(std::min(in,out)*255u/FADE);
  }
 }
 void draw(SDL_Renderer* renderer,const SDL_Rect& view){
  if(phase!=PLAY || !texture || !available || alpha==0)return;
  SDL_SetTextureAlphaMod(texture,alpha);
  SDL_RenderCopy(renderer,texture,nullptr,&view);
 }
};
} // namespace XMFDVideo
#endif
