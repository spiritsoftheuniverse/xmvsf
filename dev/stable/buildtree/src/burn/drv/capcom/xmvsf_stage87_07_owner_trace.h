#ifndef XMVSF_STAGE87_07_OWNER_TRACE_H
#define XMVSF_STAGE87_07_OWNER_TRACE_H
// XM87.07 OWNER_RAM_SPRITE_TRACE: diagnostic only, never chooses a palette.
// The 68k actor bases/camera profile were already used in the project's
// stage23 hitbox viewer (USA xmvsfu). This traces those RAM actors alongside
// each buffered CPS2 sprite entry. 'near' and 'candidate_slot' are geometric
// hypotheses ONLY, NOT proof that the actor owns that sprite.
// CPS2 OBJ entries have no persistent fighter-owner ID.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace XM8707 {
static inline unsigned R8(unsigned addr){
    return CpsRamFF && addr>=0xFF0000u && addr<=0xFFFFFFu ?
        (unsigned)CpsRamFF[(addr&0xffffu)^1u] : 0u;
}
static inline unsigned R16(unsigned addr){return (R8(addr)<<8)|R8(addr+1u);}
static inline unsigned R32(unsigned addr){return (R16(addr)<<16)|R16(addr+2u);}
static inline int S16(unsigned u){return (u&0x8000u)?(int)u-65536:(int)u;}
static inline int Abs(int v){return v<0?-v:v;}
static inline int EdgeDistance(int position,int start,int end){
    return position<start?start-position:(position>end?position-end:0);
}
struct Actor {
    unsigned base=0;
    unsigned active=0;
    unsigned anim=0;
    unsigned flags=0;
    int worldX=0,worldY=0,screenX=0,screenY=0;
};
struct Frame {
    unsigned status=0,profile=0,stage=0;
    int cameraLeft=0,cameraTop=0;
    bool valid=false;
    Actor actor[4];
};
static inline Frame Snapshot(){
    Frame out;
    if(!CpsRamFF)return out;
    out.status=R8(0xFF5000u);
    out.profile=R32(0xFFA014u);
    out.stage=R8(0xFF5113u)&15u;
    if(out.status!=8u || out.profile!=0x00FF4000u)return out;
    // Same stage-specific camera base offsets as xmvsf_stage23_engine.h.
    static const int offsets[16]={-0x2cc0,-0x2dc0,-0x2dc0,-0x2d40,
        -0x2dc0,-0x2dc0,-0x2cc0,-0x2dc0,-0x2d40,-0x2d40,
        -0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0,-0x2dc0};
    const unsigned camera=(unsigned)(0xFF8000+offsets[out.stage]);
    out.cameraLeft=S16(R16(camera+0x0cu))+0x40;
    out.cameraTop=S16(R16(camera+0x10u));
    for(unsigned i=0;i<4u;++i){
        Actor& a=out.actor[i];
        a.base=0xFF4000u+i*0x400u;
        a.active=R8(a.base);        // RAM activity hint, not confirmed on-screen
        a.anim=R16(a.base+0x06u);
        a.flags=R8(a.base+0x4bu);
        a.worldX=S16(R16(a.base+0x0cu));
        a.worldY=S16(R16(a.base+0x10u));
        a.screenX=a.worldX-out.cameraLeft;
        a.screenY=a.worldY-out.cameraTop-15;
    }
    out.valid=true;
    return out;
}
struct Recorder {
    FILE* objects=nullptr;
    FILE* actors=nullptr;
    unsigned lastFrame=0;
    unsigned session=0;
    unsigned capturedFrames=0;
    unsigned long long records=0;
    bool initialized=false,shouldCapture=false;
    Frame state;
    unsigned sampleStride=3;
    Recorder(){
        const char* objPath=std::getenv("XM8707_OBJECT_TRACE_FILE");
        const char* actPath=std::getenv("XM8707_ACTOR_TRACE_FILE");
        if(!objPath||!*objPath||!actPath||!*actPath)return;
        const char* stride=std::getenv("XM8707_SAMPLE_STRIDE");
        if(stride&&*stride){
            const int n=std::atoi(stride);
            if(n>=1&&n<=60)sampleStride=(unsigned)n;
        }
        objects=std::fopen(objPath,"wb");
        actors=std::fopen(actPath,"wb");
        if(!objects || !actors){
            if(objects)std::fclose(objects);
            if(actors)std::fclose(actors);
            objects=nullptr;actors=nullptr;
            std::fprintf(stderr,"XM87.07 OWNER_RAM_SPRITE_TRACE cannot_open object=%s actors=%s\n",objPath,actPath);
            return;
        }
        std::fprintf(objects,"session\tframe\tobj_slot\tz_order\tpass_from\tpass_to\tpriority\tpalette\ttile\tattribute_hex\twidth\theight\tx\ty\tgrade\tcandidate_slot\tdistance_px\tnear_actor\n");
        std::fprintf(actors,"session\tframe\tstatus\tprofile\tstage\tcamera_left\tcamera_top\tslot\tbase\tactivity\tanim\tflags\tworld_x\tworld_y\tscreen_x\tscreen_y\n");
        std::fprintf(stderr,"XM87.07 OWNER_RAM_SPRITE_TRACE objects=%s actors=%s stride=%u\n",objPath,actPath,sampleStride);
    }
    ~Recorder(){if(objects)std::fclose(objects);if(actors)std::fclose(actors);}
    void StartFrame(unsigned frame){
        if(!objects || !actors)return;
        if(initialized && frame==lastFrame)return;
        if(initialized && frame<lastFrame){++session;capturedFrames=0;}
        initialized=true;
        lastFrame=frame;
        state=Snapshot();
        shouldCapture=false;
        if(!state.valid)return;
        ++capturedFrames;
        // Preserve complete opening body/entry sequence, then sample 1/N.
        shouldCapture=capturedFrames<=180u ||
            (capturedFrames%sampleStride)==0u;
        if(!shouldCapture)return;
        for(unsigned i=0;i<4u;++i){
            const Actor& a=state.actor[i];
            std::fprintf(actors,"%u\t%u\t%u\t%08X\t%u\t%d\t%d\t%u\t%06X\t%u\t%u\t%u\t%d\t%d\t%d\t%d\n",
                session,frame,state.status,state.profile,state.stage,
                state.cameraLeft,state.cameraTop,i,a.base,a.active,a.anim,a.flags,
                a.worldX,a.worldY,a.screenX,a.screenY);
        }
        if((capturedFrames%256u)==0u)std::fflush(actors);
    }
    void Object(unsigned frame,unsigned objectSlot,unsigned zValue,int passFrom,int passTo,
                unsigned priority,unsigned palette,unsigned tile,unsigned rawAttr,
                unsigned width,unsigned height,int x,int y,const char* grade){
        if(!objects)return;
        StartFrame(frame);
        if(!shouldCapture || (priority!=5u&&priority!=6u&&priority!=7u))return;
        // Bounding-box proximity is *not* ownership. Projectile and fighter
        // can occupy the same rectangle. Keep the raw data for later analysis.
        const int ex=x+int(width*16u),ey=y+int(height*16u);
        int nearest=-1,distance=std::numeric_limits<int>::max();
        for(unsigned i=0;i<4u;++i){
            const Actor& a=state.actor[i];
            if(!a.active)continue;
            const int d=EdgeDistance(a.screenX,x,ex)+EdgeDistance(a.screenY,y,ey);
            if(d<distance){nearest=int(i);distance=d;}
        }
        const bool near=nearest>=0 && distance<=32;
        std::fprintf(objects,"%u\t%u\t%u\t%u\t%d\t%d\t%u\t%u\t%u\t%04X\t%u\t%u\t%d\t%d\t%s\t%d\t%d\t%d\n",
            session,frame,objectSlot,zValue,passFrom,passTo,priority,palette,tile,rawAttr,
            width,height,x,y,grade?grade:"",nearest,
            nearest>=0?distance:-1,int(near));
        if((++records%8192ull)==0ull)std::fflush(objects);
    }
};
static inline void TraceObject(unsigned frame,unsigned objectSlot,unsigned zValue,int passFrom,int passTo,
        unsigned priority,unsigned palette,unsigned tile,unsigned rawAttr,unsigned width,unsigned height,
        int x,int y,const char* grade){
    static Recorder rec;
    rec.Object(frame,objectSlot,zValue,passFrom,passTo,priority,palette,tile,rawAttr,width,height,x,y,grade);
}
} // namespace XM8707
#endif
