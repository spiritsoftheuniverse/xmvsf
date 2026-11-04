#ifndef XMVSF_STAGE87_42_HP_OBJECTS_H
#define XMVSF_STAGE87_42_HP_OBJECTS_H
// Stage 87.42: recolor ONLY original CPS-2 health-strip objects.
// Observed in the 87.17 raw OAM trace: priority7, pal2/y20 and pal4/y32.
// No completed-frame recoloring. Do not modify global palette bank.
namespace XM8742 {
static inline bool HpObject(unsigned priority,unsigned pal,unsigned tile,int x,int y) {
    if(priority!=7u || x<0 || x>370) return false;
    if(pal==2u && y==20 && tile>=88463u && tile<=88544u)return true;
    if(pal==4u && y==32 && ((tile>=88417u && tile<=88460u) ||
                             (tile>=88546u && tile<=88579u) ||
                             tile==89417u || tile==89418u))return true;
    return false;
}
static inline unsigned Cap(unsigned v){return v>255u?255u:v;}
static inline unsigned Paint(unsigned c){
    const unsigned r=c&255u,g=(c>>8)&255u,b=(c>>16)&255u;
    // Limit to original green fill, including pure #00FF00 and HUD-graded green.
    // Leave gray outline, black void, yellow markings, and name inks alone.
    if(g<50u || g<=r+16u || g<=b+16u)return c;
    const unsigned nr=Cap(g*7u/100u),ng=Cap(g*40u/100u),nb=Cap(g);
    return (c&0xff000000u)|nr|(ng<<8)|(nb<<16);
}
static inline unsigned Make(const unsigned* input,unsigned* output){
    unsigned changed=0;
    if(!input||!output)return changed;
    for(unsigned i=0;i<16u;++i){
        output[i]=(i==0u)?input[i]:Paint(input[i]);
        if(output[i]!=input[i])++changed;
    }
    return changed;
}
}
#endif
