#ifndef XMVSF_STAGE87_60_NATIVE_OBJECT_AUDIT_H
#define XMVSF_STAGE87_60_NATIVE_OBJECT_AUDIT_H
// Stage 87.60: read-only original CPS-2 object stream for global super research.
// NO gameplay/ROM/composition mutations, NO move IDs inferred.
// Used to find native projectile/effect birth and disappearance across an owner
// action transition. An OAM tile by itself is NOT a verified projectile.
// Opt-in XM8760_OAM_AUDIT_FILE, only in phase 8, native object draw pass.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM8760 {
static inline void Put32(unsigned char *out,std::uint32_t n){
    out[0]=(unsigned char)n;out[1]=(unsigned char)(n>>8);
    out[2]=(unsigned char)(n>>16);out[3]=(unsigned char)(n>>24);
}
struct Probe {
    bool init;FILE *file;unsigned lastFrame,frames;
    unsigned char data[1024u*24u];
    Probe():init(false),file(nullptr),lastFrame(~0u),frames(0u){}
    ~Probe(){if(file)std::fclose(file);}
    void Init(){
        if(init)return;
        init=true;
        const char *path=std::getenv("XM8760_OAM_AUDIT_FILE");
        if(!path||!*path)return;
        file=std::fopen(path,"wb");
        if(!file){std::fprintf(stderr,"XM87.60 OAM_OPEN_FAILED path=%s\n",path);return;}
        std::fwrite("XM8760O1",1,8,file);
        std::fprintf(stderr,"XM87.60 NATIVE_OAM_AUDIT_READY path=%s read_only=1 mode_independent=1\n",path);
    }
    void Observe(const unsigned char *obj,unsigned n,const XM8717::Source *sources,
                 bool sourceEnabled,unsigned frame,unsigned phase,bool nativePass){
        if(!nativePass||!obj||frame==lastFrame)return;
        lastFrame=frame;Init();
        if(!file||phase!=8u||frames>=18000u)return;
        if(n>1024u)n=1024u;
        for(unsigned i=0;i<n;++i){
            unsigned char *dst=data+i*24u;
            std::memcpy(dst,obj+i*8u,8u);
            std::memset(dst+8,0,16u);
            if(sourceEnabled&&sources&&XM8717::Recent(sources[i],frame)){
                const XM8717::Source& s=sources[i];
                Put32(dst+8,(std::uint32_t)s.pc);
                Put32(dst+12,(std::uint32_t)s.a0);
                Put32(dst+16,(std::uint32_t)s.frame);
                dst[20]=s.owner;
                dst[21]=s.valid;
            }
        }
        unsigned char h[12];Put32(h,frame);Put32(h+4,phase);Put32(h+8,n);
        std::fwrite(h,1,12,file);
        if(n)std::fwrite(data,24,n,file);
        ++frames;
        if((frames%120u)==0u)std::fflush(file);
    }
};
static inline Probe& State(){static Probe p;return p;}
static inline void Observe(const unsigned char *obj,unsigned n,
  const XM8717::Source *sources,bool enabled,unsigned frame,unsigned phase,bool nativePass){
    State().Observe(obj,n,sources,enabled,frame,phase,nativePass);
}
} // XM8760
#endif
