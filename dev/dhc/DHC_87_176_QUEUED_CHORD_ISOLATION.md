# XMVSF Stage 87.176 — Ryu's queued DHC chord leaked into the next 68000 frame

Source: user-supplied 87.175.zip live TSVs and native ROM dispatch evidence.
**Gameplay change limited to P1 Ryu→Ken early-cancel DHC input sampling.**
Ken's entry position, kick suppression, QCF input timing, no-charge startup,
native Shoryureppa selection, F9 recording, Tournament/Classic and Stable
are unchanged.

## Reproduction from the attached 87.175 logs

Three queued Ryu→Ken DHCs used the same sequence:

| DHC selected | Ryu DHC setup after host frame | NEXT frame native Ryu CA | Ryu exit action | Ken ROM super |
|---|---|---|---|---|
| 1865 | +A0=002A, action 2E08 | 1866: 0030 (two punch edges) | 1867: 3200 | 1889: 2E00 / 0D6722 |
| 3095 | +A0=002A, action 2E08 | 3096: 0030 | 3097: 3200 | 3119: 2E00 / 0D6722 |
| 3843 | +A0=002A, action 2E08 | 3844: 0030 | 3845: 3200 | 3867: 2E00 / 0D6722 |

At queue time, Ryu's native CA is still 0000 (the observer omitted
the unchanged frame 3843 pre-CPU line; latest frame 3842 is 0000).
The host sees the physical LP+MP chord earlier in its control arrays,
queues the DHC and stages controller 8, but the *following* CPU frame
still samples the held physical LP+MP and can re-enter the outgoing
Ryu super path. Ryu's action 2E08 is observed again before 3200 tag-out.
This can look like Ryu starts Hadoken again instead of leaving cleanly.
Ryu's outgoing projectile offense is already disabled under the existing
87.159 gate; its visual effects are not newly deleted here.

The 87.175 capture independently proves Ken's **native** 2E00 super
with D6=0C, A0=2A02, script 0D6722 in all three attempts.
The transient 0600 and 0002 movement states *before* 2E00 are
normal input/forward movement while the native QCF is being entered;
do not mislabel the entire super as a standalone dash.
One other Ryu super at frame 2677 has no separate DHC QCF+PP queue
in the supplied log. It is not proof of a failed queued DHC.

## Stage 87.176 change

A pure header XM87176Input::neutralize() gates **only** P1 physical
controller sampling for an already queued, roster-verified
Ryu(12)→Ken(14) early-cancel DHC during phases:

- WAIT_TAG_CONTROLLER: suppress leaked next-frame LP+MP to outgoing Ryu.
- WAIT_NATIVE_SWAP: keep outgoing controls neutral as native point/reserve
  ownership changes.
- WAIT_LANDING / WAIT_KEN_EXIT: keep incoming Ken free from stale physical input.
- DELIVER_NATIVE_INPUT before inputBegin: allow a clean neutral lead-in.

Neutral isolation uses the existing CpsInp001/CpsInp011 override *before*
CpsRwGetInp, then immediately restores actual physical SDL arrays. It does
**not** modify 68000 actor input-history RAM, character recognition code,
the original 2E scripts, or the DHC queue. As soon as inputBegin is reached,
the original fast 2/2/3-frame native QCF+PP/KK signal is submitted unchanged.

A new chat marker is emitted when physical buttons were actually
shielded: DHC_PHYSICAL_CHORD_SHIELDED_DURING_NATIVE_HANDOFF.
The existing 87.175 pre/post host observer remains enabled, allowing
comparison of the old 0030 next-frame CA versus the revised neutral input.

## Tests / build

- ROM-free C++11 regression: g++ -std=c++11 -Wall -Wextra -Werror,
  PASS for queued phase, swap, landing, Ken exit and preinput; PASS for
  no isolation on idle, normal tags, proof mode and other rosters.
- **Pending**: actual Windows MSYS2 executable build and deterministic
  replay of the user-provided F9 session. Source-only tests cannot
  guarantee that the live ROM's input acceptance timing is unchanged.
- Current TEST target: XMVSF_Test_87_176.exe, separate from previous
  XMVSF_Test_87_175.exe and XMVSF_Stable.exe.
- Launchers remain BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat.
  Previously recorded 87.175 F9 sessions remain available through the
  replay runner's older-version search.

## Replay acceptance criteria

1. For DHC queued at frame N, post-CPU at N+1 on outgoing Ryu should
   **not** report newly pressed punch bits CA & 0030 == 0030.
2. Native controller 8 should still reach outgoing action 3200 and
   swap Ryu/Ken ownership.
3. Ken input must remain original fast QCF+2P/2K; D6=0C and 2E00 /
   script 0D6722 should still be accepted without a charge sequence.
4. Incoming Ken still appears near opponent; do not restore his
   old jump-in from the screen edge or add meter/stock side effects.
5. Existing F9 recording, replay integrity, and Stable remain intact.

These are **goals** for the next live build, not verified runtime outcomes.
