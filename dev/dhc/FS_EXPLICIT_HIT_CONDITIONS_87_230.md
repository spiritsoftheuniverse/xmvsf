# TEST 87.230 — explicit HP → HK and grounded heavy during flying screen

## Supplied 87.229.zip evidence

The user's `87.229.zip` includes 249 records in `observer/storm_like_flight_events.tsv`, overwhelmingly from fighter ID `0000`, ROM bank `0021`; it does not include a verified grounded-heavy onset. Five ordinary Storm-profile FS conversions succeeded, but **four directly relevant HP→HK pairs** were blocked:

| Prior HP confirmed | FS conversion | Following HK confirmed | Existing decision |
| --- | --- | --- | --- |
| 2780, `180A`, HP `4` | 2789 | 2795, `1A04`, HK `32` | `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING` |
| 3187, `180A`, HP `4` | 3197 | 3203, `1A04`, HK `32` | same |
| 3422, `180A`, HP `4` | 3431 | 3439, `1A04`, HK `32` | same |
| 3653, `180A`, HP `4` | 3662 | 3666, `1A04`, HK `32` | same |

The previous 87.228/229 generic native-relaunch protection missed this straightforward condition; 87.228's delayed severe writes on LP/LK were already proven undesirable. This TEST **replaces** the generic post-FS native `1C14/0210` relaunch penalty. Existing legitimate LP/LK/MP stagger and low-combo setups are not converted.

## Two exact severe conditions

The current fighter/victim must be in active combat and **already in flying screen**. FS is tracked from the existing host-applied Storm/Cyclops FS latch until real native floor/air contact, or from the ROM-native severe `1C18/0230` pair with a recorded plausible horizontal flight magnitude (not action alone).

1. **Previous connected hit HP → current confirmed air HK.** Use real driver button edges (`4` then `32`) attached to **consecutive confirmed hit reactions**, by the same attacker and victim. Confirm the current action is a heavy aerial normal (`1804/180A/1A04/1A0A`) and attacker airborne. If LP/LK/MP/MK actually connects between HP and HK, it replaces the stored previous-hit button and the sequence fails. Plain HK after non-HP also fails. This is independent of existing one-Storm-boost-only logic.
2. **Grounded ordinary heavy hits a victim already in FS.** Attacker must be genuinely on the native floor (or native zero-Y ground convention), with native `+AA` air flag clear and active actor state. Current ground normal action must be the normal ground family `100x`, `120x`, or `140x`, with heavy ordinal 4 or A and separately proven physical HP/HK `4/32`. **`1Axx` super-jump attacks are explicitly ineligible** for this grounded rule; neither air-normal `180x` nor Wolverine's protected `2C1E/2C46` drill qualifies. Grounded medium/light actions cannot qualify. Existing Sabretooth/Cammy HP exclusions remain.

Both conditions require **fresh original ROM impact onset** (`0C` hit reaction, native substate 0 after previous 2, an unconsumed attack serial, valid real input evidence) before any pending conversion can be recorded. The actual Cyclops severe `1C18/0230`, horizontal `±0x00066200`, downward `0x00132A00` is only written if that **same exact action and attack serial** reaches native substate 2 within 24 frames. Moving to LP/LK/MP or another attack, new hit, tag, landing, or combat end cancels it. No pending severe request is allowed to bleed into a different hit.

A native already-steep severe knockdown is preserved rather than unnecessarily overwritten. Charlie's existing original air HP and first HK Cyclops severe code are unchanged. Storm standard FS remains the other independent variant; no extra synthetic Storm boost is allowed during a native FS.

## Diagnostic improvements

The existing `storm_like_flight_events.tsv` is extended **only by appended columns**, leaving its earlier fields intact:

- `previous_confirmed_hit_button`
- `attacker_grounded`
- `fs_active`

Previously the log skipped post-hit severe writes if they occurred without a new attack edge or an explicit `launch` flag. TEST 87.230 now logs every `velocity_written` conversion and all `FS_CONDITIONAL_SEVERE_CANCELLED` events. New decision names:

- `FS_HP_THEN_HK_CONFIRMED_PENDING` → `FS_HP_THEN_HK_CYCLOPS_SEVERE`
- `FS_GROUNDED_HEAVY_CONFIRMED_PENDING` → `FS_GROUNDED_HEAVY_CYCLOPS_SEVERE`
- `FS_CONDITIONAL_SEVERE_CANCELLED`, `FS_ALREADY_NATIVE_SEVERE_PRESERVED`

This should make an unconverted hit distinguishable from a late spillover in the next local recording. Since the supplied capture contains no grounded-normal impact, **grounded 100x/120x/140x action-family identification is provisional** and needs a live run; the conservative action+input+ground+native-hit combination avoids pretending that a guessed action number alone proves impact.

## Native TEST / preservation

New branch `feature/fs-hp-hk-grounded-severe-87-230`. Local build `BUILD_TEST.bat` targets `runtime/stable/XMVSF_Test_87_230.exe` with the new ROM-free `test_fs_hp_hk_ground_230.cpp` instead of the obsolete 87.228/229 generic relaunch unit. Tests cover HP→HK, LP→HK, HP→LP→HK, attack-change cancellation before release, genuine grounded HP/HK, grounded super-jump rejection, mediums, no-FS state, native landing reset, and Charlie independence.

This is committed source and regression setup, **not yet a compiled or Windows gameplay-verified fix**. Stable, prior TEST binaries, and original CPS2 ROM are unchanged. No GitHub Actions.
