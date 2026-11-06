# XMVSF 87.197 — Opponent-only freeze across real DHC tag-in

## Evidence from uploaded 87.196.zip

The 87.196 native DHC handoff TSV proves **three** DHCs reached an authentic native point/reserve swap, a genuine Ken 0212 gravity-driven jump into the existing target, real ground contact, and a native incoming super. The actual ROM timings:

| Case | Ryu original 3200 selected | Ken real swap | ROM Ken 0212 falling | Real landing | Accepted Ken ROM 2E |
|---|---:|---:|---:|---:|---:|
| 1 | 1748 | 1761 | 1762 | 1778 | **1785 2E00** |
| 2 | 2195 | 2209 | 2210 | 2225 | **2232 2E02** |
| 3 | 3247 | 3261 | 3262 | 3300 | **3307 2E02** |

Every successful incoming 2E begins 7 native frames after landing. The original native engine remains active during the 37–60 frame handoff, which means the other combatant may resume acting while Ken lands and parses the super command. The user requested a freeze across the transition.

## Implementation

**Do NOT pause the FBNeo emulator CPU**: stopping the complete CPU also stops Ken's original 0212 movement, the true ground-contact signal, and the original 2E command parser. Do NOT guess a CPS-wide freeze RAM flag from a partial ROM disassembly.

Opt-in `XM87197_TEST_OPPONENT_FREEZE=1` now captures the **opposing native point fighter** (0x4000 for P2 DHC, 0x4400 for current P1 DHC) at the instant the host confirms Ryu's original outgoing `3200` action. It stores that actor's exact original **0x400-byte** native record, including position, animation, control and damage fields. After each original ROM CPU frame during phases `WAIT_NATIVE_SWAP`, `WAIT_LANDING`, `WAIT_NATIVE_PASSIVE_JUMP`, `WAIT_KEN_EXIT` and `DELIVER_NATIVE_INPUT`, the host restores **only that opponent actor** to its frozen snapshot, masking offensive attack descriptors +70/+72/+82 in the held RAM. The before-CPU hook re-masks the same descriptors without modifying Ken, any player input port, native ROM, game clock, or the global emulator scheduler.

The freeze releases **on the same frame the real incoming Ken ROM 2E action is confirmed**, not on a guessed 20/60-frame timer. The original Ken super startup owns its native cinema from that point. Both the 87.192 reserve-ROM-classified 2E route and the older explicit ROM super target route release correctly.

**Safety / failure semantics:** Failed controller8/3200, any DHC abort, incoming actor loss, opponent ownership change, noncombat state, or 180-frame safety watchdog releases the host freeze. The watchdog only releases the opponent; it never manufactures a super, landing or frozen global timer. If another fighter replaces the opponent, the snapshot is discarded and is NEVER copied over that new actor. The original native opponent record is restored on a normal release, including its attack descriptors. Actor ID 00 (Wolverine) is allowed.

The incoming Ken actor, original Y gravity +20=6000, per-frame X target steering, and reserve-owned original 2E input always keep running. The outgoing projectiles retain the old 87.159 cutoff exactly. The main menu's local MvC1 background GIF at ~30% brightness remains unchanged.

## Explicit limitation

This is **selective opponent freezing**, not proven original CPS2 **full-world freeze**. The CPU, independently linked projectile objects, stage animations and match timer continue. Freezing the wider world correctly requires separately discovering the genuine game-owned native super-freeze scheduler and its object/time ownership; guessed global freeze memory writes or pausing all CPU frames would break Ken's native 0212 fall. The isolated test cannot prove the in-game visual/collision behavior; only a fresh 87.197 Windows F9 capture and gameplay observation can.

## Diagnostics and testing

DHC handoff TSV columns now append `oppFreezeActive`, `oppFreezeHeld`, `oppActorId`, `oppAction`, `oppX`, `oppY`, and events:

- `DHC_197_OPPONENT_FREEZE_AT_NATIVE_3200`
- `DHC_197_OPPONENT_FREEZE_RELEASED_ON_NATIVE_2E`
- `DHC_197_OPPONENT_FREEZE_RELEASED_ABORT`
- `DHC_197_OPPONENT_FREEZE_SAFETY_RELEASE`

`dev/dhc/tests/test_dhc_cinematic_opponent_hold_197.cpp` verifies only the opponent actor record is frozen; Ken X/Y continues; offensive hitbox masking; true exact snapshot restoration; action/actor ownership safeguards, 180-frame release-only watchdog, Wolverine 00, mirrored sides and opt-out on a normal tag.

The existing **only** root launchers `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat` target `XMVSF_Test_87_197.exe`, preserving Stable, earlier TEST files, warm object cache, and the original F9 runner. `XM87197_TEST_OPPONENT_FREEZE=1` is the TEST default; set `XM87197_TEST_OPPONENT_FREEZE=0` in the shell before launching `RUN_TEST.bat` for exact 87.196 A/B. No Stable promotion.

Verification requested: In a real 87.197 F9, confirm `oppFreezeActive=1` at actual 3200, persists while `WAIT_NATIVE_PASSIVE_JUMP` and `DELIVER_NATIVE_INPUT` advance Ken, then becomes zero immediately after `DHC_NATIVE_INPUT_DISPATCH_ACCEPTED`. Confirm opponent cannot act/hit prematurely and releases cleanly if the incoming ROM rejects a non-super command.
