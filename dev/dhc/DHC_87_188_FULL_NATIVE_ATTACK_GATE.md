# XMVSF 87.188 — restore full original Shinkuu DHC attack window

Evidence from the supplied 87.187 ZIP:
- 10 native Ryu Shinkuu roots, 9 completed DHCs: 5 in 2A28 ATTACK, 4 in 2A02 RECOVERY.
- Seven actual native 2A02 ATTACK inputs were rejected at frames 2145, 3987, 4435, 4888, 4924, 5348 and 5380. The 87.187 native observer recorded 282 POST_CPU ATTACK rows in 2A02, proving 2A02 is legitimate during Shinkuu.
- Frame 3987 had consumed punch history (C0=0021, CA=0020): the preexisting safe native 87.183 input gate remains mandatory. Clean-input examples include 4435, 4888, 4924, 5348 and 5380.
- The old direct window only allowed +4E=3C down to 1D. Input at 4924 (+4E=02) and 5380 (+4E=0B) would still be deferred or lost without extending the direct gate.
- The four late successful inputs were actually in the ROM 2E0A/step06 recovery action, prior to terminal script 0D3194, not after original native action exit.

Source changes:
1. Remove 87.187 invalid-mode 2A02 ATTACK veto; retain same-frame current input selection and startup request expiration.
2. Enable TEST-only full original 2E08/step04 attack from +4E=3C down to 01, with correct 2A02 or 2A28, clean native input and existing ROM script checks. Flag XM87188_TEST_FULL_ATTACK=1; opt-out restores old 87.185 direct gate.
3. First four attack frames 40..3D remain separately opt-in 87.186, and genuine STARTUP input is still never armed.
4. Preserve proven native 3200 bridge, Ken arrival/2E00, exact recovery bridge, projectile ownership and wrong-action abort without ROM state rollback.
5. Separate fresh native 2A02 and 2A28 attack test log labels; success still requires actual controller acceptance and real Ken 2E00.

Validation required:
- New native TEST is XMVSF_Test_87_188.exe; all logs/replays under test/87.188, original 87.186 and 87.187 data unchanged.
- Pure C++11 test_dhc_full_attack_gate_188.cpp covers 2A02/2A28 all 3C..01, consumed input, stale input, startup/recovery exclusions and 87.186 first-four boundary.
- Windows MSYS2 BUILD_TEST.bat, RUN_TEST.bat and historical F9 replay are not validated by source tests alone.
- Compare the seven blocked 87.187 attack commands and nine previous successes. One 87.186 2A02 attack had dispatched wrong 2C02 at 10629, so 2A02 success remains an empirical question, not an assumption. Preserve native abort and lock release on wrong dispatch.
