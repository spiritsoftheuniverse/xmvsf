#ifndef XMVSF_STAGE87_149_TOURNAMENT_RULES_AUDIT_H
#define XMVSF_STAGE87_149_TOURNAMENT_RULES_AUDIT_H
// 87.262: read-only truth table for native V4 patch installation and the
// independent Tournament knockdown switch. No gameplay writes or rule gates
// are created here. All actual decisions remain in cps_run.cpp.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xmvsf_stage87_262_game_version.h"
namespace XM87149Rules {
static inline bool flag(const char*key){
    const char*v=std::getenv(key);
    return v && std::strcmp(v,"1")==0;
}
struct Status {
    bool tournament=false,launchFlight=false,hit95=false,hand96=false,ground99=false;
    bool installed95=false,installed96=false,installed99=false;
    unsigned mask()const{
        return (unsigned(tournament)<<0u)
             | (unsigned(launchFlight)<<1u)
             | (unsigned(hit95)<<2u)
             | (unsigned(hand96)<<3u)
             | (unsigned(ground99)<<4u)
             | (unsigned(installed95)<<5u)
             | (unsigned(installed96)<<6u)
             | (unsigned(installed99)<<7u);
    }
    bool ready()const{
        return !tournament ||
            (launchFlight&&hit95&&hand96&&ground99&&
             installed95&&installed96&&installed99);
    }
};
struct Audit{
    bool initialized=false,haveLast=false;
    unsigned lastMask=0u,changes=0u;
    std::FILE* log=nullptr;
    ~Audit(){if(log)std::fclose(log);}
    void initialize(){
        if(initialized)return;
        initialized=true;
        const char*path=std::getenv("XM87149_TOURNAMENT_AUDIT");
        if(path&&*path){
            log=std::fopen(path,"wb");
            if(log)std::fputs(
                "frame\tevent\tversion4\ttournamentKnockdowns"
                "\tdizzy95Flag\thandoff96Flag\tground99Flag"
                "\tdizzy95Installed\thandoff96Installed\tground99Installed"
                "\tv4NativeReady\n",log);
        }
    }
    void frame(unsigned now,const char*driver,bool selected,
               bool native95,bool native96,bool native99){
        initialize();
        if(!driver||std::strcmp(driver,"xmvsfu")!=0)return;
        Status s;
        // In this legacy audit signature, selected now means Version 4.
        // Historical flags are not prerequisites for the official rules.
        const bool knockdowns=XM87262Game::tournamentKnockdowns();
        s.tournament=knockdowns;
        s.launchFlight=knockdowns;
        s.hit95=selected;
        s.hand96=selected;
        s.ground99=selected;
        s.installed95=native95;s.installed96=native96;s.installed99=native99;
        const unsigned next=s.mask();
        if(haveLast&&lastMask==next)return;
        haveLast=true;lastMask=next;++changes;
        const bool nativeReady=!selected ||
            (native95 && native96 && native99);
        const char*name=!selected?
            (knockdowns?"VERSION3_TOURNAMENT_KNOCKDOWNS":"VERSION3_ORIGINAL"):
            (!nativeReady?"VERSION4_NATIVE_PATCH_INCOMPLETE":
             knockdowns?"VERSION4_TOURNAMENT_KNOCKDOWNS":
                       "VERSION4_PATCHED");
        if(log){
            std::fprintf(log,"%u\t%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
                now,name,unsigned(selected),unsigned(knockdowns),
                unsigned(s.hit95),unsigned(s.hand96),unsigned(s.ground99),
                unsigned(native95),unsigned(native96),unsigned(native99),
                unsigned(nativeReady));
            std::fflush(log);
        }
        std::fprintf(stderr,
            "XM87.262 RULE_AUDIT %s knockdowns=%u dizzy95=%u handoff96=%u ground99=%u\n",
            name,unsigned(knockdowns),
            unsigned(native95),unsigned(native96),unsigned(native99));
    }
};
static inline Audit&state(){static Audit audit;return audit;}
}
#endif
