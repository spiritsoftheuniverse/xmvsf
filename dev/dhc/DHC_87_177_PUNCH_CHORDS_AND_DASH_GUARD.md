# XMVSF Stage 87.177 — All punch pairs, 3P DHC, no accidental dash

**First inspect the dash path:** In the existing 87.176 host matcher,
`XM87140Chat::matchListed` requires a button popcount of **exactly 2**,
rejecting LP+MP+HP. Its comment identifies native 3P as a dash chord,
3K as a super-jump chord, and mixed P+K as a team action. This check
was stricter than intended for a DHC occurring during the first super.
The user's 87.175 log includes LP+MP+HP at frame 1972, rejected for
missing motion/reserve mapping, **not** a direct observation of 3P
causing dash during a valid Ryu→Ken DHC. Do not claim that was proven.

## New DHC gesture (no motion skip)

During Ryu's **existing Shinkuu Hadoken** DHC input window, the reserve
Ken Shoryureppa command now recognizes the following **QCF** motions:

| Physical final punch chord | Mask | Host DHC accepted |
|---|---|---|
| LP+MP | 03 | yes |
| LP+HP | 05 | yes |
| MP+HP | 06 | yes |
| LP+MP+HP (3P) | 07 | yes |

One punch, mixed punch+kick, and every other 6-bit chord are still rejected.
All three kick buttons remain **disallowed** as a DHC super-jump; existing
two-kick Shinryuken matching is unchanged.

The host's validated character/motion queue still selects the correct
ROM-owned Ken super. 87.177 does **not** add arbitrary super targets,
change meter logic, or reinterpret unrelated fighters' moves.
It does not make 3P alone into a no-motion shortcut.

## Native dash suppression, pre- and post-queue

The ROM's **physical** P1 punch buttons and the host's **DHC gesture**
are different concerns. 3P is never presented to the original 68000 as
a DHC super command; Ken's already proven native QCF+LP+MP stream stays
exactly the same.

1. BEFORE CPS2 input sampling (`CpsRwGetInp`), if and only if
   Ryu is point (12), Ken reserve (14), action 2E08,
   mode 2A28/2A02, a verified DHC super candidate is open,
   the DHC early-cancel experiment is active, no prior DHC phase
   is running, and **3P but no kick** is held: present physical direction
   but neutral punches to the ROM input port. Immediately restore the
   physical SDL buttons after sampling so the host matcher still sees
   the user's original QCF+3P and can enqueue it.
2. AFTER a DHC command has been queued, 87.176's phase/roster
   input isolation already shields the outgoing Ryu and the Ken tag
   from old physical punches. That behavior remains.
3. DURING Ken's original fast **9-frame** QCF+LP+MP stream,
   the native input generator is unchanged; the shield stays off.
4. AFTER those 9 frames, while still awaiting native super
   acceptance, neutralize P1 again so a player holding 3P does not
   accidentally trigger a dash. Restore unrestricted physical
   controls only after the DHC phase concludes or aborts.

This is **not** a blanket dash ban. Original dash works during
ordinary gameplay; the restriction is exclusively the user-confirmed
experimental Ryu→Ken DHC sequence. No new direct 68000 input-history
writes, action-2E animation forced dispatch, charge, long jump-in,
or stage reset were introduced.

A chat marker confirms the before-queue protection:
`XM87.177_PREQUEUE_3P_DASH_SUPPRESSED_BEFORE_NATIVE_INPUT`.
87.176's existing input isolation marker and the 87.175 pre/post
CPU/host observer remain active for regression comparison.

## Code and tests

- `src/burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h`:
  pure chord validation (all 64 input masks) and prequeue dash guard.
- `src/burner/sdl/xmvsf_stage87_140_super_chat.h`:
  accepts any 2P/3P **only** for already cataloged punch super.
- `src/burn/drv/capcom/cps_run.cpp`:
  pre-CPU three-punch shield; physical host buttons preserved.
- `src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h`:
  native Ken punches explicitly use canonical 0x03.
- `src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h`:
  protect against 3P held beyond the Ken input sequence.
- `dev/dhc/tests/test_dhc_punch_chords_177.cpp`:
  ROM-free checks for 64 masks, prequeue scoping, native 2P and 9-frame
  protection. Standalone C++11 checks passed.
- The old project root launchers remain BUILD_TEST.bat, RUN_TEST.bat
  and RUN_REPLAY.bat. A successful incremental build creates separate
  runtime/stable/XMVSF_Test_87_177.exe; never overwrites
  XMVSF_Test_87_176.exe, 87.175 or Stable.

## Verification gate

Code and standalone C++ tests have passed; full Windows MSYS2 FBNeo build
and deterministic 87.175 F9 replay are **not yet run** in the analysis
container. Real acceptance needs these observations:

- QCF+03/05/06/07 each queues Ken Shoryureppa from Ryu super.
- Native 3P dash never appears during DHC but remains available in
  non-DHC gameplay.
- Ken's 2E00 / 0D6722 still starts naturally, with no charge,
  no long cross-screen jump and no forced 2E script write.
- F9 replay captures remain unchanged; Stable remains untouched.
