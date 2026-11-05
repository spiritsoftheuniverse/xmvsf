#ifndef XMVSF_STAGE87_181_OBSERVER_HEALTH_H
#define XMVSF_STAGE87_181_OBSERVER_HEALTH_H
// 87.181: persistent observer-health breadcrumbs. Runs at CPS2 frame entry,
// BEFORE post-CPU/ROM/DHC eligibility gates. Never mutates native RAM, ROM
// input or gameplay. The launcher has its own preflight/start/end markers.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM87181Health {
struct State {
    std::FILE* log=nullptr;
    bool initialized=false,firstCps=false,firstXmvsf=false,firstCombat=false;
    unsigned frames=0,gameFrames=0,combatFrames=0,lastHeartbeat=0;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* path=std::getenv("XM87181_DIAGNOSTIC_STATUS");
        if(!path || !*path)return;
        log=std::fopen(path,"ab");
        if(!log){
            std::fprintf(stderr,"XM87.181 health log open failed: %s\n",path);
            return;
        }
        std::fputs("ENGINE_HEALTH_INITIALIZED\t87.181\tCPS2 frame hook entered\n",log);
        std::fflush(log);
    }
    void report(const char*kind,unsigned frame,const char*driver,
                bool ramReady,bool romReady,bool combat,
                bool oldObsEnabled,bool newSubEnabled){
        if(!log)return;
        std::fprintf(log,
            "%s\tframe=%u\tdriver=%s\tCpsRamFF=%u\tCpsRom=%u"
            "\tcombat=%u\toldObserverReady=%u\tsubframeObserverReady=%u"
            "\tcpsFrames=%u\txmvsfFrames=%u\tcombatFrames=%u\n",
            kind,frame,driver?driver:"(null)",ramReady?1u:0u,
            romReady?1u:0u,combat?1u:0u,oldObsEnabled?1u:0u,
            newSubEnabled?1u:0u,frames,gameFrames,combatFrames);
        std::fflush(log);
    }
    // Call once per real CPS2 frame, including menu/intro. The two
    // observers are initialized at this point when driver is XMVSF.
    void frame(unsigned frame,const char*driver,
               const std::uint8_t*ram,const std::uint8_t*rom,
               bool oldObsEnabled,bool newSubEnabled){
        init();
        ++frames;
        const bool xmvsf=driver && std::strncmp(driver,"xmvsf",5)==0;
        const bool combat=xmvsf && ram && XM87142::ramB(ram,0x5000u)==8u;
        if(xmvsf)++gameFrames;
        if(combat)++combatFrames;
        if(!firstCps){
            firstCps=true;
            report("CPS2_FIRST_FRAME",frame,driver,ram!=nullptr,rom!=nullptr,
                   combat,oldObsEnabled,newSubEnabled);
        }
        if(xmvsf && !firstXmvsf){
            firstXmvsf=true;
            report("XMVSF_FIRST_FRAME",frame,driver,ram!=nullptr,rom!=nullptr,
                   combat,oldObsEnabled,newSubEnabled);
        }
        if(combat && !firstCombat){
            firstCombat=true;
            report("XMVSF_FIRST_COMBAT",frame,driver,ram!=nullptr,rom!=nullptr,
                   true,oldObsEnabled,newSubEnabled);
        }
        if(xmvsf && (!lastHeartbeat || frame<lastHeartbeat ||
           frame-lastHeartbeat>=600u)){
            lastHeartbeat=frame;
            report("XMVSF_HEARTBEAT",frame,driver,ram!=nullptr,rom!=nullptr,
                   combat,oldObsEnabled,newSubEnabled);
        }
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87181Health
#endif
