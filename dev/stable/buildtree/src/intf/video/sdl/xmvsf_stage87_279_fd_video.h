#ifndef XMVSF_STAGE87_279_FD_VIDEO_H
#define XMVSF_STAGE87_279_FD_VIDEO_H
// FD ambient MP4 sequence, drawn BEHIND parallax stars. No audio output.
// Windows Media Foundation is loaded dynamically: no extra build/link
// dependency, external player process, ffmpeg DLL, or Makefile change.
// 10s after native match phase 8: fd01.mp4 -> 5s gap -> fd02.mp4
// -> 5s gap -> fd03.mp4 -> 5s gap -> repeat. Each uses its own real
// media duration with 1s fade-in and final 1s fade-out.
// All clocks use the emulated native frame, not SDL wall-clock time.
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#if defined(_WIN32) && __has_include(<mfapi.h>) && __has_include(<mfreadwrite.h>)
#define XMFD_HAVE_MEDIA_FOUNDATION 1
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propidl.h>
#else
#define XMFD_HAVE_MEDIA_FOUNDATION 0
#endif

namespace XMFDVideo {
static const std::uint64_t FPS=60u,WAIT_INITIAL=10u*FPS,GAP=5u*FPS,FADE=FPS;
#if XMFD_HAVE_MEDIA_FOUNDATION
// Local GUID values prevent new mfuuid linker dependencies. All numeric
// values come from the published Microsoft Media Foundation SDK headers.
static const GUID kMajor={0x48eba18e,0xf8c9,0x4687,{0xbf,0x11,0x0a,0x74,0xc9,0xf9,0x6a,0x8f}};
static const GUID kSubtype={0xf7e34c9a,0x42e8,0x4714,{0xb7,0x4b,0xcb,0x29,0xd7,0x2c,0x35,0xe5}};
static const GUID kVideo={0x73646976,0,0x0010,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
static const GUID kRGB32={0x00000016,0,0x0010,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
static const GUID kFrameSize={0x1652c33d,0xd6b2,0x4012,{0xb8,0x34,0x72,0x03,0x08,0x49,0xa3,0x7d}};
static const GUID kStride={0x644b4e48,0x1e02,0x4516,{0xb0,0xeb,0xc0,0x1c,0xa9,0xd4,0x9a,0xc6}};
static const GUID kDuration={0x6c990d33,0xbb8e,0x477a,{0x85,0x98,0x0d,0x5d,0x96,0xfc,0xd8,0x8a}};
static const GUID kEnableRGB={0xfb394f3d,0xccf1,0x42ee,{0xbb,0xb3,0xf9,0xb8,0x45,0xd5,0x68,0x1d}};
static const GUID kNull={0,0,0,{0,0,0,0,0,0,0,0}};
template<class T> static void release(T*& p){if(p){p->Release();p=nullptr;}}
static std::wstring utf16(const std::string& value){
 const int n=MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,nullptr,0);
 if(n<2)return std::wstring();
 std::wstring out((size_t)n,L'\0');
 if(!MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,&out[0],n))return std::wstring();
 out.resize((size_t)n-1u);
 return out;
}
struct Decoder {
 typedef HRESULT (WINAPI* StartupFn)(ULONG,DWORD);
 typedef HRESULT (WINAPI* ShutdownFn)();
 typedef HRESULT (WINAPI* ReaderFn)(LPCWSTR,IMFAttributes*,IMFSourceReader**);
 typedef HRESULT (WINAPI* TypeFn)(IMFMediaType**);
 typedef HRESULT (WINAPI* AttributesFn)(IMFAttributes**,UINT32);
 HMODULE plat=nullptr,readwrite=nullptr;
 StartupFn startup=nullptr;ShutdownFn shutdown=nullptr;
 ReaderFn makeReader=nullptr;TypeFn makeType=nullptr;
 AttributesFn makeAttrs=nullptr;
 bool com=false,started=false,attempted=false;
 IMFSourceReader* reader=nullptr;
 SDL_Texture* texture=nullptr;
 int width=0,height=0,stride=0,clip=-1;
 std::uint64_t durationFrames=0;
 LONGLONG lastPTS=-1;
 bool uploaded=false;
 std::vector<Uint8> rgba;

