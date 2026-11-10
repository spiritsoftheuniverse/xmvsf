# XMVSF TEST 87.250 — frame-data stall and missing combo hits

## What is known (no new play trace)

The latest report after the controller fix is that some combo attacks are measured, but extended frozen samples are appended to the frame-data bars after a hit and some combo hits produce no frame-data row. The latest play did not generate an xmvsf_frame_clock TSV; this pass is therefore source-grounded, with targeted ROM-free regressions rather than claims about the user's last gameplay.

### Source defects

1. The 87.247 zero-hitstop global-animation stasis detector excluded only 2C/2E stage02 setup. Once a special advanced to stage04 or later, a simultaneous freeze of both native animation pointers/timers with no live offensive object was always charged to the native fighter clocks. This can add the frozen wall-time to a subsequent move card.
2. The combo event constructor required a sampled melee/projectile rising edge. It could lose a newly accepted native attack when opponent HP decreased on that same sample but a one-frame collision box was already gone.
3. Even after an attack's native ROM action ended, a lingering RAM melee hitbox could continue incrementing the old row's active count.
4. The UI painted every open melee card's green collision timeline up to the **current** fighter clock, not the last sampled active hitbox. A visually bloated green block could appear even when the separate attack duration itself had stopped correctly.
5. A native-only card had no observed button-to-hitbox timing but could still trigger a numeric startup float. This was an invented measurement.

## TEST 87.250 corrections

- A zero-+85 freeze is now excluded at 2C/2E stage02 **or later** only when both native animations are unchanged from the prior sample, neither fighter has a sampled melee/projectile hitbox or any linked projectile object, **and neither fighter's observed HP changed**. A confirmed damage sample always breaks the apparent freeze. If a projectile is still live or either fighter's native animation timer/pointer changes, clocks advance normally. This is deliberately conservative until new traces exist.
- A new ROM-accepted attack that causes **actual opponent HP loss** without a new offensive-box edge produces a contact-only card with a red hit marker, tracked native action duration and unknown active/startup box timings. An existing projectile object disqualifies this fallback, preventing unrelated projectile damage from claiming a new move.
- Fighter melee cards stop accumulating ACTIVE ticks as soon as their ROM action has genuinely exited, respecting the existing two-count contact handoff grace.
- Each frame-data card now stores hitboxLastFrame and lastProjectileFrame. The external panel uses those observed stamps rather than stretching open activity to its current global clock. Contact-only cards are labeled 'HIT CONFIRMED - BOX UNKNOWN'; missing and native-only startup data do not spawn invented numeric float popups.
- New local C++ test dev/dhc/tests/test_frame_data_combo_stasis_250.cpp covers 44 frozen stage04 samples, live projectile progression despite stationary fighter poses, missing box with confirmed combo HP loss, damage observed during an otherwise static-looking stage04 special, unchanged super multi-hit identity and stale hitbox after native exit.
- Updated older source-contract test dev/dhc/tests/test_frame_data_paint_order_243.py and the local TEST build script to prevent stale assertions from aborting preflight. Prior TEST 87.245–87.249 cases remain.
- Optional, capped trace (enable XMVSF_FRAME_TRACE=1 if needed later): xmvsf_frame_clock_250.tsv now includes per-player contact-only flag, last box observation frame and cumulative excluded freeze ticks. This is NOT required to compile/run or to use Frame Data.

## Verification limits

Changes are in the existing feature/menu-intro-reveal-audio-87-235 branch and are source checked. Neither the Windows native build nor the user's unseen latest gameplay was simulated. Keep Stable and all previous TEST binaries/object-cache warm. GitHub is source control only—do not use GitHub Actions.