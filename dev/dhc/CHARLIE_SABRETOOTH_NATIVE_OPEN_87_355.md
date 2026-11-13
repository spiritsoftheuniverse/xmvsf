# XMVSF 87.355 — open Charlie QCB+KKK and Sabretooth HCB+KKK

Branch feature/dhc-morphus-precommit-87-334. Original US xmvsfu. Version 4, same four existing native DHC opt-ins only. Stable and Version 3 untouched.

## Exact original commands
- Charlie reserve: Down, Down-Back, Back + KKK (real native QCB command 2, D6=06).
- Sabretooth reserve: Forward, Forward-Down, Down, Down-Back, Back + KKK (real native HCB command 6, D6=08).

These are already present in the original-native D6 table. `observer(9).zip` has authentic Charlie QCB+KKK successes and many attempts without a completed reverse motion. We are not adding an alias, synthetic attack, or invented Hyper selector.

## Changed gate
87.354 stopped all DHC motion sampling in normal gameplay to prevent Chun-Li entering during Charlie's Flash Kick without an intentional command. Retain that boundary. When the outgoing 68000 is actually in the exact original 2C/2E Hyper (same actor, D6, script and previous frame), the two requested reserve fighters may start their original motion during STARTUP or a continuing original native Hyper action, in addition to the previously narrow ATTACK window. Remaining meter, living reserve, owner, on-screen original-ROM state and no-tag flags still apply. All other incoming characters retain the old gate.

At first eligible Hyper frame, 87.354 discarded the entire frame. 87.355 seeds ONLY the actual starting cardinal direction for Charlie QCB or Sabretooth HCB when that initial input exists: original Down or Forward respectively. It records NO KKK edge and cannot accept a button already held before the Hyper; a new, actual KKK edge is still necessary. No motion is sampled in normal play and no pending DHC can start a second gesture. Intentional DHC still requires original US M68K point/reserve swap and native 2C/2E Hyper confirmation.

The existing standalone C++ regression fixture now covers first Hyper frame QCB/HCB, exact 5-direction HCB, both facing directions, and negative held-before-window KKK and normal-play cases. The local batch launcher/build version is 87.355. No new mandatory build checks, automatic promotions, GitHub Actions or Stable modifications.

## Native confirmation
With the existing four environment flags, BUILD_TEST.bat then RUN_TEST.bat xmvsfu, Version 4. Observe `NATIVE_341_REAL_SUPER_INPUT` pointing to incoming Charlie motion=2/d6=06 or Sabretooth motion=6/d6=08, followed by the genuine original M68K swap and original 2C/2E. Charlie Flash Kick with NO separate KKK cue must not tag Chun-Li. Windows compilation/gameplay have not been executed here.
