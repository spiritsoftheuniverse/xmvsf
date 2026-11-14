# XMVSF TEST 87.362 — complete native flying-screen return; quiet completed observers

## User capture: observer(20261114-174722).zip

The real 87.361 dash_startup.tsv has 858 Charlie-dominated events. The first gross world-X corruption is at frame 20905: native mode 1400, X=70FA0600, velocity zero, frame pointer 000ECF0C. The existing 87.232 return anchor sets a plausible off-screen X=0339DB00 at frame 20906. The ORIGINAL ROM dash then progresses toward the opponent: frame 20910 X=03305B00, frame 20928 X=02A29B00. But at frame 20935 the 68000 recreates X=70FA9B00 with the same 1400/000ECF0C entry animation. Another restart at 20965; then the old 87.221/226 watchdog attempts FS_DASH_LOOP_EXIT/FS_DASH_REPEAT_AFTER_WATCHDOG and 16 FS_DASH_NATIVE_CONTROLLER_RESTORED events. By frame 21483 the CPU still reasserts X=70FADB00. There are 261 FS_NATIVE_ENTRY_68K_REASSERTED_X rows. The game was not merely waiting on a dash-start timer. The underlying original 68k world-X/entry writer remains active after the return moves toward the opponent.

## Frailty in existing code

- 87.221 initially treated any dash longer than 36 frames as a loop, even if a genuine native return was moving; 87.232 corrected that to require stalled position or repeated origin.
- 87.232 successfully places an invalid-X native entry outside the opponent view and grounds it, but its autoReturn transaction never becomes COMPLETED upon reaching the opponent. It merely resets failedReturnFrames after observing two moving samples. The 68000 can launch another entry every ~30 frames.
- Its returnAnchor remains the original 448px offscreen spawn, while safeX is deliberately NOT updated during autoReturn. Subsequent recovery restores the *old* anchor even if the actor legitimately dashed into the fight.
- 87.226's native-neutral restore is CPU-RAM-only and cannot itself cancel the original 0109B4 writer. The three-frame recoveryActive cooldown and the old neutral snapshot sampling could eventually permit a new reentry and accidentally refresh the recovery snapshot from an artificial neutral.

## Bounded 87.362 change

- In the same-fighter original offscreen return only, detect two consecutive native frames where actual world X has progressed at least 48px from the original return anchor and is within the real opponent's original 384px viewport. Save last genuine in-view X. No camera estimate, input or invented velocity.
- A later invalid X with the same returning actor triggers one original-native neutral recovery at that genuine in-view X, NOT at the old offscreen spawn. Latch an anti-repeat guard across one-frame pseudo-neutral states. While the original CPU continues to reassert an impossible coordinate in the same episode, keep genuine native neutral and the last safe X. Do not intercept any ordinary sane-X voluntary dash; release the guard on actual combat action/control or fighter identity change. Genuine new hit/attack/throw remains CPU-owned.
- Do not recapture native neutral snapshots during autoReturn, recoveryActive or the completed-return guard; only genuine neutral after a fully settled recovery may become a recovery baseline.
- Preserve the previous four-frame bounded retry if the actor never genuinely reaches the real fight view.

This is a targeted defensive correction, **not a verified repair of the original 68000 writer**. The source 0109B4/01284A branch predicate may still need direct M68K diagnosis if the actor remains repeatedly forced by the ROM. The newly retained dash_startup.tsv includes FS_RETURN_REACHED_VIEW_REENTRY_STOPPED and FS_RETURN_CORRUPT_RESTART_BLOCKED for distinguishing these conditions.

## Observer cleanup

RUN_TEST.bat disables the DHC acceptance observer, unified DHC TSV, DHC subframe/opening/PC traces, Cyclops air throw TSV, Storm flight/button/voice/68000-candidate files, and health log and summary analyzers. The native Cyclops recovery, Storm gameplay/voice mute, Version-4 native DHC, Tournament behavior and F10 manual probes are not disabled. dash_startup.tsv stays ON until this screen-entry issue is validated. Old user files are not deleted, but new sessions no longer create/append these observer files.

No new mandatory BUILD_TEST steps, no GitHub Actions, and no Stable overwrite. BUILD_TEST.bat / RUN_TEST.bat / dev/dhc/BUILD_TEST.sh default to 87.362. Existing 87.232 local unit fixture adds real-return-then-corrupt-X regression, repeated reassertion, legitimate sane-X dash and actor combat reset. The Windows game and full TEST build have NOT been executed by the remote commit.