 void releaseClip(){
  release(reader);
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;clip=-1;width=height=stride=0;
  durationFrames=0;lastPTS=-1;uploaded=false;rgba.clear();
 }
 void close(){
  releaseClip();
  if(started && shutdown)shutdown();
  started=false;
  if(plat)FreeLibrary(plat);
  if(readwrite)FreeLibrary(readwrite);
  plat=readwrite=nullptr;
  if(com)CoUninitialize();
  com=false;attempted=false;
  startup=nullptr;shutdown=nullptr;makeReader=nullptr;
  makeType=nullptr;makeAttrs=nullptr;
 }
 bool init(){
  if(started)return true;
  if(attempted)return false;
  attempted=true;
  HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  if(hr==RPC_E_CHANGED_MODE)hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  com=SUCCEEDED(hr);
  if(!com){std::fprintf(stderr,"XM87.279 FD_VIDEO_COM_FAILED hr=%08lX\n",(unsigned long)hr);return false;}
  plat=LoadLibraryW(L"mfplat.dll");
  readwrite=LoadLibraryW(L"mfreadwrite.dll");
  if(!plat||!readwrite){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_MF_DLL_MISSING\n");return false;
  }
  startup=(StartupFn)GetProcAddress(plat,"MFStartup");
  shutdown=(ShutdownFn)GetProcAddress(plat,"MFShutdown");
  makeType=(TypeFn)GetProcAddress(plat,"MFCreateMediaType");
  makeAttrs=(AttributesFn)GetProcAddress(plat,"MFCreateAttributes");
  makeReader=(ReaderFn)GetProcAddress(readwrite,"MFCreateSourceReaderFromURL");
  if(!startup||!shutdown||!makeType||!makeAttrs||!makeReader){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_MF_EXPORT_MISSING\n");return false;
  }
  hr=startup(MF_VERSION,MFSTARTUP_FULL);
  if(FAILED(hr)){std::fprintf(stderr,"XM87.279 FD_VIDEO_MF_START_FAILED hr=%08lX\n",(unsigned long)hr);return false;}
  started=true;return true;
 }
 static std::string locate(int id){
  const char* names[]={"fd01.mp4","fd02.mp4","fd03.mp4"};
  if(id<0||id>2)return std::string();
  std::vector<std::string> folders={"_art/video/","../_art/video/","../../_art/video/"};
  char* base=SDL_GetBasePath();
  if(base){
   const std::string prefix(base);SDL_free(base);
   folders.push_back(prefix+"_art/video/");
   folders.push_back(prefix+"../_art/video/");
   folders.push_back(prefix+"../../_art/video/");
  }
  for(const auto& dir:folders){
   const std::string path=dir+names[id];
   SDL_RWops* file=SDL_RWFromFile(path.c_str(),"rb");
   if(file){SDL_RWclose(file);return path;}
  }
  return std::string();
 }
 bool open(SDL_Renderer* render,int id){
  if(clip==id && reader && texture)return true;
  releaseClip();
  if(!init())return false;
  const std::string path=locate(id);
  if(path.empty()){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_MISSING clip=%02d path=_art/video/fd%02d.mp4\n",id+1,id+1);
   return false;
  }
  std::wstring name=utf16(path);
  if(name.empty())return false;
  WCHAR absolute[32768]={0};
  const DWORD count=GetFullPathNameW(name.c_str(),32768,absolute,nullptr);
  if(count==0 || count>=32768)return false;
  IMFAttributes* attrs=nullptr;
  HRESULT hr=makeAttrs(&attrs,1u);
  if(SUCCEEDED(hr)&&attrs)hr=attrs->SetUINT32(kEnableRGB,1u);
  if(SUCCEEDED(hr))hr=makeReader(absolute,attrs,&reader);
  release(attrs);
  if(FAILED(hr)||!reader){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_OPEN_FAILED clip=%02d hr=%08lX\n",id+1,(unsigned long)hr);
   releaseClip();return false;
  }
  // Never route MP4 audio into gameplay's existing native audio system.
  reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE);
  hr=reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM,TRUE);
  IMFMediaType* desired=nullptr;
  if(SUCCEEDED(hr))hr=makeType(&desired);
  if(SUCCEEDED(hr))hr=desired->SetGUID(kMajor,kVideo);
  if(SUCCEEDED(hr))hr=desired->SetGUID(kSubtype,kRGB32);
  if(SUCCEEDED(hr))hr=reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,nullptr,desired);
  release(desired);
  IMFMediaType* actual=nullptr;
  if(SUCCEEDED(hr))hr=reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,&actual);
  UINT64 packed=0;
  if(SUCCEEDED(hr))hr=actual->GetUINT64(kFrameSize,&packed);
  UINT32 pitch=0;
  if(SUCCEEDED(hr))actual->GetUINT32(kStride,&pitch);
  release(actual);
  width=(int)(packed>>32);height=(int)(packed&0xffffffffu);
  stride=pitch?(int)(std::int32_t)pitch:width*4;
  if(FAILED(hr)||width<1||height<1||width>4096||height>4096||
     stride==0||((std::uint64_t)(stride<0?-(std::int64_t)stride:stride)<(std::uint64_t)width*4u)){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_MEDIA_TYPE_FAILED clip=%02d hr=%08lX w=%d h=%d stride=%d\n",
    id+1,(unsigned long)hr,width,height,stride);
   releaseClip();return false;
  }
  PROPVARIANT prop;PropVariantInit(&prop);
  hr=reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,kDuration,&prop);
  std::uint64_t hns=0;
  if(SUCCEEDED(hr) && (prop.vt==VT_UI8||prop.vt==VT_I8))
    hns=(std::uint64_t)prop.uhVal.QuadPart;
  PropVariantClear(&prop);
  durationFrames=(hns*FPS+5000000u)/10000000u;
  if(durationFrames<FADE*2u || durationFrames>FPS*3600u){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_DURATION_INVALID clip=%02d durationFrames=%llu\n",
    id+1,(unsigned long long)durationFrames);
   releaseClip();return false;
  }
  texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,width,height);
  if(!texture){releaseClip();return false;}
  SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
  SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
  rgba.resize((size_t)width*(size_t)height*4u);
  clip=id;
  std::fprintf(stderr,"XM87.279 FD_VIDEO_READY clip=%02d file=%s size=%dx%d frames=%llu stride=%d backend=MediaFoundation silent=1\n",
    id+1,path.c_str(),width,height,(unsigned long long)durationFrames,stride);
  return true;
 }
 bool decode(std::uint64_t elapsedFrames){
  if(!reader || !texture)return false;
  const LONGLONG target=(LONGLONG)(elapsedFrames*10000000u/FPS);
  if(uploaded && target<=lastPTS)return true;
  // Seek only if the emulator skipped well ahead (replay fast-forward).
  // Regular playback always streams sequentially with bounded reads.
  if(lastPTS>=0 && target-lastPTS>10000000LL){
   PROPVARIANT position;PropVariantInit(&position);
   position.vt=VT_I8;position.hVal.QuadPart=target;
   const HRESULT seek=reader->SetCurrentPosition(kNull,position);
   if(SUCCEEDED(seek)){lastPTS=-1;uploaded=false;}
  }
  for(int trial=0;trial<4;++trial){
   DWORD actualStream=0,flags=0;
   LONGLONG pts=0;
   IMFSample* sample=nullptr;
   const HRESULT hr=reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,
                                        &actualStream,&flags,&pts,&sample);
   if(FAILED(hr)){
    std::fprintf(stderr,"XM87.279 FD_VIDEO_READ_FAILED clip=%02d hr=%08lX\n",
       clip+1,(unsigned long)hr);
    release(sample);return false;
   }
   if(flags&MF_SOURCE_READERF_ENDOFSTREAM){release(sample);return uploaded;}
   if(!sample)continue;
   IMFMediaBuffer* buffer=nullptr;
   const HRESULT contiguous=sample->ConvertToContiguousBuffer(&buffer);
   release(sample);
   if(FAILED(contiguous)||!buffer){release(buffer);return false;}
   BYTE* bytes=nullptr;DWORD maxBytes=0,used=0;
   const HRESULT locked=buffer->Lock(&bytes,&maxBytes,&used);
   const size_t rowStride=(size_t)(stride<0?-(std::int64_t)stride:stride);
   const bool valid=SUCCEEDED(locked)&&bytes&&
     (std::uint64_t)rowStride*(std::uint64_t)height<=(std::uint64_t)used;
   if(valid){
    for(int y=0;y<height;++y){
     const int srcRow=stride<0?height-1-y:y; // MF negative stride is bottom-up
     const BYTE* src=bytes+(size_t)srcRow*rowStride;
     Uint8* dst=rgba.data()+(size_t)y*(size_t)width*4u;
     for(int x=0;x<width;++x){
      const int i=x*4;dst[i]=src[i+2];dst[i+1]=src[i+1];
      dst[i+2]=src[i];dst[i+3]=255u; // decoded RGB32 = B G R X
     }
    }
   }
   if(SUCCEEDED(locked))buffer->Unlock();
   release(buffer);
   if(!valid || SDL_UpdateTexture(texture,nullptr,rgba.data(),width*4)!=0){
    std::fprintf(stderr,"XM87.279 FD_VIDEO_BUFFER_FAILED clip=%02d size=%lu expected=%zu\n",
       clip+1,(unsigned long)used,rowStride*(size_t)height);
    return false;
   }
   lastPTS=pts;uploaded=true;
   if(pts>=target)break;
  }
  return uploaded;
 }
 void draw(SDL_Renderer* renderer,const SDL_Rect& target,Uint8 alpha){
  if(!renderer||!texture||!uploaded||alpha==0)return;
  SDL_SetTextureAlphaMod(texture,alpha);
  // Cover the viewport without deforming the 16:9 video.
  SDL_Rect src={0,0,width,height};
  if((std::int64_t)width*target.h>(std::int64_t)height*target.w){
   src.w=std::max(1,(int)((std::int64_t)height*target.w/target.h));
   src.x=(width-src.w)/2;
  }else{
   src.h=std::max(1,(int)((std::int64_t)width*target.h/target.w));
   src.y=(height-src.h)/2;
  }
  SDL_RenderCopy(renderer,texture,&src,&target);
 }
};
#else
struct Decoder {
 std::uint64_t durationFrames=0;
 void close(){}
 void releaseClip(){}
 bool open(SDL_Renderer*,int){return false;}
 bool decode(std::uint64_t){return false;}
 void draw(SDL_Renderer*,const SDL_Rect&,Uint8){}
};
#endif

