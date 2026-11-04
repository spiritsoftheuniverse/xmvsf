#ifndef XMVSF_STAGE87_149_TOURNAMENT_RULES_AUDIT_H
#define XMVSF_STAGE87_149_TOURNAMENT_RULES_AUDIT_H
// 87.149: READ-ONLY runtime truth table for the native Classic/Tournament
// rule hooks. This does not enable patches or change gameplay in either mode.
// The per-frame CPC-2 guard logic in cps_run.cpp is authoritative.
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
                "frame\tevent\tselectedTournament\tstormFlightFlag"
                "\thit95Flag\thandoff96Flag\tground99Flag"
                "\thit95Installed\thandoff96Installed\tground99Installed"
                "\tallTournamentRulesReady\n",log);
        }
    }
    void frame(unsigned now,const char*driver,bool selected,
               bool native95,bool native96,bool native99){
        initialize();
        if(!driver||std::strcmp(driver,"xmvsfu")!=0)return;
        Status s;
        s.tournament=selected;
        s.launchFlight=flag("XM8786_STORM_BUTTON_TEST");
        s.hit95=flag("XM8795_NATIVE_HIT_TEST");
        s.hand96=flag("XM8796_DIZZY_HANDOFF_TEST");
        s.ground99=flag("XM8799_GROUND_DEFENSE_TEST");
        s.installed95=native95;s.installed96=native96;s.installed99=native99;
        const unsigned next=s.mask();
        if(haveLast&&lastMask==next)return;
        haveLast=true;lastMask=next;++changes;
        const char*name=!selected?"CLASSIC_NATIVE_RULES":
                          s.ready()?"TOURNAMENT_RULES_ACTIVE":
                                    "TOURNAMENT_RULES_INCOMPLETE";
        if(log){
            std::fprintf(log,"%u\t%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
                now,name,unsigned(selected),unsigned(s.launchFlight),
                unsigned(s.hit95),unsigned(s.hand96),unsigned(s.ground99),
                unsigned(native95),unsigned(native96),unsigned(native99),
                unsigned(s.ready()));
            std::fflush(log);
        }
        std::fprintf(stderr,
            "XM87.149 RULE_AUDIT %s flight=%u native95=%u native96=%u native99=%u\n",
            name,unsigned(selected&&s.launchFlight),
            unsigned(selected&&native95),unsigned(selected&&native96),
            unsigned(selected&&native99));
    }
};
static inline Audit&state(){static Audit audit;return audit;}
}
#endif
