#ifndef XMVSF_STAGE87_44_HP_PEN_SPLIT_H
#define XMVSF_STAGE87_44_HP_PEN_SPLIT_H
// Stage 87.44: ROM ink pens, not post-grade color, distinguish HP fill from bezel.
// Observed in the 87.17 raw OAM trace: priority7, pal2/y20 and pal4/y32.
// No completed-frame recoloring. Do not modify global palette bank.
namespace XM8744 {
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
// Confirmed by decoding native 16x16 ROM tile pen indexes:
// palette 2 / top strip: 88501 uses 10,2,3 (backing); 88464 uses 14 (fill).
// palette 4 / bottom strip: 88418 uses 12,13,14 (backing); 88547 uses 15 (fill).
// We must not infer fill from the *graded* RGB; Double Feature can gray it.
struct Stats { unsigned fill; unsigned frame; unsigned untouched; Stats():fill(0),frame(0),untouched(0){} };
static inline unsigned Cap(unsigned v){return v>255u?255u:v;}
static inline unsigned Luma(unsigned c){
    const unsigned r=c&255u,g=(c>>8)&255u,b=(c>>16)&255u;
    return (77u*r+150u*g+29u*b+128u)>>8;
}
static inline unsigned Mono(unsigned c){
    const unsigned y=Luma(c);
    return (c&0xff000000u)|y|(y<<8)|(y<<16);
}
static inline unsigned Blue(unsigned c){
    // Explicit native fill pen: preserve a perceptible shade progression,
    // but deliver a saturated readable blue even after upstream gray grading.
    const unsigned y=Luma(c);
    const unsigned b=Cap(48u+y*135u/100u);
    return (c&0xff000000u)|Cap(b*7u/100u)|((b*43u/100u)<<8)|(b<<16);
}
static inline bool FillPen(unsigned palette,unsigned pen){
    return (palette==2u && pen==14u) || (palette==4u && pen==15u);
}
static inline unsigned Make(const unsigned* input,unsigned* output,unsigned palette,Stats* stats=0){
    Stats local;
    if(!input||!output)return 0;
    unsigned changed=0;
    for(unsigned pen=0;pen<16u;++pen){
        const unsigned original=input[pen];
        if(pen==0u){output[pen]=original; ++local.untouched;continue;}
        if(FillPen(palette,pen)){
            output[pen]=Blue(original); ++local.fill;
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