struct Sequence {
 enum Phase{PRE_MATCH,INITIAL,PLAY,GAP_PHASE};
 Decoder decoder;
 Phase phase=PRE_MATCH;
 std::uint64_t matchStart=0,segmentStart=0;
 int clip=0;
 Uint8 alpha=0;
 bool loggedUnsupported=false;
 void close(){decoder.close();phase=PRE_MATCH;matchStart=segmentStart=0;clip=0;alpha=0;loggedUnsupported=false;}
 void resetMatch(){
  decoder.releaseClip();phase=PRE_MATCH;
  matchStart=segmentStart=0;clip=0;alpha=0;
 }
 bool showing()const{return phase==PLAY&&alpha>0;}
 void tick(SDL_Renderer* render,std::uint64_t frame,unsigned stagePhase){
  alpha=0;
#if !XMFD_HAVE_MEDIA_FOUNDATION
  if(!loggedUnsupported){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_UNAVAILABLE: Windows Media Foundation headers not present\n");
   loggedUnsupported=true;
  }
  (void)render;(void)frame;(void)stagePhase;
  return;
#else
  if(stagePhase<8u){
   if(phase!=PRE_MATCH)resetMatch();
   return;
  }
  if(phase==PRE_MATCH){
   if(stagePhase!=8u)return; // arm only on first real combat phase
   matchStart=frame;segmentStart=frame;clip=0;phase=INITIAL;
   std::fprintf(stderr,"XM87.279 FD_VIDEO_MATCH_START frame=%llu firstClipAfter=600\n",(unsigned long long)frame);
  }
  if(frame<matchStart){resetMatch();return;}
  if(phase==INITIAL){
   if(frame-matchStart<WAIT_INITIAL)return;
   phase=PLAY;segmentStart=frame;decoder.releaseClip();
  }
  if(phase==GAP_PHASE){
   if(frame-segmentStart<GAP)return;
   clip=(clip+1)%3;
   phase=PLAY;segmentStart=frame;decoder.releaseClip();
  }
  if(phase!=PLAY)return;
  if(!decoder.open(render,clip)){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_SKIP clip=%02d; next after 5s\n",clip+1);
   phase=GAP_PHASE;segmentStart=frame;return;
  }
  const std::uint64_t elapsed=frame-segmentStart;
  if(elapsed>=decoder.durationFrames){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_END clip=%02d frame=%llu; gap=300\n",
      clip+1,(unsigned long long)frame);
   decoder.releaseClip();phase=GAP_PHASE;segmentStart=frame;return;
  }
  const std::uint64_t remaining=decoder.durationFrames-elapsed;
  const std::uint64_t fadeIn=std::min(FADE,elapsed);
  const std::uint64_t fadeOut=std::min(FADE,remaining);
  alpha=(Uint8)(std::min(fadeIn,fadeOut)*255u/FADE);
  if(alpha && !decoder.decode(elapsed)){
   std::fprintf(stderr,"XM87.279 FD_VIDEO_DECODE_SKIP clip=%02d\n",clip+1);
   decoder.releaseClip();phase=GAP_PHASE;segmentStart=frame;alpha=0;
  }
#endif
 }
 void draw(SDL_Renderer* renderer,const SDL_Rect& view){if(showing())decoder.draw(renderer,view,alpha);}
};
} // namespace XMFDVideo
#endif
