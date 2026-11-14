# XMVSF TEST 87.354 — no DHC sniffing during normal play

Branch feature/dhc-morphus-precommit-87-334, US xmvsfu Version 4 only. Experimental native DHC opt-in flags unchanged.

Fault: the 87.353 native Slot::sample method parsed every normal-play physical direction, spin and KKK edge even while eligible=false. Its accumulated input history could later be reused when an original native 2C/2E Hyper began, causing an unintended swap such as Chun-Li entering Charlie's Flash Kick super without an intentional DHC.

The caller now uses Slot::quiet for EVERY normal combat frame when no verified eligible native DHC window, or when there is no living reserve or meter. quiet clears all native DHC motion/KKK/spin buffers, clears any unclaimed input suppression, and only snapshots current held buttons. On the FIRST eligible native Hyper frame, Slot::openWindow discards any buttons already held; subsequent frames may recognize only a newly entered original DHC command. CLAIMED/CHAIN_ENTERED/COMMITTED never run a new directional matcher, only release bookkeeping while an already authorized original M68K handoff finishes. The full-round unified physical DHC monitor is disabled in native mode.

The native ROM action, independent normal inputs, intentional incoming D6 mappings, original CPU tag, underlying Hyper, and Version 3/Stable are untouched. No additional mandatory build tests or GitHub Actions. A local-only C++ fixture covers normal Charlie KKK input, first-frame held KKK ignored, subsequent intentional QCF+KKK producing Chun-Li's original D6 08, and cleanup of buffers after return to normal.

Windows build and actual game behavior remain unverified here. In the existing Version 4 opt-in, reproduce Charlie Flash Kick without entering a separate DHC input: Chun-Li must stay out. Then release and deliberately enter Chun-Li QCF+KKK during the native Hyper window: original native DHC should still work. Existing log test/87.354/observer/dhc_unified_274.tsv must not claim a DHC in the first case.
