# XMVSF 87.254 — ROM acceptance and stable wrapping frame bars

## Why supers appeared only after portrait release

Earlier versions saved the input timestamp but allocated a frame-data card only after encountering a sampled melee attack rectangle, projectile object, or HP contact. Thus startup was retrospectively measured from the input edge but the bar itself did not exist during the portrait/cinematic phase.

## New signal: native super acceptance, not static QCD input decoding

- Observe native CPS2 state `mode=2Axx` and action family `2Exx` on every ROM callback before early hit-stop/cinematic returns. A transition into `2E` is the authoritative super root (internal `2E02/2E10` substeps are not new supers), even if `attackPhase&0x80` was absent, for example action `2E08` during Cyclops's aimed super.
- Use a separate sampled native-super root latch rather than the paused native clock's last action, so portrait freezes never repeatedly create new cards.
- Create one provisional card on native acceptance. If a mapped button input preceded acceptance, preserve that original input wall/native tick, button, and motion tag. Otherwise label ROM and start from the observed native acceptance tick (do not invent a button).
- The card is immediately drawable even when no attack box or projectile exists. It begins with unknown collision-box timing and no fabricated active startup. Actual melee collision or a newly linked projectile later promotes the SAME card; it does not create a replacement.
- Repeated buttons while an accepted super is in progress do not reset the original frame-data input timestamp. A later genuine native 2E root after exit creates a separate card.
- A previous super's projectile must not lend contacts to a new pre-box super. Existing red contact-proof logic from 87.253 is retained.
- No character-specific action whitelist and no action/input-driven gameplay changes; this is read-only instrumentation.

## Fixed-width wrapping instead of retrospective rescaling

- Each host observation occupies **one horizontal pixel**, including dark-gray hit-stop ticks. A completed full-width lane is never squeezed, erased, or overwritten as new frames arrive.
- Overflow continues on the next stacked **pair** of hitbox/ROM-action bars underneath the earlier pair. The old 7-pixel bars are reduced to 3 pixels each with 18 pixels per wrapped row for event marks and red contacts.
- IN / ROM, SP, ON, OFF, END and red impact markers map onto their corresponding wrapped row using the same host-frame axis. Measured native startup/active/ROM durations remain separate from wall ticks.
- Card heights grow with their wrapped content. The side panel shows as many complete newest cards as fit, favoring preservation of a long active super rather than overwriting its earliest samples.
- Geometry is shared in the SDL-independent `XM30TimelineWrap` helper; new C++ regressions check the first row's coordinates remain invariant after subsequent wraps.

## Verification

The commit includes `dev/dhc/tests/test_frame_data_native_entry_wrap_254.cpp` with ROM-free scenarios for 2E08 entry without phase/input, accepted QCD input preserved, fixed wrapping geometry, a new native super after a genuine exit, and native-super cancellation of the preceding fighter-attached normal box. Legacy unit tests that intentionally expected no card before first projectile/box were updated to the new contract.

The opt-in `xmvsf_frame_clock_254.tsv` adds the last native-super input-wall tick and pre-box state per fighter. Source checks cannot prove Cyclops gameplay; Windows `BUILD_TEST.bat` / `RUN_TEST.bat` verification remains local. GitHub is source control only, Stable is not overwritten, and 87.249 controller policy was not touched.
