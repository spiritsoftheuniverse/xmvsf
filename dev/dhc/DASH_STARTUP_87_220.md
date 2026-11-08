# XMVSF TEST 87.220 — dash startup waiting removed

Branch: `feature/zero-delay-dash-87-220`, from merged 87.219 on main (`29529c1cfabf3cb082fcfd2e65be8b525c99db68`).

## Evidence and change

The repository's prior 87.47 original-68000/RAM study identifies `+A0` high byte `14` as native ground dash, `16` as the reverse candidate, and `3C` as native air dash (dispatcher `0137B2`). Other original-engine work maps `+34` to the current animation-script frame and `+3E` to its countdown. The original ROM executable and its animation script table are not modified.

TEST 87.220 removes extra per-fighter animation waiting **only after the native ROM has accepted a dash**. In the current original dash state, the live animation countdown is shortened to **1**, its minimum safe next-update value; writing literal 0 risks underflow or a held frame. Ground dashes continue to remove holding while the original fighter remains X-stationary, ending at the first real X movement. Air dash has entry-only reduction because pre-existing jump velocity makes a stationary test unreliable. No guessed dash velocity or hardcoded character timing.

No input or DHC input-port changes. No actor action/controller coercion, X/Y/velocity writes, or ROM writes. Cyclops 87.219 remains unchanged; this is native animation-time control independent of the throw recovery.

## Gameplay verification outstanding

This is a targeted, conservative **startup wait reduction**, not yet proof of first-frame dash displacement. To certify literal zero frames of dash startup for each fighter, native Windows TEST/replay must compare the press edge, ROM transition to 14/16/3C, original X displacement onset, and the edited onset. If a fighter's startup is implemented by a non-animation timer, its wait may remain; do not change unrelated movement clocks or state arbitrarily.

An optional diagnostic TSV is written when `XM87220_DASH_LOG_FILE` is set before launching RUN_TEST.bat. The tester records native entry, original/suppressed countdown, ROM script pointer, and first actual X movement.

`dev/dhc/tests/test_dash_startup_220.cpp` checks native A0 state gates, per-fighter equality, preserved motion frames, air-entry isolation, ROM sentinel protection, hitstun/input cancels, non-XMVSF and menu isolation.
