# Stage 87.184 — Original ROM Shinkuu action lifetime

Source: user's 87.183.zip native F10 probe, DHC handoff, and 87.175/87.180 observers. No private ROM, F9 savestate or movie has been committed.

## Native action boundaries, confirmed from six uninterrupted supers

| Phase | Native action/step08 | Original script +34 | Measured frames |
|---|---|---|---|
| Startup | Ryu 2E08 / 0002 | 0D3070..0D30A0 | 66–67 |
| Main attack | Ryu 2E08 / 0004 | 0D30B0..0D30E0 | 56 |
| Recovery | Ryu 2E0A / 0006 | 0D30F4..0D3194 | 41–42 |
| Action exit | first non-2E08/2E0A | normal actor script | total 163–165 |

Original F10 frames, start → attack → recovery → exit:

- 1802 → 1869 → 1925 → 1966: 164 frames
- 2105 → 2171 → 2227 → 2268: 163 frames
- 2327 → 2393 → 2449 → 2490: 163 frames
- 2543 → 2609 → 2665 → 2706: 163 frames
- 2834 → 2901 → 2957 → 2999: 165 frames
- 3515 → 3582 → 3638 → 3679: 164 frames

13 additional recorded supers ended early because the DHC bridge took over. Those are labeled INTERRUPTED, never counted as original natural action durations.

**Previous problem:** The chat parser kept a candidate for as long as 200 frames and the playable DHC gate required only 2E08 with timer +4E <= 28. Actual Shinkuu continued into 2E0A recovery before ending. That meant recovery input was rejected, while stale candidate state could outlive the actual move.

## Source changes

The new header src/burn/drv/capcom/xmvsf_stage87_184_super_action_lifetime.h classifies the real original step/action/script combination. A tracker begins on Ryu 2E08/step02/root script 0D3070, follows native 2E08/step04 and 2E0A/step06, and ends when the ROM actually leaves the action. It never defines action existence by guessing a frame count, the temporary tag-controller timer, actor movement, landing or projectile lifetime.

The existing src/burner/sdl/xmvsf_stage87_140_super_chat.h uses this lifecycle for only the playable P1 Ryu-to-Ken candidate. Other fighters retain their previous read-only input-discovery behavior.

**Action lifetime is distinct from safe native tag acceptance.**

- Startup 2E08/step02: recognized as the real start, but the executable controller-8 handoff is not yet verified there. No blind synthetic punch is added.
- Active 2E08/step04: the already-working 87.183 native input, pre-arm C0/CA safety and near-end controller timing are retained.
- Recovery 2E0A/step06: a narrowly scoped TEST-only experiment can start the original controller-8 tag while its script is earlier than terminal 0D3194, if C0/CA is safe. It does not invent Ken super animation or add charge.
- At terminal script 0D3194 and after original action exit, no new playable DHC is accepted. An aged input waiting from attack phase is released rather than carried across the 2E08-to-2E0A transition.

The new recovery experiment is opt-in in the engine source. RUN_TEST.bat in the separate 87.184 TEST enables it with XM87184_EXPERIMENTAL_RECOVERY_TAG=1. RUN_REPLAY.bat disables it with XM87184_EXPERIMENTAL_RECOVERY_TAG=0 and uses safe 87.183 replay settings. Previous TEST executables and Stable are unchanged.

The strict 87.183 consumed-punch preflight, pre-CPU physical 3P-to-native-2P normalization, prior guarded abort/latch recovery, original no-charge Ken QCF+PP, close-range arrival and F9 recordings remain.

## Evidence, tests and next gate

- Source test: dev/dhc/tests/test_shinkuu_action_lifetime_184.cpp
- Reproducible probe analyzer: dev/dhc/tools/ANALYZE_SHINKUU_ACTION_LIFETIME_184.py
- All 19 original super episodes: dev/dhc/evidence/ORIGINAL_87_183_SHINKUU_ACTION_LIFETIMES_87_184.csv

ROM-free C++11 classification/regression passed, and the original F10 read-only probe analysis found six naturally completed plus 13 DHC-interrupted actions. These validate the action boundaries, NOT the success of a new recovery tag.

Full Windows/MSYS2 compile and live recovery-tag gameplay have NOT been tested here. Test the separate runtime/stable/XMVSF_Test_87_184.exe via the unchanged BUILD_TEST.bat and RUN_TEST.bat. The expected chat markers are DHC_184_RYU_SHINKUU_ACTION_ENTER_2E08_0D3070, DHC_184_RYU_SHINKUU_ATTACK_PHASE_2E08_STEP04, DHC_184_RYU_SHINKUU_RECOVERY_PHASE_2E0A_STEP06, DHC_184_RECOVERY_PHASE_NATIVE_TAG_EXPERIMENT and DHC_184_RYU_SHINKUU_ACTION_EXIT_NATIVE.

**Acceptance check:** all proven 87.183 attack-phase handoffs remain; a late QCF+2P/3P in recovery can be accepted only before the terminal script, and a failed experimental recovery handoff releases controls without a lock. Anything attempted after action exit must never queue. The actual native controller acceptance and Ken ROM 2E00 must be confirmed from the new trace, not inferred from this code.


## BUILD_TEST false failure repaired (2026-11-05)

The actual Windows BUILD_TEST.log showed Stage 87.184 compiled and linked
`src/burn/drv/capcom/cps_run.cpp`, `src/burner/sdl/run.cpp`, and
`src/intf/video/sdl/vid_sdl2.cpp`, then failed only when the builder
searched the linked `fbneo.exe` for the literal
`DHC_184_RYU_SHINKUU_ACTION_ENTER_2E08_0D3070`.

A compiled debug string is not a reliable proof that the source was
built or that a native DHC move works. The string check was a false
post-link gate, and its failure prevented publishing an otherwise
successfully linked TEST executable.

`dev/dhc/BUILD_TEST.sh` no longer requires any historical runtime
debug strings in the binary. It still stops on real `make` errors,
requires a nonempty executable, copies to a dedicated 87.184 pending
file, then renames that file to `XMVSF_Test_87_184.exe` only after
successful staging. Neither Stable nor older numbered TEST files
are overwritten. Incremental warm-cache compilation and the full
append-only `test/build_logs/BUILD_TEST.log` remain unchanged.

This repairs the builder's false failure only. Successful Windows
runtime, the action-timing fix, and experimental recovery DHC still
require live testing.
