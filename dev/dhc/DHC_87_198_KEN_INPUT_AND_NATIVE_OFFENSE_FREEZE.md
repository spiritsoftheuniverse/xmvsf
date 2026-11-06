# XMVSF 87.198 — Ken's proven reserve motions and freeze-through-native-offense

## Actual source: uploaded 87.197.zip

Observed original ROM handoff log: **18 armed DHC requests, 14 genuine native swaps, 14 genuine Ken landings, 5 incoming ROM super accepts (2E00 or 2E02), 9 post-swap ROM non-super refusals (action 0006), 4 pre-swap wrong-action aborts**. This dataset is richer than earlier captures and retains both player-side contact positions, the actual original ROM controller, F9 replay, and 87.197 freeze instrumentation.

The `observer/reserve_owned_supers.tsv` input observations explain ALL nine dead incoming states:

- **5 / 5 confirmed incoming native supers** used reserve Ken `QCF+PP` (one) or `QCF+KK` (four). The original Ken ROM selected `2E00/D6722` or `2E02/D6976`.
- **9 / 9 incoming non-supers** used Ken `HCF+PP` or `HCF+KK`. The actual 68000 selected `0006/D3A4A`, not any original Ken 2E super. These were NOT Ken refusing a properly delivered super: they were *different captured motions*. Ryu had already genuinely tagged out before this was discovered.

Do NOT convert a human HCF into QCF or synthesize a Ken 2E move. In 87.198 TEST, `XM87198_TEST_KEN_HCF_PRETAG_REJECTION=1` adds a narrow reserve-Ken-only gate in `XM87192Owner::State::propose`: HCF with native same-type 2+ punches/kicks is rejected and logged **before** the DHC controller8/3200 or opponent freeze begins. Ryu continues his real super. Other fighter HCFs, Ken QCF and other motions retain the normal generic owner parser, and the original 68000 remains the sole super selector. `=0` A/B reproduces 87.197.

## Freeze cue from native Ken animation — NOT elapsed wall time

The prior freeze unconditionally released at `DHC_192_RESERVE_NATIVE_ROM_2E_SUPER_ACCEPTED_AND_CATALOGED`. This is Ken's native **startup**, not the initial attacking frame. The actual `xm87139_probe_0001.tsv` gives the first original-ROM 2E offensive hitbox (`hit70`) in every confirmed super:

| Ken original 2E entry | Native attack action | First real hitbox | Observed startup delay |
|---:|---|---:|---:|
| 2326 | `2E02` | 2378 (`hit70=0050`) | 52 frames |
| 10553 | `2E00` | 10600 (`hit70=0064`) | 47 frames |
| 11049 | `2E02` | 11102 (`hit70=0050`) | 53 frames |
| 11891 | `2E02` | 11943 (`hit70=0050`) | 52 frames |
| 17490 | `2E02` | 17542 (`hit70=0050`) | 52 frames |

87.198 retains the opposing native actor freeze through `SUPER_STARTED` (state 6) and releases **only when real Ken, still in original `2E` and `2Axx` controller, has an active step and the ROM's `+70` offensive hitbox is nonzero**. The helper `XM87198Cue::realKenSuperOffense` is selector-agnostic: it checks actual actor/mode/action/step/hitbox, not a character super name or a guessed 47/52-frame timer. Thus a real native super with different startup length releases at its own actual first attack.

The trigger is sampled after the real 68000 CPU has run. For this single release frame, the freeze ends **without copying the pre-DHC opponent snapshot back**: restoring a full 0x400-byte fighter record at that point could delete actual first-hit damage or hitstun that the Ken super just caused. On failed handoff, incompatible fighter, missing super or watchdog, the earlier snapshot restoration/fail-open safety applies unchanged. Original Ken 0212 motion, opponent-relative target, true ground contact, reserve-owned QCF command, outgoing projectile cutoff, original menu and ~30%-brightness GIF are unchanged.

## TEST and evidence checklist

Root `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat` remain with the same names and target only `XMVSF_Test_87_198.exe`. All test logs and F9 files remain under `test/87.198`. Stable and previous TEST binaries are never replaced.

The new isolated regression `dev/dhc/tests/test_dhc_ken_rom_owned_input_and_freeze_198.cpp` checks native Ken HCF+2P/2K denied before swap, QCF accepted, no invented selector, full 2E startup freeze, live offense cue for both 2E00/2E02, and preservation of the ROM's first-hit changes on release. The prior 87.197 freeze regression now explicitly treats phase 6 as still held.

With both new flags defaulting to 1 in RUN_TEST, an 87.198 Windows F9 should show: rejected Ken HCF proposals in `reserve_owned_supers.tsv` and NO real swap for those attempts; accepted QCF supers produce `DHC_198_NATIVE_2E_STARTUP_FREEZE_RETAINED` at 2E root and only later `DHC_198_FREEZE_RELEASED_ON_NATIVE_KEN_FIRST_ATTACK` at the native hitbox start. No fabricated super, no count-based thaw, and no RAM overwrite of the first real hit.

Limitations: this remains the deliberate **opposing-actor-only freeze** from 87.197; the independent match clock, stage animation and separate projectiles continue. The isolated C++ tests cannot substitute for a native Windows 87.198 gameplay/F9 capture proving visual timing and actual collision behavior. Ken HCF filtering is intentionally specific to the 9 recorded failures; a future new move with a verified different ROM input should not be inferred from this filter.
