# XMVSF TEST 87.252 — shared native-super projectile and hit-confirm tracking

## Evidence

The user-provided `xmvsf_frame_clock_251.tsv` contains 2,400 sampled frames and six Cyclops native `2E00` super sequences: host 353–500, 727–875, 1331–1480, 1802–1950, 2062–2207, and 2240–2385. The first super-card offensive box occurs about 70 host samples after native entry. Several 2E sequences register 9–14 defender HP changes. This trace contains no Ryu/Ken super sample; the user's Ryu/Ken red-tick failure is separate reported gameplay evidence rather than a fact measured by this TSV.

## Shared rules (no character/move-name exception)

1. A ROM-accepted `2E` super plus a freshly linked projectile object can create a provisional frame-data card before any kind-5 attack rectangle is sampled. An already-existing unrelated projectile does not count as a new object. The collision-box startup and active duration remain unknown until the first real observed box.
2. Newly linked objects observed while the same native `2E` attack is owned are attached to its original super record; the loss of its first object does not prematurely finish a multi-object beam or projectile super. The fighter's independent native-action duration still ends when ROM ownership ends.
3. Red contact ticks are assigned to the matching live native super or linked projectile owner, rather than simply the most recent card. A super projectile remains eligible to receive damage markers after its fighter has exited the attack. The visual timeline includes confirmed late contact frames.
4. Early hits sampled during native hitstop with no existing card are buffered against the accepted native super root and transferred when the attack earns a card. Do not attribute pre-box hits from old unrelated projectiles to the new super.
5. A provisional no-box record is promoted to measured active collision only on an actual sampled attack box, not on object existence or health loss alone. Multi-hit contacts do not spawn duplicate super cards.

## Regressions and scope

Added `dev/dhc/tests/test_frame_data_super_universal_252.cpp` with five ROM-free tests: object-only aimed beam transitioning to measurable box, successive linked objects with damage after native 2E exit (Ryu-style), melee super hitstop and red ticks (Ken-style), rejected old-projectile attribution, and deferred frozen pre-box contact. Retains previous 87.239–87.251 regression suites.

Only frame-data observer, presentation bounds, local regression tests, and TEST version/scripts were changed. No modification to combat simulation or 87.249 controller policy. GitHub is source control only: execute `BUILD_TEST.bat` / `RUN_TEST.bat` locally. Source commit alone is not proof of successful Windows native compilation or Ryu/Ken gameplay behavior.
