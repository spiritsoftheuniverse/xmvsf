#ifndef XMVSF_STAGE87_280_FD_GIF_H
#define XMVSF_STAGE87_280_FD_GIF_H
// Final Destination GIF ambience. GIFs are source assets, not decoded into
// an unbounded IMG_Animation frame array. Windows Imaging Component streams
// one frame at a time into a single 384x224 SDL texture. This avoids the
// extremely large allocation/long blocking decode of the ~40-74 MB GIFs.
// No additional DLL, third-party dependency, new build gate, or sound output.
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#if defined(_WIN32) && __has_include(<wincodec.h>)
#define XMFD_GIF_WIC 1
#include <windows.h>
#include <wincodec.h>
#include <propidl.h>
#else
#define XMFD_GIF_WIC 0
#endif

namespace XMFDVideo {
static const std::uint64_t FPS=60, INITIAL=10*FPS, GAP=5*FPS, FADE=FPS;
static const int VIEW_W=384,VIEW_H=224;
static std::string locate(int index){
 const char* names[]={"fd01.gif","fd02.gif","fd03.gif"};
 if(index<0||index>2)return {};
 std::vector<std::string> roots={"_art/video/","../_art/video/","../../_art/video/"};
 char* base=SDL_GetBasePath();
 if(base){
  const std::string prefix(base);SDL_free(base);
  roots.push_back(prefix+"_art/video/");
  roots.push_back(prefix+"../_art/video/");
  roots.push_back(prefix+"../../_art/video/");
 }
 for(const std::string& root:roots){
  const std::string path=root+names[index];
  SDL_RWops* f=SDL_RWFromFile(path.c_str(),"rb");
  if(f){SDL_RWclose(f);return path;}
 }
 return {};
}

#if XMFD_GIF_WIC
template<typename T>static void release(T*& p){if(p){p->Release();p=nullptr;}}
static int number(IWICMetadataQueryReader* reader,const wchar_t* key,int fallback){
 if(!reader)return fallback;
 PROPVARIANT v;PropVariantInit(&v);
 const HRESULT hr=reader->GetMetadataByName(key,&v);
 if(FAILED(hr))return fallback;
 int out=fallback;
 switch(v.vt){
  case VT_UI1:out=v.bVal;break;
  case VT_UI2:out=v.uiVal;break;
  case VT_UI4:out=(int)v.ulVal;break;
  case VT_I2:out=v.iVal;break;
  case VT_I4:out=v.lVal;break;
  default:break;
 }
 PropVariantClear(&v);
 return out;
}
static std::wstring wide(const std::string& s){
 const int size=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);
 if(size<2)return {};
 std::wstring w((size_t)size,L'\0');
 if(!MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],size))return {};
 w.resize((size_t)size-1u);
 return w;
}
struct FrameInfo {
 std::uint64_t first=0;
 unsigned duration=6;
 unsigned left=0,top=0,width=0,height=0;
 int disposal=0;
};
struct Decoder {
 IWICImagingFactory* factory=nullptr;
 IWICBitmapDecoder* gif=nullptr;
 SDL_Texture* texture=nullptr;
 std::vector<FrameInfo> frames;
 std::vector<Uint8> canvas,previous,viewPixels,patch;
 UINT w=0,h=0,count=0,indexed=0;
 int clip=-1,drawn=-1;
 int lastDisposal=0;
 FrameInfo lastRect;
 std::uint64_t durationFrames=0;
 bool ready=false,ownedCOM=false,comAttempted=false,failed=false;

