# TEST 87.409 — Tournament FS: superjump-normal scope only

**Branch remains `feature/native-stage-capture-catalog-87-408`.**
This intentionally changes the old 87.230/87.231 host-only FS behavior:
**grounded normals and ordinary jumps no longer inherit a Tournament
flying-screen knockback**, even if the victim was already in flying screen.

## Leak discovered in current source

`xmvsf_stage87_86_storm_button_guard.h` has two RAM-writing paths:
1. `fsConditionalPending`: sets up a Storm floor or Cyclops severe for
   an *already-active FS victim*, then writes at victim reaction substate 2
2. `pendingSevere`: fallback fresh aerial-heavy conversion, also delayed
   until victim substate 2

Before 87.409, `confirmedHeavyNormal` accepted all physical HP/HK
buttons for **18xx ordinary air normals or 10xx/12xx/14xx grounded
normals**, and the baseline was authorized merely by a victim's prior
`stormFlightUsedUntilLanding` or `postFlightNeedsNativeLanding`.
The separate `groundedFSHeavy` rule explicitly rewrote grounded HP/HK
to Cyclops severe during FS. The fallback allowed 18xx heavy too.
Finally, `attackerAir` checked Y only; a grounded actor with a
just-above-floor Y after a landing could pass.

## New strict guards

The host conversion now requires ALL of:
- **Original attack is in the 1Axx native superjump-normal action family**
  and is the heavy HP/HK variation 1A04 or 1A0A
- Actual native attacker airborne state: RAM `actor+AA` high byte is
  nonzero, AND current actor Y is above native `actor+254` floor
- Newly confirmed physical HP/HK edge, not an inferred animation or
  carried physical input
- Original native hit onset and existing flying-screen / launch evidence
  as already required by the respective converter
- Same fighter IDs, move action and attack serial through deferred
  substate-2 resolution; no grounded or 18xx delayed conversions

Previously approved native Storm / Cyclops severe actions are not
globally changed; this gates **host Tournament writes only**.
Version 3/4, Classic, native ROM reactions, ordinary jump velocity,
hit damage, player input, stage/camera registers and HUD stay unchanged.

The previous post-FS grounded HP/HK override is intentionally removed
by this user-requested rule change. Grounded or ordinary-jump
hits still replace the 'last connected button' in the combo ledger,
so LP/LK/MP still interrupt an SJ HP→HK sequence. The active FS latch
still survives until native victim landing; it simply cannot authorize
a non-superjump hit.

`xmvsf_stage87_80_flying_velocity.h` and
`xmvsf_stage87_83_rom_flight_guard.h` are opt-in experiment headers
and not invoked by the current `cps_run.cpp` runtime path. The
actual active Tournament host writer is `XM8786Flight::state().frame()`.

## Validation

Fast preflight `test_tournament_fs_superjump_scope_409.cpp` checks:
- ground LP/MP/HP/HK and ordinary jump LP/MP/HP/HK leave victim native
  action, animation profile and both 16.16 velocities untouched,
  even during existing FS
- stale grounded / 18xx hit resolutions cannot fire either deferred
  host conversion path
- 1A04 no longer qualifies when native airborne AA has cleared
- a genuine airborne 1A04 superjump HP→HK still becomes Cyclops severe
- changing the originating superjump move before resolution cancels

Older 87.230, 87.231 and 87.223 regression fixtures have been updated
to reflect the new airborne-flag requirement and user-specified
non-superjump exclusion. The old 'Storm knockdown for every FS hit'
and 'grounded HP/HK severe during FS' expectations no longer apply.

Local command:
```powershell
git switch feature/native-stage-capture-catalog-87-408
git pull --ff-only
.\BUILD_TEST.bat
.\RUN_TEST.bat
```

The new pure C++ smoke test runs before expensive native Make; the
compiled EXE must contain `XM87.409 FS_SUPERJUMP_SCOPE` in addition to
the earlier HUD, F6, Boomer and screenshot-catalog signatures.
No GitHub Actions, Stable overwrite, warm-cache clearing, or extra
full regression suite is introduced.

**Verification note:** 1Axx is the superjump-normal *action family*
established by current source. No current cross-character native
superjump-history flag has been validated here, so this deliberately
fails closed for 18xx/ground and relies on confirmed 1Axx + AA for
eligibility. If F10 later demonstrates a 1Axx action running in an
ordinary jump for a particular fighter, add that character's original
ROM jump-state evidence instead of guessing global modes.

Native Windows compilation and in-game visual/combat acceptance still
must be performed by the local build/test scripts.
