# TEST 87.226 — Charlie HP native / HK Cyclops severe, flying-screen dash reentry, and centered boot window

## What 87.225.zip proves

This user's supplied local gameplay capture spans approximately native combat frames 1,890–10,674.

**Charlie:** The Tournament observer identifies Charlie as fighter ID `0020`, native bank `0028`. The `1804` connected air HP input had physical button tag `4` and the `1A04` super-jump air HK input had tag `32`. These are *different button identities* despite the shared/historic 68k action conventions. The 87.225 artificial flying-screen converter still generated `CHARLIE_STORM_FIXED_BASE_XY_150PCT` on both actions. It did not settle the repeat-pressure question.

**Dash:** `observer/dash_startup.tsv` documents a jump from ordinary stage coordinates around `0x02Bxxxxx` to **`0x7041D600`** at frame **10,026**, entering Charlie's ground dash `1400`. The **36-frame 87.221 watchdog fired 12 times**, at frames **10,062; 10,099; 10,136; 10,173; 10,210; 10,247; 10,284; 10,321; 10,358; 10,395; 10,432; 10,469**. Each time the next emulated frame immediately recreated the same dash, repeatedly returning to `0x7041D600`. Therefore the watchdog *does fire*, but does not clear recurring original-CPU dash reentry or the invalid world-coordinate state. The run exited successfully (`RUN_TEST_EXIT code=0`); this was not a process crash.

## Requested flying-screen behavior

Explicitly separate two native-reference writers in `xmvsf_stage87_86_storm_button_guard.h`:

1. **`applyStormFlyingScreen`**: retains the earlier 87.223 Storm-style measured horizontal and gentle Y flight, together with its native recorded reaction/index. All other eligible Tournament connected air-heavy finishers keep this variant.
2. **`applyCyclopsSevereFlight`**: original 87.86 Cyclops down+MP-type severe knockdown, `1C18/0230`, horizontal `±0x00066200`, downward `0x00132A00`. This is reserved for **Charlie confirmed air HK**, after the existing native connected-air follow-up qualification.

**Charlie air HP** (`0020/0028/1804`) is excluded from host synthetic FS entirely; the original 68000 hit reaction and velocities remain unchanged. Charlie's physical HK edge must be unambiguously `0x20`, and the action must be one of its permitted air HK forms (`1A04` documented in supplied trace). Mere action `1A04` or bank `0028` without button evidence does not receive the special severe variant. The previous 87.224/225 Charlie 1.5× multiplier is no longer used. The native already-steep slam protection, one synthetic boost until actual native landing, and Classic-mode safeguards remain.

## Dash repeated-reentry recovery

The 87.220/221 dash code keeps instant legitimate dash startup and the original 36-frame safety watchdog. TEST 87.226 adds:

- **Last sane X reference** for each active character, retained only from their own native controllable state at plausible stage coordinates (16.16 X below `0x10000000`). This is a conservative range used to detect the recorded huge `0x7041D600` fault, not a screen edge.
- If a **ground dash** begins/continues with a grossly invalid X, a fresh same-character ROM neutral snapshot, and last sane X from within 600 frames, restore **the real native neutral script/action/index and that last sane X**, clearing horizontal velocity and acceleration.
- After such a recovery, or after an **immediate ground-dash reentry within five frames of the old 36-frame watchdog**, quarantine repeated ground dash states until the original 68000 produces **three consecutive non-ground-dash frames**. Normal unrelated attacks, jumps, air dashes, tags, and native hitstun are not rewritten by this guard.
- Preserve the original 87.221 watchdog for less obvious stuck dashes and the 87.220 zero-wait *legitimate* dash startup. No ROM opcodes are modified.

New `observer/dash_startup.tsv` decision labels: `FS_DASH_INVALID_WORLD_X_RECOVERED`, `FS_DASH_REENTRY_SUPPRESSED`, `FS_DASH_REPEAT_AFTER_WATCHDOG`. The existing `FS_DASH_LOOP_EXIT` remains for the initial watchdog transition.

This explicitly addresses the observed 12 repeated 37-frame loops rather than merely shortens their timer.

## Initial boot-window centering

`xmvsf_stage14_boot.h` already created the shared window with `SDL_WINDOWPOS_CENTERED`, but the user's restored-size startup still appeared aligned left. TEST 87.226 centers the *actual desktop window rectangle* on the work area of the nearest Windows monitor **after creation/restored sizing, before the first presented menu frame**. Uses `GetWindowRect`, `MonitorFromWindow`, `GetMonitorInfo`, and `SetWindowPos` on Windows, with an SDL display-bounds fallback. This runs only for a newly created startup window, not on game entry, menu return, maximize, or after the user manually repositions it. No saved geometry or video render dimensions are modified.

## Test and preservation

New local ROM-free tests:
- `test_charlie_severe_hk_226.cpp`: native unchanged HP, physical-HK Cyclops severe, ambiguous-HK and non-Charlie Storm preservation, natural original steep reaction.
- `test_flying_screen_dash_reentry_226.cpp`: the `0x7041D600` world-X jump, repeated immediate dash revival, three native non-dash frame rearm, ordinary dash still immediate, fallback 36-frame watchdog.

Build: `BUILD_TEST.bat` → `runtime/stable/XMVSF_Test_87_226.exe`; run with `RUN_TEST.bat`. The old 87.224 Charlie x1.5 source test remains committed for historical comparison, but the 87.226 build runs the new expectations. Stable, previous TEST executables, and the original ROM remain unchanged. GitHub is source/version control only; no GitHub Actions.

Native Windows compilation and real-match verification still require the user's local run. If any original-ROM control flag also causes reentry, the new dash logs will show whether quarantine persists; the current captured evidence identifies invalid X and repeated reentry but does not prove the specific executing 68000 opcode that initiates the corrupted coordinate.
