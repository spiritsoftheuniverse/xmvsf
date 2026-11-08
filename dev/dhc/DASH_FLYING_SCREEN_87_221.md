# TEST 87.221 — native flying-screen dash loop exit

Evidence: user-supplied 87.220.zip, `observer/dash_startup.tsv`. Storm native fighter ID `0004`, mode `1400`, starts at frame 12527 and remains in dash until 12981 (~455 frames). Her dash script repeatedly returns from `000AC2EC` to `000AC268` and X position jumps back to the same origin, creating endless automatic dashing. Other recorded dash sessions are around 20–29 frames.

Fix: keep 87.220 instant dash startup. For each fighter, remember an actual ROM neutral controller and its animation/script pointers while genuinely idle. Only after **37 consecutive committed native frames** in a ground dash, restore that same fighter's captured neutral controller and zero residual X velocity. No guessed idle animation pointer. No changes to world position, opponent, flying-screen state, Cyclops throw, or air dash. A fighter without a valid native neutral snapshot is left untouched rather than forced into a potentially corrupt state.

Scope: native combat phase, original fighter identity, original dash mode 14/16. Normal dashes under 37 frames never reach the watchdog. An exit is logged as `FS_DASH_LOOP_EXIT` in `test/87.221/observer/dash_startup.tsv`.

TEST build and replay scripts output `XMVSF_Test_87_221.exe` and preserve prior binaries and F9 recordings. Native Windows build/gameplay must still verify the recovery for Storm and other characters.
