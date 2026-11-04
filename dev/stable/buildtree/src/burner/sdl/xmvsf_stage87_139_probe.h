#ifndef XMVSF_STAGE87_139_PROBE_H
#define XMVSF_STAGE87_139_PROBE_H
// 87.139 F10 diagnostic recorder. Read-only. NEVER edits RAM, video/OAM,
// controller values, tag state, frame order, sound, pause, or game logic.
// One global state shared by CPS2, SDL input and presenter translation units.
// Output: inputs+all four native actors and buffered native sprite palette/tile
// samples around a confirmed tag. F10 creates a new numbered pair on every start.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
namespace XM87139Probe {
static inline unsigned b(const std::uint8_t* r,unsigned a){return r[(a&65535u)^1u];}
static inline unsigned w(const std::uint8_t* r,unsigned a){return (b(r,a)<<8)|b(r,a+1u);}
static inline std::uint32_t d(const std::uint8_t* r,unsigned a){
    return (std::uint32_t(w(r,a))<<16)|w(r,a+2u);
}
struct Actor {
    unsigned active=0,id=0,bank=0,action=0,mode=0,anim=0;
    unsigned x=0,y=0,face=0,hit70=0,hit72=0,gate82=0,gate83=0;
    unsigned nativeD6=0,controller256=0,controller259=0,step08=0,timer4E=0,entryA4=0;
    std::uint32_t script34=0,script38=0,vx=0,vy=0;
};
static inline Actor actor(const std::uint8_t* r,unsigned a){
    Actor o;
    o.active=w(r,a);o.id=w(r,a+0x52u);o.bank=w(r,a+0x40u);
    o.action=w(r,a+0x60u);o.mode=w(r,a+0xA0u);
    o.anim=w(r,a+0x42u);o.x=w(r,a+0x0Cu);o.y=w(r,a+0x10u);
    o.face=b(r,a+0x4Bu)&1u;o.hit70=w(r,a+0x70u);
    o.hit72=w(r,a+0x72u);o.gate82=w(r,a+0x82u);
    o.gate83=w(r,a+0x83u);o.script34=d(r,a+0x34u);
    o.script38=d(r,a+0x38u);o.vx=d(r,a+0x14u);
    o.vy=d(r,a+0x1Cu);
    o.nativeD6=b(r,a+0xD6u);
    o.controller256=b(r,a+0x256u);
    o.controller259=b(r,a+0x259u);
    o.step08=w(r,a+0x08u);
    o.timer4E=w(r,a+0x4Eu);
    o.entryA4=b(r,a+0xA4u);
    return o;
}
// Already exported from the native SDL input mapping. The mapped samples
// complement (do not replace) actual post-CPU CPS2 button/port readouts.
extern "C" int xm87137ReadMappedInput(unsigned side,unsigned ago,
                                     unsigned* frame,unsigned* direction,
                                     unsigned* held,unsigned* pressed);
struct State {
    bool active=false,error=false;
    unsigned session=0,frames=0,tagEvents=0,lastFrame=~0u;
    unsigned lastOamFrame=~0u,oamCount=0,tagUntil=0;
    std::FILE *trace=nullptr,*oam=nullptr;
    std::string tracePath,oamPath;
    bool priorValid[4]={false,false,false,false};
    Actor prior[4];
    ~State(){stop();}
    void stop(){
        if(trace){
            std::fprintf(trace,"# F10_STOP native_frames=%u tag_events=%u%s\n",
                frames,tagEvents,frames==0u?" NO_COMBAT_FRAMES_SEEN":"");
            std::fflush(trace);std::fclose(trace);trace=nullptr;
        }
        if(oam){std::fflush(oam);std::fclose(oam);oam=nullptr;}
        active=false;
    }
    bool toggle(bool inGame){
        if(active){stop();std::fprintf(stderr,"XM87.139 PROBE_STOP frames=%u tags=%u file=%s oam=%s\n",
                 frames,tagEvents,tracePath.c_str(),oamPath.c_str());return true;}
        if(!inGame)return false;
        error=false;const char* dir=std::getenv("XM87139_PROBE_DIR");
        if(!dir||!*dir){error=true;std::fprintf(stderr,"XM87.139 PROBE_ERROR directory not configured\n");return false;}
        char path[2048], oamName[2048];
        bool found=false;
        for(unsigned tries=0;tries<9999u;++tries){
            const unsigned serial=session+1u+tries;
            std::snprintf(path,sizeof(path),"%s/xm87139_probe_%04u.tsv",dir,serial);
            std::snprintf(oamName,sizeof(oamName),"%s/xm87139_oam_%04u.tsv",dir,serial);
            std::FILE* old=std::fopen(path,"rb");
            std::FILE* oldOam=std::fopen(oamName,"rb");
            found=!old&&!oldOam;
            if(old)std::fclose(old);
            if(oldOam)std::fclose(oldOam);
            if(found){session=serial;break;}
        }
        if(!found){error=true;return false;}
        trace=std::fopen(path,"wb");oam=std::fopen(oamName,"wb");
        if(!trace||!oam){
            stop();error=true;
            std::fprintf(stderr,"XM87.139 PROBE_ERROR cannot create files at %s\n",dir);
            return false;
        }
        tracePath=path;oamPath=oamName;frames=tagEvents=oamCount=0;
        lastFrame=lastOamFrame=~0u;tagUntil=0;
        for(unsigned i=0;i<4;++i)priorValid[i]=false;
        std::fputs("frame\trole\tevent\tphase\tstage\tp1buttons\tp2buttons"
          "\tcpi01A\tcpi01C\tcpi01E\tmapP1frame\tmapP1dir\tmapP1held\tmapP1pressed"
          "\tmapP2frame\tmapP2dir\tmapP2held\tmapP2pressed"
          "\tmeterP1\tmeterP2\tactivity\tfighterId\tbank40\taction60\tmodeA0"
          "\tanim42\tx\ty\tfacingBit\tvx\tvy\thit70\thit72\tgate82\tgate83"
          "\tscript34\tscript38\tnativeD6\tcontroller256\tcontroller259"
          "\tstep08\ttimer4E\tentryA4\n",trace);
        std::fputs("frame\tslot\tpriority\tpalette\ttile\twidth\theight\trawX\trawY"
          "\tattr\tsourceWord0\tsourceWord1\tpassFrom\tpassTo\n",oam);
        // Even if a menu handler formerly stole F10 or no combat frames
        // arrive, starting/stopping is immediately observable on disk.
        std::fputs("# F10_START armed; awaiting native combat frames\n",trace);
        std::fflush(trace);
        std::fputs("# F10_START armed; raw OAM follows a native tag\n",oam);
        std::fflush(oam);
        active=true;
        std::fprintf(stderr,"XM87.139 PROBE_START inputs=%s nativeOAM=%s\n",
                     tracePath.c_str(),oamPath.c_str());
        return true;
    }
    void frame(const std::uint8_t* r,const char* driver,unsigned frameId,
               bool runahead,unsigned p1Buttons,unsigned p2Buttons,
               unsigned cpiA,unsigned cpiC,unsigned cpiE,
               unsigned mf1,unsigned md1,unsigned mh1,unsigned mp1,
               unsigned mf2,unsigned md2,unsigned mh2,unsigned mp2) {
        if(!active||!trace||!r||runahead||frameId==lastFrame||
           !driver||std::strncmp(driver,"xmvsf",5)!=0)return;
        lastFrame=frameId;
        const unsigned phase=b(r,0x5000u),stage=b(r,0x5113u)&15u;
        if(phase!=8u){
            for(unsigned i=0;i<4;++i)priorValid[i]=false;
            return;
        }
        const unsigned bases[4]={0x4000u,0x4400u,0x4800u,0x4C00u};
        Actor current[4];
        for(unsigned i=0;i<4;++i)current[i]=actor(r,bases[i]);
        for(unsigned side=0;side<2u;++side){
            const unsigned point=side,reserve=side+2u;
            if(priorValid[point]&&priorValid[reserve] &&
               prior[point].id==current[reserve].id &&
               prior[reserve].id==current[point].id &&
               current[point].id!=prior[point].id) {
                ++tagEvents;tagUntil=frameId+90u;
                std::fprintf(stderr,"XM87.139 TAG_OBSERVED frame=%u side=%u old=%04X new=%04X\n",
                    frameId,side+1u,prior[point].id,current[point].id);
            }
        }
        for(unsigned i=0;i<4;++i){
            const Actor& a=current[i];
            const char* event=!priorValid[i]?"FIRST":
               prior[i].id!=a.id?"IDENTITY":
               prior[i].action!=a.action?"ACTION":
               prior[i].mode!=a.mode?"MODE":
               prior[i].anim!=a.anim?"ANIMATION":"FRAME";
            std::fprintf(trace,
                "%u\t%u\t%s\t%u\t%u\t%02X\t%02X\t%02X\t%02X\t%02X"
                "\t%u\t%02X\t%02X\t%02X\t%u\t%02X\t%02X\t%02X"
                "\t%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X"
                "\t%u\t%u\t%u\t%08X\t%08X\t%04X\t%04X\t%04X\t%04X\t%08X\t%08X"
                "\t%02X\t%02X\t%02X\t%04X\t%04X\t%02X\n",
                frameId,i,event,phase,stage,p1Buttons,p2Buttons,cpiA,cpiC,cpiE,
                mf1,md1,mh1,mp1,mf2,md2,mh2,mp2,
                b(r,0x4214u),b(r,0x4614u),a.active,a.id,a.bank,a.action,
                a.mode,a.anim,a.x,a.y,a.face,(unsigned)a.vx,(unsigned)a.vy,
                a.hit70,a.hit72,a.gate82,a.gate83,
                (unsigned)a.script34,(unsigned)a.script38,
                a.nativeD6,a.controller256,a.controller259,a.step08,
                a.timer4E,a.entryA4);
            prior[i]=a;priorValid[i]=true;
        }
        ++frames;
        if((frames%60u)==0u){std::fflush(trace);if(oam)std::fflush(oam);}
    }
    // Raw OAM source, called from real native object pass, before any DF
    // palette filtering. Bounded to 90 frames AFTER an observed identity swap.
    void oamFrame(unsigned frameId,const std::uint16_t* p,unsigned count,
                  unsigned passFrom,unsigned passTo,bool specialCapture) {
        if(!active||!oam||!p||specialCapture||frameId> tagUntil ||
           !tagUntil||count>0x400u)return;
        // One priority-6/7 sampling pass per native frame. A repeated
        // stage extraction must never be mistaken for fresh game objects.
        if(!(passFrom<=6u && passTo>=6u) || frameId==lastOamFrame)return;
        lastOamFrame=frameId;
        for(unsigned i=0;i<count;++i){
            const unsigned xw=(p[i*4+0]>>8)|(p[i*4+0]<<8 & 0xFF00u);
            const unsigned yw=(p[i*4+1]>>8)|(p[i*4+1]<<8 & 0xFF00u);
            const unsigned nw=(p[i*4+2]>>8)|(p[i*4+2]<<8 & 0xFF00u);
            const unsigned a=(p[i*4+3]>>8)|(p[i*4+3]<<8 & 0xFF00u);
            const unsigned pri=xw>>13u;
            if(pri<5u || pri>7u)continue;
            const unsigned tile=nw|((yw&0x6000u)<<3u);
            std::fprintf(oam,"%u\t%u\t%u\t%u\t%06X\t%u\t%u\t%04X\t%04X\t%04X\t%04X\t%04X\t%u\t%u\n",
                frameId,i,pri,a&31u,tile,((a>>8u)&15u)+1u,
                ((a>>12u)&15u)+1u,xw,yw,a,xw,yw,passFrom,passTo);
            if(++oamCount>=20000u){std::fflush(oam);return;}
        }
        if((frameId%30u)==0u)std::fflush(oam);
    }
};
inline State& state(){static State p;return p;}
} // namespace XM87139Probe
#endif
