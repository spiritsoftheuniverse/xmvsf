#ifndef XMVSF_STAGE87_10_TEMPEST_FRAGMENT_FX_H
#define XMVSF_STAGE87_10_TEMPEST_FRAGMENT_FX_H
// XM87.10 TEMPEST_FRAGMENT_COLOR: capture-proven independent fragment groups.
// Stage87.09 frames 4983..5010: palette21 priority6, 24..26 unique
// OAM tiles in a 112+ by 112+ pixel run, with a moving bounding rectangle.
// Many later objects were mislabeled lighting_body and lost their RGB.
// This is deliberately not a palette-wide rule; fighter/lighting sprite
// groups and the Stage87.08 large-super rule are unchanged.
// Disable with XM8710_TEMPEST_FRAGMENTS=0 to compare.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unordered_set>
#include <vector>
namespace XM8710 {
static inline unsigned Read(const UINT16* words,unsigned index){
    return unsigned(BURN_ENDIAN_SWAP_INT16(words[index]));
}
static inline int Signed10(unsigned x){
    x &= 0x3ffu;
    return (x&0x200u)?int(x)-1024:int(x);
}
static inline bool Enabled(){
    static const bool on=[](){
        const char* p=std::getenv("XM8710_TEMPEST_FRAGMENTS");
        return !(p && p[0]=='0' && p[1]=='\0');
    }();
    return on;
}
struct Cache {
    const UINT16* ptr=nullptr;
    unsigned frame=~0u;
    unsigned count=0u;
    int shiftX=0,shiftY=0;
    std::vector<unsigned char> fragment;
    void Rebuild(const UINT16* words,unsigned n,unsigned atFrame,int sx,int sy){
        ptr=words;count=n;frame=atFrame;shiftX=sx;shiftY=sy;
        fragment.assign(n,0u);
        for(unsigned begin=0;begin<n;){
            const unsigned pri=Read(words,begin*4u)>>13u;
            const unsigned pal=Read(words,begin*4u+3u)&31u;
            unsigned end=begin+1u;
            for(;end<n;++end){
                if((Read(words,end*4u)>>13u)!=pri ||
                    ((Read(words,end*4u+3u)&31u)!=pal))break;
            }
            // Strict capture-supported shape; a palette 21 object alone,
            // a few repeated body tiles, or the normal fighter run cannot qualify.
            if(pri==6u && pal==21u && end-begin>=20u && end-begin<=48u){
                std::unordered_set<unsigned> unique;
                int xmin=32767,xmax=-32768,ymin=32767,ymax=-32768;
                for(unsigned i=begin;i<end;++i){
                    const unsigned xRaw=Read(words,i*4u);
                    const unsigned yRaw=Read(words,i*4u+1u);
                    const unsigned attr=Read(words,i*4u+3u);
                    const int x=Signed10(xRaw)+sx,y=Signed10(yRaw)+sy;
                    const int right=x+16*int(((attr>>8u)&15u)+1u);
                    const int bottom=y+16*int(((attr>>12u)&15u)+1u);
                    if(x<xmin)xmin=x;
                    if(y<ymin)ymin=y;
                    if(right>xmax)xmax=right;
                    if(bottom>ymax)ymax=bottom;
                    unique.insert(Read(words,i*4u+2u)|((yRaw&0x6000u)<<3u));
                }
                if(unique.size()>=20u && xmax-xmin>=112 && ymax-ymin>=112){
                    for(unsigned i=begin;i<end;++i)fragment[i]=1u;
                }
            }
            begin=end;
        }
    }
    bool Candidate(const UINT16* words,int n,unsigned atFrame,
                   int sx,int sy,unsigned slot){
        if(!words || n<=0 || n>4096 || slot>=(unsigned)n)return false;
        if(words!=ptr || count!=(unsigned)n || atFrame!=frame ||
           sx!=shiftX || sy!=shiftY){
            Rebuild(words,(unsigned)n,atFrame,sx,sy);
        }
        return fragment[slot]!=0u;
    }
};
static inline bool Fragment(const UINT16* words,int count,unsigned frame,
                            int sx,int sy,unsigned slot){
    if(!Enabled())return false;
    static Cache cache;
    return cache.Candidate(words,count,frame,sx,sy,slot);
}
} // namespace XM8710
#endif