 void releaseClip(){
  release(gif);
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;clip=-1;w=h=count=indexed=0;drawn=-1;
  ready=false;durationFrames=0;lastDisposal=0;lastRect=FrameInfo{};
  frames.clear();canvas.clear();previous.clear();viewPixels.clear();patch.clear();
 }
 void close(){
  releaseClip();
  release(factory);
  if(ownedCOM)CoUninitialize();
  ownedCOM=false;comAttempted=false;failed=false;
 }
 bool beginCOM(){
  if(factory)return true;
  if(comAttempted)return false;
  comAttempted=true;
  HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  if(hr==RPC_E_CHANGED_MODE){
   // The SDL thread already has a COM apartment; reuse it.
   hr=S_OK;ownedCOM=false;
  }else if(SUCCEEDED(hr))ownedCOM=true;
  if(FAILED(hr)){std::fprintf(stderr,"XM87.280 FD_GIF_COM_FAIL hr=%08lX\n",(unsigned long)hr);return false;}
  hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
                      IID_IWICImagingFactory,(void**)&factory);
  if(FAILED(hr)||!factory){
   std::fprintf(stderr,"XM87.280 FD_GIF_WIC_FACTORY_FAIL hr=%08lX\n",(unsigned long)hr);
   return false;
  }
  return true;
 }
 bool open(SDL_Renderer* render,int index){
  if(index==clip && gif)return true;
  releaseClip();
  if(!beginCOM())return false;
  const std::string path=locate(index);
  if(path.empty()){
   std::fprintf(stderr,"XM87.280 FD_GIF_MISSING clip=%d expected=_art/video/fd%02d.gif\n",index+1,index+1);
   return false;
  }
  const std::wstring file=wide(path);
  if(file.empty())return false;
  HRESULT hr=factory->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,
           WICDecodeMetadataCacheOnDemand,&gif);
  if(FAILED(hr)||!gif){
   std::fprintf(stderr,"XM87.280 FD_GIF_OPEN_FAIL clip=%d hr=%08lX\n",index+1,(unsigned long)hr);
   releaseClip();return false;
  }
  hr=gif->GetFrameCount(&count);
  if(FAILED(hr)||count==0||count>40000u){releaseClip();return false;}
  IWICMetadataQueryReader* metadata=nullptr;
  gif->GetMetadataQueryReader(&metadata);
  w=(UINT)std::max(0,number(metadata,L"/logscrdesc/Width",0));
  h=(UINT)std::max(0,number(metadata,L"/logscrdesc/Height",0));
  release(metadata);
  IWICBitmapFrameDecode* first=nullptr;
  if(SUCCEEDED(gif->GetFrame(0,&first))&&first){
   UINT fw=0,fh=0;first->GetSize(&fw,&fh);
   if(!w)w=fw;if(!h)h=fh;
  }
  release(first);
  if(w<1||h<1||w>4096||h>4096){
   std::fprintf(stderr,"XM87.280 FD_GIF_DIMENSIONS_INVALID clip=%d %ux%u\n",index+1,w,h);
   releaseClip();return false;
  }
  texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,VIEW_W,VIEW_H);
  if(!texture){releaseClip();return false;}
  SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear); // video only; stage/stars remain nearest
  frames.resize((size_t)count);
  canvas.assign((size_t)w*h*4u,0u);
  viewPixels.resize((size_t)VIEW_W*VIEW_H*4u);
  clip=index;failed=false;
  std::fprintf(stderr,"XM87.280 FD_GIF_OPEN clip=%d file=%s source=%ux%u frames=%u streaming=1 nativeTexture=384x224\n",
    index+1,path.c_str(),w,h,count);
  return true;
 }
 bool indexSome(unsigned budget=40){
  if(ready)return true;
  if(!gif||failed)return false;
  for(unsigned work=0;work<budget && indexed<count;++work,++indexed){
   IWICBitmapFrameDecode* frame=nullptr;
   HRESULT hr=gif->GetFrame(indexed,&frame);
   if(FAILED(hr)||!frame){release(frame);failed=true;return false;}
   FrameInfo m;
   frame->GetSize(&m.width,&m.height);
   IWICMetadataQueryReader* meta=nullptr;
   frame->GetMetadataQueryReader(&meta);
   const int delay=number(meta,L"/grctlext/Delay",10);
   m.duration=(unsigned)std::max(1,(std::max(2,delay)*60+50)/100);
   m.left=(unsigned)std::max(0,number(meta,L"/imgdesc/Left",0));
   m.top=(unsigned)std::max(0,number(meta,L"/imgdesc/Top",0));
   m.disposal=number(meta,L"/grctlext/Disposal",0);
   m.first=durationFrames;
   durationFrames+=m.duration;
   frames[indexed]=m;
   release(meta);release(frame);
  }
  if(indexed==count){
   ready=true;
   std::fprintf(stderr,"XM87.280 FD_GIF_INDEXED clip=%d frames=%u durationFrames=%llu durationSeconds=%.2f\n",
       clip+1,count,(unsigned long long)durationFrames,(double)durationFrames/60.);
  }
  return ready;
 }
 void clearRect(const FrameInfo& r){
  for(unsigned y=r.top;y<h && y<r.top+r.height;++y)
   for(unsigned x=r.left;x<w && x<r.left+r.width;++x){
    Uint8* d=canvas.data()+((size_t)y*w+x)*4u;
    d[0]=d[1]=d[2]=0;d[3]=255;
   }
 }
 bool compose(unsigned i){
  if(!gif || i>=count || !ready)return false;
  const FrameInfo& info=frames[i];
  if(drawn>=0){
   if(lastDisposal==2)clearRect(lastRect);
   else if(lastDisposal==3 && previous.size()==canvas.size())canvas=previous;
  }
  if(info.disposal==3)previous=canvas;
  IWICBitmapFrameDecode* frame=nullptr;
  HRESULT hr=gif->GetFrame(i,&frame);
  if(FAILED(hr)||!frame){release(frame);return false;}
  IWICFormatConverter* convert=nullptr;
  hr=factory->CreateFormatConverter(&convert);
  if(SUCCEEDED(hr))hr=convert->Initialize(frame,GUID_WICPixelFormat32bppBGRA,
                   WICBitmapDitherTypeNone,nullptr,0.,WICBitmapPaletteTypeCustom);
  const size_t bytes=(size_t)info.width*info.height*4u;
  if(SUCCEEDED(hr) && bytes<=((size_t)4096*4096*4u)){
   patch.resize(bytes);
   hr=convert->CopyPixels(nullptr,info.width*4u,(UINT)bytes,patch.data());
  }else hr=E_FAIL;
  release(convert);release(frame);
  if(FAILED(hr)){std::fprintf(stderr,"XM87.280 FD_GIF_FRAME_FAIL clip=%d i=%u hr=%08lX\n",
                              clip+1,i,(unsigned long)hr);return false;}
  for(unsigned py=0;py<info.height && info.top+py<h;++py){
   const Uint8* src=patch.data()+(size_t)py*info.width*4u;
   Uint8* dst=canvas.data()+((size_t)(info.top+py)*w+info.left)*4u;
   for(unsigned px=0;px<info.width && info.left+px<w;++px){
    const Uint8* q=src+(size_t)px*4u;
    Uint8* p=dst+(size_t)px*4u;
    const unsigned a=q[3];
    if(a==255u){p[0]=q[2];p[1]=q[1];p[2]=q[0];p[3]=255;}
    else if(a){
     p[0]=(Uint8)((q[2]*a+p[0]*(255u-a)+127u)/255u);
     p[1]=(Uint8)((q[1]*a+p[1]*(255u-a)+127u)/255u);
     p[2]=(Uint8)((q[0]*a+p[2]*(255u-a)+127u)/255u);
     p[3]=255;
    }
   }
  }
  lastRect=info;lastDisposal=info.disposal;drawn=(int)i;
  return true;
 }
 bool update(std::uint64_t elapsed){
  if(!ready || !texture || !gif || durationFrames==0)return false;
  // The GIF metadata is indexed once in small batches during the initial
  // 10s wait and each later 5s gap. Pixel data is decoded ONLY when needed.
  const auto it=std::upper_bound(frames.begin(),frames.end(),elapsed,
       [](std::uint64_t t,const FrameInfo& m){return t<m.first;});
  unsigned target=it==frames.begin()?0u:(unsigned)(it-frames.begin()-1u);
  if(target>=count)target=count-1;
  if(drawn>(int)target){
   std::fill(canvas.begin(),canvas.end(),0u);
   drawn=-1;lastDisposal=0;previous.clear();
  }
  if(drawn==(int)target)return true;
  // Bounded per-paint GIF decompression; never decode dozens of frames
  // on one game frame during a replay/fast-forward jump.
  for(int n=0;n<2 && drawn<(int)target;++n)
   if(!compose((unsigned)(drawn+1)))return false;
  // Downsample the full composited GIF to the native viewport only.
  // Keep aspect ratio via center-cover, never warp native game graphics.
  unsigned cropW=w,cropH=h,originX=0,originY=0;
  if((std::uint64_t)w*VIEW_H>(std::uint64_t)h*VIEW_W){
   cropW=(unsigned)std::max<std::uint64_t>(1,(std::uint64_t)h*VIEW_W/VIEW_H);
   originX=(w-cropW)/2u;
  }else{
   cropH=(unsigned)std::max<std::uint64_t>(1,(std::uint64_t)w*VIEW_H/VIEW_W);
   originY=(h-cropH)/2u;
  }
  for(int y=0;y<VIEW_H;++y){
   const size_t sy=originY+((size_t)y*cropH/VIEW_H);
   for(int x=0;x<VIEW_W;++x){
    const size_t sx=originX+((size_t)x*cropW/VIEW_W);
    const Uint8* p=canvas.data()+(sy*w+sx)*4u;
    Uint8* d=viewPixels.data()+((size_t)y*VIEW_W+x)*4u;
    d[0]=p[0];d[1]=p[1];d[2]=p[2];d[3]=255;
   }
  }
  const int upload=SDL_UpdateTexture(texture,nullptr,viewPixels.data(),VIEW_W*4);
  if(upload!=0)std::fprintf(stderr,"XM87.280 FD_GIF_UPLOAD_FAIL clip=%d %s\n",clip+1,SDL_GetError());
  return upload==0;
 }
 void draw(SDL_Renderer* r,const SDL_Rect& view,Uint8 opacity){
  if(!r || !texture || drawn<0 || !opacity)return;
  SDL_SetTextureAlphaMod(texture,opacity);
  SDL_RenderCopy(r,texture,nullptr,&view);
 }
};
#else
// Non-Windows build/SDK fallback: SDL_image's animation interface.
// Only ONE GIF is held at a time; one reusable streaming GPU texture.
// Windows intentionally uses WIC to avoid buffering enormous GIFs.
struct Decoder {
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
 IMG_Animation* gif=nullptr;
#endif
 SDL_Texture* texture=nullptr;
 int clip=-1,drawn=-1;
 std::uint64_t durationFrames=0;
 std::vector<std::uint64_t> starts;
 bool ready=false,failed=false;
 void releaseClip(){
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
  if(gif){IMG_FreeAnimation(gif);gif=nullptr;}
#endif
  if(texture)SDL_DestroyTexture(texture);
  texture=nullptr;clip=-1;drawn=-1;durationFrames=0;
  starts.clear();ready=false;failed=false;
 }
 void close(){releaseClip();}
 bool open(SDL_Renderer* r,int index){
  if(clip==index)return ready;
  releaseClip();
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
  const std::string path=locate(index);
  gif=path.empty()?nullptr:IMG_LoadAnimation(path.c_str());
  if(!gif||gif->count<1){failed=true;return false;}
  texture=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,gif->w,gif->h);
  if(!texture){releaseClip();return false;}
  SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
  SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
  for(int i=0;i<gif->count;++i){
   starts.push_back(durationFrames);
   int delay=gif->delays?gif->delays[i]:100;
   durationFrames+=(std::uint64_t)std::max(1,(std::max(20,delay)*60+500)/1000);
  }
  clip=index;ready=true;
  std::fprintf(stderr,"XM87.280 FD_GIF_SDL_IMAGE clip=%d frames=%d duration=%llu (fallback)\n",
     index+1,gif->count,(unsigned long long)durationFrames);
  return true;
#else
  (void)r;(void)index;return false;
#endif
 }
 bool indexSome(unsigned=40){return ready;}
 bool update(std::uint64_t elapsed){
#if SDL_IMAGE_VERSION_ATLEAST(2,6,0)
  if(!gif||!ready)return false;
  const auto i=std::upper_bound(starts.begin(),starts.end(),elapsed);
  int index=i==starts.begin()?0:(int)(i-starts.begin()-1);
  if(index==drawn)return true;
  SDL_Surface* frame=gif->frames[index];
  if(!frame)return false;
  SDL_Texture* fresh=SDL_CreateTextureFromSurface(SDL_GetRenderer(SDL_GetWindowFromID(0)),frame);
  // The SDL fallback uses only one active GPU texture; avoid shared copies.
  if(!fresh)return false;
  if(texture)SDL_DestroyTexture(texture);
  texture=fresh;
  SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
  drawn=index;
  return true;
#else
  (void)elapsed;return false;
#endif
 }
 void draw(SDL_Renderer* r,const SDL_Rect& view,Uint8 a){
  if(r&&texture&&drawn>=0&&a){
   SDL_SetTextureAlphaMod(texture,a);
   SDL_RenderCopy(r,texture,nullptr,&view);
  }
 }
};
#endif

