# XMVSF TEST 87.251 — Cyclops projectile supers

## New evidence: xmvsf_frame_clock_250.tsv (2,400 host samples)
- Cyclops native action 2E00 hits the defender 13 times between host samples 130–189 (32 HP lost), yet the frame-data event count remains 3.
- Another 2E00 sequence hits 11 times between 373–422 (51 HP lost), yet the event count remains 4.
- At host 59, 2E00 stage02 has a newly accepted pending 2E action; by its beam's first damage at 130 the pending action family has been replaced with 00, while the pending origin is now 2E. This is direct evidence of lost startup ownership.
- This capture contains Cyclops and the dummy, not Ryu. Ryu's Shinkuu Hadouken is a user comparison, not a fact measured from this TSV.
- Existing projectile presence and HP loss are real; a missing kind-5 active hitbox does not prove a collision box never existed.

## Corrections
1. Retain a still-pending, ROM-accepted 2E super through later attack-button presses while its 2E family is still current. Do not drop its original mapped input, action, spawn candidate or move startup. New presses after the event is created, or after the original 2E action exits, remain eligible for future attacks.
2. If an accepted new native action damages the opponent but no offensive box is captured, permit a contact-only row for a freshly linked projectile identity. Explicitly reject borrowing an old linked projectile from a prior move.
3. Contact-only projectile records preserve the *actual linked object base* and observed spawn when available. The row's active hitbox duration remains unknown (0 measured), and no numeric startup popup is invented.
4. Opt-in 87.251 clock trace distinguishes linked projectile object count from sampled projectile attack boxes, and shows the pending spawned object base.
5. Add four ROM-free regression scenarios in test_frame_data_projectile_super_251.cpp; wire into the local BUILD_TEST.sh. Preserve the 87.246 repeating-super one-card contract and the 87.249 controller baseline.

## Verification
GitHub source code only. Build via BUILD_TEST.bat and run on Windows via RUN_TEST.bat. No GitHub Actions, stable overwrite or gameplay simulation claimed.
