#ifndef XMVSF_STAGE87_45_HP_RGB_AND_OPENING_H
#define XMVSF_STAGE87_45_HP_RGB_AND_OPENING_H
// Stage 87.45: native RGB888 packing + source-owned outlines at game-screen opening.
// Observed in the 87.17 raw OAM trace: priority7, pal2/y20 and pal4/y32.
// No completed-frame recoloring. Do not modify global palette bank.
namespace XM8745 {
// The original primary strip (pal2/y20) and reserve strip (pal4/y32)
// use separate ROM tile sheets. A tag can change which sheet occupies the
// forward row on EITHER side; checking only the original palette+row pairing
// left the incoming point character's HP green.
static inline bool KnownHpTile(unsigned tile){
    return (tile>=88417u && tile<=88579u) || tile==89417u || tile==89418u;
}
static inline bool LegacyHpObject(unsigned priority,unsigned pal,unsigned tile,int x,int y){
    if(priority!=7u || x<0 || x>370)return false;
    if(pal==2u && y==20 && tile>=88463u && tile<=88544u)return true;
    if(pal==4u && y==32 && ((tile>=88417u && tile<=88460u) ||
          (tile>=88546u && tile<=88579u) || tile==89417u || tile==89418u))return true;
    return false;
}
static inline bool HpObject(unsigned priority,unsigned pal,unsigned tile,int x,int y) {
    // No screen-wide green detection; only priority-7, verified strip tile
    // families in either original HP strip row on the native 384px viewport.
    return priority==7u && (pal==2u || pal==4u) &&
           x>=0 && x<=370 && (y==20 || y==32) && KnownHpTile(tile);
}
// Native HUD RGB888 is 0x00RRGGBB: BurnHighCol is the authoritative packer.
// Do not reverse R/B, which produced the previous orange health fill.
// Bezel ink remains grayscale; fill ink is identified by ROM pen, not color.
// Confirmed by decoding native 16x16 ROM tile pen indexes:
// palette 2 / top strip: 88501 uses 10,2,3 (backing); 88464 uses 14 (fill).
// palette 4 / bottom strip: 88418 uses 12,13,14 (backing); 88547 uses 15 (fill).
// We must not infer fill from the *graded* RGB; Double Feature can gray it.
struct Stats { unsigned fill; unsigned frame; unsigned untouched; Stats():fill(0),frame(0),untouched(0){} };
static inline unsigned Cap(unsigned v){return v>255u?255u:v;}
static inline unsigned Luma(unsigned c){
    const unsigned r=(c>>16)&255u,g=(c>>8)&255u,b=c&255u;
    return (77u*r+150u*g+29u*b+128u)>>8;
}
static inline unsigned Mono(unsigned c){
    const unsigned y=Luma(c);
    return (unsigned)BurnHighCol((int)y,(int)y,(int)y,0);
}
static inline unsigned Blue(unsigned c){
    // Explicit native fill pen: preserve a perceptible shade progression,
    // but deliver a saturated readable blue even after upstream gray grading.
    const unsigned y=Luma(c);
    const unsigned b=Cap(48u+y*135u/100u);
    return (unsigned)BurnHighCol((int)Cap(b*7u/100u),(int)(b*43u/100u),(int)b,0);
}
// Only screen-load/transition/combat phases. Ownership is still mandatory
// in the renderer; this does not classify HUD or stage scenery as fighters.
static inline bool OpeningPhase(int phase){
    return phase>=2 && phase<=8 && (phase&1)==0;
}
static inline bool FillPen(unsigned palette,unsigned pen){
    return (palette==2u && pen==14u) || (palette==4u && pen==15u);
}
static inline bool NativeGreenFill(unsigned c){
    // BurnHighCol RGB888: 0x00RRGGBB. The incoming-point bar can use a
    // different native pen than the original foreground strip. Restrict this
    // fallback to verified HP objects; never scan the finished framebuffer.
    const unsigned r=(c>>16)&255u,g=(c>>8)&255u,b=c&255u;
    return g>=72u && g>=r+28u && g>=b+28u &&
           (g*100u>=(r+1u)*145u) && (g*100u>=(b+1u)*145u);
}
static inline unsigned Make(const unsigned* input,unsigned* output,unsigned palette,Stats* stats=0){
    Stats local;
    if(!input||!output)return 0;
    unsigned changed=0;
    for(unsigned pen=0;pen<16u;++pen){
        const unsigned original=input[pen];
        if(pen==0u){output[pen]=original; ++local.untouched;continue;}
        if(FillPen(palette,pen) || NativeGreenFill(original)){
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
