# TEST 87.231 — FS Storm floor, severe knockdown is an upgrade

## What failed across 87.223–87.230

The approved 87.223/87.226 mechanics can write a *Storm-profile* flying-screen knockdown: native reaction/index 1C18/0230 with the recorded **gentle** X/Y velocities. The separate Cyclops severe routine writes the same reaction/index but the distinct **steep downward** Y=0x00132A00 and short X=±0x00066200. Charlie original air HP is ROM-native, and his accepted first HK is Cyclops severe.

A one-flight-per-native-landing latch (`stormFlightUsedUntilLanding`) originally protected against synthetic repeated boosts. Subsequent 87.228 changes used the same flying-screen status to suppress extra conversion, and 87.229 removed the delayed heavy penalty when logs showed it leaking into LP/LK. 87.230 then added narrowly correct HP→HK / grounded-heavy severe conditions, but left ordinary FS-connected hits under these early exits:

- `NOT_HEAVY_NATIVE_PRESERVED` for LP/LK;
- `MEDIUM_NATIVE_PRESERVED` for MP/MK;
- `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING` for other heavies.

**Those exits never applied even the baseline Storm knockdown.** The 87.230 ROM-free suite actually asserted that some such hits should remain untouched. This is an interaction/regression-test design error: a positive severe condition was bolted onto an exclusionary original baseline instead of a monotonic Storm-floor → Cyclops-upgrade priority.

## Supplied 87.230.zip evidence

The `observer/storm_like_flight_events.tsv` contains 238 events and four `STORM_HP_NATIVE_SIGNATURE_FS_INDEX_AND_VELOCITY` writes. Following the final recorded Storm conversion at frame 2367, an active FS appears on confirmed normal hit onsets at 2389 (LP), 2409 (LP), 2420 (LK), 2440 (MK), and 2458 (HK). No one of those five records a subsequent Storm or Cyclops write in the capture; the HK is explicitly labeled `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING`. This capture **does not** demonstrate a confirmed grounded FS heavy and has no `FS_HP_THEN_HK_CYCLOPS_SEVERE` release. Treat any manually observed gameplay success separately from claims this particular log proves.

## Priority, bound to each original ROM hit

1. **Native already-steep severe:** preserve original action/index and motion unchanged.
2. **Explicit severe upgrades:** if previous **connected normal** was HP and the current **connected airborne normal** is HK while FS is active, use Cyclops severe. A connected grounded normal HP/HK during FS independently uses severe, only with ground flag clear and native grounded action. The accepted separate Charlie HK severe rule remains intact.
3. **All other connected normals during FS:** apply Storm-profile flying-screen knockdown (LP, MP, HP, LK, MK, HK, including ordinary grounded normals against an airborne FS victim). An ordinary FS hit is **not** made native/soft simply because another Storm conversion occurred earlier in the airborne sequence.
4. **No active FS:** retain the existing 87.223/226 native-first and first-heavy initiation rules; do not invent knockdown on isolated ground hits.

The two variants use the existing independent `applyStormFlyingScreen` and `applyCyclopsSevereFlight` writers, not a shared severe velocity. Protected Charlie original air HP, Sabretooth/Cammy HP, Wolverine Drill Claw and Cyclops native severe remain exceptions. This does not change gameplay in Classic mode.

For either variant, a *fresh, confirmed* ROM hit at substate 0 arms a pending release with its attacker/victim, action, attack serial, previous connected button, and grounded/severe classification. Only the **same** native impact reaching substate 2 and a recognized victim hit-state pair within 24 frames causes a write. A different action, new attack, tag, landing, disabled mode, or expired pending state cancels it. There is **no** stale Cyclops knockback on a later LP/LK/MP.

Active FS may be tracked by either the host Storm latch or observed native severe FS latch; a single active latch suffices. Both still reset only on the agreed native landing, tag, or non-combat state. The previous-connected-button logger on release is now the value saved at originating hit onset, instead of the button already updated by that hit.

New TSV decisions: `FS_STORM_HIT_CONFIRMED_PENDING`, `FS_STORM_KNOCKDOWN`, `FS_CHARLIE_HK_CONFIRMED_PENDING`, `CHARLIE_HK_CYCLOPS_SEVERE_KNOCKDOWN`, `FS_HP_THEN_HK_CYCLOPS_SEVERE`, `FS_GROUNDED_HEAVY_CYCLOPS_SEVERE`, `FS_HIT_CONVERSION_CANCELLED`, `FS_ALREADY_NATIVE_SEVERE_PRESERVED`.

## Regression coverage and verification

The old 87.230 unit's expectations that LP/LK/MP during FS retain soft native motion were **wrong for the requested behavior**; the 87.231 build substitutes `test_fs_storm_baseline_231.cpp` rather than simply adding another severe-only test. That test exercises all six airborne normals; four grounded light/medium inputs; HP→HK severe and grounded HP/HK severe; HP→LP→HK order; grounded MP breaking HP adjacency; action changes before impact step 2; either FS latch independently; no-FS ground hits; native steep preservation; Charlie native HP and independent severe HK; Sabretooth/Cammy HP and Wolverine Drill Claw exemptions; grounded detection excluding super jump; and real landing rearm.

Branch: `feature/fs-storm-baseline-severe-priority-87-231`. Local scripts: `BUILD_TEST.bat` / `RUN_TEST.bat`; target: `runtime/stable/XMVSF_Test_87_231.exe`; capture: `test/87.231/observer/storm_like_flight_events.tsv`. Previous binaries, Stable and ROM images are never overwritten. Native Windows compilation, ROM-free executable assertions, and actual gameplay still require a local run. No GitHub Actions.

Native grounded action mapping is only *partially* supported by the supplied 87.230 capture (grounded `1204`/HK impacts were seen **without** active FS). A grounded HP/HK during active FS still needs a new live trace to establish its exact matching transition and timing; do not claim that it is fully verified from ROM-free fixtures.
