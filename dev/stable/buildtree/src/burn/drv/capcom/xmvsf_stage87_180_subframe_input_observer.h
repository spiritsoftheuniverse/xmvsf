#ifndef XMVSF_STAGE87_180_SUBFRAME_INPUT_OBSERVER_H
#define XMVSF_STAGE87_180_SUBFRAME_INPUT_OBSERVER_H
// Stage 87.180 read-only 68000 intra-frame actor-input observer.
// The 87.179 capture proves post-frame CA varies for ostensibly identical
// host 2P priming attempts; log the intermediate original-ROM updates BEFORE
// adjusting signal timing again. No ROM/actor RAM/SDL input writes.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87180Subframe {
struct State {
    bool initialized=false,enabled=false;
    std::FILE* log=nullptr;
    unsigned lastFrame=~0u;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* path=std::getenv("XM87180_NATIVE_SUBFRAME_LOG");
        if(!path || !*path)return;
        log=std::fopen(path,"ab"); // never truncate old F9 or observer data
        if(!log){
            std::fprintf(stderr,"XM87.180 subframe log open failed: %s\n",path);
            return;
        }
        enabled=true;
        std::fputs("# XM87.180 SUBFRAME SESSION: read-only, "
                   "native original RAM sampled between SekRun stages\n",log);
        std::fputs("frame\tcycles\tstage\tdhc_phase\tpoint\treserve"
                   "\taction\tmodeA0\tD6\tctrl256\tctrl259"
                   "\tC0\tC2\tC4\tC6\tC8\tCA"
                   "\tCpi01A\tCpi01C\tCpi01E"
                   "\tinjected\tinjectedDir\tinjectedButtons\n",log);
        std::fflush(log);
    }
    void sample(const std::uint8_t* ram,unsigned frame,unsigned cycles,
                const char*stage,unsigned phase,bool candidate,
                bool synthetic,unsigned direction,unsigned buttons,
                unsigned portA,unsigned portC,unsigned portE){
        init();
        if(!enabled || !ram)return;
        if(XM87142::ramB(ram,0x5000u)!=8u)return;
        const unsigned point=XM87142::ramW(ram,0x4052u);
        const unsigned reserve=XM87142::ramW(ram,0x4852u);
        // Current runtime experiment is strictly P1 Ryu->Ken.
        if(point!=0x12u || reserve!=0x14u)return;
        const unsigned action=XM87142::ramW(ram,0x4060u);
        if(phase==0u && !(candidate && action==0x2E08u))return;
        std::fprintf(log,
            "%u\t%u\t%s\t%u\t%04X\t%04X"
            "\t%04X\t%04X\t%02X\t%02X\t%02X"
            "\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X"
            "\t%02X\t%02X\t%02X\t%u\t%02X\t%02X\n",
            frame,cycles,stage?stage:"UNKNOWN",phase,point,reserve,
            action,XM87142::ramW(ram,0x40A0u),
            XM87142::ramB(ram,0x40D6u),
            XM87142::ramB(ram,0x4256u),
            XM87142::ramB(ram,0x4259u),
            XM87142::ramW(ram,0x40C0u),
            XM87142::ramW(ram,0x40C2u),
            XM87142::ramW(ram,0x40C4u),
            XM87142::ramW(ram,0x40C6u),
            XM87142::ramW(ram,0x40C8u),
            XM87142::ramW(ram,0x40CAu),
            portA,portC,portE,synthetic?1u:0u,direction,buttons);
        // F9/full-state logs need to survive a crash between phases.
        if(lastFrame!=frame){lastFrame=frame;std::fflush(log);}
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87180Subframe
#endif
