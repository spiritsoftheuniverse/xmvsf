# TEST 87.229 — stop delayed severe knockdown leaking from HP/HK into LP/LK/MP

## User-supplied 87.228.zip: exact onsets and late writes

The log contains **1,091 rows** in `observer/storm_like_flight_events.tsv` and **32 host velocity writes**: 28 original-style Storm flying-screen conversions and **4 87.228 post-flight Cyclops severe writes**. All four of those new severe writes occurred with **LIGHT attacks active**, never a verified heavy finisher.

| Severe-write frame | Previous Storm-FS conversion | Native launch onset and attack | Attack active at severe write | Outcome |
| --- | --- | --- | --- | --- |
| 8193 | 8137 | 8184, air LP `1800`, button tag 1 | air LK `1A00`, button bits 8 | `POST_FS_NATIVE_RELAUNCH_SEVERE_LANDING` |
| 8862 | 8806 | 8852, air LP `1800`, button tag 1 | air LK `1A00`, button bits 8 | same |
| 10055 | 9996 | 10045, air LP `1800`, button tag 1 | air LK `1A00`, button bits 8 | same |
| 10327 | 10272 | 10318, air LP `1800`, button tag 1 | air LP `1800`, button bits 8 (next attack transition beginning) | same |

The severe event happened **9–10 frames after** the LP onset, at victim's hit substate 2. The original 87.228 guard armed solely because native victim action/index was `1C14/0210` and its post-flight landing latch was still active. It **did not check the input strength or bind the pending conversion to the originating attack**. This is why staggered attempts to transition from a heavy airborne finisher into a light/medium air-to-low string received unexpected heavy knockback.

All rows in this capture have attacker ID `0000`, bank `0021`, victim ID `0006`; this recording proves the phenomenon on that particular fighter pairing and should not be misrepresented as confirmation of every Charlie or Wolverine attack input. The original confirmed Charlie HP/HK implementation remains unchanged.

## Correction

In `xmvsf_stage87_86_storm_button_guard.h`:

- A post-flight **native upward relaunch** can arm severe knockdown only when the **current hit** is a heavy action (`1804`, `180A`, `1A04`, or `1A0A`) with fresh **unambiguous physical HP or HK button evidence** (`0x04` or `0x20`). ROM action codes alone do not qualify. Charlie's already approved native air HP and explicit Sabretooth/Cammy HP exclusions remain protected.
- The pending conversion records the **attacker's exact action and current attack serial**, alongside attacker and victim identities. If the action changes, if another attack starts (including another edge of the same move), if victim/attacker changes, if the native hit state expires, or if the victim actually lands, the pending severe conversion is **cancelled**.
- LP (`1800/1A00`), LK, MP (`1802/1808/1A02/1A08`), MK, and ordinary soft followups **cannot arm the post-flight severe**. They remain under original native ROM behavior; the code does not rewrite their velocities or reaction states after the fact.
- The accepted original Storm and Cyclops flying-screen functions, initial Charlie HK Cyclops severe, Charlie HP native behavior, dash recovery, and renderer-menu handoff are preserved. This change targets only the **extra 87.228 delayed post-flight relaunch** writer.
- When a qualifying **new** HP/HK does itself launch the opponent natively, it may still receive the Cyclops severe finish **during the exact same attack**, with label `POST_FS_SAME_HEAVY_NATIVE_RELAUNCH_SEVERE`. Never apply the old label `POST_FS_NATIVE_RELAUNCH_SEVERE_LANDING` to later lights.

## Regression

The existing `test_post_fs_native_relaunch_228.cpp` is upgraded as the 87.229 cached ROM-free unit and now tests:

- An initial native launch and valid first Storm FS.
- Ten successive native LP/LK up-launch + hit-substate-2 transitions, **none** artificially given severe knockback.
- A native medium followup preserved.
- A fresh, confirmed physical HK with a continuous unchanged attack **can** get the severe finish.
- A verified HP followed by LK before native hit step 2 **must cancel** the pending severe without changing its already native X/Y/action/index.
- Charlie's accepted initial Cyclops HK severe remains intact, initial HP native unchanged, LP after HK native, and another truly new HK handled separately.
- Actual native floor/air status still clears the post-flight gate.

Target `runtime/stable/XMVSF_Test_87_229.exe` on branch `feature/fs-relaunch-attack-gate-87-229`. Stable and old TEST binaries remain untouched. No GitHub Actions; Windows build/gameplay tests have not yet run.

**Tradeoff:** This intentionally removes the 87.228 attempt to stop an infinite by penalizing **native light relaunches**. The previous attempt was demonstrably interfering with legitimate light-to-low setups. If a genuine LP/LK infinite remains, it should be diagnosed separately with a timing- or recovery-specific rule; reintroducing severe knockback on an ordinary light hit would be a regression.
