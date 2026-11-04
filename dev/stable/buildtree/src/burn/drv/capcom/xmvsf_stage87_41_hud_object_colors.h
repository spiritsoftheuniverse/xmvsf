#ifndef XMVSF_STAGE87_41_HUD_OBJECT_COLORS_H
#define XMVSF_STAGE87_41_HUD_OBJECT_COLORS_H
// XM87.41: Final native CPS-2 priority-7 HUD sprite palettes. Never scan
// composited screen pixels. Grounded in xm8717_draw_provenance.tsv and the
// corresponding real ROM graphics tiles, not nearby/background RGB thresholds.
// No writes to ROM, guest RAM, or the global CpsPal palette bank.
namespace XM8741 {
enum Kind { Other=0, PortraitBackdrop=1, HealthStrip=2 };
static inline bool NearPortraitEdge(int x) { return (x>=-20 && x<=32) || (x>=348 && x<=388); }
static inline bool HpX(int x){return x>=-4 && x<=370;}
static inline Kind Classify(unsigned priority,unsigned palette,unsigned tile,int x,int y) {
    if(priority!=7u)return Other;
    // Real outer portrait backdrop blocks: palette 2, rows -2 and 8.
    // Character portrait art is a separate palette12/15 object, and its
    // character-dependent pixels MUST NOT be touched.
    if(palette==2u && tile>=89113u && tile<=89115u &&
       (y==-2 || y==8) && NearPortraitEdge(x))return PortraitBackdrop;
    // Existing bar strip sheets: rows 20 and 32; object layouts are mirrored
    // in ROM but HP color changes belong to these tiles regardless of side.
    if(HpX(x) && palette==2u && y==20 &&
       ((tile>=88463u && tile<=88465u) ||
        (tile>=88500u && tile<=88502u) ||
        tile==88530u || tile==88513u || tile==88472u ||
        tile==88519u || tile==88523u || tile==88529u ||
        tile==88466u))return HealthStrip;
    if(HpX(x) && palette==4u && y==32 &&
       ((tile>=88417u && tile<=88419u) ||
        (tile>=88546u && tile<=88548u) ||
        tile==88447u || tile==89418u || tile==89417u ||
        tile==88436u || tile==88446u || tile==88551u ||
        tile==88439u || tile==88481u || tile==88563u ||
        tile==88427u || tile==88443u))return HealthStrip;
    return Other;
}
static inline unsigned Cap(unsigned n){return n>255u?255u:n;}
static inline unsigned Pack(unsigned original,unsigned r,unsigned g,unsigned b){
    return (original&0xff000000u)|Cap(r)|(Cap(g)<<8)|(Cap(b)<<16);
}
static inline unsigned RecolorPen(unsigned c,Kind kind){
    const unsigned r=c&255u,g=(c>>8)&255u,b=(c>>16)&255u;
    unsigned hi=r>g?r:g;hi=hi>b?hi:b;
    unsigned lo=r<g?r:g;lo=lo<b?lo:b;
    if(kind==PortraitBackdrop){
        // Shift blue/cyan backdrop ink toward saturated red without flattening
        // dark edge shades or altering neutral and magenta inks.
        if(hi<36u || hi-lo<20u || b<r+18u || b+10u<g || g+8u<r)return c;
        return Pack(c,hi,lo+(hi-lo)*18u/100u,lo);
    }
    if(kind==HealthStrip){
        // Only actual green pens in certified health-bar strip objects.
        // Includes pure (0,255,0), unlike the broken first blue pass.
        if(g<40u || g<r+24u || g<b+24u)return c;
        return Pack(c,g*8u/100u,g*43u/100u,g);
    }
    return c;
}
static inline unsigned Make(const unsigned* original,unsigned* dest,Kind kind){
    if(!original || !dest)return 0;
    unsigned count=0;
    for(unsigned i=0;i<16u;++i){
        dest[i]=i==0u?original[i]:RecolorPen(original[i],kind);
        count+=(dest[i]!=original[i]);
    }
    return count;
}
}
#endif
