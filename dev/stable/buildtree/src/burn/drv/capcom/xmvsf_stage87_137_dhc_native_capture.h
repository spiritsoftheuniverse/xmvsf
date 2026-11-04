#ifndef XMVSF_STAGE87_137_DHC_NATIVE_CAPTURE_H
#define XMVSF_STAGE87_137_DHC_NATIVE_CAPTURE_H
// Stage 87.137 DHC-native evidence collector. Read-only; opt-in; no ROM,
// game-RAM, native input, sound, draw or cycle changes.
// Enable XM87137_DHC_CAPTURE_TSV, optionally XM87137_DHC_CAPTURE_RAM.
// The trace is for discovering native tag/super transitions, NOT a DHC action
// detector: +A0 and +60 action changes alone do not prove any super.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87137Capture {
static inline unsigned byte(const std::uint8_t* r,unsigned addr) {
    return r[(addr&65535u)^1u]; // CPS-2 host word-swapped CpsRamFF
}
static inline unsigned word(const std::uint8_t* r,unsigned a) {
    return (byte(r,a)<<8)|byte(r,a+1u);
}
static inline std::uint32_t longword(const std::uint8_t* r,unsigned a) {
    return (std::uint32_t(word(r,a))<<16)|word(r,a+2u);
}
struct Actor {
    unsigned activity,id,bank,action,mode,superjump,anim,step06,step08;
    unsigned x,y,hitId70,hitId72,gate82,gate83;
    std::uint32_t script34,script38;
};
static inline Actor read(const std::uint8_t* r,unsigned base) {
    Actor a={};
    a.activity=word(r,base); a.id=word(r,base+0x52u);
    a.bank=word(r,base+0x40u); a.action=word(r,base+0x60u);
    a.mode=word(r,base+0xA0u);a.superjump=word(r,base+0xAAu);
    a.anim=word(r,base+0x42u);a.step06=word(r,base+0x06u);
    a.step08=word(r,base+0x08u);a.x=word(r,base+0x0Cu);
    a.y=word(r,base+0x10u);a.hitId70=word(r,base+0x70u);
    a.hitId72=word(r,base+0x72u);a.gate82=word(r,base+0x82u);
    a.gate83=word(r,base+0x83u);a.script34=longword(r,base+0x34u);
    a.script38=longword(r,base+0x38u);
    return a;
}
struct Recorder {
    bool initialized=false;
    bool enabled=false;
    bool previousValid[4]={false,false,false,false};
    Actor previous[4]={};
    unsigned previousFrame=0u,frames=0u;
    std::FILE *tsv=nullptr,*binary=nullptr;
    ~Recorder(){if(tsv)std::fclose(tsv);if(binary)std::fclose(binary);}
    void init(const char* driver) {
        if(initialized)return;
        if(!driver||std::strncmp(driver,"xmvsf",5)!=0)return;
        initialized=true;
        const char* path=std::getenv("XM87137_DHC_CAPTURE_TSV");
        if(!path||!*path)return;
        tsv=std::fopen(path,"wb");
        if(!tsv)return;
        enabled=true;
        std::setvbuf(tsv,nullptr,_IOFBF,64*1024);
        std::fputs("frame\trole\tphase\tp1buttons\tp2buttons\tcpiA\tcpiC\tcpiE"
                   "\tstockP1\tstockP2\tevent\tactivity\tcharacterId\tbank"
                   "\taction60\tmodeA0\tsuperjumpAA\tanim42\tstep06\tstep08"
                   "\tx\ty\tid70\tid72\tgate82\tgate83\tscript34\tscript38\n",tsv);
        const char* bin=std::getenv("XM87137_DHC_CAPTURE_RAM");
        if(bin&&*bin){
            binary=std::fopen(bin,"wb");
            if(binary)std::fwrite("XM87137R",1,8,binary);
        }
    }
    void frame(const std::uint8_t* ram,const char* driver,unsigned frameNumber,
               unsigned p1buttons,unsigned p2buttons,
               unsigned cpiA,unsigned cpiC,unsigned cpiE) {
        init(driver);
        if(!enabled||!ram||frameNumber==previousFrame)return;
        previousFrame=frameNumber;
        const unsigned phase=byte(ram,0x5000u);
        if(phase!=8u){
            for(unsigned i=0;i<4;++i)previousValid[i]=false;
            return;
        }
        if(frames>=18000u)return; // bounded capture: at most five minutes at 60fps
        const unsigned stockP1=byte(ram,0x4214u);
        const unsigned stockP2=byte(ram,0x4614u);
        if(binary){
            const std::uint32_t head[6]={std::uint32_t(frameNumber),
               std::uint32_t(p1buttons),std::uint32_t(p2buttons),
               std::uint32_t(cpiA),std::uint32_t(cpiC),std::uint32_t(cpiE)};
            std::fwrite(head,sizeof(head[0]),6,binary);
            std::fwrite(ram+0x4000u,1,0x1400u,binary); // four actors + global state
        }
        const unsigned bases[4]={0x4000u,0x4400u,0x4800u,0x4C00u};
        for(unsigned i=0;i<4;++i){
            const Actor a=read(ram,bases[i]);
            const char* event=!previousValid[i]?"FIRST":
                a.id!=previous[i].id?"IDENTITY":
                a.action!=previous[i].action?"ACTION":
                a.mode!=previous[i].mode?"MODE":
                a.anim!=previous[i].anim?"ANIMATION":"FRAME";
            std::fprintf(tsv,"%u\t%u\t%u\t%02X\t%02X\t%02X\t%02X\t%02X"
                "\t%u\t%u\t%s\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X"
                "\t%04X\t%04X\t%u\t%u\t%04X\t%04X\t%04X\t%04X\t%08X\t%08X\n",
                frameNumber,i,phase,p1buttons,p2buttons,cpiA,cpiC,cpiE,
                stockP1,stockP2,event,a.activity,a.id,a.bank,a.action,a.mode,
                a.superjump,a.anim,a.step06,a.step08,a.x,a.y,a.hitId70,
                a.hitId72,a.gate82,a.gate83,
                (unsigned)a.script34,(unsigned)a.script38);
            previous[i]=a;previousValid[i]=true;
        }
        ++frames;
        if(frames%120u==0u){std::fflush(tsv);if(binary)std::fflush(binary);}
    }
};
inline Recorder& state(){static Recorder p;return p;}
} // namespace XM87137Capture
#endif
