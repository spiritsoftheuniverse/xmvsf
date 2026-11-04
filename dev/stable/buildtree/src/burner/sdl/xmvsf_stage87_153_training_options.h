#ifndef XMVSF_STAGE87_153_TRAINING_OPTIONS_H
#define XMVSF_STAGE87_153_TRAINING_OPTIONS_H
// Host-only Training preferences. Never serialize an emulator savestate,
// fighter RAM, control binding, or session-specific input latch.
// Intentionally standalone: C++11 tests can exercise the same disk parser.
#include <cstdio>
#include <cstring>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
namespace XM87153Training {
static const char* const PATH="config/games/xmvsfu_training_options.cfg";
static const char* const MAGIC="XMVSF_TRAINING_OPTIONS_V1\n";
struct Options {
    unsigned hitboxes=0,displayData=0,infiniteMeter=0,dizzyAllowed=1;
    unsigned damageMode=0,dummy1HP=10,dummy2HP=10,playerSelect=0,frameData=0;
    bool valid() const {
        return hitboxes<=1u && displayData<=1u && infiniteMeter<=1u &&
               dizzyAllowed<=1u && damageMode<=1u &&
               dummy1HP>=1u && dummy1HP<=10u &&
               dummy2HP>=1u && dummy2HP<=10u &&
               playerSelect<=1u && frameData<=1u;
    }
};
inline bool load(const char* path,Options& dest) {
    if(!path || !*path)return false;
    std::FILE* in=std::fopen(path,"rb");
    if(!in)return false;
    char line[256]={};
    if(!std::fgets(line,sizeof(line),in) || std::strcmp(line,MAGIC)!=0){
        std::fclose(in);return false;
    }
    Options parsed;
    bool seen[9]={};
    static const char* keys[9]={"hitboxes","display_data","infinite_meter",
        "dizzy_allowed","damage_mode","dummy1_hp_tenths","dummy2_hp_tenths",
        "player_select","frame_data"};
    unsigned* values[9]={&parsed.hitboxes,&parsed.displayData,&parsed.infiniteMeter,
        &parsed.dizzyAllowed,&parsed.damageMode,&parsed.dummy1HP,&parsed.dummy2HP,
        &parsed.playerSelect,&parsed.frameData};
    while(std::fgets(line,sizeof(line),in)){
        char key[64]={},tail=0;
        unsigned n=0;
        if(std::sscanf(line,"%63[^=]=%u %c",key,&n,&tail)!=2)continue;
        for(unsigned i=0;i<9u;++i)if(std::strcmp(key,keys[i])==0){
            *values[i]=n;seen[i]=true;break;
        }
    }
    const bool ok=std::ferror(in)==0;
    std::fclose(in);
    if(!ok || !parsed.valid())return false;
    for(unsigned i=0;i<9u;++i)if(!seen[i])return false;
    dest=parsed;
    return true;
}
inline bool save(const char* path,const Options& p) {
    if(!path || !*path || !p.valid())return false;
    // Write an adjacent temporary file. A failed save never truncates the
    // previously remembered user configuration; Stable is never touched.
    const std::string temporary=std::string(path)+".pending";
    std::FILE* out=std::fopen(temporary.c_str(),"wb");
    if(!out)return false;
    const int count=std::fprintf(out,
        "XMVSF_TRAINING_OPTIONS_V1\n"
        "hitboxes=%u\n"
        "display_data=%u\n"
        "infinite_meter=%u\n"
        "dizzy_allowed=%u\n"
        "damage_mode=%u\n"
        "dummy1_hp_tenths=%u\n"
        "dummy2_hp_tenths=%u\n"
        "player_select=%u\n"
        "frame_data=%u\n",
        p.hitboxes,p.displayData,p.infiniteMeter,p.dizzyAllowed,p.damageMode,
        p.dummy1HP,p.dummy2HP,p.playerSelect,p.frameData);
    const bool wrote=count>0 && std::fflush(out)==0;
    const bool closed=std::fclose(out)==0;
    if(!wrote || !closed){std::remove(temporary.c_str());return false;}
#ifdef _WIN32
    if(!MoveFileExA(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        std::remove(temporary.c_str());return false;
    }
#else
    if(std::rename(temporary.c_str(),path)!=0){
        std::remove(temporary.c_str());return false;
    }
#endif
    return true;
}
}
#endif
