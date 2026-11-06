# XMVSF 87.202 — 87.201 F9 failure classification and native air selector rebound

This is a narrow, gated DHC follow-up to the supplied 87.201 F9 archive. GitHub Actions are not used. The native Windows build and gameplay validation remain local.

## Observed 87.201 outcomes (39 armed DHC attempts)

- 24 genuine incoming native Ken 2E02 acceptances. All 24 preserve freeze through super startup and release on a real ROM offensive hitbox. Zero instances of missing freeze release among accepted cases.
- 12 failures: Ryu 2E0E/2A1C/step06 -> next CPU 2E0C/2A00/step02, script D31A8, native D6=0A, controller256=00, controller259=08. Original executed PPC 010392 writes the new action. The existing host controller rejected it solely because it differed from captured 2E0E.
- 1 failure: Ryu 2E0E -> native ordinary 0212/1C00 at frame 7089. NOT eligible for bridging.
- 2 failures: Ryu 2E0C -> native ordinary 0214/1C02 at frames 6991 and 15018. NOT eligible for bridging. The second begins in 2E0C step02, so first-frame eligibility should be audited separately rather than broadly vetoing the requested full action lifetime.
- All 15 original aborts correctly release the chain lock and host-owned controller259=08 latch; do not rewrite their native selected actions on abort.

The new helper accepts only the twelve identical ROM-observed rebound signatures: exact Ryu/Ken actor ownership, original 2E0E captured action/mode/step/D6, plausible aligned 2E0E ROM script, next-frame 2E0C/step02 native script D31A8, consumed +256=00, host latch +259=08, original captured +259=00 and no time gap. The incoming 2E0C root must also resolve in the real ROM to D31A8. It then enters the **existing** 87.185 3200 original tag-script bridge, which still requires the ordinary native point/reserve swap and Ken's ROM-owned incoming super. It does not manufacture Ken supers, assume all 2E selectors are equivalent, or override real 0212/0214/2Cxx actions.

## Validation and A/B control

The new source predicate and isolated C++11 regression are in the current buildtree and dev/dhc/tests. Local BUILD_TEST compiles the regression before the native incremental make and reports failures in the persistent build transcript. Default RUN_TEST and RUN_REPLAY enable XM87202_TEST_AIR_FOLLOWUP_REBOUND=1; set it to 0 in the parent CMD for A/B comparison (RUN_TEST inherits overrides; replay currently explicitly sets 1). RUN_REPLAY retains its historical F9 session fallback. BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat names remain unchanged; 87.202 gets a unique TEST exe and test-data directory, never Stable.

This is a **code fix informed by executed native PC evidence**, not yet a measured 87.202 gameplay result. The isolated test proves predicate strictness only. The next Windows F9 replay should check whether all 12 candidates reach ROM 3200, swap, landing, genuine Ken 2E input acceptance and offense-based freeze release; whether the three ordinary-action aborts remain safe; and whether any new behavior appears on P2/other rosters. The 87.201 log had 2 sessions in most observers and 3 F9 movie sessions (19 segments; 42868 frames); frame numbers reset and must not be naively deduplicated across sessions.
