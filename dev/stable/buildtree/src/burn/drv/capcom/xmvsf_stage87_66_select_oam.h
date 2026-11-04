#ifndef XMVSF_STAGE87_66_SELECT_OAM_H
#define XMVSF_STAGE87_66_SELECT_OAM_H
// Original, pre-filtered CPS2 selection sprite stream. No drawing mutations.
// Objects are preserved with raw 8-byte words, original compressed slot,
// optional verified writer provenance, and native sprite-buffer shifts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
extern int xm14TrainingMode;
namespace XM8766OAM {
static inline void U32(unsigned char*p,unsigned n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(i*8));}
struct Probe {
    FILE*file;bool initialized;unsigned lastFrame,count;
    unsigned char data[1024u*24u];
    Probe():file(NULL),initialized(false),lastFrame(~0u),count(0){}
    ~Probe(){if(file)std::fclose(file);}
    void Init(){if(initialized)return;initialized=true;
        const char*dir=std::getenv("XM8766_SELECTION_TRACE");if(!dir||!*dir)return;
        char path[1200];std::snprintf(path,sizeof(path),"%s/xm8766_select_oam.bin",dir);
        file=std::fopen(path,"wb");if(!file){std::fprintf(stderr,"XM87.66 SELECT_OAM_OPEN_FAILED path=%s\n",path);return;}
        std::fwrite("XM8766S1",1,8,file);
        std::fprintf(stderr,"XM87.66 SELECT_OAM_READY path=%s\n",path);
    }
    void Observe(const unsigned char*obj,unsigned n,const XM8717::Source*sources,
       bool enabled,unsigned frame,unsigned phase,int shiftX,int shiftY,bool nativePass){
        if(!xm14TrainingMode||!nativePass||!obj||phase>=8u||frame==lastFrame)return;
        lastFrame=frame;Init();if(!file||count>=12000u)return;
        if(n>1024u)n=1024u;
        for(unsigned i=0;i<n;++i){
            unsigned char*d=data+i*24u;std::memcpy(d,obj+i*8u,8u);std::memset(d+8,0,16);
            if(enabled&&sources&&XM8717::Recent(sources[i],frame)){
                const XM8717::Source&s=sources[i];
                U32(d+8,(unsigned)s.pc);U32(d+12,(unsigned)s.a0);
                U32(d+16,(unsigned)s.frame);d[20]=s.owner;d[21]=s.valid;
            }
        }
        unsigned char header[20];U32(header,frame);U32(header+4,phase);U32(header+8,n);
        U32(header+12,(unsigned)shiftX);U32(header+16,(unsigned)shiftY);
        std::fwrite(header,1,20,file);if(n)std::fwrite(data,24,n,file);
        if(++count%60u==0u)std::fflush(file);
    }
};
static inline Probe& State(){static Probe p;return p;}
static inline void Observe(const unsigned char*obj,unsigned n,const XM8717::Source*s,bool enabled,
  unsigned frame,unsigned phase,int sx,int sy,bool nativePass){
    State().Observe(obj,n,s,enabled,frame,phase,sx,sy,nativePass);
}
}
#endif
