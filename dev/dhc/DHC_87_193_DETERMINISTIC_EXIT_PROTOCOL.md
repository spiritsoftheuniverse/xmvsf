# XMVSF 87.193 — deterministic outgoing native exit / mirrorable ownership

## Root cause: 87.192.zip native trace, NOT a Shinryuken ROM selection failure

All six reserve-owned QCF+PP handoffs succeeded: Ryu entered ROM action `3200`, Ken arrived with kick suppressed, and his original ROM selected `2E00` Shoryureppa. Both reserve-owned QCF+KK attempts failed BEFORE the swap:

- At host frame **1570** Ryu was in native Tatsumaki `2E02`/step06. Controller 8 was armed. At **1571**, original 68000 punch/history word `C0=0301`, `C6=0302` (kicks in upper bits), then native action `0214`/mode `0602` (forward+MK), not outgoing `3200`.
- At host frame **4236** Ryu was in native Hadoken `2E08`/step04. Controller 8 was armed. At **4237**, `C0=0301`, `C6=0302` and action `0002`/mode `0200` replaced the tag.
- After the host writes the DHC, the next frame's host port was **already neutralized** (inputSynthetic=1, injectedButtons=00). That neutralization occurs too late: the prior physical kick state is still processed by the native input history on the controller-8 frame.
- Successful ground QCF+PP DHCs instead showed native `C0=0031/C6=0032/CA=0030` or `C0=0061/C6=0062/CA=0060`, and action `3200` appeared on the very next frame. Their reserve command subsequently started the native super.

## One deterministic exit and two distinct input owners

### OUTGOING (native tag controller)

- `XM87193Exit::route(physicalButtons)` never recognizes any super by name. For two pure punches, preserve the original native punch pair exactly. For 3P or two/three pure kicks, route **only the outgoing actor's temporary CPS input port** to canonical native LP+MP `0x03`. Mixed chords or a single button cannot authorize a DHC.
- `XM87193Exit::beforeCpu()` decides this before `CpsRwGetInp`, using the existing original-ROM super candidate, tracked native action, TEST mode and idle DHC transaction. After `CpsRwGetInp`, the launcher restores the original physical SDL controls as it already did for the 3P shield.
- `XM87193Exit::beginNativeExit(ram,side)` is a SINGLE reusable outgoing operation: snapshot action/mode/script/D6/controller and step fields, then arm the ORIGINAL 68000 controller 8 and zero 06/08/0A and prepare A0=002A. It takes an actor side address rather than named Ryu/Ken move IDs. The native ROM must still select outgoing action `3200`; an incorrect action aborts without pretending a tag happened.

### INCOMING (original reserve-owned super)

- `XM87192Owner::Proposal` retains the player's **unchanged original motion and buttons**. For QCF+LK+MK, the outgoing controller sees LP+MP for native tag acceptance; Ken's later port receives the preserved QCF+LK+MK gesture. If Ken's original 68000 selects a valid `2E` hyper, it is accepted and catalogued; `2C` specials and ordinary moves remain rejected.
- No hand-coded 'Shinryuken action 2E02' selection is added. The original-ROM reserve parser remains the only authority on its move.

## Mirrorability / scope

The helper computes both point and reserve actor bases from `side` (`4000/4800` for P1; `4400/4C00` for P2) and accepts *either actor*. Tests verify controller8 changes only the selected actor's registers and leave the other actor, opponent and reserve untouched. The existing native Ryu→Ken executor remains the only activated gameplay route until the reverse direction's original tag-in state, coordinates and super parser are validated. Mirrorable code is not the same as claiming two-way fighter swaps are proved.

## Testing and A/B rollback

Build TEST with the existing `BUILD_TEST.bat`, then use `RUN_TEST.bat` / `RUN_REPLAY.bat`; binary `XMVSF_Test_87_193.exe`, logs under `test/87.193`. Default `XM87193_TEST_DETERMINISTIC_EXIT=1`. Set this variable to 0 in the parent Windows CMD to compare against the 87.192 raw-KK behavior without disabling the functioning 87.192 reserve-owned command discovery. Stable and all previous TEST builds remain unchanged.

New C++11 regression `test_dhc_deterministic_exit_193.cpp` tests: all three original two-punch combinations stay unchanged; 3P/2K/3K normalize to canonical LP+MP on the outgoing pre-CPU port; original KK remains reserved for incoming; no normalization outside an actual DHC super or with mixed/single attacks; mirrored actor addressing; unchanged script, D6 and reserve RAM; snapshot and controller8 preparation in one function.

Expected native evidence from F9: frame after reserve QCF+KK should show **C0/CA native punch edge**, then real outgoing `3200` rather than Ryu `0214`/`0002`, followed by Ken native `2E02` with its original ROM script `0D6976`. That last result is an expectation to test, NOT a verified 87.193 result. If native input is still consumed incorrectly, use the recorded native PC trace to correct the precise CPU port stage; do not fall back to arbitrary timers or a hardcoded Shinryuken animation.
