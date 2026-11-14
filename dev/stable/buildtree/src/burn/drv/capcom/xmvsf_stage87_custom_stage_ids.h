#ifndef XMVSF_STAGE87_CUSTOM_STAGE_IDS_H
#define XMVSF_STAGE87_CUSTOM_STAGE_IDS_H
// 87.364: the native CPS2 sprite-only capture MUST recognize every
// custom-stage ID. Previously, new stages were added to the menu and
// native camera but this older graphics gate still rejected IDs > 6,
// silently restoring the forest framebuffer instead of the stage art.
// This small SDL-independent header is shared by the menu and native
// CPS2 sprite compositor: new stages have ONE maximum ID to advance.
namespace XM87CustomStage {
static const int kLast=7; // 1 Wily, 2 FD, 3 Guile, 4 Sigma, 5 Akuma, 6 Ryu, 7 Boomer
static const int kNativeStageCount=12;
static inline bool ownsNativeSprites(int id){
    return id>=1 && id<=kLast;
}
} // XM87CustomStage
#endif
