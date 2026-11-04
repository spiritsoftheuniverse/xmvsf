#ifndef XMVSF_STAGE87_55_SUPER_PROBE_H
#define XMVSF_STAGE87_55_SUPER_PROBE_H
// ROM super-combo investigation: no writable game RAM, no game timing changes.
// ROM owns execution (Cps2Frame->SekRun); action +A0 is NOT known to mean
// super without an actual labeled capture. +AA is SUPERJUMP, not SUPER.
// Enable with XM8755_SUPER_AUDIT_FILE (TSV), optionally XM8755_SUPER_RAM_FILE.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM8755 {
struct Probe {
    FILE *tsv=nullptr,*binary=nullptr;
    bool ready[2]={false,false};
    unsigned previousAction[2]={0,0};
    unsigned lastFrame=~0u, frames=0u;
    bool initialized=false;
    void Init(){
        if(initialized)return;
        initialized=true;
        const char* path=std::getenv("XM8755_SUPER_AUDIT_FILE");
        if(!path || !path[0])return;
        tsv=std::fopen(path,"wb");
        if(!tsv){std::fprintf(stderr,"XM87.55 SUPER_PROBE_OPEN_FAILED path=%s\n",path);return;}
        std::fputs("frame\tphase\tplayer\ttype\taction_word\taction_byte\tprevious_action_byte\tanimation_word\tjump_mode_AA\tworld_x\tworld_y\tA0_AF_cpu_bytes\n",tsv);
        const char* binPath=std::getenv("XM8755_SUPER_RAM_FILE");
        if(binPath && *binPath){
            binary=std::fopen(binPath,"wb");
            if(binary)std::fwrite("XM8755R1",1,8,binary);
        }
        std::fprintf(stderr,"XM87.55 SUPER_PROBE_READY tsv=%s binary=%d limit=18000_frames\n",path,int(binary!=nullptr));
    }
    ~Probe(){if(tsv)std::fclose(tsv);if(binary)std::fclose(binary);}
    static unsigned Byte(const unsigned char* raw,unsigned offset){return raw[offset^1u];}
    static unsigned Word(const unsigned char* raw,unsigned offset){return (Byte(raw,offset)<<8)|Byte(raw,offset+1u);}
    void Observe(const unsigned char* ram,unsigned frame,int phase,bool firstPass){
        if(!firstPass || !ram || frame==lastFrame)return;
        lastFrame=frame;
        Init();
        if(!tsv || frames>=18000u)return;
        if(phase!=8){ready[0]=ready[1]=false;return;}
        ++frames;
        if(binary){
            const std::uint32_t hdr[2]={std::uint32_t(frame),std::uint32_t(phase)};
            std::fwrite(hdr,sizeof(std::uint32_t),2,binary);
            std::fwrite(ram+0x4000u,1,0x800u,binary); // raw host word-swapped P1/P2 data
        }
        for(unsigned p=0;p<2u;++p){
            const unsigned base=0x4000u+p*0x400u;
            if(!ram[base]){ready[p]=false;continue;}
            const unsigned action=Byte(ram,base+0xA0u);
            const unsigned prev=ready[p]?previousAction[p]:action;
            const char* type=!ready[p]?"FIRST":(action!=prev?"ACTION_CHANGE":"FRAME");
            char hex[33];
            static const char dig[]="0123456789ABCDEF";
            for(unsigned k=0;k<16u;++k){
                const unsigned b=Byte(ram,base+0xA0u+k);
                hex[2*k]=dig[b>>4];hex[2*k+1]=dig[b&15u];
            }
            hex[32]=0;
            std::fprintf(tsv,"%u\t%d\t%u\t%s\t%04X\t%02X\t%02X\t%04X\t%02X\t%u\t%u\t%s\n",
                frame,phase,p+1u,type,Word(ram,base+0xA0u),action,prev,
                Word(ram,base+0x06u),Byte(ram,base+0xAAu),
                Word(ram,base+0x0Cu),Word(ram,base+0x10u),hex);
            if(action!=prev){
                std::fprintf(stderr,"XM87.55 ROM_ACTION frame=%u p=%u from=%02X to=%02X anim=%04X AA=%02X (not super-labeled)\n",
                    frame,p+1u,prev,action,Word(ram,base+0x06u),Byte(ram,base+0xAAu));
            }
            previousAction[p]=action;ready[p]=true;
        }
        if(frames%120u==0u){std::fflush(tsv);if(binary)std::fflush(binary);}
        if(frames==18000u)std::fprintf(stderr,"XM87.55 SUPER_PROBE_LIMIT reached\n");
    }
};
static inline Probe& State(){static Probe p;return p;}
static inline void Observe(const unsigned char* ram,unsigned frame,int phase,bool firstPass){
    State().Observe(ram,frame,phase,firstPass);
}
}
#endif
