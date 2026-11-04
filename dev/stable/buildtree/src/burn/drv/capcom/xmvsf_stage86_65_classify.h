#ifndef XMVSF_STAGE86_65_CLASSIFY_H
#define XMVSF_STAGE86_65_CLASSIFY_H
// Stage86.65: data-driven exact scenery families and conservative pre-fight
// logo wall rule.  No changes to the locked Stage86.54 Wily object filter.
#include <cstdio>
#include <cstdlib>
struct XM8665SceneryRule {unsigned priority,palette,family;};
static inline bool xm8665ExtraScenery(unsigned priority,unsigned palette,unsigned tile){
    struct Config {
        XM8665SceneryRule entries[256]{};
        unsigned count=0;
        Config(){
            const char* path=std::getenv("XM8665_SCENERY_RULES_FILE");
            if(!path || !*path)return;
            FILE* file=std::fopen(path,"rb");
            if(!file){std::fprintf(stderr,"XM86.65 SCENERY_RULES cannot_open=%s\n",path);return;}
            char line[192];
            while(std::fgets(line,sizeof(line),file)){
                if(line[0]=='#'||line[0]==';'||line[0]=='\n')continue;
                unsigned p=0,k=0,f=0;char end=0;
                if(std::sscanf(line," %u , %u , %u %c",&p,&k,&f,&end)!=3)continue;
                if(p<=7u && k<32u && f<16384u && count<256u)
                    entries[count++]={p,k,f};
            }
            std::fclose(file);
            std::fprintf(stderr,"XM86.65 SCENERY_RULES exact_family_count=%u\n",count);
        }
    };
    static const Config config{};
    for(unsigned i=0;i<config.count;++i){
        const auto& r=config.entries[i];
        if(r.priority==priority && r.palette==palette && r.family==tile/256u)return true;
    }
    return false;
}
// During the X-Men logo crush, upper-priority, multi-cell backdrop objects
// are not HUD just because they use priority 7. Restrict to pre-fight phases;
// HUD numeral family is separately protected in the caller.
static inline bool xm8665IntroWall(unsigned status,unsigned priority,
                                    unsigned width,unsigned height){
    return status>=2u && status<=6u && priority==7u &&
           (width>=5u || height>=5u);
}
#endif
