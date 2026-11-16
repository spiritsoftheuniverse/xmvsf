# XMVSF 87.364 — fix recurring new-stage forest fallback

## Root cause found in existing implementations

The Boomer stage (custom ID 7) was added to the stage menu, native stage-05 selector, camera observer, and SDL renderer in 87.363. But `cps_draw.cpp` still contained an older hard-coded graphics-capture gate `(xm77StageResolved<1 || xm77StageResolved>6)`, documented as Wily/FD/Guile/Sigma/Akuma/Ryu only.

This made `xm77CaptureSprites()` set `xm77SpriteValid=0` and exit, even with a valid native camera and Boomer art. The Boomer painter correctly requires the 384×224 native fighter/FX alpha capture to preserve native sprite composition, so it returned false and the SDL compositor drew the unchanged original forest framebuffer. This is why merely adding a new stage to the menu or camera cannot guarantee any visual appears.

The original palette-handling path also had an independent `xm77StageResolved<=6` limit. Both missed the same new ID.

## Fix

- Added an SDL-independent shared custom-stage ID header `xmvsf_stage87_custom_stage_ids.h`. `XM87CustomStage::kLast=7` and `ownsNativeSprites(id)` are used by the native CPS2 graphics capture and native stage palette handling; the Random stage pool also derives from that same maximum.
- Expanded the original native sprite-only capture ownership to Boomer ID 7, still requiring actual native custom-stage session, 384×224 source, live CPS2, matching camera and native sprites.
- Does **not** remove the valid render gates, discard the fighter/HUD texture, call the original full framebuffer on top of authored art, write game RAM, or change any other stage's graphics.
- The stage renderer and audio code from 87.363 are left unchanged. The fixed 87.364 scripts compile only through the user's existing local Windows build path; Stable is preserved.

## Regression convention

When adding another custom stage, advance `kLast` in the shared ID header when adding the stage-menu entry. Do not add another separate hard-coded `<=N` or `>N` gate in `cps_draw.cpp`. Test stage-05 capture and Boomer painter with actual native combat after local `BUILD_TEST.bat`. A previous 87.363 build still cannot show Boomer's authored art because the old CPP gate is compiled into that binary.

Source verified; full Windows compilation and live stage display have not been executed by this remote commit.
