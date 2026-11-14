# XMVSF 87.359 — Charlie's native QCB+KKK during an outgoing Hyper

## New evidence

The user's observer(20261114-152602).zip has a genuine native outgoing Cyclops Mega Optic Blast -> Charlie QCB+KKK at frames 1610–1611, with the original tag/tail executed, actual fighter swap and original Charlie D6=06 2C24 Hyper. Thus Charlie's native D6 route is valid, but this is only one success and does not establish reliable recognition. No NATIVE_357_CHARLIE_QCB_KKK_CUE_HELD occurs, so the earlier deferred branch did not resolve the apparent unreliability in this capture. The capture contains no raw per-direction joystick history for the failed, early-gated attempts: a facing flip is a plausible parser failure, not independently proven by this log.

## Change

87.357 still recomputed relative Back from the actor's current facing flag on EVERY sample. A change mid-command can transform Down, Down-Back, Back into Down, Down-Forward, Forward, breaking the QCB recognizer. Only Charlie's incoming original Super gets a small directional history which locks the orientation at the actual Down sample. It accepts exact Down -> Down-Back -> Back and a fast Down -> Back within 12 native frames, *only while an original outgoing native 2A/2C or 2A/2E Hyper is running*. It rejects QCF in the locked orientation. A real new KKK edge, or the existing 10-frame held edge after a fully completed QCB, is still needed. No input is observed during normal gameplay.

Once the actual Charlie motion+KKK completes, the already-existing 87.357 pending input queue retains that deliberate request until native DHC cancellation becomes eligible or the original Hyper ends/owner changes. It does not synthesize a swap, force native Hyper, or alter stock. The original native M68K tag and ROM route are still authoritative. Sabretooth, Chun-Li, Version 3 and Stable are unchanged.

A new NATIVE_358_CHARLIE_3K_SECTOR event logs only a new 3K press DURING an outgoing native Hyper with Charlie as reserve: raw direction, current facing, facing locked at Down, QCB progress, eligibility, and stock. This will distinguish early input vs motion mismatch on the next capture without full-round DHC input sniffing.

Local-only C++ fixture adds both facings, a facing flip mid motion, fast D-to-B, QCF negative, actual queued native D6=06, and normal-play cleanup. No new build gates or GitHub Actions. Source is committed, but Windows BUILD_TEST.bat and real game execution are unverified.

## 87.359 correctness follow-up

The original generic motion tracker was still allowed to select Charlie QCB+KKK independently of the new facing-locked detector. When physical input was QCF and the actor reversed facing after Down, the generic parser could see a phantom QCB. For incoming Charlie only, exclude the generic QCB candidate from the native decision priority loop. His original QCB+KKK now comes exclusively from the newly locked physical Down,Down-Back,Back sweep, preserving the separate original QCF+PP and QCF+KKK native commands. The existing fixture includes a QCF sweep with facing reversal that must NOT select Charlie QCB.
