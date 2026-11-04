#ifndef XMVSF_STAGE87_09_FRAGMENT_TRACE_H
#define XMVSF_STAGE87_09_FRAGMENT_TRACE_H
// XM87.09 TEMPEST_FRAGMENT_TRACE: diagnostic only. Do not alter palette selection.
// CPS2 sprite list slots are not permanent fighter identities. Record their
// contiguous same-priority/palette runs, even when much smaller than Stage87.08.
// Groups are not automatically considered effects; this is discovery data.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <unordered_set>
namespace XM8709 {
static inline unsigned W(const UINT16* words,unsigned i){return unsigned(BURN_ENDIAN_SWAP_INT16(words[i]));}
static inline int S10(unsigned u){u &= 1023u;return (u&512u)?int(u)-1024:int(u);}
static inline int Edge(int point,int low,int high){return point<low?low-point:(point>high?point-high:0);}
struct Capture {
    FILE* fp=nullptr;
    unsigned lastFrame=~0u;
    const UINT16* lastWords=nullptr;
    unsigned long long lines=0;
    Capture(){
        const char* path=std::getenv("XM8709_FRAGMENT_TRACE_FILE");
        if(!path||!*path)return;
        fp=std::fopen(path,"wb");
        if(!fp){std::fprintf(stderr,"XM87.09 fragment trace cannot_open %s\n",path);return;}
        std::fputs("frame\tpriority\tpalette\tslot_begin\tslot_end\tobjects\tunique_tiles\txmin\txmax\tymin\tymax\tnearest_actor\tmin_actor_edge_distance\tstage8708_eligible\n",fp);
        std::fprintf(stderr,"XM87.09 TEMPEST_FRAGMENT_TRACE path=%s\n",path);
    }
    ~Capture(){if(fp)std::fclose(fp);}
    void See(const UINT16* words,int n,unsigned frame,int shiftX,int shiftY){
        if(!fp||!words||n<=0||n>4096)return;
        if(frame==lastFrame && words==lastWords)return; // several rendering passes / frame
        if(frame<lastFrame)std::fflush(fp);
        lastFrame=frame;lastWords=words;
        const XM8707::Frame actors=XM8707::Snapshot();
        if(!actors.valid)return;
        for(unsigned begin=0;begin<(unsigned)n;){
            const unsigned pri=W(words,begin*4u)>>13u;
            const unsigned pal=W(words,begin*4u+3u)&31u;
            unsigned end=begin+1u;
            for(;end<(unsigned)n;++end){
                if((W(words,end*4u)>>13u)!=pri||(W(words,end*4u+3u)&31u)!=pal)break;
            }
            const unsigned count=end-begin;
            if((pri==6u || pri==7u) && pal>=6u && pal<=23u && count>=4u){
                int xmin=INT_MAX,xmax=INT_MIN,ymin=INT_MAX,ymax=INT_MIN;
                std::unordered_set<unsigned> unique;
                for(unsigned i=begin;i<end;++i){
                    const unsigned xRaw=W(words,i*4u),yRaw=W(words,i*4u+1u);
                    const unsigned attr=W(words,i*4u+3u);
                    const int x=S10(xRaw)+shiftX, y=S10(yRaw)+shiftY;
                    const int right=x+16*int(((attr>>8u)&15u)+1u);
                    const int bottom=y+16*int(((attr>>12u)&15u)+1u);
                    if(x<xmin)xmin=x;
                    if(y<ymin)ymin=y;
                    if(right>xmax)xmax=right;
                    if(bottom>ymax)ymax=bottom;
                    unique.insert(W(words,i*4u+2u)|((yRaw&0x6000u)<<3u));
                }
                int nearest=-1,minD=INT_MAX;
                for(unsigned j=0;j<4u;++j){
                    const XM8707::Actor& a=actors.actor[j];
                    if(!a.active)continue;
                    const int d=Edge(a.screenX,xmin,xmax)+Edge(a.screenY,ymin,ymax);
                    if(d<minD){minD=d;nearest=int(j);}
                }
                const bool eligible=pri==6u&&pal>=12u&&count>=64u&&(ymax-ymin)>=220;
                std::fprintf(fp,"%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                    frame,pri,pal,begin,end-1u,count,unsigned(unique.size()),xmin,xmax,ymin,ymax,
                    nearest,nearest<0?-1:minD,eligible?1:0);
                if((++lines%2048ull)==0ull)std::fflush(fp);
            }
            begin=end;
        }
    }
};
static inline void Record(const UINT16* words,int n,unsigned frame,int shiftX,int shiftY){
    static Capture cap;cap.See(words,n,frame,shiftX,shiftY);
}
} // namespace XM8709
#endif
