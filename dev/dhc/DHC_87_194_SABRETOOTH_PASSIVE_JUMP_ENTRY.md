# XMVSF 87.194 — Sabretooth-style passive native jump landing for DHC

## Original evidence from the two user 87.193 ZIPs

`87.193.zip` contains 14 armed Ryu→Ken native DHCs. 11 produced genuine native 3200 and point swaps, 8 resulted in true Ken ROM 2E supers, 3 failed to enter 3200, and 3 swapped but ended in non-super ROM actions. Shinryuken `2E02` at script `0D6976` worked in multiple native observations. Preserving the source exit and original Ken parser is essential.

At native swap frame 1753, Ken first appears at x304, floor y472. At 1754 the ROM actually selects Ken incoming `2A0C` script `0D5702` at x304,y408 (64px ABOVE ground). The old DHC implementation changes Ken's action to `2A08` script `0D567E` but simultaneously writes X=692,Y=472, clears +14..+20 velocity/acceleration, and at 1755 sends him directly to `0000` idle. That is a ground teleport, not a native jump. The premature native idle transition is consistent with the reported stray dash/entry audio.

`87.193(1).zip` includes a separate short Ryu/Sabretooth run appended to some observer logs. At frame 1638 the original ROM swaps P1 point to Sabretooth ID 0A, which proceeds as action 0000, mode 0000, D6 00, original script `0BD212→0BD222→0BD232→0BD242`. This establishes an ORIGINAL passive neutral/stand entry reference, not a verified Sabretooth DHC tag-in animation script. Its appended per-frame probe is from another session, so the exact Sabretooth horizontal jump trajectory cannot be inferred from that file. No Sabretooth script is injected into Ken.

## 87.194 TEST approach

1. Preserve the entire 87.193 native Ryu controller-8 exit and real 3200 point swap unchanged.
2. On the actual Ken incoming 2A0C frame, capture his REAL actor X,Y,floor and native physical motion. Suppress only the attack script by resolving Ken's existing non-attacking 2A08, but DO NOT set Y to the floor, zero native horizontal/vertical velocity or enter neutral while airborne. Stage becomes WAIT_NATIVE_PASSIVE_JUMP.
3. The ROM owns native Y motion. Track frame-to-frame Y DESCENT and use that motion—not an invented animation timer—to progressively move horizontal X from native spawn to the same previously established opponent-relative target (foe X ±48, Stage-05 bounds 192..768). Do not move X if the ROM never visibly descends. Record each jump sample in dhc_native_handoff.tsv.
4. No captured DHC command reaches Ken while he is airborne: the native host CPS input port is neutralized during phase 12. At an ACTUAL floor contact following multiple native downward frames, set the already-known Ken native original ROM 0000/0000 idle controller. Only then zero movement, place his exact X at its existing target, and start his retained 87.192 original super command. No new Ken super chooser or hardcoded Shinryuken animation.
5. If 2A08's original controller does NOT descend in this situation, the branch reports DHC_194_NATIVE_ENTRY_NO_DESCENT_DID_NOT_FORCE_LANDING and aborts only DHC bookkeeping. It never fakes Y=floor or starts Ken's super while floating. Such an observation would mean we must locate the true nonattacking native aerial controller, not reinvent a jump timer.
6. Each new controller-8 DHC arm resets old jump state to avoid second-DHC false locks. Every abort clears pending reserve ownership and jump state. A true ROM action failure remains a genuine non-super refusal; only already verified successful swaps/ROM 2E supers count.

## Original-versus-new action distinction

- **Sabretooth native point swap**: original idle/stand 0000 and script progression 0BD212, no gameplay mutation. The visual claim of a natural jump is supplied by the user; current diagnostic packet lacks uninterrupted position/velocity traces of that particular entry.
- **Ken old DHC**: native 2A0C for one frame, then forced Y=floor, X=target, velocity 0, immediate forced 2A08→0000 next frame.
- **Ken new TEST DHC**: actual 2A0C entrance provides spawn/vertical state; nonattacking Ken ROM 2A08, original in-air motion preserved, horizontal landing guidance, actual floor touchdown, then native idle/stand and native reserve-owned input.

## Test, reversibility and logging

`BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat` continue to be the only root launcher names. Versioned executable `XMVSF_Test_87_194.exe`; build, logs and replay under `test/87.194`. TEST default `XM87194_TEST_NATIVE_PASSIVE_JUMP=1`, with all 87.193 previous switches intact. Set XM87194_TEST_NATIVE_PASSIVE_JUMP=0 in the caller CMD for exact 87.193 instant-entry A/B comparison. Stable and prior TEST versions are untouched.

New C++11 regression `test_dhc_native_passive_jump_entry_194.cpp`: first frame remains native spawn X and never claims ground; observed native descending Y incrementally moves to exact opponent-relative target; mirrored target and clamp; instant one-frame floor cannot masquerade as landing; reserve input is isolated during jump; no timer-defined permission to land.

**Not yet asserted as native proof:** 2A08 with original +14..+20 motion preserved must still produce real descending Y in a Windows live run. F9 archive should record full WAIT_NATIVE_PASSIVE_JUMP trace and final landing/super events. If the ROM stops advancing Y, the correct next move is to trace Sabretooth's natural entry controller and Ken's 2A0C scheduler, then choose the native passive airborne controller with actual evidence.
