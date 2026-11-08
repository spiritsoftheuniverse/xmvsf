# TEST 87.232 — original 68k off-screen ground re-entry; preserve voluntary zero-delay dash

## Evidence from the user's 87.231.zip

The supplied `observer/dash_startup.tsv` contains **754 event rows**, including one prior `FS_DASH_LOOP_EXIT` and 46 `FS_DASH_NATIVE_CONTROLLER_RESTORED` events. The older recovery was still repeatedly resetting a neutral actor controller, yet the original 68000 recreated the same broken position and dash.

### This is two different failures, not one dash-speed problem

**1. False watchdog:** At native **frame 6570**, fighter ID `0002` is still moving left in native `1400` ground dash: X=`00FD5A00`, VX=`FFFF0000`, and the previous frames show X progressing `01005A00 → 00FF5A00 → 00FE5A00 → 00FD5A00`. The old 87.221 watchdog unconditionally terminated the dash at 36 elapsed frames. This was NOT a stuck idle dash and could interrupt a legitimate longer off-screen return.

**2. Native 68k re-entry source:** The 87.231 executed PC observer records **four original CPU frames, 11042–11045**, with the same writer sequence on the actual fighter A6=`FF4000`:

| Executed PPC | Native effect |
| --- | --- |
| `012844` | controller mode `0002→0000` |
| `010392` | actor action `0008→0000` |
| **`0109B4`** | original CPU world X **`01350700→B27C0700`** |
| **`01284A`** | original CPU mode **`0000→1400`** (ground dash) |
| `01038E` | native ROM action `0000→0400` |

The PC trace is *executed-instruction evidence*, not merely a static opcode guess. However, it alone does NOT prove which 68k operands/branch predicate make `0109B4` write the wrong X. Restoring nine actor fields, or even the old 0x100-byte native idle snapshot, cannot repair an upstream CPU path that is reexecuted every frame. That explains why 87.226–87.231 still logged suppressed automatic reentries.

## New behavior (experimental until Windows gameplay)

1. **Voluntary native dash:** Retains 87.220 zero-startup-wait timing (`+3E→1` when stationary and in its ordinary dash). No invented velocity, script, input, ROM patch, dash facing, or player position.
2. **Long moving native dash:** No 36-frame cutoff merely because of duration. A safety exit now requires 36 actual stationary X samples or repeated native script wraps **back near the original world X**. A moving entry is allowed to finish under the ROM. Already-corrupt X retains the independent immediate guard.
3. **Original CPU-selected off-screen ground dash with valid X:** If the native ground-dash actor is at least the CPS2 native view width (384 pixels) +64px beyond the active opponent AND the original actor still shows airborne Y/flag, use only the actor's own `+254` native floor to ground it. Clear current vertical velocity and the *air high byte* of `+AA`, preserving its other flag byte. Keep its original off-screen X, mode, frame script, action, and current countdown. The native return's original setup interval is NOT shortened.
4. **Only when the actual 68k X has become impossible** (e.g., `B27C0700`): Require a valid earlier same-fighter X, valid original native floor, active real opponent, and grounded native dash. Compute an outside-view world-space entry anchor **448px away from the opponent, on the fighter's previously established side**, then preserve the original ROM dash mode/script/countdown, ground it on its own native floor and let the ROM drive the dash. If any reference is invalid, use the older conservative recovery and do NOT invent a camera coordinate.
5. **Unresolved repeated native corruption:** Give the original 68k a bounded four-frame opportunity to progress from the staged entry. If `0109B4` still recreates the invalid coordinate on every frame, restore the previously verified native neutral controller and keep the actor grounded at the off-screen entry point rather than spawning them in the visible center. This is a **diagnosed remaining writer problem**, not a claim that a host fake dash solves it. All reentry mode/identity/session changes reset the new state.
6. **No global forced grounding:** On-screen attacks, standard jump/superjump, air dashes, hitstun, throws, and normal voluntary dashes do not enter this special entry path. The native code does not rewrite the original 68000 binary or impose universal camera bounds.

### Logs that distinguish outcomes

`test/87.232/observer/dash_startup.tsv`:
- `FS_NATIVE_VALID_OFFSCREEN_GROUNDED` — original 68k already chose a sane off-screen X; grounded without X write
- `FS_NATIVE_GROUND_OFFSCREEN_ENTRY` — bad 68k world X replaced by grounded off-screen native dash staging
- `FS_NATIVE_ENTRY_68K_REASSERTED_X` — the 68k still rewrote the X next frame
- `FS_NATIVE_ENTRY_UNRESOLVED_68K_SOURCE` — bounded native retry failed; controller recovery applied
- `FS_DASH_LOOP_EXIT` — now reserved for actual non-progress/return-to-origin loops rather than normal long dashes

The existing `charlie_native_68000_pc_writes.tsv` appends actual decrypted opcode words at executed PPC, next three words, 68000 PC, D0–D7/A0–A7 register values, original Y and native floor. Its older columns remain intact. The host trace wraps and forwards the existing Musashi callback and only samples up to four FPS-recovery CPU frames per fighter, as before.

Since `local_rom_dump/xmvsfu_decrypted_opcodes_host_swapped.bin` and the matching program-data dump already exist in the repository, `dev/dhc/tools/ANALYZE_FS_ENTRY_68K_232.py` reads those **locally**, without saving/distributing ROM bytes, to produce `test/87.232/observer/fs_entry_68000_opcode_windows.tsv`. It records exact words around PPC `010392/0109B4/012844/01284A/01038E/0137B2`, the control path supported by the original PC trace. This auxiliary audit has no build gate and uses no GitHub Actions.

## Test/build and limitations

Branch `feature/fs-native-offscreen-reentry-68k-87-232`, build target `runtime/stable/XMVSF_Test_87_232.exe`. Existing `BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat` now use this isolated TEST version, leave old TEST executables and Stable untouched, and wire the ROM-free `test_fs_native_offscreen_entry_232.cpp` alongside existing dash/FS regression tests.

The new test checks 72 frames of **moving native dash** without watchdog cancellation, a separate two-cycle script/X-origin repeat that **still must trigger anti-infinite recovery**, instantaneous voluntary dash, real native off-screen X with ground normalization, bad-X `B27C0700` anchored outside the opponent's view, original native entry frame timer and script untouched, four bounded native retries and fallback, no-op when references are missing, HP untouched, and byte-swapped 68k opcode extraction.

**Not yet certified:** This environment cannot execute the user's local Windows 68000 gameplay or inspect the large GitHub binary dump through the connector. Actual source opcode words are written by the included offline script when it runs locally. The live capture must confirm whether grounding changes the upstream branch and finishes the return naturally, or whether `0109B4` still repeatedly asserts bad X. If the latter, those exact opcode/operand/register windows are needed to patch the genuine native branch instead of guessing at flags or masking a perpetual re-entry. Do not describe 87.232 as fully gameplay-verified.
