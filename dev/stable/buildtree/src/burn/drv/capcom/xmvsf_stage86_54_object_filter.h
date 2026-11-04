#ifndef XMVSF_STAGE86_54_OBJECT_FILTER_H
#define XMVSF_STAGE86_54_OBJECT_FILTER_H
// Stage86.54 permanent Wily composite object classifier.
// Exact ROM-inventory provenance: dev/stage84/XM83_CPS2_GFX.h.
// This function is invoked ONLY while creating the disposable native-OBJ
// overlay for Wily stage05; never changes RAM, Z order, or native CPS2 draw.
// Object tiles are reconstructed from CPS2 low tile + Y high address bits.
static inline bool xm8654KnownNativeScenery(unsigned priority, unsigned tile, unsigned palette)
{
    // Stage84 ROM-verified 109xxx scenery family, pal 25..29 at priority7;
    // on native stage05 the same family occurs at priority6.
    if ((priority == 6 || priority == 7) &&
        tile >= 109000U && tile < 110000U &&
        palette >= 25U && palette <= 29U) return true;
    // Additional ROM-verified stage05 scenery objects from Stage84.
    if (priority == 6) {
        if (palette == 26U && tile >= 109931U && tile <= 110050U) return true;
        if (palette == 28U && tile >= 110106U && tile <= 110109U) return true;
        if ((palette == 29U || palette == 30U) &&
            tile >= 110110U && tile <= 110140U) return true;
    }
    return false;
}
static inline bool xm8654KeepNativeObject(unsigned priority, unsigned tile,
                                           unsigned palette, unsigned width,
                                           unsigned height)
{
    // Stage86.9 ROM-verified 2x2 large HUD numeral atlas.
    if (priority == 0 && palette == 1 && width == 2 && height == 2 &&
        tile >= 87808U && tile <= 87826U && ((tile - 87808U) % 2U) == 0)
        return true;
    // Stage81 and Stage84 live inventories: fighters and HUD occupy 6/7.
    // Never admit unknown low-priority stage-only sprites merely because they
    // share a palette/tile family with gameplay (Stage86.53 regression).
    if (priority != 6 && priority != 7) return false;
    // XM87.118 FOREST_WILDLIFE_REMOVED: the CRC-verified XMVSF 961023
    // graphics ROM contains bird animation cells 110080..110175 and forest
    // animal cells 110336..110463. The ROM supplies artwork, not object
    // ownership; restrict this exclusion to the custom Wily native-OBJ
    // capture, which is the only caller of KeepNativeObject. The normal
    // native forest, CPU state, fighter sprites, and HUD are untouched.
    if ((priority == 6u || priority == 7u) && palette >= 25u && palette <= 30u &&
        ((tile >= 110080u && tile <= 110175u) ||
         (tile >= 110336u && tile <= 110463u))) return false;
    return !xm8654KnownNativeScenery(priority, tile, palette);
}
#endif