struct Sequence {
 enum Phase{PRE_MATCH,INITIAL,PLAY,GAP_PHASE};
 Decoder decoder;
 Phase phase=PRE_MATCH;
 std::uint64_t matchStart=0,segmentStart=0,lastFrame=0;
 int clip=0;
 Uint8 alpha=0;
 bool failedClip[3]={false,false,false};
 void close(){decoder.close();phase=PRE_MATCH;matchStart=segmentStart=lastFrame=0;clip=0;alpha=0;
   for(int i=0;i<3;++i)failedClip[i]=false;}
 void resetMatch(){decoder.releaseClip();phase=PRE_MATCH;matchStart=segmentStart=lastFrame=0;clip=0;alpha=0;
   for(int i=0;i<3;++i)failedClip[i]=false;}
 bool showing()const{return phase==PLAY && alpha!=0;}
 void preindex(SDL_Renderer* renderer){
  if(failedClip[clip])return;
  if(!decoder.open(renderer,clip)){failedClip[clip]=true;return;}
  decoder.indexSome(40u);
 }
 void tick(SDL_Renderer* renderer,std::uint64_t frame,unsigned status){
  alpha=0;
  if(status<8u){if(phase!=PRE_MATCH)resetMatch();return;}
  if(phase==PRE_MATCH){
   if(status!=8u)return;
   phase=INITIAL;clip=0;matchStart=segmentStart=frame;
   lastFrame=frame;
   std::fprintf(stderr,"XM87.280 FD_GIF_MATCH_START frame=%llu first=600f\n",
       (unsigned long long)frame);
  }
  if(frame<lastFrame){resetMatch();return;} // rewind / new replay
  lastFrame=frame;
  if(phase==INITIAL){
   preindex(renderer); // amortize metadata during the ten-second wait
   if(frame-matchStart<INITIAL)return;
   phase=PLAY;segmentStart=frame;
  }
  if(phase==GAP_PHASE){
   preindex(renderer); // amortize metadata during each five-second gap
   if(frame-segmentStart<GAP)return;
   phase=PLAY;segmentStart=frame;
  }
  if(phase!=PLAY)return;
  preindex(renderer);
  if(failedClip[clip] || !decoder.ready){
   // Missing codec/assets do not stall battle or freeze the other clips.
   std::fprintf(stderr,"XM87.280 FD_GIF_SKIP clip=%d indexReady=%d\n",clip+1,(int)decoder.ready);
   decoder.releaseClip();phase=GAP_PHASE;segmentStart=frame;clip=(clip+1)%3;return;
  }
  const std::uint64_t elapsed=frame-segmentStart;
  if(elapsed>=decoder.durationFrames){
   std::fprintf(stderr,"XM87.280 FD_GIF_END clip=%d frames=%llu nextAfter=300f\n",
       clip+1,(unsigned long long)decoder.durationFrames);
   decoder.releaseClip();phase=GAP_PHASE;segmentStart=frame;clip=(clip+1)%3;return;
  }
  const std::uint64_t fadeIn=std::min<std::uint64_t>(FADE,elapsed);
  const std::uint64_t fadeOut=std::min<std::uint64_t>(FADE,decoder.durationFrames-elapsed);
  alpha=(Uint8)(std::min(fadeIn,fadeOut)*255u/FADE);
  if(alpha && !decoder.update(elapsed)){
   std::fprintf(stderr,"XM87.280 FD_GIF_FRAME_SKIP clip=%d\n",clip+1);
   decoder.releaseClip();phase=GAP_PHASE;segmentStart=frame;clip=(clip+1)%3;alpha=0;
  }
 }
 void draw(SDL_Renderer* r,const SDL_Rect& viewport){
  if(showing())decoder.draw(r,viewport,alpha);
 }
};
} // namespace XMFDVideo
#endif
