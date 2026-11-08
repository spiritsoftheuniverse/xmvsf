# TEST 87.233 — air-to-ground before original 68000 off-screen dash-in

## Actual supplied 87.232 capture: what worked and what is still missing

The user's local `87.232.zip` contains `observer/dash_startup.tsv` with 297 event rows. It records one positive **`FS_NATIVE_GROUND_OFFSCREEN_ENTRY` at frame 1858**, actor ID `0000`/ground mode `1400` (original X was invalid `50841000`). Later actor ID `0002` has many real, advancing ground-dash `1400/1402` frames but **no logged airborne-before-ground transition**. There is no executed pre-entry 68000 callback in this ZIP (the 87.232 `charlie_native_68000_pc_writes.tsv` contains only its header). The capture does *not* prove Wolverine's exact 68000 action or fighter ID.

Thus 87.232 works **after** the original CPU has selected ground-dash mode. A still-airborne, off-screen actor can miss that entire branch, remain in the air, and fall into the camera rather than using the existing return dash. The correction is to prepare native floor state *before* the 68k chooses ground dash, not change the player's voluntary dash timing.

The uploaded 87.232 source-backed opcode windows show:
- `0109B4: D56E 000C` — an `ADD.W D2,(0x0C,A6)`-family instruction writing the actor's world-X word. The PC trace in 87.231 showed this writer producing a wildly invalid X.
- `01284A: 1D40 00A0` — `MOVE.B D0,(0xA0,A6)`, the native ground-mode dispatch that produced `1400`.
- `01273E` and `012772` are known original native air-flag/superjump control sites from the earlier movement study; the 87.233 local opcode auditor includes these windows for comparison. This is *not* a claim that either is the verified off-screen landing subroutine.

## Implementation

In `xmvsf_stage87_233_airborne_entry.h`, a standalone per-fighter C++ state machine qualifies an **original-native offscreen return**:

- Same original active fighter and active opponent, inside match, valid fighter native floor `+254`, and plausible original world X on both actors
- Horizontal separation at least 384 native CPS2 view pixels +64 margin, using the **same conservative test as 87.232**
- Still airborne by `+AA` high-byte or actor Y `+10` not on native floor
- No native hitstun (`+06`), action below the established ground/air normal attack families beginning at `1000`, and no already active ground dash
- Two distinct original committed frames; do not treat a single positional blip as a return

The existing trusted 87.232 `settleReturnOnNativeFloor()` performs the **only** initial RAM correction: `+10` original floor, `+12` Y fraction cleared, `+1C` vertical velocity zero, and `+AA` high air byte zero (low byte preserved). No X/velocity or current 68k `+60` action, `+A0` mode, `+34/+38` native frame script, or `+3E` countdown is fabricated at this stage.

When the **original 68000** subsequently selects ground dash `1400/1600`, the pending 87.233 transaction hands the SAME actor to the already verified 87.232 `autoReturn` latch. The original native script and countdown remain untouched; ordinary player dashes continue using the accepted 87.220 immediate-start rule. If the 68k produces an invalid world X, or teleports the actor more than 128 pixels in the first two native handoff frames, the transition restores the last established off-screen **air entry anchor** instead of popping into the middle of the screen. Never use the opponent's animation/controller snapshot.

If the original CPU briefly reasserts airborne Y/flag before native ground dash, it can be grounded at most four additional times, with no synthetic dash. If ground dash does not appear within 24 native frames, the attempt is retired until the actor comes back into view. New hit, attack, tag, opponent loss, or real return to the fight cancels it. No continuous host forcing and no animation-state rewinds.

These are deliberate fail-closed conditions: airborne fighters actively performing a normal/special, superjumping *within* the horizontal fight view, or in native hitstun are not stripped of attacks or knockback just to satisfy an offscreen heuristic.

## New native logs

`test/87.233/observer/dash_startup.tsv` adds:

- `FS_AIR_OFFSCREEN_NATIVE_GROUND_PREP`: two-frame qualified airborne fighter was placed on the actual original floor, before any ground dash
- `FS_AIR_OFFSCREEN_NATIVE_GROUND_REASSERTED`: 68k briefly reasserted air state during the pending transaction
- `FS_AIR_TO_NATIVE_GROUND_DASH`: real 68000 ground dash arrived, staged as existing 87.232 native return
- `FS_AIR_TO_NATIVE_DASH_CORRUPT_X_RECOVERED`: returned native dash wrote an impossible X, corrected to preground offscreen anchor
- `FS_AIR_TO_NATIVE_DASH_WARP_RECOVERED`: native return made an abrupt on-screen X jump; anchor protected
- `FS_AIR_NATIVE_GROUND_DASH_MISSING`: still no original native ground dash after bounded opportunity, not silently marked successful
- `FS_AIR_RETURN_CANCELLED`: attack, control, opponent, or native combat state interrupted the transition

The original 68000 PC callback is extended to record **up to 12 native pending-air-return CPU frames per fighter**, tracing X, mode, action, air flags, original Y/floor, executed PPC, relevant decrypted opcode words and D/A registers. It adds `FS_AIR_RETURN` as a trace kind; older Charlie/FS_DASH tracking remains. The opcode audit output now goes under `test/87.233/observer/fs_entry_68000_opcode_windows.tsv`.

## Regression coverage, isolation, and verification

Branch: `feature/fs-airborne-offscreen-ground-return-87-233`; local build target: `runtime/stable/XMVSF_Test_87_233.exe`. The native build and test runners are the existing `BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat`, revised only for the isolated TEST version. Previous Stable and TEST binaries are untouched. No GitHub Actions.

The new `test_fs_airborne_offscreen_entry_233.cpp` runs the same mechanism for four different original fighter ID fixtures (not a guessed Wolverine ID); tests two-frame qualifying, native grounded Y/air clearing, unchanged 68k action/script/timer, true subsequent `1400` handoff, zero-wait voluntary dash still available, native bad X and large on-screen warp recovery, bounded re-ground and absent-dash timeouts, and exclusion of in-hit/normal-attack/on-screen superjump/missing opponent. It is wired **alongside** the older 87.232 test, the 87.231 Storm floor/severe regression, and the 87.220–227 dash tests. The standalone portable decision module compiled locally with GCC 14 in C++17 and passed focused synthetic checks. The full MSYS2 native build and real ROM behavior are not verified until an actual Windows run.

### Remaining original-68k question

Grounding the actor early is a grounded intervention supported by the established `+254/+AA/+10/+1C` RAM contracts. It does **not yet prove** that every airborne action/mode will make the original 68000 enter `1400`. The newly logged `FS_AIR_NATIVE_GROUND_DASH_MISSING` and `FS_AIR_RETURN` traces distinguish that missing CPU dispatch from a successful offscreen dash. If Wolverine's exact native air state never selects `1400`, use that evidence to identify its original recovery/landing branch instead of inventing a universal dash script or damaging legitimate aerial attacks.
