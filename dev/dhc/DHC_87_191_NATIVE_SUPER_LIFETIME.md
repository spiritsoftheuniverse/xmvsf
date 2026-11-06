# XMVSF 87.191 — DHC eligibility equals the actual native super lifetime

## Why

The user's 87.189(1) F9/native capture contains 29 completed native handoffs from 32 armed attempts, across ground and air Ryu supers. The remaining arbitrary phase, early/late timing and air-mode eligibility filters are no longer an appropriate basis for the mechanic.

## Single availability rule

Once the original 68000 ROM enters an actual per-fighter ROM-verified 2E super root, and UNTIL its actual native action/controller exits, a fresh DHC gesture uses the existing Ryu→Ken controller-8 handoff. No +4E timer, A0 low-byte mode, named Hadoken/Tatsu animation, air/ground pose, root script step, or guessed recovery start/end is an availability gate.

The first ROM root frame opens the candidate; a separate human DHC gesture on the present frame is required. The original super-launch gesture cannot also serve as a delayed DHC. The original Ryu 32/00 outgoing tag, Ken kickless entrance, ROM Ken super input, fighter swap and projectile cutoff remain the same proven code.

## Remaining guards are correctness, not arbitrary timing

- Must be P1 Ryu with Ken as verified reserve; must have a verified reserve ROM super; only one active DHC; no normal-tag mutation.
- Must be a genuinely new present-frame selected input. No stale motion or held punches carried from startup.
- Preserve native consumed-punch C0/CA preflight. If the 68000 already spent the chord, do not forcibly hijack its unrelated action.
- The original CPU chooses whether controller-8 selects native outgoing 3200. If it chooses 0002, 2C02, 0214 or another action, the existing abort and chain unlock execute. Never counterfeit success or rewind the ROM.

## Native 3P input protection

The existing normal-dash collision is handled uniformly throughout a confirmed super: pre-CPU 3P is normalized to canonical native 2P using the last exact ROM-owned action+script and controller. This ends at original super exit, and does not affect ordinary gameplay outside a DHC candidate. Air 2A06 is no longer blocked merely for differing from 2A1C/2A28.

## Test and rollback

All C++11 lifecycle tests now require fresh input acceptance during the observed ground/air/Tatsu root and follow-up stages, plus exact abort on exit and no arming from stale input. TEST file names are versioned 87.191 and logs go under test/87.191; standard BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat stay intact. The 87.190 tested branch and Stable remain untouched.

Set XM87191_TEST_OPEN_SUPER_LIFETIME=0 in the parent CMD to compare previous 87.190 eligibility. Default 1 in TEST. An actual Windows native F9 replay is still required to learn what the ROM itself accepts in each phase; the test does not assert unverified CPU behavior.
