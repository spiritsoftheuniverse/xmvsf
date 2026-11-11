#ifndef XMVSF_STAGE87_287_FD_FX_H
#define XMVSF_STAGE87_287_FD_FX_H
// 87.287: Final Destination FX overlay. Four XMFDBIN2 packs, independent
// dynamic frame counts/dimensions, low-priority decode into RAM on stage highlight.
// Black and near-black are keyed to transparency; additive compositing cannot
// darken the original FD video. Effects are drawn above sky and below stars.
// No video IO, PNG decompression, or chroma-key processing on the render thread.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
namespace XMFDVideoFX {
static const unsigned CLIPS=4u, HOLD=4u, FADE=30u, GAP=24u;
static const unsigned START_AFTER_MAIN=300u; // 5 seconds into first fd01 playback
static const char* const NAMES[CLIPS]={"fdfx01","fdfx02","fdfx03","fdfx04"};
static unsigned le32(const unsigned char* p){
 return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);
}
static std::uint64_t le64(const unsigned char* p){
 return (std::uint64_t)le32(p)|((std::uint64_t)le32(p+4)<<32);
}
static std::string findPack(unsigned clip){
 std::vector<std::string> roots={"_art/video/","../_art/video/","../../_art/video/"};
 char* base=SDL_GetBasePath();
 if(base){
  const std::string prefix(base);SDL_free(base);
  roots.push_back(prefix+"_art/video/");
  roots.push_back(prefix+"../_art/video/");
  roots.push_back(prefix+"../../_art/video/");
 }
 for(const std::string& root:roots){
  const std::string file=root+NAMES[clip]+".fdb";
  SDL_RWops* rw=SDL_RWFromFile(file.c_str(),"rb");
  if(rw){SDL_RWclose(rw);return file;}
 }
 return {};
}
struct Entry { std::uint64_t offset=0; unsigned bytes=0; };
struct Cache {
 SDL_mutex* mutex=nullptr;
 SDL_Thread* thread=nullptr;
 bool stop=false,enabled=false;
 // 0 = not probed; 1 = metadata available; 2 = absent/invalid.
 unsigned state[CLIPS]={0,0,0,0};
 unsigned count[CLIPS]={0,0,0,0};
 std::vector<SDL_Surface*> frames[CLIPS];
 static void keyBlack(SDL_Surface* surface){
  // SDL_PIXELFORMAT_BGRA32 byte order: B,G,R,A on Windows.
  // Remove black/gray backdrop and dark fringe; preserve saturated FX RGB.
  // ADD blend plus zero-alpha black guarantees no darkening at the seam.
  for(int y=0;y<surface->h;++y){
   unsigned char* line=(unsigned char*)surface->pixels+(size_t)y*surface->pitch;
   for(int x=0;x<surface->w;++x){
    unsigned char* px=line+4u*(size_t)x;
    const unsigned peak=std::max((unsigned)px[0],
                        std::max((unsigned)px[1],(unsigned)px[2]));
    if(peak<=12u){px[0]=px[1]=px[2]=px[3]=0u;continue;}
    // Gradual alpha ramp across faint antialiased fringe pixels.
    const unsigned coverage=std::min(255u,(peak-12u)*255u/28u);
    px[3]=(unsigned char)((unsigned)px[3]*coverage/255u);
   }
  }
 }
 static int SDLCALL run(void* ptr){
  Cache& d=*(Cache*)ptr;
  SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
  for(unsigned clip=0;clip<CLIPS;++clip){
   for(;;){
    SDL_LockMutex(d.mutex);
    const bool quit=d.stop,active=d.enabled;
    SDL_UnlockMutex(d.mutex);
    if(quit)return 0;
    if(active)break;
    SDL_Delay(8);
   }
   const std::string file=findPack(clip);
   SDL_RWops* rw=file.empty()?nullptr:SDL_RWFromFile(file.c_str(),"rb");
   bool valid=rw!=nullptr;
   unsigned w=0,h=0,n=0,format=0,rowBytes=0;
   Sint64 size=-1;
   std::vector<Entry> entries;
   if(valid){
    unsigned char hdr[32]={};
    valid=SDL_RWread(rw,hdr,1u,32u)==32u;
    if(valid){
     w=le32(hdr+8);h=le32(hdr+12);n=le32(hdr+16);
     format=le32(hdr+20);rowBytes=le32(hdr+24);
     size=SDL_RWseek(rw,0,RW_SEEK_END);
     const bool v1=std::memcmp(hdr,"XMFDBIN1",8u)==0&&format==1u;
     const bool v2=std::memcmp(hdr,"XMFDBIN2",8u)==0&&format==2u;
     // Avoid unbounded allocations on malformed or accidental huge packs.
     valid=(v1||v2)&&w>0u&&h>0u&&w<=4096u&&h<=4096u&&
           (std::uint64_t)w*h*4u<=16777216u&&n>0u&&n<=4096u&&
           rowBytes==w*4u&&le32(hdr+28)==0u;
     if(valid && v1)
      valid=size==(Sint64)(32u+(std::uint64_t)n*w*h*4u);
     if(valid && v2){
      const std::uint64_t payloadStart=32u+(std::uint64_t)n*16u;
      valid=size>=(Sint64)payloadStart && SDL_RWseek(rw,32,RW_SEEK_SET)==32;
      if(valid){
       entries.resize(n);
       std::uint64_t previous=payloadStart;
       for(unsigned i=0;i<n;++i){
        unsigned char item[16]={};
        if(SDL_RWread(rw,item,1u,16u)!=16u){valid=false;break;}
        const std::uint64_t off=le64(item);
        const unsigned bytes=le32(item+8);
        if(off<previous||bytes<8u||bytes>16777216u||
           off>(std::uint64_t)size||
           bytes>(std::uint64_t)size-off||le32(item+12)!=0u){
         valid=false;break;
        }
        entries[i].offset=off;entries[i].bytes=bytes;
        previous=off+bytes;
       }
      }
     }
    }
   }
   if(!valid){
    std::fprintf(stderr,"XM87.287 FD_FX_PACK_UNAVAILABLE clip=%s file=%s\n",
                 NAMES[clip],file.empty()?"missing":file.c_str());
    if(rw)SDL_RWclose(rw);
    SDL_LockMutex(d.mutex);d.state[clip]=2u;SDL_UnlockMutex(d.mutex);
    continue;
   }
   SDL_LockMutex(d.mutex);
   d.count[clip]=n;
   d.frames[clip].resize(n,nullptr);
   d.state[clip]=1u;
   SDL_UnlockMutex(d.mutex);
   std::fprintf(stderr,"XM87.287 FD_FX_READY name=%s frames=%u native=%ux%u format=%u\n",
                NAMES[clip],n,w,h,format);
   for(unsigned i=0;i<n;++i){
    for(;;){
     SDL_LockMutex(d.mutex);
     const bool quit=d.stop,active=d.enabled;
     SDL_UnlockMutex(d.mutex);
     if(quit){SDL_RWclose(rw);return 0;}
     if(active)break;
     SDL_Delay(8);
    }
    SDL_Surface* surface=nullptr;
    if(format==2u){
     const Entry& e=entries[i];
     std::vector<unsigned char> png(e.bytes);
     const bool ok=SDL_RWseek(rw,(Sint64)e.offset,RW_SEEK_SET)==(Sint64)e.offset &&
        SDL_RWread(rw,png.data(),1u,e.bytes)==e.bytes;
     if(ok){
      SDL_RWops* memory=SDL_RWFromConstMem(png.data(),(int)e.bytes);
      if(memory){
       SDL_Surface* decoded=IMG_Load_RW(memory,1);
       if(decoded){
        if(decoded->w==(int)w&&decoded->h==(int)h)
         surface=SDL_ConvertSurfaceFormat(decoded,SDL_PIXELFORMAT_BGRA32,0);
        SDL_FreeSurface(decoded);
       }
      }
     }
    }else{
     surface=SDL_CreateRGBSurfaceWithFormat(0,(int)w,(int)h,32,
                                            SDL_PIXELFORMAT_BGRA32);
     if(surface){
      const Sint64 offset=32+(Sint64)i*(Sint64)w*h*4;
      bool ok=SDL_RWseek(rw,offset,RW_SEEK_SET)==offset;
      if(ok&&surface->pitch==(int)w*4){
       const size_t bytes=(size_t)w*h*4u;
       ok=SDL_RWread(rw,surface->pixels,1u,bytes)==bytes;
      }else if(ok){
       for(unsigned y=0;ok&&y<h;++y){
        unsigned char* row=(unsigned char*)surface->pixels+(size_t)y*surface->pitch;
        ok=SDL_RWread(rw,row,1u,w*4u)==w*4u;
       }
      }
      if(!ok){SDL_FreeSurface(surface);surface=nullptr;}
     }
    }
    if(surface)keyBlack(surface);
    SDL_LockMutex(d.mutex);
    if(!d.stop)d.frames[clip][i]=surface;
    else if(surface)SDL_FreeSurface(surface);
    SDL_UnlockMutex(d.mutex);
    if(!surface)std::fprintf(stderr,"XM87.287 FD_FX_FRAME_FAIL name=%s frame=%u\n",
                              NAMES[clip],i);
    // Yield while navigating the menu rather than using all available CPU.
    SDL_Delay(1);
   }
   SDL_RWclose(rw);
   std::fprintf(stderr,"XM87.287 FD_FX_PRELOADED name=%s frames=%u\n",NAMES[clip],n);
  }
  return 0;
 }
 void setEnabled(bool enable){
  if(enable&&!mutex){
   mutex=SDL_CreateMutex();
   if(!mutex)return;
   thread=SDL_CreateThread(run,"XMFD FX predecode",this);
   if(!thread){SDL_DestroyMutex(mutex);mutex=nullptr;return;}
  }
  if(mutex){
   SDL_LockMutex(mutex);enabled=enable;SDL_UnlockMutex(mutex);
  }
 }
 unsigned status(unsigned clip,unsigned& framesOut){
  if(!mutex||clip>=CLIPS){framesOut=0u;return 0u;}
  SDL_LockMutex(mutex);
  const unsigned result=state[clip];
  framesOut=count[clip];
  SDL_UnlockMutex(mutex);
  return result;
 }
 SDL_Surface* get(unsigned clip,unsigned index){
  if(!mutex||clip>=CLIPS)return nullptr;
  SDL_LockMutex(mutex);
  SDL_Surface* surface=index<frames[clip].size()?frames[clip][index]:nullptr;
  SDL_UnlockMutex(mutex);
  return surface;
 }
 void shutdown(){
  if(!mutex)return;
  SDL_LockMutex(mutex);stop=true;SDL_UnlockMutex(mutex);
  if(thread){SDL_WaitThread(thread,nullptr);thread=nullptr;}
  for(unsigned c=0u;c<CLIPS;++c){
   for(SDL_Surface* surface:frames[c])if(surface)SDL_FreeSurface(surface);
   frames[c].clear();state[c]=count[c]=0u;
  }
  SDL_DestroyMutex(mutex);mutex=nullptr;
  stop=enabled=false;
 }
};
inline Cache& cache(){static Cache state;return state;}
struct Sequence {
 enum Phase{WAIT,PLAY,BREAK};
 Phase phase=WAIT;
 std::uint64_t lastFrame=0u,segmentStart=0u;
 unsigned clip=0u,shown=~0u,shownClip=~0u;
 SDL_Texture* texture[2]={nullptr,nullptr};
 int active=0,texW=0,texH=0;
 Uint8 alpha=0u;
 void reset(){
  phase=WAIT;lastFrame=segmentStart=0u;clip=0u;shown=shownClip=~0u;
  alpha=0u;
 }
 void close(){
  for(SDL_Texture*& t:texture){if(t)SDL_DestroyTexture(t);t=nullptr;}
  active=texW=texH=0;reset();
  cache().shutdown();
 }
 bool upload(SDL_Renderer* render,SDL_Surface* surface){
  if(!render||!surface)return false;
  if(!texture[0]||surface->w!=texW||surface->h!=texH){
   for(SDL_Texture*& t:texture){if(t)SDL_DestroyTexture(t);t=nullptr;}
   texW=surface->w;texH=surface->h;active=0;
   for(SDL_Texture*& t:texture){
    t=SDL_CreateTexture(render,SDL_PIXELFORMAT_BGRA32,
                        SDL_TEXTUREACCESS_STREAMING,texW,texH);
    if(!t)return false;
    SDL_SetTextureBlendMode(t,SDL_BLENDMODE_ADD);
    SDL_SetTextureScaleMode(t,SDL_ScaleModeLinear);
   }
  }
  const int next=1-active;
  if(SDL_UpdateTexture(texture[next],nullptr,surface->pixels,surface->pitch)!=0)
   return false;
  active=next;
  return true;
 }
 void tick(SDL_Renderer* render,std::uint64_t frame,const XMFDVideo::Sequence& main){
  if(main.phase==XMFDVideo::Sequence::PRE_MATCH || frame<lastFrame){
   if(phase!=WAIT)reset();
   return;
  }
  lastFrame=frame;
  if(phase==WAIT){
   // Delay is relative to the first main FD clip actually starting,
   // not relative to the beginning of the match.
   if(main.phase!=XMFDVideo::Sequence::PLAY || main.clip!=0u ||
      frame<main.segmentStart+START_AFTER_MAIN)return;
   phase=PLAY;segmentStart=frame;clip=0u;
   std::fprintf(stderr,"XM87.287 FD_FX_SEQUENCE_START mainOffset=300f\n");
  }
  if(phase==BREAK){
   if(frame-segmentStart<GAP)return;
   segmentStart+=GAP;
   clip=(clip+1u)%CLIPS;
   phase=PLAY;
   shown=shownClip=~0u;
  }
  if(phase!=PLAY)return;
  unsigned frames=0u;
  const unsigned state=cache().status(clip,frames);
  if(state==0u)return; // still scanning the FX pack, never block video
  if(state==2u || frames==0u){
   phase=BREAK;segmentStart=frame;alpha=0u;return;
  }
  const std::uint64_t duration=(std::uint64_t)frames*HOLD;
  const std::uint64_t elapsed=frame-segmentStart;
  if(elapsed>=duration){
   phase=BREAK;segmentStart=frame;alpha=0u;
   std::fprintf(stderr,"XM87.287 FD_FX_SEQUENCE_END name=%s\n",NAMES[clip]);
   return;
  }
  const unsigned requested=(unsigned)(elapsed/HOLD);
  if(shownClip!=clip||shown!=requested){
   SDL_Surface* surface=cache().get(clip,requested);
   if(surface&&upload(render,surface)){shownClip=clip;shown=requested;}
  }
  if(shownClip!=clip || !texture[active]){alpha=0u;return;}
  // Do not darken the main FD layer; only additive color is visible.
  const std::uint64_t fade=std::min((std::uint64_t)FADE,
          std::min(elapsed,duration-elapsed));
  alpha=(Uint8)(fade*255u/FADE);
 }
 bool draw(SDL_Renderer* render,const SDL_Rect& view){
  if(!render||alpha==0u||shownClip!=clip||!texture[active]||
     texW<=0||texH<=0||view.w<=0||view.h<=0)return true;
  // Aspect-ratio-preserving COVER: fill the viewport, crop source center.
  // No anisotropic resizing when FX clips have differing native resolutions.
  SDL_Rect src={0,0,texW,texH};
  if((std::int64_t)texW*view.h>(std::int64_t)texH*view.w){
   src.w=std::max(1,(int)((std::int64_t)texH*view.w/view.h));
   src.x=(texW-src.w)/2;
  }else{
   src.h=std::max(1,(int)((std::int64_t)texW*view.h/view.w));
   src.y=(texH-src.h)/2;
  }
  SDL_SetTextureAlphaMod(texture[active],alpha);
  return SDL_RenderCopy(render,texture[active],&src,&view)==0;
 }
};
} // namespace XMFDVideoFX
#endif
