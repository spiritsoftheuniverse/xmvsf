#ifndef XMVSF_STAGE87_43_HP_INK_SPLIT_H
#define XMVSF_STAGE87_43_HP_INK_SPLIT_H
// Stage 87.43: classify native HP strips, and split green fill from gray bezel.
// Observed in the 87.17 raw OAM trace: priority7, pal2/y20 and pal4/y32.
// No completed-frame recoloring. Do not modify global palette bank.
namespace XM8743 {
static inline bool HpObject(unsigned priority,unsigned pal,unsigned tile,int x,int y) {
    if(priority!=7u || x<0 || x>370) return false;
    if(pal==2u && y==20 && tile>=88463u && tile<=88544u)return true;
    if(pal==4u && y==32 && ((tile>=88417u && tile<=88460u) ||
                             (tile>=88546u && tile<=88579u) ||
                             tile==89417u || tile==89418u))return true;
    return false;
}
// Stage87.43: blue HP fill and neutral silver backing are TWO ink classes.
// Only a strongly saturated native green pen can be a health-fill pen.
// Every other opaque pen in a verified HP-strip sprite is monochrome,
// preserving its luminance, pixel/pen geometry and transparency.
struct Stats { unsigned fill; unsigned frame; unsigned untouched; Stats():fill(0),frame(0),untouched(0){} };
static inline unsigned Cap(unsigned v){return v>255u?255u:v;}
static inline unsigned Mono(unsigned c){
    const unsigned r=c&255u,g=(c>>8)&255u,b=(c>>16)&255u;
    const unsigned y=(77u*r+150u*g+29u*b+128u)>>8;
    return (c&0xff000000u)|y|(y<<8)|(y<<16);
}
static inline bool IsHpFill(unsigned c){
    const unsigned r=c&255u,g=(c>>8)&255u,b=(c>>16)&255u;
    const unsigned other=(r>b?r:b);
    // Prevent a cool/green-biased gray bezel from becoming blue.
    return g>=80u && g>=r+52u && g>=b+52u && (g*100u>=(other+1u)*155u);
}
static inline unsigned Blue(unsigned c){
    const unsigned g=(c>>8)&255u;
    const unsigned nr=Cap(g*7u/100u),ng=Cap(g*40u/100u),nb=Cap(g);
    return (c&0xff000000u)|nr|(ng<<8)|(nb<<16);
}
static inline unsigned Make(const unsigned* input,unsigned* output,Stats* stats=0){
    Stats local;
    if(!input||!output)return 0;
    unsigned changed=0;
    for(unsigned pen=0;pen<16u;++pen){
        const unsigned original=input[pen];
        if(pen==0u){output[pen]=original; ++local.untouched;continue;}
        if(IsHpFill(original)){
            output[pen]=Blue(original);
            ++local.fill;
        }else{
            output[pen]=Mono(original);
            if(output[pen]!=original)++local.frame;
            else ++local.untouched;
        }
        if(output[pen]!=original)++changed;
    }
    if(stats)*stats=local;
    return changed;
}
}
#endif
