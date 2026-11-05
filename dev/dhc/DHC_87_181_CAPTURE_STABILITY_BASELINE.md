# XMVSF 87.181 — First repeatably stable DHC input baseline

Source: user's 87.181.zip, captured 2026-11-05, with the original
87.175 native actor observer, 87.180 subframe CPU observer, DHC event
trace, ROM dispatch trace, super chat, and one completed F9 V2
recording. This is **read-only analysis**; no new gameplay commits.

## Validated output

- **15 controller-8 handoffs armed**; 13 completed native Ken
  Shoryureppa action 2E00 with original script 0D6722. Each of the 13
  is independently corroborated by rom_dispatch.tsv and the native
  action logs. 13/15 = 86.7%, versus 4 completed out of 16 total
  queued commands (25%) in the previous 87.179 submitted run.
  The denominators differ: 87.179 counted all queue requests,
  including one pre-handoff block; 87.181 here counts ARMED tags.
- **Two aborted tags, in one problem cluster**: 7015 -> 7016 and
  7263 -> 7264, rather than an ongoing match-wide input deadlock.
  Both abort paths report DHC_178_STALE_HOST_259_LATCH_CLEARED and
  DHC_ABORT_RELEASED_CHAIN_LOCK. The next armed attempt at frame 7579
  reaches Ken 2E00 at 7602. Subsequent successes continue.
- **One additional intentionally abandoned delayed request**:
  the DHC queued at 2813 enters the waiting phase, then at 2815
  logs DHC_180_UNVERIFIED_DELAYED_PRIME_SKIPPED and
  DHC_180_DELAYED_HANDOFF_RELEASED_NO_NATIVE_EDGE.
  This was not an armed controller-8 failure. It shows the
  deliberately conservative 87.180 safe gate working, rather
  than successful early DHC support.
- Punch-pair counts among 15 armed tags:
  LP+MP 4/4, LP+HP 4/4, MP+HP 4/5, and 3P 1/2.
  All four supported punch combinations are demonstrated to succeed
  at least once; 3P is NOT yet perfectly reliable.
- F9 V2 recording was sealed successfully: total 9,218 frames in
  one session, two segments (117 and 9,101 frames), FINAL present.
- The 87.181 observer health report says NATIVE_OBSERVER_CAPTURED:
  13,864 native actor rows and 17,627 subframe rows. Both loggers
  were ready and combat was reached. Stage 87.181 fixed the silent
  no-TSV problem seen in 87.180.

## The two real breakups seen within the 68000 frame

The following values come from observer/native_subframe_test.tsv.
They distinguish native CPU action selection from later host DHC
writes and **do not prove a specific ROM instruction PC**.

### 7016 (physical MP+HP, queued at 7015)

- BEFORE_CPU: Ryu action 2E08, A0=002A, controller +256=08,
  +259=08, C2=0001, CA=0000.
- AFTER_PRE_DISPLAY_CPU: controller +256 consumed (00);
  new native punch edge CA=0060, C2=0001, action still 2E08.
- AFTER_SCAN_0: native input history has C2=0061 and CA=0000;
  Ryu has already chosen action 0002 and mode 0200.
- Host observes the wrong action, clears the DHC-owned +259
  latch and aborts; it **does not** force a tag or revert 0002.

### 7264 (physical 3P, queued at 7263)

- BEFORE_CPU: C0/C2=0041, reflecting HP already held together
  with directional input; Ryu action 2E08, A0=002A, controller
  +256/+259=08.
- AFTER_SCAN_1: controller +256 consumed, new LP+MP edge CA=0030,
  previous raw C2=0041. Native 3P gesture is normalized to 2P
  in the host/native port bridge.
- AFTER_SCAN_2: Ryu chooses native action 2C04, mode 2A00,
  D6=00. DHC cannot accept this as outgoing tag.
- Host clears only the DHC-owned +259 latch and releases the
  chain, leaving the ROM's original 2C04 untouched.

### Contrast: 7580 (physical LP+MP, queued at 7579)

- AFTER_PRE_DISPLAY_CPU: +256 consumed, CA=0030, Ryu still
  in 2E08, A0=002A.
- AFTER_SCAN_0: Ryu A0 changes to 2A00 with 2E08 intact.
- AFTER_DHC_HOST: outgoing action becomes 3200 and D6=14;
  the host bridge explicitly initializes the original 3200
  script AFTER native controller 8 acceptance. It would be
  incorrect to claim the ROM independently selected 3200 in
  the pre-host CPU sample. Subsequent Ken 2E00 / script 0D6722
  is genuinely selected by the ROM command path.

## Engineering conclusion and next step

**Do not rewrite or retime the 2P/3P input generator yet.** The 87.181
baseline is significantly more stable; the two failure outcomes are
native-arbitration races, not a universal absent button edge.
A new blind pulse may reintroduce the 87.179 regressions.

Next development pass should compare:
- pre-CPU controller latch, action 2E08, A0 and native prior C0/C2
- 68000 command/state transitions between PRE_DISPLAY and scan slices
- the already held-punch condition at 7263/7264
- the Ryu 2E08 -> 0002 reset at 7016, which happens even though
  a new punch edge briefly exists

A narrowly scoped native instruction-PC trace or ROM handler
instrumentation for the real command arbitration is justified.
Only add a gameplay change after we can show which 68000 branch
caused each wrong-action outcome. Protect 87.181 and Stable.

Evidence:
dev/dhc/evidence/ORIGINAL_87_181_DHC_STABILITY_15_ARMED_ATTEMPTS.csv
