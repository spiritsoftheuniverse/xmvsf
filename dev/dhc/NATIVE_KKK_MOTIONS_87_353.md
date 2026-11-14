# XMVSF TEST 87.353 — native three-kick command timing

Version 4 original US xmvsfu, existing four experimental native DHC environment flags, branch feature/dhc-morphus-precommit-87-334.

## Evidence

From user observer(8).zip, the native input monitor sees real physical KKK (hex 38), even during eligible windows. In particular, frame 60877 successfully enters original Charlie D6=06 action 2C24 from a QCB+KKK; this is not a missing native D6 entry. At frames 62306, 62342, 62365 P1 attempts KKK with Sabretooth on the bench but the HCB motion was not complete at the moment of the button edge. The log also has many neutral-direction PPP rejections, which are not evidence of valid missing Hyper motions. No native QCF+KKK Chun-Li acceptance is established by that capture.

Native command table already maps Sabretooth HCB+KKK to D6=08, Chun-Li QCF+KKK to D6=08, and Charlie QCB+KKK to D6=06. It correctly handles simultaneous terminal-direction KKK but 87.352 requires a NEW button edge on the exact sample the motion completes. An early three-kick press, still held at terminal direction, cannot trigger.

## Version 87.353 change

In the existing shared native Slot sampler, remember the actual rising edge of all-three-kicks (hex 38) for up to 10 core frames, only while all three kicks continue to be held. Accept the original fully completed motion at its real terminal sampled direction if the edge is still fresh, even when no new button is pressed on the terminal frame. Clear the edge on release, changed button mask, ownership reset, expiry, or successful claim. It does not invent a motion, reassign Hyper D6, introduce a PP/KK alias, or relax Super/stock/valid living reserve/Version 4 gates.

The existing DHC TSV receives NATIVE_353_BUFFERED_KKK_CLAIM only for real handoffs that used a prior KKK edge. Native M68K tag/swap and original 2C/2E confirmation events remain the authority for execution. Synthetic fixtures now cover all three original D6 commands, both facings, ordinary terminal KKK, early-held KKK, expired/released KKK and unmatched direction.

Only the existing local Windows build/test scripts move to 87.353; no GitHub Actions, new build-time tests, stable executable changes, or Version 3 changes. Synthetic fixture is committed but full Windows BUILD_TEST and original gameplay were not run here.

## Local gameplay

Set the same four DHC flags active in PowerShell, run BUILD_TEST.bat and RUN_TEST.bat xmvsfu, choose Version 4. Test Sabretooth HCB+KKK, Chun-Li QCF+KKK, Charlie QCB+KKK on both facings, deliberately pressing KKK a few frames before the final direction and also on the last direction. Capture test/87.353/observer/dhc_unified_274.tsv. Check NATIVE_353_BUFFERED_KKK_CLAIM, then original tag and real 2C/2E, and confirm gameplay.
