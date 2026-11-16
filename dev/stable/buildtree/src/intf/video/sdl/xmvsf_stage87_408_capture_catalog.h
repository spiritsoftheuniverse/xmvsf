#ifndef XMVSF_STAGE87_408_CAPTURE_CATALOG_H
#define XMVSF_STAGE87_408_CAPTURE_CATALOG_H
// Append-only visual stage catalog: screenshot -> actual CPS2 RAM address
// observations. No ROM reads/writes, stage forcing or tile-map guessing here.
// Native stages remain labeled UNNAMED until the user identifies the image.
#include <cstdio>
#include <cstring>
#include <string>
namespace XM87408Catalog {
struct Snapshot {
    unsigned selector=0xffu;          // live FF5082 value from last emulated frame
    unsigned firstSelector=0xffu;     // first observed non-FF FF5082 value
    unsigned nativeStage=0u;         // live FF5113 from last emulated frame
    unsigned status=0xffu;           // live FF5000 game phase
    unsigned generation=0u;          // host stage-roll generation
    int choice=0;                    // host choice; 0 Random, 1..7 custom
    int resolved=0;                  // 0 native, 1..7 custom art
    bool observed=false;
};
inline const char* customName(int id){
    switch(id){
    case 1:return "Wily Factory";
    case 2:return "Final Destination";
    case 3:return "Guile Stage";
    case 4:return "Sigma Stage";
    case 5:return "Akuma Stage";
    case 6:return "Ryu Stage";
    case 7:return "Boomer Kuwanger";
    default:return "UNNAMED";
    }
}
inline bool append(const char*png,unsigned gameFrame,const Snapshot& s){
    if(!png || !*png)return false;
    const std::string path(png);
    const std::string::size_type slash=path.find_last_of("/\\");
    if(slash==std::string::npos){
        std::fprintf(stderr,"XM87.408 STAGE_CATALOG_ERROR screenshot has no directory: %s\n",png);
        return false;
    }
    const std::string catalog=path.substr(0,slash+1)+"stage_catalog.tsv";
    bool needsHeader=true;
    if(std::FILE* existing=std::fopen(catalog.c_str(),"rb")){
        needsHeader=std::fseek(existing,0,SEEK_END)!=0 ||
                    std::ftell(existing)==0;
        std::fclose(existing);
    }
    std::FILE* out=std::fopen(catalog.c_str(),"ab");
    if(!out){
        std::fprintf(stderr,"XM87.408 STAGE_CATALOG_ERROR cannot append: %s\n",catalog.c_str());
        return false;
    }
    bool ok=true;
    if(needsHeader){
        ok=std::fputs(
          "game_frame\tscreenshot\tselector_address\tselector_at_capture_hex"
          "\tfirst_non_ff_selector_hex\tstage_address\tnative_stage_hex"
          "\tgame_status_hex\thost_choice\thost_resolved"
          "\tstage_type\tstage_name\tstage_generation\tverified_combat\n",out)>=0;
    }
    // The native stage NAME is deliberately not inferred from an ID:
    // screenshot review provides the name, native stage ID provides identity.
    const bool combat=s.observed && s.status==8u && s.nativeStage!=0u;
    const char* type=s.resolved>0?"CUSTOM":"NATIVE";
    const char* name=s.resolved>0?customName(s.resolved):"UNNAMED";
    if(ok){
        const int count=std::fprintf(out,
            "%u\t%s\tFF5082\t%02X\t%02X\tFF5113\t%02X\t%02X"
            "\t%d\t%d\t%s\t%s\t%u\t%d\n",
            gameFrame,png,s.selector&255u,s.firstSelector&255u,
            s.nativeStage&255u,s.status&255u,s.choice,s.resolved,
            type,name,s.generation,int(combat));
        ok=count>0;
    }
    if(std::fflush(out)!=0)ok=false;
    if(std::fclose(out)!=0)ok=false;
    std::fprintf(stderr,
        "XM87.408 STAGE_CATALOG_%s path=%s screenshot=%s "
        "FF5082=%02X first_selector=%02X FF5113=%02X status=%02X "
        "custom=%d verified_combat=%d\n",
        ok?"APPENDED":"FAILED",catalog.c_str(),png,s.selector&255u,
        s.firstSelector&255u,s.nativeStage&255u,s.status&255u,
        s.resolved,int(combat));
    return ok;
}
}
#endif
