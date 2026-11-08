# TEST 87.228 — native relaunch after flying screen, traced from 87.227.zip

## Runtime evidence, not a velocity hypothesis

User supplied `87.227.zip` (November 8, 2026). The real `observer/storm_like_flight_events.tsv` contains 2,129 records. In the main aerial capture, the existing Storm-profile flying-screen conversion logged a valid severe state/index (`1C18/0230`) with Storm's gentle launch motion (`VX=0x0010AA00`, `VY=0x0000B000`). Later, **19 confirmed connected heavy attempts** were correctly reported as `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING`; the host did NOT generate another synthetic flying-screen boost.

But that existing latch only suppresses additional **host-created** flights. The original 68000 repeatedly restarts an airborne victim in native `1C14/0210` with new light attacks, then `1C16/0220` soft followups. The capture around frames 3328–3911 contains many additional four-hit airborne strings before the original engine reports a real landing. For example, a new launch is observed at frames 3328, 3410, 3491, 3574, 3658, 3742, 3827 and 3911, with intervening heavy-hit attempts blocked by the old synthetic-flight latch but still connected natively. That explains the observed ~40-hit infinite: **native relaunch**, not failure to multiply velocity or to count one flying-screen boost.

A later fighter profile `0022`/`0028` has two successful Storm-profile heavy conversions at frames 17150 and 17426, showing that the broad fallback remains applicable across fighter profiles, but this log alone cannot prove every player-observed Wolverine/Charlie input or the full displayed combo counter. The dash log in this particular ZIP contains no `FS_DASH_NATIVE_CONTROLLER_RESTORED` decision; it does not validate or refute the separate 87.227 dash correction.

## 87.228 behavior

In `xmvsf_stage87_86_storm_button_guard.h`, preserve BOTH independent flying-screen writers from the approved 87.226 balance pass:

- Storm-style flying-screen remains the same: original `1C18/0230` pairing and measured gentle X/Y launch.
- Charlie air HP is untouched ROM-native. Charlie's first connected HK continues to use the previously accepted Cyclops severe `1C18/0230` and `0x00132A00` downward motion; **no changes to its initial result**.

After **either authorized flying-screen** starts, a per-victim *needs real native landing* latch survives the existing per-combo hit-count reset. Only when the real 68000 subsequently reports all of:

1. A **new confirmed airborne hit** before actual grounded floor/air status.
2. Victim's real native `1C14/0210` upward-relaunch transition, at hit substate **0**.
3. The same victim and attacker when that relaunch finishes its native substate **2** within 24 frames.

...does the host write the established Cyclops-style severe downward movement (`1C18/0230`, `VY=0x00132A00`) for the *relaunch* instead of letting another upward launch restart. This is labeled `POST_FS_NATIVE_RELAUNCH_SEVERE_LANDING` in `storm_like_flight_events.tsv`. It does NOT generate another Storm boost. Ordinary medium, light, or soft hit states after the initial flying screen are still allowed until the original game actually attempts an additional upward launcher.

The latch re-arms only after the real floor coordinate and native air flag agree on landing. It clears on victim tag, non-combat state, or true landing, not on a brief combo gap. No ROM opcode is guessed or rewritten; no immunity/OTG hack is introduced.

This is a new **experimental native relaunch anti-infinite**; it is a corrective post-CPU native state writer, not a claim that the original ROM enforces this rule.

## Tests and safety

`test_post_fs_native_relaunch_228.cpp` reproduces the observed pattern: a genuine native launch, connected air combo, first Storm-style FS, ordinary allowed medium followup, **ten successive native 1C14/0210 up-relaunch attempts** (the 40-hit scenario), and each actual release into Cyclops severe downward flight; checks native floor rearm and Charlie's unchanged original HP/direct HK. `BUILD_TEST.sh` includes it in the existing unit cache.

Target `runtime/stable/XMVSF_Test_87_228.exe` on branch `feature/post-fs-native-relaunch-87-228`. Stable and all prior TEST executables unchanged. No GitHub Actions. A local Windows build and gameplay run remain necessary to confirm whether the desired finite combo emerges and whether the new native launch gate is executed at the expected 68000 substate.

Useful runtime label to check: `POST_FS_NATIVE_RELAUNCH_SEVERE_LANDING` in `test/87.228/observer/storm_like_flight_events.tsv`; a post-flight native launch still occurring repeatedly with no such label would indicate another hit-onset/substate pattern that should be captured before any stronger edits.
