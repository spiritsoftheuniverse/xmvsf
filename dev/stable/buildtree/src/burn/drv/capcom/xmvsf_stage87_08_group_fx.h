#ifndef XMVSF_STAGE87_08_GROUP_FX_H
#define XMVSF_STAGE87_08_GROUP_FX_H
// XM87.08 GROUP_SUPER_FX: cautious sprite-run detection derived from the
// Stage87.07 RAM/object trace. Not a persistent character identity system.
// A long same-priority, same-palette OAM run that stretches >=220px vertically
// is not an ordinary fighter-body cluster in the verified Stage87.07 capture.
// Can be disabled with XM8708_GROUP_SUPERS=0 for A/B rendering comparison.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
namespace XM8708 {
struct Cache {
    const UINT16* ptr=nullptr;
    unsigned frame=~0u;
    unsigned count=0u;
    int yShift=0;
    std::vector<unsigned char> super;
    static unsigned Read(const UINT16* words,unsigned index){
        return (unsigned)BURN_ENDIAN_SWAP_INT16(words[index]);
    }
    static int Signed10(unsigned x){
        x &= 0x3ffu;
        return (x&0x200u)?int(x)-1024:int(x);
    }
    void Rebuild(const UINT16* words,unsigned n,unsigned atFrame,int shift){
        ptr=words;frame=atFrame;count=n;yShift=shift;
        super.assign(n,0u);
        for(unsigned begin=0u;begin<n;){
            const unsigned p=Read(words,begin*4u)>>13u;
            const unsigned a=Read(words,begin*4u+3u);
            const unsigned pal=a&31u;
            unsigned end=begin+1u;
            for(;end<n;++end){
                if((Read(words,end*4u)>>13u)!=p ||
                    ((Read(words,end*4u+3u)&31u)!=pal))break;
            }
            // The captured ordinary fighter clusters never reached 64
            // contiguous objects; valid large super groups were palette
            // 13, 14 and 17, all inside this broader safe FX bank interval.
            if(p==6u && pal>=12u && pal<=23u && end-begin>=64u){
                int minY=32767,maxY=-32768;
                for(unsigned i=begin;i<end;++i){
                    const unsigned raw=Read(words,i*4u+1u);
                    const unsigned attr=Read(words,i*4u+3u);
                    const int y=Signed10(raw)+shift;
                    const int bottom=y+16*(int)(((attr>>12u)&15u)+1u);
                    if(y<minY)minY=y;
                    if(bottom>maxY)maxY=bottom;
                }
                if(maxY-minY>=220){
                    for(unsigned i=begin;i<end;++i)super[i]=1u;
                }
            }
            begin=end;
        }
    }
    bool Candidate(const UINT16* words,int n,unsigned atFrame,int shift,unsigned slot){
        if(!words || n<=0 || n>4096 || slot>=(unsigned)n)return false;
        if(words!=ptr || atFrame!=frame || count!=(unsigned)n || yShift!=shift)
            Rebuild(words,(unsigned)n,atFrame,shift);
        return !!super[slot];
    }
};
static inline bool Enabled(){
    static const bool enabled=[](){
        const char* p=std::getenv("XM8708_GROUP_SUPERS");
        return !(p&&p[0]=='0'&&p[1]=='\0');
    }();
    return enabled;
}
static inline bool GroupSuper(const UINT16* words,int count,unsigned frame,int shift,unsigned slot){
    if(!Enabled())return false;
    static Cache cache;
    return cache.Candidate(words,count,frame,shift,slot);
}
} // namespace XM8708
#endif
