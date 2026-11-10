#ifndef XMVSF_STAGE249_CONTROLLER_POLICY_H
#define XMVSF_STAGE249_CONTROLLER_POLICY_H
// TEST 87.249: pure routing contract shared by boot->game map installation
// and the SDL input callback. Controller selection is runtime-only.
namespace XM249Policy {
static inline bool needsVirtualMap(bool savedAssigned,bool menuSelected) {
    return savedAssigned || menuSelected;
}
static inline bool useSemanticPad(bool menuSelected,bool savedAssigned,
                                  bool savedKeyboard,bool sameHardware) {
    // Preserve custom bindings for the SAME assigned physical device.
    // An Xbox/DS4Windows device selected by menu A can override an old
    // Fighter Pi/keyboard mapping just for this running match.
    return menuSelected && (!savedAssigned || savedKeyboard || !sameHardware);
}
static inline bool deliver(bool savedPress,bool semanticPress,
                           bool semanticPad) {
    return semanticPad?semanticPress:savedPress;
}
}
#endif
