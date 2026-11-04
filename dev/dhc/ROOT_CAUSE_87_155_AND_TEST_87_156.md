# Native Ken DHC: what 87.155 proves and why 87.156 is an input-only milestone

This is not a new release-pack or replacement Stable. The repository branch
`feature/dhc-mvc2-stage87` contains only the continuing experimental TEST.

## Key observation in uploaded 87.155 capture

`dhc_native_handoff.tsv`:
- Ryu super active at frame 2004, 2E08/2A28; both Ken ROM targets resolve.
- At frame 2013 DHC chose raw +256/+259=08, +A0=002A. The ROM consumed
  +256 but remained Ryu 2E08, without the native 3200 tag or Ken ownership.
- 2026 timeout released the lock; at 2042 Ryu began **another Ryu 2E08
  super**, not a Ken super. At 3333/4178 the same raw controller staging
  led back to Ryu normal 0002, no native outgoing 3200.
- The capture proves actual Ken Shoryureppa at frame **2306**, action
  `2E00`, script `0D6722`, after Ken became active through the game's
  normal ownership swap. Both 2E00/0D6722 and 2E02/0D6976 resolve, but
  resolution alone is not action-controller invocation.
- The *real native* tag command is visible at frame **3052** as
  P1 six-button bitmask `24` = HP+HK. At 3053, ROM action becomes
  `3200`, mode `2A00`, D6=14. At 3067, point/reserve swap.
  This is concrete input-to-controller-to-swap evidence.

## Previous conceptual error

We conflated three things: (1) ROM animation/script pointers, (2) the native
action/controller dispatcher, and (3) the command/motion parser. An
animation pointer does not invoke the CPU routine. Writing controller 08
into an active Ryu hyper state does not reproduce a real native tag and
can select a different special or super.

We also sent QCF+**3P** or QCF+**3K** instead of a true native **2P/2K**
Hyper Combo. In the game, PPP means dash and KKK means super jump. QCF +
HP+HK is its own Variable Combination (two-character Hyper Combo). Three
punches appeared in user captures after the required two punches had
already been pressed; they were not proof that three were necessary.

## 87.156 controlled proof (NOT yet DHC)

1. During Ryu Shinkuu Hadoken queue Ken 2E00 with QCF+any TWO punches,
   or Ken 2E02 with QCF+any TWO kicks. Mixed P+K and all-three chords
   are deliberately not accepted as Ken super requests.
2. Wait for the ROM to finish Ryu's 2E08/2E0A and return to a natural
   neutral action. Never write Ryu +A0, +D6, +256, +259 or his animation.
3. Feed **HP+HK (0x24)** briefly through the *actual CPS2 input ports*.
   Verify that the ROM itself chooses 3200/2A00 and swaps ownership.
4. Let Ken's genuine entry, kick and recovery finish without writing his
   location, sprite script, A0, D6, or hitbox fields.
5. When Ken's real action controller is ready, feed native QCF+LP+MP
   (0x03) or QCF+LK+MK (0x18). Verify **Ken** and the exact 2E00/
   0D6722 or 2E02/0D6976 ROM action/script.
6. Abort safely on every timeout. Instrumented results are in
   `test/87.156/dhc_native_handoff.tsv`, `rom_dispatch.tsv`,
   `super_chat.tsv`, and the F10 frame recording.

The proof is intentionally **sequential**. It is only the correct first
step towards *true cancel-timed DHC*. Once both native commands and swap
are reproduced on actual Windows emulation, locate the 68000 dispatch
path by comparing the native pre-tag/super-controller states and move the
acceptance point into Ryu's super cancel window. Do not declare success
until incoming Ken **performs the requested native super** before the
outgoing super naturally ends.

Tests on GitHub Actions are synthetic and validate RAM safety, expected
selection, masks, ROM lookups, and bookkeeping. They cannot substitute
for a real ROM execution trace.
