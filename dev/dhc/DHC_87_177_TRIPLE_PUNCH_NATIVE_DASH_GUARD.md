# XMVSF Stage 87.177 — Accept QCF+3P for DHC, suppress unwanted dash

**Engine TEST change.** The currently running prototype remains Ryu (12) to
Ken (14), P1, after Ryu's ROM-confirmed Shinkuu Hadoken (action 2E08),
with early cancel enabled. **No new forced ROM move actions.**

## Check first: what 87.175 actually records

The user's supplied 87.175.zip demonstrates three native Ryu→Ken DHC
transactions at P1 host frames 1865, 3095 and 3843. All three are
**LP+MP**, not triple punch. There is **no recorded QCF+3P test** in this
capture and thus no empirical proof of how the original ROM would handle 3P
during Ryu's super.

The 87.175 incoming Ken observer shows action 0600 at frames 1884/1886,
action 0002 at 1888 (during the injected QCF forward/button edge),
and **native action 2E00 / script 0D6722** at 1889. This transient
movement cannot be attributed to a 3P dash because the captured
injected Ken button mask was **03 (two punches), not 07**.

The previous host matcher rejected 3P by requiring exactly two
buttons. Stage 87.177 addresses that *host limitation* without inventing
a new Ken super address or altering his verified native QCF+2P execution.

## What changed

1. Original 2P combinations remain recognized. An exact 3P chord
   (LP+MP+HP = physical mask 07) is additionally accepted as a **DHC
   request** only if P1 Ryu's current native actor state is action 2E08,
   A0=2A28 or 2A02, and reserve fighter is Ken 14. It still requires
   the same QCF motion (down → down-forward → forward). An ordinary
   neutral-state 3P is **not** a DHC request.
2. Before CpsRwGetInp, the raw P1 3P chord is suppressed **only**
   inside the eligible outgoing native DHC super window and only while
   the experimental early-cancel is enabled. The physical SDL input
   array is restored **immediately after** native port sampling so
   the host's input-history matcher still sees the actual 3P request.
   The 68000 never sees an accidental 3P dash during that eligible
   handoff frame.
3. Once the host queues the DHC, Stage 87.176 input isolation continues
   shielding the lingering physical input during the outgoing native
   tag and point swap. Stage 87.177 additionally covers DHC phase
   WAIT_RYU_SUPER_END (9) if a gesture was made before the safe tag
   timer, preventing a held 3P from leaking while waiting.
4. The new accepted 3P route selects exactly the same ROM-verified
   Ken Shoryureppa request as QCF+2P. The actual incoming CPS2
   stream remains **QCF+LP+MP (0x03)**, the already proven native
   sequence. The close-range arrival, native kick suppression, fast
   2/2/3-frame QCF, no charge and 2E00 confirmation stay unchanged.
5. The 3K superjump chord, mixed punch/kick chords, normal dash,
   ordinary tags, other rosters and Stable are not modified.
   A broad diagnostic super-candidate tail is insufficient to enable
   3P; the original fighter's *current* action/mode must match.

## Integration

- Punch chord matcher: src/burner/sdl/xmvsf_stage87_140_super_chat.h
- Authoritative chord predicate and prequeue gate:
  src/burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h
- P1 input port boundary: src/burn/drv/capcom/cps_run.cpp
- Holdover DHC physical-input shield:
  src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h
- Original fast incoming Ken ROM QCF stream:
  src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h
  (not changed for 3P)
- Diagnostic: XM87.177_PREQUEUE_3P_DASH_SUPPRESSED_BEFORE_NATIVE_INPUT;
  DHC chat logs the QCF+3P command when queued.

## Testing

- ROM-free standalone C++11 chord test: PASS for all 64 six-button
  masks with 2P preserved; pure 3P accepted with the native DHC
  allowance, never outside it; 3K and mixed P/K rejected.
- ROM-free DHC wait-state test: PASS, phase 9 shields outgoing Ryu
  while phase 8 releases Ken at his native inputBegin.
- Original 87.175 source capture check: PASS, three 2P DHC queues
  confirmed and Ken's intermediate 0600/0002 movement shown to have
  no 3P input in those attempts.
- **Windows FBNeo build and new 3P gameplay behavior are not yet
  verified**. The previous 87.175 F9 replay validates the original
  2P path but contains no 3P request; verify new 3P live in RUN_TEST.

## Launchers and protections

The same BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat are retained.
Build publishes runtime/stable/XMVSF_Test_87_177.exe only after
successful compile and preserves 87.176, 87.175, 87.167, Stable and F9
recordings. Existing replay runner searches old 87.* sessions.

Expected live 3P outcome: QCF+3P during Ryu 2E08 produces the
prequeue shield diagnostic and DHC QUEUED KEN SHORYUREPPA, then
ROM-accepted outgoing tag 3200, Ken arrival, and Ken-native
2E00/0D6722 with no 3P dash and no super charge.
