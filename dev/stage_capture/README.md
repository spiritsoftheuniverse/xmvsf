# Stage 87.203: live stage-capture status in your existing MSYS2 terminal

Run in the ALREADY OPEN MSYS2 UCRT64 window:

    cd /f/xmvsf
    bash dev/stage_capture/WATCH_CAPTURE.sh

The watcher follows test/87.203/stage_capture/status.log, and accepts an alternate path as its first argument. It opens no new window, does not change game inputs, and does not alter Stable or GitHub Actions. Ctrl+C stops the watcher without stopping the game. NO_COLOR=1 disables terminal colors.

PART_DONE is blue (one independently verified part); LEVEL_DONE green (the entire level independently certified); INCOMPLETE yellow; ERROR red. CAPTURE_START and PROGRESS are neutral. The saved file stays plain UTF-8 with explicit event names.

A separate Windows game process cannot inject output into an arbitrary pre-existing MSYS window without a receiver. This follower is the receiver; it can run while the game is launched elsewhere.

The offline checker is:

    python dev/stage_capture/VERIFY_CAPTURE_STATUS.py --root test/87.203/stage_capture

It checks a stage builder's capture_manifest.json and SHA-256 artifact receipts. Five parts are required: scroll1, scroll2, scroll3, sprites, animation. Green also requires capture_closed=true, coverage_complete=true, replay_sealed=true, unresolved_gaps=[]. The builder must independently prove coverage and animation loops. The checker cannot infer unseen tiles or untriggered animations from artwork, and must never fabricate green.

This commit is the live-terminal/status-verification infrastructure, not a claim that the full native recorder or automatic level builder is finished. Files under test/ remain local and git-ignored.
