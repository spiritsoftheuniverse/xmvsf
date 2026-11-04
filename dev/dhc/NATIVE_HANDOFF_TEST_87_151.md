# Stage 87.151 — native Ken super startup, all-button input attempts, and DHC rearm

## 87.150 evidence (user's uploaded 87.150(1).zip)

- Ryu's native outgoing 3200/tag swap works; Ken's first 2A0C kick is replaced on frame 2318 by a kickless 2A08 native exit.
- At 2320 DHC changed Ken to D6=0A with +A0=2A28, but **left the actor on 2A08**. On frame 2321 the actual 68000 selected **2C32 / ROM script 0D667A**, not the queued verified Shoryureppa startup 2C00 / D5C46 or 2C04 / D5E2E.
- F10 observed Ken moving on 2C32 then 2C34 until 2348; hit70 remained zero. The old `(action&0xFF00)==0x2C00` check incorrectly logged SUCCESS for the entire 2C family even when no requested super opening had been demonstrated.
- At 2427 fighter ownership reverted to Ryu. The old handler reported KEN_SUPER_LOST_POINT_SLOT and aborted with `chainUsed=true` still set. New genuine Ryu supers at 2556/2787 and queued reserve commands at 2667/2880/2918 could not get through the chain gate.

## 87.151 experiment

1. Continue preserving the working native tag exit and first-frame kickless 2A08 suppression.
2. After 2A08, **initialize Ken's ROM-resolved 2C00/Shoryureppa or 2E00/Shinryuken opening script** using the existing native 68000 animation-field initializer; keep corresponding native controller D6=0A/0C and mode A0=2A28. This is *not* a guessed graphic or standalone animation.
3. Do not call a broad 2Cxx/2Exx family a success. Confirm only the selected **exact native action**, native D6/A0, and actual progression of its matching ROM script into the next animation record.
4. If Shoryureppa instead returns the previously captured 2C32/2C34 non-hitting dash on the first ROM frame, allow **one** bounded second native opening attempt through ROM-resolved 2C04/D5E2E; restore the opponent-relative starting position and zero inherited dash velocity. Failure returns to an input-retryable idle state instead of remaining locked.
5. Release DHC chain ownership on abort, native point swap, verified return to neutral, combat reset or observation timeout. New attempts still require a fresh reserved-super input edge during a separately verified outgoing Ryu super; ordinary native tags are untouched.
6. Observe every direction and one-, two-, three-, or mixed-button attack edge for both P1 and P2; recognize QCF/QCB/DPF/DPB/HCF/HCB and double QCF/QCB **even with no known super mapping**. Log attempted motion/chord in `super_chat.tsv`. Only verified Ryu/Ken entries can currently be selected from the match-start ROM queue; unknown fighter IDs must NOT inject fake supers or spend meter.
7. Preserve the independent Tournament rules and Classic mode gating. No DHC work modifies their native hooks.

## Build and test

On the current experimental `feature/dhc-mvc2-stage87` branch, pull and use only the original root `BUILD_TEST.bat` and `RUN_TEST.bat`. They target a new, dedicated executable `runtime/stable/XMVSF_Test_87_151.exe`; **never overwrite** `XMVSF_Stable.exe` or any prior TEST build.

Inspect `test/87.151/dhc_native_handoff.tsv` for `DHC_KEN_NATIVE_SUPER_OPENING_SCRIPT_PRIMED`, `KEN_2C32_DASH_REJECTED_TRY_NATIVE_2C04`, `DHC_KEN_NATIVE_SUPER_ROM_ACTION_CONFIRMED`, `DHC_ABORT_RELEASED_CHAIN_LOCK`, and the rearm events. Inspect `test/87.151/super_chat.tsv` for `DHC INPUT ATTEMPT` for single-button, mixed-chord and alternate-motion probes; `test/87.151/tournament_rules.tsv` should still report active flags when Tournament is selected.

Try Ken QCF+PP Shoryureppa, then Ken QCF+KK Shinryuken, each after a separate genuine Ryu super. On a failed first attempt, a subsequent input must not be rejected because of the old chain. The isolated C++11 tests are synthetic; **native Windows combat success is not claimed until another user playtest**.

No ROM bytes or other proprietary game assets are added.