# Stage 87.144 — first gameplay-affecting native DHC proof-of-concept

The architectural target remains: match-start native super queue -> reserve
super input during point super -> ROM-owned tag-out -> ROM-owned flying
tag-in without kick -> landing beside opponent -> queued native super
with portrait-free startup, extra meter and first-active freeze release.

This TEST advances beyond read-only proxy logs and attempts an actual
native fighter transition. It is **not yet verified playable DHC**.

## What was implemented

The Stage 87.143 prepared match-start super queue is retained.

New header:
dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h

The current proof-of-concept is strictly **P1 Ryu (ID 12) point ->
Ken (ID 14) reserve**. It is enabled only when the experimental TEST
sets XM87144_DHC_NATIVE_TEST=1. It triggers only after:
- a real ROM-resolved Ryu 2E/08 super-script is already running,
- the command parser recognizes a supported Ken reserve super,
- the Ken entry is selected from the prepared match-start super queue,
- P1 Ryu remains in the native super state 2A02, and
- a matching native ROM outgoing script can be resolved.

On activation, it uses the native Ryu outgoing action 32/00 selected
from the game's own ROM fighter table, reproducing the first animation
frame assignments used by native CPU routines 01038E and 001AE6.
It does **not** alter ordinary tags, initial input port bits, opponent,
reserve actor, or screen sprites.

A 60-frame guard waits for the actual ROM-side point/reserve identity
swap. It NEVER synthesizes a swap. If no swap occurs, it records
NO_NATIVE_SWAP_WITHIN_60_FRAMES and stops the experiment.

Only if a real swap occurs, it watches incoming Ken's normal native
2A/0C flying tag, then 2A/0E grounded tag-kick transition (as in captured
87.141 frame 3018). On the DHC-only landing frame, it moves Ken to
48 native units from the live opponent on the *same side Ryu was on*
at activation, sets opponent-facing and clears motion, and selects the
queued Ken super action using the original ROM script header. This
attempts to replace the normal tag kick with the incoming super.
Normal tags and the unrelated player are unaffected.

## Known limits / what still needs ROM verification

- The host mirrors the native ROM initial script setup, rather than
  running a 68000 subroutine. The ROM may need additional side effects
  before it will execute the swap. The TEST records this explicitly.
- Only Ryu->Ken P1 is enabled. There is no general roster/side handler.
- The Ken super choice currently uses 2C/04 for QCF+PP or 2E/00
  for QCF+KK. Both are real Ken ROM scripts, but their complete
  native move initialization and command-to-variant relationship need
  verification. If the original ROM's other super setup state is absent,
  an animation may start without an actual hitbox/attack.
- There is NO additional meter debit in this build; the captured FF4214
  value has not been verified for charging in this forced context.
- NO first-active freeze timing, background darkening or super
  portrait suppression is implemented yet.
- Stage 87.144 does not forcibly cut the original super's previously
  spawned projectiles/FX. Native action override alone might leave FX.
- The TEST is deliberately bounded to one attempt per match and uses a
  strict phase guard to protect ordinary tag ins.
- This is an experimental first gameplay effect, not a claim that the
  full DHC mechanic works.

## One gameplay test

Select branch feature/dhc-mvc2-stage87 and Pull origin.
Run the original root BUILD_TEST.bat, then RUN_TEST.bat. The latter
publishes and launches the NEW dedicated binary
F:\xmvsf\runtime\stable\XMVSF_Test_87_144.exe.
It sets opt-in XM87144_DHC_NATIVE_TEST=1 in the TEST environment only.
Stable and all prior executables remain untouched.

For the first test, select Ryu (point) / Ken (reserve) on P1.
Start Ryu's real Shinkuu Hadoken (QCF+PP), then **during the super**
input a Ken queued super command (QCF+PP or QCF+KK).
Watch for the native outgoing tag and Ken's arrival. Ordinary tags
should continue without any of these effects.

The chat shows DHC ROM HANDOFF status (Shift+G opens history).
The decisive new file is
F:\xmvsf\test\87.144\dhc_native_handoff.tsv

A success path would include:
 DHC_APPROVED_ROM_OUTGOING_ACTION
 REAL_ROM_POINT_RESERVE_SWAP
 DHC_NATIVE_SUPER_REPLACES_KICK__UNVERIFIED_FX

A failed native transfer reports
 NO_NATIVE_SWAP_WITHIN_60_FRAMES
which means our emulated native script selection alone did not
invoke the ROM team's handoff machinery. That is actionable and must
be traced before adding extra effects.

The existing super_chat.tsv, rom_dispatch.tsv, and optional F10 files
are also in test\87.144.

Isolated C++11 tests exercise the expected state-machine behavior
against synthetic ROM tables and simulated *native* identity swaps,
including no ordinary-tag edits, no meter change and opponent
invariance. These tests DO NOT prove actual Windows/native 68000
playability. Full game runtime must be observed.
